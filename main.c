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

#define PORT 8081
#define MAX_EVENTS 64
#define BUFFER_SIZE 4096
#define ABORT_ON_ERROR(_fd) do {                            \
    if (_fd == -1) {                                        \
        printf("operation failed: %s \n", strerror(errno)); \
        exit(1);                                            \
    }                                                       \
} while(0)

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
    ABORT_ON_ERROR(flags);
    int fn = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    ABORT_ON_ERROR(fn);
}

// Register a FD to Kqueue to monitor any changes
static void kqueue_add(int fd, int kq, int filt)
{
    struct kevent kev;
    EV_SET(&kev, fd, filt, EV_ADD, 0, 0, NULL);
    int result = kevent(kq, &kev, 1, NULL, 0, NULL);
    ABORT_ON_ERROR(result);
}

// Remove a FD from kqueue
static void kqueue_del(int fd, int kq, int filt)
{
    struct kevent kev;
    EV_SET(&kev, fd, filt, EV_DELETE, 0, 0, NULL);
    int result = kevent(kq, &kev, 1, NULL, 0, NULL);
    ABORT_ON_ERROR(result);
}

int main(void)
{
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 'AF_INET=ipv4 -- 'SOCK_STREAM=TCP connection
    ABORT_ON_ERROR(server_socket_fd);

    /*
     * sin_port = htons(PORT): the port that the socket will listen. Convert the 'bytes order/endianness to Big-Endian
     * sin_addr = INADDR_ANY:  Will allow any connections from anywhere.
     */
    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    /**
     * Since the tester restarts your program quite often, setting REUSE_PORT
     * ensures that we don't run into 'Address already in use' errors
     */
    const int reuse = 1;
    int setsockopt_result = setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    ABORT_ON_ERROR(setsockopt_result);

    int b = bind(server_socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    ABORT_ON_ERROR(b);

    // Maximum length of the queue of pending connections
    const int connection_backlog = 10;

    /**
     * `listen()` indicates that the server socket is ready to accept incoming connections
     * returns 0 if connection was successful
     */
    int listen_result = listen(server_socket_fd, connection_backlog);
    ABORT_ON_ERROR(listen_result);
    set_nonblocking(server_socket_fd);

    printf("Server started in Port: %d \n", PORT);
    printf("\tWaiting for clients to connect...\n");

    int kq = kqueue();
    ABORT_ON_ERROR(kq);
    kqueue_add(server_socket_fd, kq, EVFILT_READ);

    struct kevent events[MAX_EVENTS];

    while (1)
    {
        int n = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        for (int i = 0; i < n; i++)
        {
            int fd = events[i].ident;

            /*
             * Check if the FD from kqueue equals to the server_fd
             * When you make a request to the PORT that the server_socket_fd is binded to; Appears as activity in kqueue
             */
            if (fd == server_socket_fd)
            {
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);

                int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_addr, &client_addr_size);

                if (client_socket_fd == EWOULDBLOCK)
                    continue;

                ABORT_ON_ERROR(client_socket_fd);

                printf("DEU CERTO \n");
            }
        }
    }

    return 0;
}

