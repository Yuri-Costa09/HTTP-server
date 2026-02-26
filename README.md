# HTTP Server in C

A blazingly fast, event-driven HTTP server built from scratch in C, leveraging the power of BSD kqueue for high-performance I/O multiplexing.

## What This Does

This is a minimal yet functional HTTP server that listens on port 8081 and responds to client requests with a simple HTTP/1.1 response. It demonstrates how modern servers handle multiple concurrent connections without spawning threads or processes, instead using a single event loop powered by kqueue.

When a client connects, the server accepts the connection, reads the HTTP request, and immediately responds with "Hello, World!" wrapped in proper HTTP headers. Pretty straightforward, actually.

## Core Concepts at Play

**File Descriptors**: Every open resource (sockets, files, pipes) gets an integer identifier that the kernel uses to track and manage it. This server juggles multiple file descriptors simultaneously.

**Non-blocking I/O**: The server socket and all client sockets are set to non-blocking mode, meaning operations return immediately instead of waiting. This allows the event loop to check on many connections without getting stuck.

**kqueue**: This is a BSD/macOS kernel facility for efficient event notification. Instead of checking all file descriptors to see which ones have activity (like select() does), kqueue tells you exactly which ones are ready. The events list only contains what actually happened, making it extremely efficient.

**Byte Order (Endianness)**: Network protocols expect big-endian byte order. The `htons()` function converts the port number to network byte order before binding.

## How It Works

1. Create a server socket and bind it to port 8081
2. Set the socket to non-blocking mode so it won't block the event loop
3. Register the server socket with kqueue to monitor for incoming connections
4. Enter an infinite event loop that:
   - Waits for kqueue to report events
   - When a client connects, accept the connection and register the new socket with kqueue
   - When a client sends data, read it from the socket
   - Send back an HTTP/1.1 response
   - Close the connection and remove it from kqueue

The `ABORT_ON_ERROR` macro is used throughout to ensure that any system call failure immediately exits the program with a descriptive error message.

## Building and Running

```bash
mkdir -p cmake-build-debug
cd cmake-build-debug
cmake ..
cmake --build .
./http_server
```

Then visit `http://localhost:8081` in your browser, or use curl:

```bash
curl http://localhost:8081
```

## Important Note

This code relies heavily on BSD/macOS-specific features, particularly kqueue. You probably wouldn't be able to run this on Linux or Windows without significant modifications. The kqueue API doesn't exist on those platforms. So just trust me that this works perfectly on a Mac. I tested it. It's good.

## Compilation Notes

The project uses CMake for building. Make sure you have a C compiler (clang or gcc) installed on your macOS system.

## Technical Highlights

- Zero dynamic memory allocation (everything uses the stack)
- Single-threaded event loop (no mutex nightmares)
- Efficient event notification via kqueue
- Proper HTTP/1.1 response formatting with Content-Length headers
- Error handling throughout using the ABORT_ON_ERROR macro

## Acknowledgments

Special thanks to [cabelitos](https://github.com/cabelitos) for the guidance and support during the development of this project. His insights on systems programming were invaluable, and also for challenging me on that ;)

