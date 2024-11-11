#ifndef __LIBOPENROBI_TICKET_H__
#define __LIBOPENROBI_TICKET_H__

#include "orobi_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t id;
    uint32_t ip;
    uint16_t wait;
    struct orobi_ticket* next;
} orobi_ticket_t;

orobi_error_t orobi_ticket_create(uint16_t id, uint32_t ip, uint16_t ms, orobi_ticket_t** ticket_buffer);
orobi_error_t orobi_ticket_remove(uint16_t id, orobi_ticket_t** ticket_buffer);
orobi_error_t orobi_ticket_find(uint16_t id, orobi_ticket_t* ticket_buffer, orobi_ticket_t** ticket);
orobi_error_t orobi_ticket_free(orobi_ticket_t* ticket_buffer);

#ifdef __cplusplus
}
#endif

#endif // __OROBI_TICKET_H__
