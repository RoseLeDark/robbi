#ifndef __LIBOPENROBI_SECURE_H__
#define __LIBOPENROBI_SECURE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "tweetnacl.h"
#include "orobi_common.h"

#define OROBI_MAXMESSAGESIZE              4096
#define OROBI_MURMUR_SEED                 42
#define OROBI_MAX_PACKET_AGE_SEC          30  // Maximales Alter eines Pakets
#define OROBI_NONCE_COUNTER_THRESHOLD     0xFFFFFFFF  // Schwelle für Nonce-Reset
#define OROBI_ERROR_BUFFER_SIZE           128

#ifdef __cplusplus
extern "C" {
#endif


// Erweiterter Nonce-Struct mit Zeitstempel und Counter
typedef struct {
    unsigned char            bytes[crypto_box_NONCEBYTES];
    uint32_t                 counter;
    time_t                   timestamp;
} orobi_secure_nonce_t;

// Erweitertes Paket mit Zeitstempel und Nonce
typedef struct {
    char                     message[OROBI_MAXMESSAGESIZE];
    uint16_t                 message_size;
    uint64_t                 api_key;        // random_id_low
    uint64_t                 packet_hash;    // Hash aus message + size + api_key
    time_t                   timestamp;        // Zeitstempel für Replay-Schutz
    orobi_secure_nonce_t     nonce;    // Nonce für diese Nachricht
} orobi_packet_t;

typedef struct {
    unsigned char            encrypted_data[sizeof(packet_t) + crypto_box_ZEROBYTES];
    uint64_t                 crypt_hash;     // Murmur hash der verschlüsselten Daten
    orobi_secure_nonce_t     nonce;    // Kopie der Nonce für Empfänger
} orobi_crypt_packet_t;

// Kontext-Struktur für den Zustand der Kommunikation
typedef struct {
    uint128_t             id;
    unsigned char         public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char         secret_key[crypto_box_SECRETKEYBYTES];
    orobi_secure_nonce_t  last_seen_nonce;
    char                  last_error[OROBI_ERROR_BUFFER_SIZE];
    orobi_error_t         last_status;
} orobi_secure_t;

uint64_t         orobi_murmur3_64(const void* data, size_t len, uint64_t seed);

void             orobi_secure_init(orobi_secure_t* ctx, uint128_t id, const unsigned char* public_key, const unsigned char* secret_key);
orobi_error_t    orobi_secure_close(orobi_secure_t* ctx);
orobi_error_t    orobi_create_packet(orobi_secure_t* ctx, orobi_packet_t* packet, const char* message, uint16_t size);
// Verschlüsselt ein Paket mit erweiterten Sicherheitsfeatures
orobi_error_t    orobi_encrypt_packet(orobi_secure_t* ctx, const orobi_packet_t* packet, orobi_crypt_packet_t* crypt_packet, const unsigned char* their_public_key);
// Decrypt and validate
orobi_error_t    orobi_decrypt_packet(orobi_secure_t* ctx, const orobi_crypt_packet_t* crypt_packet, orobi_packet_t* packet, const unsigned char* their_public_key) ;

#ifdef __cplusplus
}
#endif

#endif // __LIBOPENROBI_SECURE_H__
