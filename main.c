#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// syscall to manipulate FDs — allowing you to change properties such as file locking,
// access modes (read/write), status flags (non-blocking, async I/O), and descriptor
#include <fcntl.h>
// lib that has Kqueue / Kevent.
#include <sys/event.h>
#include <netinet/in.h>    // struct sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>     // inet_pton(), inet_ntop() (se precisar)
#include <unistd.h>        // close()

#define PORT 8080
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096

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
 *
 *
 * docs: https://man.freebsd.org/cgi/man.cgi?kqueue
 */

// Turns FD non-blocking.
static void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// Register a FD to Kqueue to monitor any changes
static void kqueue_add(int fd, int kq)
{
    struct kevent kev;
    EV_SET(&kev, kq, fd, EV_ADD, 0, 0, 0);
    kevent(kq, &kev, 1, NULL, 0, NULL);
}

// Remove a FD from kqueue
static void kqueue_del(int fd, int kq)
{
    struct kevent kev;
    EV_SET(&kev, kq, fd, EV_ADD, 0, 0, 0);
    kevent(kq, &kev, 1, NULL, 0, NULL);
}

int main(void)
{
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 'AF_INET=ipv4 -- 'SOCK_STREAM=TCP connection
    if (server_socket_fd == -1)
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

    int b = bind(server_socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
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
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0)
    {
        printf("SO_REUSEPORT failed: %s \n", strerror(errno));
        exit(1);
    }

    // Maximum length of the queue of pending connections
    const int connection_backlog = 10;

    /**
     * `listen()` indicates that the server socket is ready to accept incoming connections
     * returns 0 if connection was successful
     */
    if (listen(server_socket_fd, connection_backlog) != 0)
    {
        printf("Listen failed: %s \n", strerror(errno));
        exit(1);
    }
    set_nonblocking(server_socket_fd);

    printf("Server started in Port: %d \n", PORT);
    printf("\tWaiting for clients to connect...\n");

    int kq = kqueue();
    kqueue_add(server_socket_fd, kq);

    struct kevent events[MAX_EVENTS];

    while (1)
    {
        int n = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].ident;
        }
    }

    return 0;
}

