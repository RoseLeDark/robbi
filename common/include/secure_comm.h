#ifndef __LIBOPENROBI_SECURE_H__
#define __LIBOPENROBI_SECURE_H__

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "tweetnacl.h"
#include "esp_system.h"

#define OROBI_MAXMESSAGESIZE              4096
#define OROBI_MURMUR_SEED                 42
#define OROBI_MAX_PACKET_AGE_SEC          30  // Maximales Alter eines Pakets
#define OROBI_NONCE_COUNTER_THRESHOLD     0xFFFFFFFF  // Schwelle für Nonce-Reset
#define OROBI_ERROR_BUFFER_SIZE           128

typedef enum {
    OROBI_OK = 0,
    OROBI_ERROR_INVALID_INPUT = -1,
    OROBI_ERROR_PACKET_TOO_OLD = -2,
    OROBI_ERROR_NONCE_REPLAY = -3,
    OROBI_ERROR_ENCRYPTION_FAILED = -4,
    OROBI_ERROR_DECRYPTION_FAILED = -5,
    OROBI_ERROR_HASH_MISMATCH = -6,
    OROBI_ERROR_PACKET_VALIDATION_FAILED = -7,
    OROBI_ERROR_MEMORY = -8
} orobi_error_t;

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

typedef union {
    struct {
        uint64_t              high;
        uint64_t              low;
    };
    uint64_t    key[2];
} uint128_t;
// Kontext-Struktur für den Zustand der Kommunikation
typedef struct {
    uint128_t             id;
    unsigned char         public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char         secret_key[crypto_box_SECRETKEYBYTES];
    orobi_secure_nonce_t  last_seen_nonce;
    char                  last_error[OROBI_ERROR_BUFFER_SIZE];
    orobi_error_t         last_status;
} orobi_secure_t;

void orobi_secure_init(orobi_secure_t* ctx, uint128_t id, const unsigned char* public_key, const unsigned char* secret_key);



uint8_t orobi_create_packet(packet_t* packet, const char* message, uint16_t size, uint64_t api_key, uint64_t id_high);
uint8_t orobi_validate_packet(const packet_t* packet, uint64_t id_high);
uint8_t orobi_encrypt_packet(const packet_t* packet, crypt_packet_t* crypt_packet, const unsigned char* their_public_key, const unsigned char* our_secret_key);
uint8_t orobi_decrypt_packet(const crypt_packet_t* crypt_packet, packet_t* packet, const unsigned char* their_public_key, const unsigned char* our_secret_key);

#endif // __LIBOPENROBI_SECURE_H__
