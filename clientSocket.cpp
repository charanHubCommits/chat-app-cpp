#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<string>
#include<cstring>
#include<unistd.h>
#include<netdb.h>
#include<thread>
#include<chrono>

class Client {
	private:
	int sockFd;
	struct addrinfo *res;
	std::string username;
	public:
	Client(const char* ip,const char* port):sockFd(-1), res(nullptr),username("unknown") {
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

		std::cout << "Connected successfully\nSet a username" << std::endl;

		std::getline(std::cin,username);	
		send(sockFd,username.c_str(),username.size(),0);
		std::cout << "Enter 'end_session' to end this session\nEnter username and msg in format: ";
		std::cout << "<username> <message> to send a msg" << std::endl;	
			
		std::thread recvThread(&Client::receiveMsg,this);
		
		std::string msg;

		while(true) {
    			std::getline(std::cin,msg);

    			if(msg == "end_session") {
       				 break;
   			 }
			if(msg.find(" ")!=std::string::npos) {
				auto time = std::chrono::system_clock::now();
				auto time_t = std::chrono::system_clock::to_time_t(time);
				std::string timeStamp = (std::string)ctime(&time_t);
				timeStamp.pop_back(); //removing \n

				int receiverIndex = msg.find(" ");
				std::string receiver = msg.substr(0,receiverIndex);
				std::string finalMsg = msg.substr(receiverIndex+1);
				
				std::string packet = username+"|"+receiver+"|"+finalMsg+"|"+timeStamp;

				ssize_t sent = send(sockFd,packet.c_str(),packet.size(),0);

    				if(sent <= 0) {
        				std::cout << "Failed to send\n";
        				break;
   				}
				continue;
			}
			
			std::cout << "Enter valid message format!" << std::endl;
    		}
		shutdown(sockFd,SHUT_RDWR);
		recvThread.join();
	}

	void receiveMsg() {
		char buffer[1024];

		while(true){
			ssize_t n = recv(sockFd,buffer,sizeof(buffer),0);
			if(n<=0) {
				std::cout << "Disconnected from Server!\n";
				break;
			}
			buffer[n]= '\0';
			std::cout << buffer << std::endl; 

		}
	}

	~Client() {
		std::cout << "Connection closed!" <<std::endl;
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


