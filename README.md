# Chat App C++

Terminal-based multi-client chat application built with C++ and POSIX sockets.

The project contains:
- A TCP server that accepts multiple clients (up to 20).
- A client program that connects to the server, sets a username, sends direct messages, and listens for incoming messages concurrently.

## Features
- TCP socket communication using `AF_INET` and `SOCK_STREAM`.
- Address resolution with `getaddrinfo`.
- Poll-based server loop (`poll`) for handling multiple client sockets.
- Username registration and duplicate-name rejection on the server.
- Direct user-to-user messaging in the format: `<receiver_username> <message>`.
- Concurrent send/receive on the client using `std::thread`.

## Project Structure
- `Server.cpp` - Server implementation and entry point.
- `clientSocket.cpp` - Client implementation and entry point.
- `LICENSE` - Project license.

## Protocol / Message Flow
1. Client connects to the server.
2. Client sends a username.
3. Server checks whether the username is unique.
4. Client sends messages as:
   - `<receiver_username> <message>`
5. Client app appends sender info internally as `:<sender_username>` before transmission.
6. Server routes the message to the receiver (if found), and sends status to sender:
   - `msg sent` when parsing succeeds.
   - `user not found` when target username does not exist.
   - `Invalid format` when message format is incorrect.

## Build
Use a Unix-like system (Linux/macOS/WSL) with a C++ compiler.

```bash
g++ -std=c++17 Server.cpp -o server
g++ -std=c++17 clientSocket.cpp -o client -pthread
```

## Run
Start server (default port is `4000`):

```bash
./server
```

Start one or more clients:

```bash
./client 127.0.0.1 4000
```

## Client Usage
- On connect, enter your username.
- Send messages using:
  - `<receiver_username> <message>`
- Type `end_session` to disconnect the client.

## Notes and Current Limitations
- Maximum simultaneous clients: 20.
- Server uses IPv4 (`AF_INET`) only.
- Messages are plain text (no encryption).
- This is currently direct messaging only (no broadcast rooms/channels).

## Future Improvements
- Add broadcast/group chat support.
- Add authentication and password handling.
- Add TLS or message encryption.
- Improve protocol framing and validation for larger/structured messages.
