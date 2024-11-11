#include "orobi_command.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

orobi_error_t orobi_command_parse(const char* command_str, orobi_command_t* command) {
    if (!command_str || !command) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    int compass, motor, duration_ms;
    if (sscanf(command_str, "A%d%d%d#", &compass, &motor, &duration_ms) != 3) {
        return OROBI_ERROR_INVALID_COMMAND;
    }

    if (compass < 0 || compass > 999 || motor < 0 || motor > 99 || duration_ms < 1 || duration_ms > 9999) {
        return OROBI_ERROR_COMMAND_OVERFLOW;
    }

    command->compass = (uint16_t)compass;
    command->motor = (uint8_t)motor;
    command->duration_ms = (uint16_t)duration_ms;
    command->next = NULL;

    return OROBI_OK;
}

orobi_error_t orobi_command_validate(const orobi_command_t* command) {
    if (!command) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    if (command->compass > 360 && command->compass < 361) {
        return OROBI_ERROR_UNSUPPORTED_COMMAND;
    }

    return OROBI_OK;
}

orobi_error_t orobi_command_tostring(const orobi_command_t* command, char* command_str, size_t command_str_size) {
    if (!command || !command_str) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    int len = snprintf(command_str, command_str_size, "A%03d%02d%04d#",
                       command->compass, command->motor, command->duration_ms);
    if (len < 0 || (size_t)len >= command_str_size) {
        return OROBI_ERROR_BUFFER_OVERFLOW;
    }

    return OROBI_OK;
}

orobi_error_t orobi_command_parse_seq(const char* commands_str, orobi_command_t** head) {
    if (!commands_str || !head) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    const char* cmd_start = commands_str;
    orobi_command_t* current = NULL;
    orobi_command_t* prev = NULL;
    uint8_t seq_nr = 0;

    while (*cmd_start) {
        const char* cmd_end = strchr(cmd_start, '#');
        if (!cmd_end) {
            break;
        }
        
        size_t cmd_len = cmd_end - cmd_start + 1;
        char cmd[cmd_len + 1];
        strncpy(cmd, cmd_start, cmd_len);
        cmd[cmd_len] = '\0';

        orobi_command_t* new_command = (orobi_command_t*)malloc(sizeof(orobi_command_t));
        if (!new_command) {
            return OROBI_ERROR_MEMORY;
        }
        memset(new_command, 0, sizeof(orobi_command_t));
        new_command->seq_nr = seq_nr++;

        orobi_error_t parse_result = orobi_command_parse(cmd, new_command);
        if (parse_result != OROBI_OK) {
            free(new_command);
            return parse_result;
        }

        if (!current) {
            *head = new_command;
        } else {
            current->next = new_command;
        }
        current = new_command;
        cmd_start = cmd_end + 1;
    }

    return OROBI_OK;
}

orobi_error_t orobi_command_status_create(const orobi_command_t* command, orobi_command_status_t* status) {
    if (!command || !status) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    status->seq_nr = command->seq_nr;
    status->status = OROBI_COMMAND_STATUS_OK;
    status->timestamp = time(NULL);
    status->next = NULL;

    return OROBI_OK;
}

orobi_error_t orobi_command_status_set(orobi_command_status_t** head, const orobi_command_status_t* status) {
    if (!head || !status) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_command_status_t* new_status = (orobi_command_status_t*)malloc(sizeof(orobi_command_status_t));
    if (!new_status) {
        return OROBI_ERROR_MEMORY;
    }
    memcpy(new_status, status, sizeof(orobi_command_status_t));

    new_status->next = *head;
    *head = new_status;

    return OROBI_OK;
}

orobi_error_t orobi_command_status_tostring(const orobi_command_status_t* head, char* out, size_t out_size) {
    if (!head || !out) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    size_t offset = 0;
    const orobi_command_status_t* current = head;
    while (current) {
        int len = snprintf(out + offset, out_size - offset, "#%d:%d:%ld#",
                           current->seq_nr, current->status, current->timestamp);
        if (len < 0 || (size_t)len >= (out_size - offset)) {
            return OROBI_ERROR_BUFFER_OVERFLOW;
        }
        offset += len;
        current = current->next;
    }

    return OROBI_OK;
}

orobi_error_t orobi_command_status_free(orobi_command_status_t* head) {
    if (!head) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_command_status_t* current = head;
    while (current != NULL) {
        orobi_command_status_t* next = current->next;
        free(current);
        current = next;
    }
    
    return OROBI_OK;
}
