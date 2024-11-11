#include "secure_comm.h"
#include "tweetnacl.h"
//#include "esp_system.h"

static void __orobi_generate_nonce(orobi_secure_nonce_t* nonce) {
    // Erhöhe Counter
    nonce->counter++;
    
    // Reset Counter wenn Schwelle erreicht
    if (nonce->counter >= NONCE_COUNTER_THRESHOLD) {
        nonce->counter = 0;
    }
    
    // Aktualisiere Zeitstempel
    nonce->timestamp = time(NULL);
    
    // Generiere zufällige Bytes
    for (int i = 0; i < crypto_box_NONCEBYTES - 8; i++) {
        nonce->bytes[i] = (unsigned char)esp_random();
    }
    
    // Füge Counter in die letzten 8 Bytes ein
    memcpy(&nonce->bytes[crypto_box_NONCEBYTES - 8], &nonce->counter, sizeof(uint32_t));
    memcpy(&nonce->bytes[crypto_box_NONCEBYTES - 4], &nonce->timestamp, sizeof(uint32_t));
}

// Überprüft, ob eine Nonce bereits verwendet wurde
static bool __orobi_is_nonce_valid(const orobi_secure_t* ctx, const orobi_secure_nonce_t* nonce) {
    // Prüfe Zeitstempel
    time_t current_time = time(NULL);
    if (current_time - nonce->timestamp > OROBI_MAX_PACKET_AGE_SEC) {
        return false;
    }
    
    // Prüfe Counter
    if (nonce->counter <= ctx->last_seen_nonce.counter &&
        nonce->timestamp <= ctx->last_seen_nonce.timestamp) {
        return false;
    }
    
    return true;
}

// Murmur3 Hash Implementation
// Verbesserte Murmur3 Hash Implementation mit Seed-Mixing
uint64_t orobi_murmur3_64(const void* data, size_t len, uint64_t seed) {
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;
    
    // Zusätzliches Seed-Mixing für bessere Verteilung
    seed ^= len;
    seed *= 0x94d049bb133111ebULL;
    
    uint64_t h = seed;
    
    const uint64_t* blocks = (const uint64_t*)(data);
    const size_t nblocks = len / 8;
    
    for(size_t i = 0; i < nblocks; i++) {
        uint64_t k = blocks[i];
        
        k *= m;
        k ^= k >> r;
        k *= m;
        
        h ^= k;
        h *= m;
        h ^= h >> r;
    }
    
    const uint8_t* tail = (const uint8_t*)(data + nblocks*8);
    uint64_t k = 0;
    
    switch(len & 7) {
        case 7: k ^= ((uint64_t)tail[6]) << 48;
        case 6: k ^= ((uint64_t)tail[5]) << 40;
        case 5: k ^= ((uint64_t)tail[4]) << 32;
        case 4: k ^= ((uint64_t)tail[3]) << 24;
        case 3: k ^= ((uint64_t)tail[2]) << 16;
        case 2: k ^= ((uint64_t)tail[1]) << 8;
        case 1: k ^= ((uint64_t)tail[0]);
                k *= m;
    };
    
    h ^= k;
    h *= m;
    h ^= h >> r;
    h *= m;
    h ^= h >> r;
    
    return h;
}
void orobi_secure_init(orobi_secure_t* ctx, uint128_t id, const unsigned char* public_key, const unsigned char* secret_key) {
    memset(ctx, 0, sizeof(orobi_secure_t));
    ctx->id.high = id.high;
    ctx->id.low = id.low;
    
    memcpy(ctx->public_key, public_key, crypto_box_PUBLICKEYBYTES);
    memcpy(ctx->secret_key, secret_key, crypto_box_SECRETKEYBYTES);
    ctx->last_status = OROBI_OK;
}

orobi_error_t orobi_secure_close(orobi_secure_t* ctx) {
    if (!ctx ) {
        ctx->last_status = OROBI_ERROR_INVALID_INPUT ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Invalid input parameters");
        return ctx->last_status;
    }

    ctx->id.high = 0;
    ctx->id.low = 0;

    free(ctx);
    ctx = NULL;

    return OROBI_OK;
}

// Erstellt ein Paket mit erweiterten Sicherheitsfeatures
orobi_error_t orobi_create_packet(orobi_secure_t* ctx, orobi_packet_t* packet, 
                            const char* message, uint16_t size) {
    if (!ctx || !packet || !message || size > OROBI_MAXMESSAGESIZE ) {
        ctx->last_status = OROBI_ERROR_INVALID_INPUT ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Invalid input parameters");
        return ctx->last_status;
    }

    memset(packet, 0, sizeof(orobi_packet_t));
    memcpy(packet->message, message, size);
    packet->message_size = size;
    packet->api_key = ctx->id.low;
    packet->timestamp = time(NULL);
    
    // Generiere neue Nonce
    __orobi_generate_nonce(&packet->nonce);
    
    // Erstelle Hash aus allen relevanten Feldern
    uint8_t* hash_data = malloc(size + sizeof(uint16_t) + sizeof(uint64_t) + 
                               sizeof(time_t) + crypto_box_NONCEBYTES);
    if (!hash_data) {
        ctx->last_status = OROBI_ERROR_OUTOFMEM;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Memory allocation failed");
        return ctx->last_status;
    }
    
    size_t hash_size = 0;
    memcpy(hash_data + hash_size, message, size);
    hash_size += size;
    memcpy(hash_data + hash_size, &size, sizeof(uint16_t));
    hash_size += sizeof(uint16_t);
    memcpy(hash_data + hash_size, &ctx->id.low, sizeof(uint64_t));
    hash_size += sizeof(uint64_t);
    memcpy(hash_data + hash_size, &packet->timestamp, sizeof(time_t));
    hash_size += sizeof(time_t);
    memcpy(hash_data + hash_size, packet->nonce.bytes, crypto_box_NONCEBYTES);
    hash_size += crypto_box_NONCEBYTES;
    
    packet->packet_hash = orobi_murmur3_64(hash_data, hash_size, ctx->id.high);
    free(hash_data);
    
    ctx->last_status = OROBI_OK;
    return OROBI_OK;
}

