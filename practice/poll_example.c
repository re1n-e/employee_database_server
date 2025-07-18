#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>

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

int find_client_by_fd(int fd)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (clientstates[i].fd == fd)
            return i;
    }
    return -1;
}

int main(int argc, char **argv)
{
    int listen_fd, conn_fd, slotFd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENT + 1];
    int opt = 1;
    int nfds = 0, freeSlot = -1;
    init_clients();

    // Create a socket
    if ((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(fds, 0, sizeof(fds));
    fds[0].fd = listen_fd;
    fds[0].events = POLLIN;
    nfds++;

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
        nfds = 1;
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientstates[i].fd != -1)
            {
                fds[nfds].fd = clientstates[i].fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int n_events = poll(fds, nfds, -1);
        if (n_events == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN)
        {
            conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
            if (conn_fd == -1)
            {
                perror("accept");
                continue;
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            int freeSlot = find_free_client();
            if (freeSlot == -1)
            {
                printf("Server full. Closing connection.\n");
                close(conn_fd);
            }
            else
            {
                clientstates[freeSlot].fd = conn_fd;
                clientstates[freeSlot].state = STATE_CONNECTED;
                printf("Slot %d assigned fd %d\n", freeSlot, conn_fd);
            }
            n_events--;
        }

        for (int i = 1; i < nfds && n_events > 0; i++)
        {
            if (fds[i].revents & POLLIN)
            {
                int fd = fds[i].fd;
                int slot = find_client_by_fd(fd);
                if (slot == -1)
                    continue;

                ssize_t bytes_read = read(fd, clientstates[slot].buffer, sizeof(clientstates[slot].buffer));
                if (bytes_read <= 0)
                {
                    close(fd);
                    clientstates[slot].fd = -1;
                    clientstates[slot].state = STATE_DISCONNECTED;
                    printf("Client %d disconnected\n", slot);
                }
                else
                {
                    clientstates[slot].buffer[bytes_read] = '\0';
                    printf("Received from client %d: %s\n", slot, clientstates[slot].buffer);
                }
                n_events--;
            }
        }
    }
    return 0;
}