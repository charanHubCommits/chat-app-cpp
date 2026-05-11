#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<sys/types.h>
#include<netdb.h>
#include<poll.h>
#include<vector>
#include<unordered_map>

#define PORT "4000"
#define MAX_CLIENTS 20

class Server {
	private:
		int serverFd = -1,bindStatus = 0;
		struct addrinfo *res = nullptr;
		std::vector<struct pollfd> pfds;
		std::vector<int> clientFds;
		std::unordered_map<std::string ,int> clientMp;
	public:
		Server() {
			struct addrinfo hints;
			memset(&hints,0,sizeof hints);
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_flags = AI_PASSIVE;

			int status = getaddrinfo(NULL,PORT,&hints,&res);

			if(status!=0) {
				std::cout << "Cannot resolve addr" << gai_strerror(status) << std::endl;
				return;
			}

			serverFd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

			if(serverFd<0) {
				std::cout << "Socket creation failed!" << std::endl;
				freeaddrinfo(res);
				return;

			}
			int opt = 1;
			setsockopt(serverFd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
			
			bindStatus = bind(serverFd,res->ai_addr,res->ai_addrlen);

			if(bindStatus<0) {
				std::cout << "Bind Failed!" << std::endl;
				close(serverFd);
				freeaddrinfo(res);
				res = nullptr;
				serverFd = -1;
				return;
			}
			
			pfds.resize(MAX_CLIENTS+1);
			pfds[0].fd=serverFd;
			pfds[0].events = POLLIN;

			for(int i = 1;i<pfds.size();i++) {
				pfds[i].fd = -1;
				pfds[i].events = 0;
			}
		}

		void getPacketDetails(const std::string &packet,std::array<std::string,4> &packetDetails) {
			size_t senderIndex = packet.find("|");
			std::string sender = packet.substr(0,senderIndex);
			std::string remainingPacket = packet.substr(senderIndex+1);
							
			size_t receiverIndex = remainingPacket.find("|");	
			std::string receiver = remainingPacket.substr(0,receiverIndex);
			remainingPacket = remainingPacket.substr(receiverIndex+1);
			
			size_t msgIndex = remainingPacket.find("|");
			std::string message = remainingPacket.substr(0,msgIndex);

			std::string timeStamp = remainingPacket.substr(msgIndex+1);

			packetDetails = {sender,receiver,message,timeStamp};
		}

		void run() {
			if(serverFd < 0 || bindStatus < 0) {
				std::cout << "Server not available!" << std::endl;
				return;
			}

			listen(serverFd,5);
			std::cout << "Server running at port: " << PORT << "Max clients: " << MAX_CLIENTS<< std::endl;

			while(true) {
				int num_fds = 1;

				for(int i=1;i<pfds.size();i++) {
					if(pfds[i].fd!=-1) num_fds = i+1;
				}

				int pollCount = poll(pfds.data(),num_fds,-1);

				if(pollCount < 0) {
					std::cerr << "Poll error!\n";

					break;
				}

				if(pfds[0].revents & POLLIN) {
					sockaddr_in clientAddr{};
					socklen_t clientAddrLen = sizeof(clientAddr);


					int clientFd = accept(serverFd,(sockaddr*)&clientAddr,&clientAddrLen);

					if(clientFd < 0) {
						continue;
					}

					for(int i=0;i<pfds.size();i++) {
						if(pfds[i].fd == -1) {
							pfds[i].fd = clientFd;
							pfds[i].events = POLLIN;
							pfds[i].revents = 0;

							std::cout<<"New client connected" << std::endl;
							char username[20];
							ssize_t n = recv(clientFd,username,sizeof(username)-1,0);
							if(n<=0) {
								close(clientFd);
								continue;
							}
							username[n] = '\0';
							std::string name(username);
							while(clientMp.find(name)!=clientMp.end()) {
								std::string warn = "Username already exists!\nEnter another username: ";
								send(clientFd,warn.c_str(),warn.size(),0);
								memset(username,0,sizeof(username)-1);

							 	n = recv(clientFd,username,sizeof(username)-1,0);
								if(n<=0) {
									close(clientFd);
									continue;
								}
								name = std::string(username);
							}


							clientMp[name] = clientFd;
							std::cout << "New user added " << name <<" " << clientFd << std::endl;
							break;
						}
					}

					if(pfds[num_fds-1].fd !=-1 && num_fds > MAX_CLIENTS) {
						close(clientFd);
					}

					if(--pollCount <=0) continue;
				}

				for(int i=1;i<num_fds;i++) {
					if(pfds[i].fd==-1) continue;

					if(pfds[i].revents & (POLLIN | POLLERR | POLLHUP)) {
						char buffer[2048];

						ssize_t bytesReceived = recv(pfds[i].fd,buffer,sizeof(buffer)-1,0);

						if(bytesReceived > 0) {
							buffer[bytesReceived] = '\0';

							std::string packet(buffer);
							std::cout << "User sent " << packet << std::endl;

							std::array<std::string, 4> packetDetails;
							getPacketDetails(packet,packetDetails);

							/* PacketDetails
							 * index 0 - sender
							 * index 1 - receiver
							 * index 2 - message
							 * index 3 - timeStamp */

							std::string receiver = packetDetails[1];
	
							if(clientMp.find(receiver) != clientMp.end()) {
								std::string finalMsg = packetDetails[0]+": "+packetDetails[2]+" "+packetDetails[3];
								send(clientMp[receiver],finalMsg.c_str(),finalMsg.size(),0);
								std::string ack = "msg sent";
								send(pfds[i].fd,ack.c_str(),ack.size(),0);
							}
							else {
								std::string err = "user not found";
								send(pfds[i].fd,err.c_str(),err.size(),0);
							}
						}else if(bytesReceived <= 0) {
							close(pfds[i].fd);
							for(auto it = clientMp.begin(); it != clientMp.end(); ) {
    								if(it->second == pfds[i].fd) {
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
			freeaddrinfo(res);
			if(serverFd >=0) {
				close(serverFd);
			}

			for(int i=1;i<pfds.size();i++) {
				if(pfds[i].fd!=-1) {
					close(pfds[i].fd);
				}
			}
		}
};

int main() {
	Server chatServer;

	chatServer.run();

	return 0;
}
