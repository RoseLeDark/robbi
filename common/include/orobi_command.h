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

#define OROBI_COMMAND_COMPASS          000    // 000-360 sind compass daten für orobi_command
#define OROBI_COMMAND_SEND_SENSOR      365    // Sende Sensor Daten
#define OROBI_COMMAND_SEND_STATUS      370    // Sende Status Daten (orobi_command_status_tostring)
#define OROBI_COMMAND_SEND_AKKU        380    // Sende Akku Daten: ("CAKKU%#")
#define OROBI_COMMAND_LOCK_MODE        390    // Gehe in Lock Modus und warte
#define OROBI_COMMAND_DEBUG            395    // Gehe in Debug Modus - Deaktiviere Motorshield und sende daten an ESP32 -> Bodenstation
#define OROBI_COMMAND_MAIN_MODE        391    // Gehe zurück in Normalen Modus
#define OROBI_COMMAND_RUN_TICKET       400    // Baue Direkte verbindung zu ein anderen Gerät - TicketID ist MSMS Bereich, erfrage IP von Bodenstation
#define OROBI_COMMAND_DEL_TICKET       401    // Lösche Ticket im Buffer
#define OROBI_COMMAND_RETRY_TICKET     402    // Versuche Ticket erneut auszuführen
#define OROBI_COMMAND_ALARM            999    // ALARM von Bodenstation

typedef struct orobi_command {
    uint16_t compass;
    uint8_t motor;
    uint16_t duration_ms;
    uint8_t seq_nr;
    struct orobi_command* next;
} orobi_command_t;

typedef struct orobi_command_status {
    uint8_t seq_nr;
    orobi_command_status_t status;
    time_t timestamp;
    struct orobi_command_status* next;
} orobi_command_status_t;

// Funktion zum Parsen eines Steuerstrings
orobi_error_t orobi_command_parse(const char* command_str, orobi_command_t* command);

// Funktion zum Überprüfen der Kommandos
orobi_error_t orobi_command_validate(const orobi_command_t* command);

// Funktion zum Erstellen eines Steuerstrings
orobi_error_t orobi_command_tostring(const orobi_command_t* command, char* command_str, size_t command_str_size);

// Funktion zum Parsen einer Befehlssequenz
orobi_error_t orobi_command_parse_seq(const char* commands_str, orobi_command_t** head);

// Funktion zum Erstellen eines Statusobjekts
orobi_error_t orobi_command_status_create(const orobi_command_t* command, orobi_command_status_t* status);

// Funktion zum Setzen eines Statusobjekts
orobi_error_t orobi_command_status_set(orobi_command_status_t** head, const orobi_command_status_t* status);

// Funktion zum Konvertieren des Status in einen String
orobi_error_t orobi_command_status_tostring(const orobi_command_status_t* head, char* out, size_t out_size);

// Funktion zum Freigeben von Statusobjekten
orobi_error_t orobi_command_status_free(orobi_command_status_t* command);

#ifdef __cplusplus
}
#endif

#endif // __LIBOPENROBI_COMMAND_H__
