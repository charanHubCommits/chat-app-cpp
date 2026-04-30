#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<unistd.h>
#include<string>
#include<cstring>
#include<sys/types.h>
#include<netdb.h>

#define PORT "4000"

class Server {
	private:
		int serverFd = -1,bindStatus = 0;
		struct addrinfo hints,*res;
	public:
		Server() {
			while(true) {
				memset(&hints,0,sizeof hints);
				hints.ai_family = AF_INET;
				hints.ai_socktype = SOCK_STREAM;
				hints.ai_flags = AI_PASSIVE;

				int status = getaddrinfo(NULL,PORT,&hints,&res);

				if(status!=0) {
				std::cout << "Cannot resolve addr" << std::endl;
				break;
				}

				serverFd = socket(AF_INET,SOCK_STREAM,0);

				if(serverFd<0) {
					std::cout << "Socket creation failed!" << std::endl;
					break;

				}
			
				bindStatus = bind(serverFd,res->ai_addr,res->ai_addrlen);

				if(bindStatus<0) {
					std::cout << "Bind Failed!" << std::endl;
					break;
				}

				break;
			}
		}
		void run() {
			if(serverFd < 0 || bindStatus < 0) {
				std::cout << "Server not available!" << std::endl;
				return;
			}

			listen(serverFd,5);
			
			sockaddr_in clientAddr{};
			socklen_t clientAddrLen = sizeof(clientAddr);


			int clientFd = accept(serverFd,(sockaddr*)&clientAddr,&clientAddrLen);

			char buffer[1024];

			while(true){
				read(clientFd,buffer,sizeof(buffer)-1);
				std::string msg = buffer;
				if(msg=="~Server"){
					std::cout << "Server stopped" <<std::endl;
					break;
				}

				std::cout << "User sent " << msg << std::endl;
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

	std::cout << "Server running at port: " << PORT << std::endl;

	chatServer.run();

	return 0;
}
