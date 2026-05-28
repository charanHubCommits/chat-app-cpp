#include <array>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define PORT "4000"
#define MAX_CLIENTS 1024

class TcpSocket {
public:
  int sockFd = -1, bindStatus = 0;
  struct addrinfo *res = nullptr;

  TcpSocket() {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, PORT, &hints, &res);

    if (status != 0) {
      std::cout << "Cannot resolve addr" << gai_strerror(status) << std::endl;
      return;
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sockFd < 0) {
      std::cout << "Socket creation failed!" << std::endl;
      freeaddrinfo(res);
      return;
    }
    int opt = 1;
    setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bindStatus = bind(sockFd, res->ai_addr, res->ai_addrlen);

    if (bindStatus < 0) {
      std::cout << "Bind Failed!" << std::endl;
      close(sockFd);
      freeaddrinfo(res);
      res = nullptr;
      sockFd = -1;
      return;
    }
  }

  ~TcpSocket() {
    freeaddrinfo(res);
    if (sockFd >= 0) {
      close(sockFd);
    }
  }
};

class Server {
private:
  TcpSocket sock;
  std::vector<struct pollfd> pfds;
  std::vector<int> clientFds;
  std::unordered_map<std::string, int> clientMp;
  std::unordered_map<int, bool> isRegistered;

public:
  Server() {
    pfds.resize(MAX_CLIENTS + 1);
    pfds[0].fd = sock.sockFd;
    pfds[0].events = POLLIN;

    for (int i = 1; i < pfds.size(); i++) {
      pfds[i].fd = -1;
      pfds[i].events = 0;
    }
  }

  void getPacketDetails(const std::string &packet,
                        std::array<std::string, 4> &packetDetails) {
    size_t senderIndex = packet.find("|");
    std::string sender = packet.substr(0, senderIndex);
    std::string remainingPacket = packet.substr(senderIndex + 1);

    size_t receiverIndex = remainingPacket.find("|");
    std::string receiver = remainingPacket.substr(0, receiverIndex);
    remainingPacket = remainingPacket.substr(receiverIndex + 1);

    size_t msgIndex = remainingPacket.find("|");
    std::string message = remainingPacket.substr(0, msgIndex);

    std::string timeStamp = remainingPacket.substr(msgIndex + 1);

    packetDetails = {sender, receiver, message, timeStamp};
  }

  void run() {
    if (sock.sockFd < 0 || sock.bindStatus < 0) {
      std::cout << "Server not available!" << std::endl;
      return;
    }

    listen(sock.sockFd, 5);
    std::cout << "Server running at port: " << PORT
              << "Max clients: " << MAX_CLIENTS << std::endl;

    while (true) {
      int num_fds = 1;

      for (int i = 1; i < pfds.size(); i++) {
        if (pfds[i].fd != -1)
          num_fds = i + 1;
      }

      int pollCount = poll(pfds.data(), num_fds, -1);

      if (pollCount < 0) {
        std::cerr << "Poll error!\n";

        break;
      }

      if (pfds[0].revents & POLLIN) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        int clientFd =
            accept(sock.sockFd, (sockaddr *)&clientAddr, &clientAddrLen);

        if (clientFd < 0) {
          continue;
        }

        isRegistered[clientFd] = false;

        for (int i = 0; i < pfds.size(); i++) {
          if (pfds[i].fd == -1) {
            pfds[i].fd = clientFd;
            pfds[i].events = POLLIN;
            pfds[i].revents = 0;

            std::cout << "New client connected" << std::endl;
            break;
          }
        }

        if (pfds[num_fds - 1].fd != -1 && num_fds > MAX_CLIENTS) {
          close(clientFd);
        }

        if (--pollCount <= 0)
          continue;
      }

      for (int i = 1; i < num_fds; i++) {
        if (pfds[i].fd == -1)
          continue;

        if (pfds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
          char buffer[2048];

          ssize_t bytesReceived =
              recv(pfds[i].fd, buffer, sizeof(buffer) - 1, 0);

          if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            std::string packet(buffer);
            std::cout << "User sent " << packet << std::endl;

            if (!isRegistered[pfds[i].fd]) {
              if (clientMp.find(packet) != clientMp.end()) {
                std::string err = "Username already exists!";
                send(pfds[i].fd, err.c_str(), err.size(), 0);
              } else {
                clientMp[packet] = pfds[i].fd;
                isRegistered[pfds[i].fd] = true;
                std::string ack = "Registerd successfully!";
                send(pfds[i].fd, ack.c_str(), ack.size(), 0);
              }
            } else {
              std::array<std::string, 4> packetDetails;
              getPacketDetails(packet, packetDetails);

              /* PacketDetails
               * index 0 - sender
               * index 1 - receiver
               * index 2 - message
               * index 3 - timeStamp */

              std::string receiver = packetDetails[1];

              if (clientMp.find(receiver) != clientMp.end()) {
                std::string finalMsg = packetDetails[0] + ": " +
                                       packetDetails[2] + " |" +
                                       packetDetails[3];
                send(clientMp[receiver], finalMsg.c_str(), finalMsg.size(), 0);
                std::string ack = "msg sent";
                send(pfds[i].fd, ack.c_str(), ack.size(), 0);
              } else {
                std::string err = "user not found";
                send(pfds[i].fd, err.c_str(), err.size(), 0);
              }
            }

          } else if (bytesReceived <= 0) {
            close(pfds[i].fd);
            for (auto it = clientMp.begin(); it != clientMp.end();) {
              if (it->second == pfds[i].fd) {
                it = clientMp.erase(it);
              } else {
                ++it;
              }
            }

            pfds[i].fd = -1;

            continue;
          }
        }
      }
    }
  }

  ~Server() {
    for (int i = 1; i < pfds.size(); i++) {
      if (pfds[i].fd != -1) {
        close(pfds[i].fd);
      }
    }
    std::cout << "Session Ended" << std::endl;
  }
};

int main() {
  Server chatServer;

  chatServer.run();

  return 0;
}
