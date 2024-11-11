#include "orobi_ticket.h"
#include <stdlib.h>
#include <string.h>

orobi_error_t orobi_ticket_create(uint16_t id, uint32_t ip, uint16_t wait, orobi_ticket_t** ticket_buffer) {
    if (!ticket_buffer) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_ticket_t* new_ticket = (orobi_ticket_t*)malloc(sizeof(orobi_ticket_t));
    if (!new_ticket) {
        return OROBI_ERROR_MEMORY;
    }

    new_ticket->id = id;
    new_ticket->ip = ip;
    new_ticket->wait = wait;
    new_ticket->next = *ticket_buffer;
    *ticket_buffer = new_ticket;

    return OROBI_OK;
}

orobi_error_t orobi_ticket_remove(uint16_t id, orobi_ticket_t** ticket_buffer) {
    if (!ticket_buffer) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_ticket_t* current = *ticket_buffer;
    orobi_ticket_t* prev = NULL;

    while (current != NULL) {
        if (current->id == id) {
            if (prev == NULL) {
                *ticket_buffer = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return OROBI_OK;
        }
        prev = current;
        current = current->next;
    }

    return OROBI_ERROR_INVALID_INPUT;
}

orobi_error_t orobi_ticket_find(uint16_t id, orobi_ticket_t* ticket_buffer, orobi_ticket_t** ticket) {
    if (!ticket_buffer || !ticket) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_ticket_t* current = ticket_buffer;
    while (current != NULL) {
        if (current->id == id) {
            *ticket = current;
            return OROBI_OK;
        }
        current = current->next;
    }

    return OROBI_ERROR_INVALID_INPUT;
}

orobi_error_t orobi_ticket_free(orobi_ticket_t* ticket_buffer) {
    if (!ticket_buffer) {
        return OROBI_ERROR_INVALID_INPUT;
    }

    orobi_ticket_t* current = ticket_buffer;
    while (current != NULL) {
        orobi_ticket_t* next = current->next;
        free(current);
        current = next;
    }

    return OROBI_OK;
}