// Verschlüsselt ein Paket mit erweiterten Sicherheitsfeatures
orobi_error_t orobi_encrypt_packet(orobi_secure_t* ctx, const orobi_packet_t* packet,
                             orobi_crypt_packet_t* crypt_packet,
                             const unsigned char* their_public_key) {
    if (!ctx || !packet || !crypt_packet || !their_public_key) {
        ctx->last_status = OROBI_ERROR_INVALID_INPUT;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Invalid input parameters");
        return ctx->last_status;
    }

    // Kopiere Nonce
    memcpy(&crypt_packet->nonce, &packet->nonce, sizeof(secure_nonce_t));
    
    // Vorbereitung der Verschlüsselung
    unsigned char* temp = malloc(sizeof(orobi_packet_t) + crypto_box_ZEROBYTES);
    if (!temp) {
        ctx->last_status = OROBI_ERROR_OUTOFMEM;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Memory allocation failed");
        return ctx->last_status;
    }
    
    memset(temp, 0, crypto_box_ZEROBYTES);
    memcpy(temp + crypto_box_ZEROBYTES, packet, sizeof(orobi_packet_t));
    
    // Verschlüsseln
    if (crypto_box(crypt_packet->encrypted_data, temp, 
                  sizeof(packet_t) + crypto_box_ZEROBYTES,
                  packet->nonce.bytes, their_public_key, ctx->secret_key) != 0) {
        free(temp);
        ctx->last_status = OROBI_ERROR_ENCRYPTION_FAILED ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Encryption failed");
        return ctx->last_status;
    }
    
    free(temp);
    
    // Erstelle Hash der verschlüsselten Daten
    crypt_packet->crypt_hash = orobi_murmur3_64(crypt_packet->encrypted_data,
                                         sizeof(crypt_packet->encrypted_data),
                                         OROBI_MURMUR_SEED);
    
    ctx->last_status = SECURE_OK;
    return OROBI_OK;
}

// Entschlüsselt und validiert ein Paket
orobi_error_t orobi_decrypt_packet(orobi_secure_t* ctx,
                                          const orobi_crypt_packet_t* crypt_packet,
                                          orobi_packet_t* packet,
                                          const unsigned char* their_public_key) {
    if (!ctx || !packet || !crypt_packet || !their_public_key) {
        ctx->last_status = OROBI_ERROR_INVALID_INPUT;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Invalid input parameters");
        return ctx->last_status;
    }

    // Überprüfe Hash der verschlüsselten Daten
    uint64_t calculated_crypt_hash = orobi_murmur3_64(crypt_packet->encrypted_data,
                                               sizeof(crypt_packet->encrypted_data),
                                               MURMUR_SEED);
    if (calculated_crypt_hash != crypt_packet->crypt_hash) {
        ctx->last_status = OROBI_ERROR_HASH_MISMATCH ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Encrypted data hash mismatch");
        return ctx->last_status;
    }
    
    // Prüfe Nonce auf Replay
    if (!__orobi_is_nonce_valid(ctx, &crypt_packet->nonce)) {
        ctx->last_status = ERROR_NONCE_REPLAY;
        snprintf(ctx->last_error, ERROR_BUFFER_SIZE, "Invalid nonce (possible replay attack)");
        return ctx->last_status;
    }
    
    // Entschlüsselung vorbereiten
    unsigned char* temp = malloc(sizeof(packet_t) + crypto_box_ZEROBYTES);
    if (!temp) {
        ctx->last_status = OROBI_ERROR_OUTOFMEM;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Memory allocation failed");
        return ctx->last_status;
    }
    
    // Entschlüsseln
    if (crypto_box_open(temp, crypt_packet->encrypted_data,
                       sizeof(crypt_packet->encrypted_data),
                       crypt_packet->nonce.bytes,
                       their_public_key, ctx->secret_key) != 0) {
        free(temp);
        ctx->last_status = OROBI_ERROR_ENCRYPTION_FAILED ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Decryption failed");
        return ctx->last_status;
    }
    
    // Kopiere entschlüsselte Daten
    memcpy(packet, temp + crypto_box_ZEROBYTES, sizeof(packet_t));
    free(temp);
    
    // Validiere Paket
    uint8_t* hash_data = malloc(packet->message_size + sizeof(uint16_t) + 
                               sizeof(uint64_t) + sizeof(time_t) + 
                               crypto_box_NONCEBYTES);
    if (!hash_data) {
        ctx->last_status = OROBI_ERROR_OUTOFMEM;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Memory allocation failed");
        return ctx->last_status;
    }
    
    size_t hash_size = 0;
    memcpy(hash_data + hash_size, packet->message, packet->message_size);
    hash_size += packet->message_size;
    memcpy(hash_data + hash_size, &packet->message_size, sizeof(uint16_t));
    hash_size += sizeof(uint16_t);
    memcpy(hash_data + hash_size, &packet->api_key, sizeof(uint64_t));
    hash_size += sizeof(uint64_t);

    uint64_t calculated_hash = orobi_murmur3_64(hash_data, hash_size, ctx->id.high);
    if(calculated_hash != packet->packet_hash ) {
        ctx->last_status = OROBI_ERROR_HASH_MISMATCH ;
        snprintf(ctx->last_error, OROBI_ERROR_BUFFER_SIZE, "Packet data hash mismatch");
        return ctx->last_status;
    }
    return OROBI_OK;
}
 
