#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<string>
#include<cstring>
#include<unistd.h>
#include<netdb.h>

int main(int argc,char **argv) {
	if(argc < 3 ) {
		std::cout << "enter a server address and port" << std::endl;

		return 0;
	}

	struct addrinfo hints,*res;

	memset(&hints,0,sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;



	int status = getaddrinfo(argv[1],argv[2],&hints,&res);

	if(status!=0) {
		std::cerr << "cannot get addr" << std::endl;

		return 1;
	}

	std::cout << "Protocol: " << res->ai_protocol << std::endl;

	int sock_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);

	if(sock_fd == -1) {
		std::cerr << "failed to create a socket";
		freeaddrinfo(res);
		return 2;
	}

	int isConnected=connect(sock_fd,res->ai_addr,res->ai_addrlen);

	if(isConnected < 0) {
		std::cout << "Failed to connect" << std::endl;
		freeaddrinfo(res);

		return 3;
	}

	std::cout << "Connected successfully" << std::endl;
	std::cout << "Enter 'end_session' to end this session" << std::endl;

		std::cout << "Enter a msg to send to server" << std::endl;

	while(1) {
		std::string msg;
		std::getline(std::cin,msg);

		if(msg=="end_session") break;
		msg+='\n';
		int len = msg.size();

		send(sock_fd,msg.c_str(),len,0);
	}
	close(sock_fd);
	freeaddrinfo(res);

	return 0;
}


