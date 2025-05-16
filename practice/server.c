#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

typedef enum
{
    PROTO_HELLO = 0,
} proto_type_e;

typedef struct
{
    proto_type_e type;
    unsigned short len;
} __attribute__((packed)) proto_hdr_t;

void handle_connection(int fd)
{
    char buf[4096] = {0};

    proto_hdr_t hdr;
    hdr.type = htonl(PROTO_HELLO);
    hdr.len = htons(sizeof(int));

    memcpy(buf, &hdr, sizeof(hdr));

    int32_t data = htonl(1);
    memcpy(buf + sizeof(hdr), &data, sizeof(data));

    ssize_t sent = write(fd, buf, sizeof(hdr) + sizeof(data));
    if (sent == -1)
    {
        perror("write");
    }
}

int main()
{
    struct sockaddr_in serverInfo = {0};
    struct sockaddr_in clientInfo = {0};
    socklen_t clientSize = sizeof(clientInfo);

    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = INADDR_ANY;
    serverInfo.sin_port = htons(5555);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1)
    {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 5) == -1)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    printf("Server listening on port 5555...\n");

    while (1)
    {
        int cfd = accept(fd, (struct sockaddr *)&clientInfo, &clientSize);
        if (cfd == -1)
        {
            perror("accept");
            continue;
        }

        handle_connection(cfd);
        close(cfd);
    }

    close(fd);
    return 0;
}
