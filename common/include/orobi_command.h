#ifndef __LIBOPENROBI_COMMAND_H__
#define __LIBOPENROBI_COMMAND_H__

#include "orobi_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum orobi_command_status {
    OROBI_COMMAND_STATUS_OK,
    OROBI_COMMAND_STATUS_ERROR,
    OROBI_COMMAND_STATUS_NODATA,
    OROBI_COMMAND_STATUS_INVALID_COMMAND
} orobi_command_status_t;


typedef enum orobi_command_packet {
    OROBI_COMMAND_MOTORDATA,            // orobi_motordata_t motor
    OROBI_COMMAND_STARTDATA,            // orobi_motordata_t start
    OROBI_COMMAND_INT,                  // uint32_t value
    OROBI_COMMAND_FLOAT,                // float fvalue
    OROBI_COMMAND_STRING,               // char string[16]
    OROBI_COMMAND_USER                  // void* packet;
}orobi_command_packet_t;

typedef struct orobi_motordata {
    uint16_t speed;
    uint16_t rotation;
    bool     buttons[4];        // 0: break, 1: light on/off, 2: resaviert, 3: reserved
} orobi_motordata_t;

typedef struct orobi_start {
    char wifi_ssid[16];
    char wifi_passwd[16];
    uint8_t rw_port;
    uint32_t key_station[2]; // 0: low und 1: high Word (low: public_key, high: private_key)
    uint16_t api_key;
} orobi_start_t;

typedef struct orobi_command {
    orobi_command_packet_t type;
    union {
        orobi_motordata_t motor;
        orobi_start_t start;
        uint32_t value;
        float fvalue;
        char string[16];
        void* packet;
    };
    uint32_t hash;        /// < Hash aus: PC -> esp32: L & H von PC, ESP32 -> PC: L & H von ESP32 
    uint16_t lowWord;     /// < PC -> esp32: L von PC, ESP32 -> PC: L von ESP32 
} orobi_command_t;

#define OROBI_COMMAND_SIZE sizeof(orobi_command_t)

typedef struct orobi_network_packet {        // ESP32 <-> PC
    uint8_t     seq_nr;
    uint16_t    api_key;
    union {
        void*             raw_packet;
        orobi_command_t*  packet; // <--- To Mega
    };
    char        compressed : 1;
    uint32_t    compress_size;        
    uint32_t    hash;            // Key muss in esp32 programm und PC gleich sein
} orobi_netpacket_t;

// Funktion zum Überprüfen der Kommandos
orobi_error_t orobi_command_validate(const orobi_command_t* command);

orobi_error_t orobi_netpacket_parse(void* data, size_t size, orobi_netpacket_t* out);
orobi_error_t orobi_netpacket_validate(const orobi_netpacket_t* net);

#ifdef __cplusplus
}
#endif

#endif // __LIBOPENROBI_COMMAND_H__
