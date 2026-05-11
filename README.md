# Chat App (C++)

Terminal-based multi-client chat built with C++ and POSIX sockets. A small TCP server routes **direct** messages between registered clients; each client uses a background thread so incoming traffic does not block typing.

## What is in this repo

| File | Role |
|------|------|
| `Server.cpp` | TCP server: bind, `poll` loop, usernames, routing |
| `clientSocket.cpp` | TCP client: connect, username, send/receive threads |
| `LICENSE` | MIT License |

## Concepts Demonstrated

- TCP socket programming
- POSIX networking APIs
- I/O multiplexing with `poll`
- Concurrent programming with `std::thread`
- Client-server architecture
- Custom application-layer protocol design
- Connection lifecycle management
- Username/session management

## Architecture

```text
+-----------+        +----------------+        +-----------+
| Client A  | <----> | poll()-based   | <----> | Client B  |
| thread RX |        | TCP Server     |        | thread RX |
+-----------+        +----------------+        +-----------+

## Features

- TCP (`AF_INET`, `SOCK_STREAM`) with `getaddrinfo` for address resolution.
- Server multiplexes sockets with `poll` (up to **20** simultaneous clients, `MAX_CLIENTS`).
- First message after connect is the **username**; the server rejects duplicates and prompts for another name until one is free.
- **Direct** messaging: you send to a specific online username.
- Multi-threading for simultaneous receive and send operations.

## Wire format (client → server)

After registration, each outbound message is encoded using a pipe-delimited application protocol:

```text
<your_username>|<receiver_username>|<message_body>|<timestamp>
```

- `your_username` is whatever you entered at startup.
- `receiver_username` is the first token and `<message_body>` is the rest of the line after the first space (see usage below in Client usage section).
- `timestamp` is added by the client (`std::chrono` / `ctime`, trailing newline stripped).


## Server → client behavior

- **Delivered to recipient:** one line of the form `sender: message timestamp` (built from the parsed packet).
- **Back to sender (ack):** `msg sent` if the receiver is connected, or `user not found` if that username is not in the server map.
- **Duplicate username (during login):** `Username already exists!\nEnter another username: ` then the server waits for another username attempt.


## Build

Requires a Unix-like OS (Linux, macOS, WSL) and a C++17 toolchain (`g++` or compatible).

```bash
g++ -std=c++17 Server.cpp -o server
g++ -std=c++17 clientSocket.cpp -o client -pthread
```

## Run

Default server port is **4000** (`PORT` in `Server.cpp`).

```bash
./server
```

Client: pass **host** and **port** as arguments:

```bash
./client 127.0.0.1 4000
```

## Client usage

1. On connect, enter your username (must be unique among connected clients).
2. Send a direct message as:  
   `<receiver_username> <message>`  
   The first space separates receiver from the message; the message may contain spaces.
3. Type `end_session` to stop sending, shut down the socket, and exit after the receive thread finishes.

## Limits and caveats

- **IPv4 only** (`AF_INET` in server and client hints).
- **Plain text** over the wire (no TLS).
- **Direct messaging only** (no groups/channels).
- Server login reads the username into a **fixed buffer** (short names only in practice).
- Message sizes are bounded by fixed `recv` buffers on server and client.

## License

This project is released under the [MIT License](LICENSE) (Copyright (c) 2026 Charan Mandakuriti).

## Future extensions

- Broadcast or named channels.
- Authentication and TLS.
- Length-prefixed or framed messages and stricter validation for arbitrary payload size.
