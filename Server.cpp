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

#define PORT "4000"
#define MAX_CLIENTS 20

class Server {
	private:
		int serverFd = -1,bindStatus = 0;
		struct addrinfo *res;
		std::vector<struct pollfd> pfds;
		std::vector<int> clientFds;
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

			serverFd = socket(AF_INET,SOCK_STREAM,0);

			if(serverFd<0) {
				std::cout << "Socket creation failed!" << std::endl;
				freeaddrinfo(res);
				return;

			}
			
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
						char buffer[1024];

						ssize_t bytesRecieved = recv(pfds[i].fd,buffer,sizeof(buffer)-1,0);

						if(bytesRecieved > 0) {
							buffer[bytesRecieved] = '\0';

							std::string msg(buffer);
							std::cout << "User sent " << msg << std::endl;

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
		}
};

int main() {
	Server chatServer;

	chatServer.run();

	return 0;
}
