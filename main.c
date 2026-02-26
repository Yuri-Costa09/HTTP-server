#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>    // struct sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>     // inet_pton(), inet_ntop() (se precisar)
#include <unistd.h>        // close()

#define PORT 8080

/*
 * First: I'll write about some really important concepts to know:
 *
 * FILE DESCRIPTORS: File descriptor is an int identifier for an open resource, used by the Linux Kernel.
 * e.g. (Open Files, Sockets, etc.)
 *
 * BYTE ORDER / Endianness: Is the order that the bytes are going to be stored, or streamed by network. There are two
 * main orders:
 * - Big-Endian (BE) - Stores the most significant byte first in the lowest address of Memory.
 * - Little-Endian (LE) - Stores the least significant byte first in the lowest address of Memory.
 */
int main(void)
{
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 'AF_INET=ipv4 -- 'SOCK_STREAM=TCP connection
    if (socket_fd == -1)
    {
        printf("SERVER SOCKET creation failed: %s \n", strerror(errno));
        exit(1);
    }

    /*
     * sin_port = htons(PORT): the port that the socket will listen. Convert the 'bytes order/endianness to Big-Endian
     * sin_addr = INADDR_ANY:  Will allow any connections from anywhere.
     */
    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    int b = bind(socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (b == -1)
    {
        printf("BIND failed: %s \n", strerror(errno));
        exit(1);
    }

    /**
     * Since the tester restarts your program quite often, setting REUSE_PORT
     * ensures that we don't run into 'Address already in use' errors
     */
    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
    {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        exit(1);
    }

    // Maximum length of the queue of pending connections
    const int connection_backlog = 10;

    /**
     * `listen()` indicates that the server socket is ready to accept incoming connections
     *
     * returns 0 if connection was successful
     */
    if (listen(socket_fd, connection_backlog) != 0)
    {
        printf("Listen failed: %s \n", strerror(errno));
        exit(1);
    }

    while (1)
    {
        // to continue https://github.com/IonelPopJara/http-server-c/blob/master/app/server.c
        // https://en.wikipedia.org/wiki/Berkeley_sockets
        printf("Server started.\n");
        printf("\tWaiting for clients to connect...\n");

        struct sockaddr_in client_addr;
        int client_size = sizeof(client_addr);

        int client_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_size);
        if (client_socket_fd == -1)
        {
            printf("Failed to connect: %s \n", strerror(errno));
            exit(1);
        }
        printf("Client connected\n");

    }

    return 0;
}

