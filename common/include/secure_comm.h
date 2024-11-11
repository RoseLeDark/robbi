// secure_comm.h
#ifndef SECURE_COMM_H
#define SECURE_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tweetnacl.h"

#define MAX_MESSAGE_SIZE 4096
#define MURMUR_SEED 42

typedef struct {
    char message[MAX_MESSAGE_SIZE];
    uint16_t message_size;
    uint64_t api_key;        // Das ist random_id_low
    uint64_t packet_hash;    // Hash aus message + size + api_key
} packet_t;

typedef struct {
    unsigned char encrypted_data[sizeof(packet_t) + crypto_box_ZEROBYTES];
    uint64_t crypt_hash;     // Murmur hash der verschlüsselten Daten
} crypt_packet_t;
