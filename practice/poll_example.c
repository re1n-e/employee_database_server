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
    int nfds = 0, freeSlot = -1;
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
    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listen_fd, 10) == -1)
    {
        perror("listen");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port: %d\n", PORT);

    while (1)
    {
        // reset FDS
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        // Add the listening socket to the read set
        FD_SET(listen_fd, &read_fds);
        nfds = listen_fd + 1;

        // Add active connections to the read set
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientstates[i].fd != -1)
            {
                FD_SET(clientstates[i].fd, &read_fds);
                if (clientstates[i].fd >= nfds)
                    nfds = clientstates[i].fd + 1;
            }
        }

        // wait for an activity on one of the fds
        if (select(nfds, &read_fds, &write_fds, NULL, NULL) == -1)
        {
            perror("select");
            close(listen_fd);
            exit(EXIT_FAILURE);
        }

        // Check for new connetions
        if (FD_ISSET(listen_fd, &read_fds))
        {
            if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len)) == -1)
            {
                perror("accept");
                close(listen_fd);
                exit(EXIT_FAILURE);
            }
            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Find a free slot for a new connection
            freeSlot = find_free_client();
            if (freeSlot == -1)
            {
                printf("Server full: closing new connections\n");
                close(conn_fd);
            }
            else
            {
                clientstates[freeSlot].fd = conn_fd;
                clientstates[freeSlot].state = STATE_CONNECTED;
            }
        }

        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientstates[i].fd != -1 && FD_ISSET(clientstates[i].fd, &read_fds))
            {
                ssize_t bytes_read = read(clientstates[i].fd, &clientstates[i].buffer, sizeof(clientstates[i].buffer));
                if (bytes_read <= 0)
                {
                    close(clientstates[i].fd);
                    clientstates[i].fd = -1;
                    clientstates[i].state = STATE_DISCONNECTED;
                    printf("Closing connection or error\n");
                }
                else
                {
                    printf("Recived Data from client: %s\n", clientstates[i].buffer);
                }
            }
        }
    }
    return 0;
}