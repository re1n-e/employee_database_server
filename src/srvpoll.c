#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <string.h>
#include <parse.h>
#include <stdlib.h>
#include "srvpoll.h"

void init_client(clientstate_t *states)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        states[i].fd = -1;
        states[i].state = STATE_NEW;
        memset(states[i].buffer, '\0', BUFF_SIZE);
    }
}

int find_free_slot(clientstate_t *states)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (states[i].fd == -1)
        {
            return i;
        }
    }
    return -1;
}

int find_slot_by_fd(clientstate_t *states, int fd)
{
    for (int i = 0; i < MAX_CLIENT; i++)
    {
        if (states[i].fd == fd)
            return i;
    }
    return -1;
}

void pool_loop(unsigned short port, struct dbheader_t *dbhdr, struct employee_t *employees)
{
    int listen_fd, conn_fd, slotFd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct pollfd fds[MAX_CLIENT + 1];
    int opt = 1;
    int nfds = 0, freeSlot = -1;
    clientstate_t clientStates[MAX_CLIENT];
    init_clients(&clientStates);

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
    server_addr.sin_port = htons(port);

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

    printf("Server listening on port: %d\n", port);

    while (1)
    {
        nfds = 1;
        for (int i = 0; i < MAX_CLIENT; i++)
        {
            if (clientStates[i].fd != -1)
            {
                fds[nfds].fd = clientStates[i].fd;
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
                clientStates[freeSlot].fd = conn_fd;
                clientStates[freeSlot].state = STATE_CONNECTED;
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

                ssize_t bytes_read = read(fd, clientStates[slot].buffer, sizeof(clientStates[slot].buffer));
                if (bytes_read <= 0)
                {
                    close(fd);
                    clientStates[slot].fd = -1;
                    clientStates[slot].state = STATE_DISCONNECTED;
                    printf("Client %d disconnected\n", slot);
                }
                else
                {
                    clientStates[slot].buffer[bytes_read] = '\0';
                    printf("Received from client %d: %s\n", slot, clientStates[slot].buffer);
                }
                n_events--;
            }
        }
    }
}