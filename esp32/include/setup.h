// setup.h
#ifndef SETUP_H
#define SETUP_H

#include <stdint.h>
#include <stdbool.h>
#include <esp_system.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "tweetnacl.h"
#include "cJSON.h"

typedef struct {
    uint64_t       random_id_high;
    uint64_t       random_id_low;
    unsigned char  private_key[crypto_box_SECRETKEYBYTES];
    unsigned char  public_key[crypto_box_PUBLICKEYBYTES];
    unsigned char  pc_public_key[crypto_box_PUBLICKEYBYTES];
    bool           pc_key_received;
} setup_data_t;

// Funktionsprototypen
bool setup_check(void);
void setup_run(void);

#endif // SETUP_H
