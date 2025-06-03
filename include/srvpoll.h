#ifndef SRVPOLL_H
#define SRVPOLL_H

#include "parse.h"

#define MAX_CLIENT 256
#define BUFF_SIZE 4096

typedef enum
{
    STATE_NEW,
    STATE_CONNECTED,
    STATE_DISCONNECTED,
} state_e;

typedef struct
{
    int fd;
    state_e state;
    char buffer[BUFF_SIZE];
} clientstate_t;

void init_client(clientstate_t *states);
int find_free_slot(clientstate_t *states);
int find_slot_by_fd(clientstate_t *states, int fd);
void pool_loop(unsigned short port, struct dbheader_t *dbhdr, struct employee_t *employees);

#endif