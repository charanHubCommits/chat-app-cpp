# Chat App (C++)

Terminal-based multi-client chat built with C++ and POSIX sockets. A TCP server routes **direct** messages between registered clients; each client runs a background receive thread so incoming traffic does not block typing.

## Repository layout

| File | Role |
|------|------|
| `Server.cpp` | TCP server: `getaddrinfo`, `poll` loop, username registry, message routing |
| `clientSocket.cpp` | TCP client: connect, register username, send/receive threads |
| `LICENSE` | MIT License |

## Architecture

```text
+-----------+        +------------------+        +-----------+
| Client A  | <----> | poll()-based     | <----> | Client B  |
| (RX thread)|        | TCP Server       |        | (RX thread)|
+-----------+        +------------------+        +-----------+
```

1. Client connects and sends a **username** as the first payload.
2. Server maps `username → socket fd` in an `unordered_map`.
3. Further outbound traffic uses a pipe-delimited packet; the server forwards to the receiver’s socket.

## Features

- **TCP** (`AF_INET`, `SOCK_STREAM`) with `getaddrinfo` for address resolution.
- **I/O multiplexing** via `poll()` on the listening socket and all client fds.
- **Up to 1024 clients** (`MAX_CLIENTS`); `SO_REUSEADDR` on the listening socket.
- **Username registration** on first message; duplicate names are rejected.
- **Direct messaging** to a specific online username.
- **Concurrent I/O** on the client: `std::thread` for `recv` while the main thread reads stdin.

## Concepts demonstrated

- TCP socket programming and connection lifecycle
- POSIX networking (`socket`, `bind`, `listen`, `accept`, `send`, `recv`, `poll`)
- I/O multiplexing with `poll`
- `std::thread` for concurrent receive/send
- Client–server architecture and a custom application-layer protocol
- Session/username management with in-memory maps

## Wire protocol

### Client → server (registration)

The first `send` after connect is the raw username string (no delimiter, no newline required by the protocol).

| Outcome | Server response |
|---------|-----------------|
| Username free | `Registerd successfully!` |
| Username taken | `Username already exists!` |

Until registration succeeds, every inbound message on that connection is treated as a username attempt.

### Client → server (message)

After registration, each message is pipe-delimited:

```text
<sender>|<receiver>|<message_body>|<timestamp>
```

- `sender` — local username entered at startup.
- `receiver` — first token before the first space in the user’s input line.
- `message_body` — remainder of the line after that space (may contain spaces).
- `timestamp` — added by the client via `std::chrono` / `ctime` (trailing newline stripped).

Example user input:

```text
alice Hello there
```

Encoded packet (conceptually):

```text
bob|alice|Hello there|Mon May 18 12:00:00 2026
```

### Server → client

| Case | Payload |
|------|---------|
| Delivered to recipient | `sender: message \|timestamp` |
| Ack to sender (receiver online) | `msg sent` |
| Ack to sender (receiver offline) | `user not found` |

## Build

Requires a Unix-like OS (Linux, macOS, WSL) and a C++17 toolchain (`g++` or compatible).

```bash
g++ -std=c++17 Server.cpp -o server
g++ -std=c++17 clientSocket.cpp -o client -pthread
```

## Run

The server listens on port **4000** (`PORT` in `Server.cpp`). Start the server, then one or more clients:

```bash
./server
```

```bash
./client 127.0.0.1 4000
```

The client requires **host** and **port** as arguments (`argc >= 3`).

## Client usage

1. Connect and enter a **unique** username when prompted.
2. Send a direct message as:  
   `<receiver_username> <message>`  
   The first space separates receiver from the body; the body may contain spaces.
3. Type `end_session` to stop sending, call `shutdown` on the socket, and exit after the receive thread finishes.

Invalid input (no space) prints: `Enter valid message format!`

## Implementation notes

| Topic | Detail |
|-------|--------|
| Server class layout | `TcpSocket` (bind/listen) + `Server` (`poll`, routing) |
| Client class layout | `TcpSocket` (resolve/connect) + `Client` (chat loop, threads) |
| Listen backlog | `5` (`listen(sock, 5)`) |
| Recv buffers | Server: 2048 bytes; client: 1024 bytes |
| Disconnect | Server closes fd, removes username from map, clears `pollfd` slot |

## Limits and caveats

- **IPv4 only** (`AF_INET` in server and client `addrinfo` hints).
- **Plain text** over the wire (no TLS or authentication).
- **Direct messaging only** — no broadcast, groups, or channels.
- **Port fixed in source** — server does not accept a CLI port override.
- **Username at connect** — if registration fails (duplicate name), the client must be restarted; there is no in-app re-prompt loop.
- **Message size** — bounded by fixed `recv` buffers; very long lines may be truncated or split unpredictably.
- **No framing** — messages rely on single `recv` calls fitting one logical packet.

## Future extensions

- Broadcast or named channels.
- Authentication and TLS.
- Configurable port/host via CLI or config file.
- Length-prefixed or framed messages for arbitrary payload size.
- Graceful duplicate-username retry without restarting the client.

## License

This project is released under the [MIT License](LICENSE) (Copyright (c) 2026 Charan Mandakuriti).
