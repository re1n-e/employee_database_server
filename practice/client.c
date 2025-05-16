#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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
    ssize_t bytesRead = read(fd, buf, sizeof(proto_hdr_t) + sizeof(int));
    if (bytesRead < sizeof(proto_hdr_t) + sizeof(int))
    {
        perror("read");
        return;
    }

    proto_hdr_t hdr = {0};
    memcpy(&hdr, buf, sizeof(proto_hdr_t));
    hdr.type = ntohl(hdr.type);
    hdr.len = ntohs(hdr.len);

    int data = -1;
    memcpy(&data, buf + sizeof(proto_hdr_t), sizeof(int));
    data = ntohl(data);

    if (hdr.type != PROTO_HELLO)
    {
        printf("Invalid header protocol: %d\n", hdr.type);
        return;
    }
    if (hdr.len != sizeof(int))
    {
        printf("Invalid header len: %d\n", hdr.len);
        return;
    }
    if (data != 1)
    {
        printf("Invalid protocol version: %d\n", data);
        return;
    }

    printf("Protocol version v1.\n");
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <ip address>\n", argv[0]);
        return -1;
    }

    struct sockaddr_in serverInfo = {0};
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr.s_addr = inet_addr(argv[1]);
    serverInfo.sin_port = htons(5555);

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) == -1)
    {
        perror("connect");
        close(fd);
        return -1;
    }

    handle_connection(fd);

    close(fd);
    return 0;
}
