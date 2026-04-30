# Chat App C++

A simple terminal-based client-server chat application using **C++ POSIX sockets**. This project demonstrates basic networking concepts, including address resolution, socket creation, and TCP communication.

## Features
*   **TCP/IP Networking**: Reliable data transfer using stream sockets.
*   **OOP Design**: The server is encapsulated within a `Server` class for better resource management (RAII).
*   **Dynamic Resolution**: Uses `getaddrinfo` for flexible host and port lookups.

## Project Structure
*   `server.cpp`: Handles incoming connections and prints received messages.
*   `client.cpp`: Connects to a specified server and sends user-input messages.

## How to Build
Ensure you are on a Linux/Unix-based system with `g++` installed.

1. **Compile the Server:**
   ```bash
   g++ server.cpp -o server
   ```

2. **Compile the Client:**
   ```bash
   g++ client.cpp -o client
   ```

## How to Run
1. **Start the Server** in one terminal:
   ```bash
   ./server
   ```
   The server will start listening on port `4000`.

2. **Start the Client** in a separate terminal:
   ```bash
   ./client 127.0.0.1 4000
   ```

3. **Chat**: Type messages in the client terminal to see them appear on the server. Type `end_session` in the client to quit, or send `~Server` to stop the server.

## Requirements
*   **OS**: Linux, macOS, or WSL (Windows Subsystem for Linux).
*   **Compiler**: GCC/G++ or Clang.

## Future Improvements
*  [ ] Support for multiple simultaneous clients using `poll()` or threading.
*  [ ] Implement a two-way chat interface.
*  [ ] Add data encryption for secure communication.
