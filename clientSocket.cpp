#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<string>
#include<cstring>
#include<unistd.h>
#include<netdb.h>

class Client {
	private:
	int sockFd;
	struct addrinfo *res;
	public:
	Client(const char* ip,const char* port):sockFd(-1), res(nullptr) {
		struct addrinfo hints;

		memset(&hints,0,sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int status = getaddrinfo(ip,port,&hints,&res);

		if(status!=0) {
		std::cerr << "cannot get addr" << std::endl;

		return;
		}

		std::cout << "Protocol: " << res->ai_protocol << std::endl;

		sockFd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

		if(sockFd == -1) {
			std::cerr << "failed to create a socket";
			freeaddrinfo(res);
			return;
		}

	}

	void chat() {
		int isConnected=connect(sockFd,res->ai_addr,res->ai_addrlen);

		if(isConnected < 0) {
			std::cout << "Failed to connect" << std::endl;
			freeaddrinfo(res);

			return;
		}

		std::cout << "Connected successfully" << std::endl;
		std::cout << "Enter 'end_session' to end this session" << std::endl;

		std::cout << "Enter a msg to send to server" << std::endl;

		while(1) {
			std::string msg;
			std::getline(std::cin,msg);

			if(msg=="end_session") break;
			int len = msg.size();

			int sentBytes = send(sockFd,msg.c_str(),len,0);

			char* buffer;
			int peekRes = recv(sockFd,buffer,1,MSG_PEEK);

			if(sentBytes <= 0 || peekRes==0) {
				std::cout << "Server closed!" << std::endl;
				break;
			}
		}	
	}

	~Client() {
		if(sockFd!=-1)
			close(sockFd);
		if(res!=nullptr)
			freeaddrinfo(res);
	}

};

int main(int argc,char **argv) {
	if(argc < 3 ) {
		std::cout << "enter a server address and port" << std::endl;

		return 0;
	}

	Client user(argv[1],argv[2]);

	user.chat();
			
	
	return 0;
}


