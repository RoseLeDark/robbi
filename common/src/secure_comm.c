#include "secure_comm.h"
#include "tweetnacl.h"

// Murmur3 Hash Implementation
// Verbesserte Murmur3 Hash Implementation mit Seed-Mixing
uint64_t murmur3_64(const void* data, size_t len, uint64_t seed) {
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

// Erstellt ein Paket und berechnet den Hash
void create_packet(packet_t* packet, const char* message, uint16_t size, uint64_t api_key, uint64_t id_high) {
    memset(packet, 0, sizeof(packet_t));
    memcpy(packet->message, message, size);
    packet->message_size = size;
    packet->api_key = api_key;
    
    // Hash aus Nachricht + Größe + API-Key mit id_high
    uint8_t hash_data[MAX_MESSAGE_SIZE + sizeof(uint16_t) + sizeof(uint64_t)];
    size_t hash_size = 0;
    
    memcpy(hash_data + hash_size, message, size);
    hash_size += size;
    memcpy(hash_data + hash_size, &size, sizeof(uint16_t));
    hash_size += sizeof(uint16_t);
    memcpy(hash_data + hash_size, &api_key, sizeof(uint64_t));
    hash_size += sizeof(uint64_t);
    
    packet->packet_hash = murmur3_64(hash_data, hash_size, id_high);
}

// Validiert ein empfangenes Paket
bool validate_packet(const packet_t* packet, uint64_t id_high) {
    uint8_t hash_data[MAX_MESSAGE_SIZE + sizeof(uint16_t) + sizeof(uint64_t)];
    size_t hash_size = 0;
    
    memcpy(hash_data + hash_size, packet->message, packet->message_size);
    hash_size += packet->message_size;
    memcpy(hash_data + hash_size, &packet->message_size, sizeof(uint16_t));
    hash_size += sizeof(uint16_t);
    memcpy(hash_data + hash_size, &packet->api_key, sizeof(uint64_t));
    hash_size += sizeof(uint64_t);
    
    uint64_t calculated_hash = murmur3_64(hash_data, hash_size, id_high);
    return calculated_hash == packet->packet_hash;
}

// Verschlüsselt ein Paket
bool encrypt_packet(const packet_t* packet, crypt_packet_t* crypt_packet,
                   const unsigned char* their_public_key,
                   const unsigned char* our_secret_key) {
    unsigned char nonce[crypto_box_NONCEBYTES] = {0};  // In Produktion: Zufällig generieren
    unsigned char temp[sizeof(packet_t) + crypto_box_ZEROBYTES];
    
    // Padding für TweetNaCl
    memset(temp, 0, crypto_box_ZEROBYTES);
    memcpy(temp + crypto_box_ZEROBYTES, packet, sizeof(packet_t));
    
    // Verschlüsseln
    if (crypto_box(crypt_packet->encrypted_data, temp, 
                  sizeof(temp), nonce,
                  their_public_key, our_secret_key) != 0) {
        return false;
    }
    
    // Hash der verschlüsselten Daten
    crypt_packet->crypt_hash = murmur3_64(crypt_packet->encrypted_data, 
                                         sizeof(crypt_packet->encrypted_data),
                                         MURMUR_SEED);
    return true;
}

// Entschlüsselt ein Paket
bool decrypt_packet(const crypt_packet_t* crypt_packet, packet_t* packet,
                   const unsigned char* their_public_key,
                   const unsigned char* our_secret_key) {
    // Überprüfe zuerst den Hash der verschlüsselten Daten
    uint64_t calculated_crypt_hash = murmur3_64(crypt_packet->encrypted_data,
                                               sizeof(crypt_packet->encrypted_data),
                                               MURMUR_SEED);
    if (calculated_crypt_hash != crypt_packet->crypt_hash) {
        return false;
    }
    
    unsigned char nonce[crypto_box_NONCEBYTES] = {0};  // Muss gleich sein wie beim Verschlüsseln
    unsigned char temp[sizeof(packet_t) + crypto_box_ZEROBYTES];
    
    // Entschlüsseln
    if (crypto_box_open(temp, crypt_packet->encrypted_data,
                       sizeof(crypt_packet->encrypted_data), nonce,
                       their_public_key, our_secret_key) != 0) {
        return false;
    }
    
    // Kopiere entschlüsselte Daten ohne Padding
    memcpy(packet, temp + crypto_box_ZEROBYTES, sizeof(packet_t));
    return true;
}
