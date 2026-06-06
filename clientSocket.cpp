#include <chrono>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

class TcpSocket {
public:
  int sockFd;
  struct addrinfo *res;

  TcpSocket() : sockFd(-1), res(nullptr) {
    /*struct addrinfo hints;

    memset(&hints,0,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(ip,PORT,&hints,&res);

    if(status!=0) {
            std::cerr << "Couldn't connect to server!" << std::endl;

            return;
    }

    sockFd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

    if(sockFd == -1) {
            std::cerr << "Failed to create socket!" << std::endl;

            return;
    }*/
  }

  void setAddrInfo(std::string &ip, std::string &port) {
    struct addrinfo hints;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &res);

    if (status != 0) {
      std::cerr << "Couldn't connect to server!" << std::endl;

      return;
    }

    sockFd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sockFd == -1) {
      std::cerr << "Failed to create socket!" << std::endl;

      return;
    }
  }

  ~TcpSocket() {
    if (sockFd != -1)
      close(sockFd);
    if (res != nullptr)
      freeaddrinfo(res);
  }
};

class Client {
private:
  TcpSocket sock;
  std::string username;

public:
  Client(std::string &ip, std::string &port) { sock.setAddrInfo(ip, port); }

  void sendMsg(std::string &msg) {
    auto time = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::string timeStamp = (std::string)ctime(&time_t);
    timeStamp.pop_back(); // removing \n

    int receiverIndex = msg.find(" ");
    std::string receiver = msg.substr(0, receiverIndex);
    std::string finalMsg = msg.substr(receiverIndex + 1);

    std::string packet =
        username + "|" + receiver + "|" + finalMsg + "|" + timeStamp;

    ssize_t sent = send(sock.sockFd, packet.c_str(), packet.size(), 0);

    if (sent <= 0) {
      std::cout << "Failed to send\n";
      return;
    }
  }

  void chat() {
    int isConnected =
        connect(sock.sockFd, sock.res->ai_addr, sock.res->ai_addrlen);

    if (isConnected < 0) {
      std::cout << "Failed to connect" << std::endl;

      return;
    }

    std::cout << "Connected successfully\n";
    std::cout << "Enter username to register\n";

    std::getline(std::cin, username);
    send(sock.sockFd, username.c_str(), username.size(), 0);

    std::thread recvThread(&Client::receiveMsg, this);

    std::cout << "Enter 'end_session' to end this session\nEnter username and "
                 "msg in format: ";
    std::cout << "<username> <message> to send a msg" << std::endl;

    std::string msg;

    while (true) {
      std::getline(std::cin, msg);

      if (msg == "end_session") {
        break;
      }
      if (msg.find(" ") != std::string::npos) {
        sendMsg(msg);
        continue;
      }

      std::cout << "Enter valid message format!" << std::endl;
    }
    shutdown(sock.sockFd, SHUT_RDWR);
    recvThread.join();
  }

  void receiveMsg() {
    char buffer[1024];

    while (true) {
      ssize_t n = recv(sock.sockFd, buffer, sizeof(buffer), 0);
      if (n <= 0) {
        std::cout << "Disconnected from Server!\n";
        break;
      }
      buffer[n] = '\0';
      std::cout << buffer << std::endl;
    }
  }

  ~Client() { std::cout << "Connection closed!" << std::endl; }
};

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cout << "enter a ip and port of the server" << std::endl;

    return 0;
  }

  std::string ip(argv[1]);
  std::string port(argv[2]);

  Client user(ip, port);

  user.chat();

  return 0;
}
