
// setup.c
#include "setup.h"
#include "esp_log.h"
#include "driver/uart.h"
#include <string.h>

static const char *TAG = "SETUP";
static setup_data_t setup_data;
static nvs_handle_t nvs_handle;

#define OPENROBBI_SETUP_NAMESPACE    "OpenROBI"
#define OPENROBBI_SETUP_DATA_KEY     "orob_6782"

// Hilfsfunktion zum Speichern der Setup-Daten
static void save_setup_data(void) {
    esp_err_t err = nvs_open(OPENROBBI_SETUP_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle");
        return;
    }

    err = nvs_set_blob(nvs_handle, OPENROBBI_SETUP_DATA_KEY , &setup_data, sizeof(setup_data_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error saving setup data");
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS");
    }

    nvs_close(nvs_handle);
}

// Hilfsfunktion zum Parsen des PC Public Keys
static bool parse_pc_public_key(const char* json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (root == NULL) return false;

    cJSON *pc_key = cJSON_GetObjectItem(root, "orob_ground_key");
    if (!pc_key || !pc_key->valuestring) {
        cJSON_Delete(root);
        return false;
    }

    // Convert hex string to binary
    char *hex = pc_key->valuestring;
    for(int i = 0; i < crypto_box_PUBLICKEYBYTES; i++) {
        sscanf(&hex[i*2], "%2hhx", &setup_data.pc_public_key[i]);
    }

    cJSON_Delete(root);
    setup_data.pc_key_received = true;
    save_setup_data();
    return true;
}

// Prüft, ob Setup-Daten vorhanden sind
bool setup_check(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    err = nvs_open(OPENROBBI_SETUP_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle");
        return false;
    }

    size_t required_size = sizeof(setup_data_t);
    size_t actual_size;
    err = nvs_get_blob(nvs_handle, OPENROBBI_SETUP_DATA_KEY, NULL, &actual_size);
    
    if (err != ESP_OK || actual_size != required_size) {
        ESP_LOGI(TAG, "No valid setup data found");
        nvs_close(nvs_handle);
        return false;
    }

    err = nvs_get_blob(nvs_handle, OPENROBBI_SETUP_DATA_KEY, &setup_data, &actual_size);
    nvs_close(nvs_handle);
    
    return (err == ESP_OK && setup_data.pc_key_received);
}

static void setup_loop(void) {
    static uint64_t last_print = 0;
    uint64_t current_time = esp_timer_get_time() / 1000000;

    // Periodische JSON-Ausgabe
    if (current_time - last_print >= 10) {
        cJSON *root = cJSON_CreateObject();
        
        char id_high[21];
        char id_low[21];
        snprintf(id_high, sizeof(id_high), "%llu", setup_data.random_id_high);
        snprintf(id_low, sizeof(id_low), "%llu", setup_data.random_id_low);
        
        cJSON_AddStringToObject(root, "id_high", id_high);
        cJSON_AddStringToObject(root, "id_low", id_low);
        
        char public_key_hex[crypto_box_PUBLICKEYBYTES * 2 + 1];
        for(int i = 0; i < crypto_box_PUBLICKEYBYTES; i++) {
            sprintf(&public_key_hex[i*2], "%02x", setup_data.public_key[i]);
        }
        cJSON_AddStringToObject(root, "public_key", public_key_hex);
        
        char *json_str = cJSON_Print(root);
        printf("%s\n", json_str);
        
        free(json_str);
        cJSON_Delete(root);
        
        last_print = current_time;
    }

    // Check for PC public key response
    uint8_t buf[512];
    int len = uart_read_bytes(UART_NUM_0, buf, sizeof(buf) - 1, pdMS_TO_TICKS(100));
    if (len > 0) {
        buf[len] = '\0';
        if (parse_pc_public_key((char*)buf)) {
            printf("PC public key received and saved. Please reset the device.\n");
            while(1) {
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

// Führt das Setup durch
void setup_run(void) {
    // UART Konfiguration
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_0, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 1024, 0, 0, NULL, 0));

    printf("Setup Mode - Start setup? (y/n)\n");
    
    char c;
    while (1) {
        if (uart_read_bytes(UART_NUM_0, (uint8_t*)&c, 1, pdMS_TO_TICKS(100)) == 1) {
            if (c == 'y' || c == 'Y') {
                break;
            } else if (c == 'n' || c == 'N') {
                printf("Setup aborted\n");
                return;
            }
        }
    }

    printf("Enter random seed text: ");
    char seed_text[64];
    size_t len = uart_read_bytes(UART_NUM_0, (uint8_t*)seed_text, sizeof(seed_text) - 1, pdMS_TO_TICKS(5000));
    seed_text[len] = '\0';

    // Generate random IDs and keys
    setup_data.random_id_high = esp_random();
    setup_data.random_id_low = esp_random();
    setup_data.pc_key_received = false;

    unsigned char random_seed[32];
    for(int i = 0; i < 32; i++) {
        random_seed[i] = seed_text[i % len] ^ i;
    }
    crypto_box_keypair(setup_data.public_key, setup_data.private_key);

    // Save initial setup data
    save_setup_data();
    
    printf("Initial setup complete. Waiting for PC public key...\n");
    
    // Enter setup loop until PC key is received
    while(!setup_data.pc_key_received) {
        setup_loop();
    }
}
