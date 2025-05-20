#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#define MAX_CLIENT 256
#define PORT 8080
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

clientstate_t clientstates[MAX_CLIENT];

void init_clients()
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        clientstates[i].fd = -1;
        clientstates[i].state = STATE_NEW;
        memset(&clientstates[i].buffer, '\0', BUFF_SIZE);
    }
}

int find_free_client()
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (clientstates[i].fd == -1)
        {
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv)
{
    int listen_fd, conn_fd, slotFd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set read_fds, write_fds;

    init_clients();

    // Create a socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    // set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind
    if (bind(listen_fd, (sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listen_fd, 10) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port: %d\n", PORT);
}