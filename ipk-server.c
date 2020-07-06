#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <pwd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define OPERATION 21
#define FORUSER 11
#define QUEUE 5
#define BUFF_SIZE 1024

using namespace std;
std::ifstream infile("/etc/passwd");

typedef struct parametres {
		int port_number;
		//1 == -n	2 == -f		3 == -l
		string operation;
		string server_hostname;
		string login;
		string prefix;
}Tparametres;

/***
*	Function for parse arguments
*	@return int portnumber
*/
int arguments(int argc, char *argv[]){
	int port_number;	
	if(argc == 3){
		if(!strcmp(argv[1], "-p")){
			port_number = atoi(argv[2]);
			return port_number;
		}
		else{
			fprintf(stderr, "Expected -p <port number>\n");
			exit(EXIT_FAILURE);
		}
	}
	else{
		fprintf(stderr, "Expected -p <port number>\n");
		exit(EXIT_FAILURE);	
	}	
}
/**
*	function for connection
*	@param int port, port number
*	@param struct sockaddr_in *serv, structure for binding
*	@return 0 if success
**/
int connection(int port, int *sock, struct sockaddr_in *serv){
	int s, rc;
	struct sockaddr_in server;
	
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "Error by creating socket\n");
		exit(EXIT_FAILURE);
	}
	
	memset(&server, 0, sizeof(server));
	server.sin_family = PF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

	int reuse = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))<0){
		cerr<<"Reuse addr error\n";
		exit(EXIT_FAILURE);
	}
	#ifdef SO_REUSEPORT
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse))<0){
		cerr<<"Reuse port error\n";
		exit(EXIT_FAILURE);
	}
	#endif
	if((rc = bind(s, (struct sockaddr *)&server, sizeof(server))) < 0){
		fprintf(stderr, "Bind error\nTry change port number\n");
		exit(EXIT_FAILURE);
	}

	if(listen(s, QUEUE) < 0){
		fprintf(stderr, "Listen error\n");
		exit(EXIT_FAILURE);
	}

	*sock = s;
	*serv = server;
	
	return 0;
}

/**
*	function for communication with client
*/
int communication(int *socket){
	char buffer[1024];
	int r = *socket;
	struct passwd *f;
	Tparametres param;
	char com[1024];
	memset(com, 0, 1024);
	
	int k;
	k = recv(r, com, 1024, 0);
	if(k < 0){
		cerr<<"Recv error\n";
		exit(EXIT_FAILURE);
	}

	string a;
	a.clear();
	a.append(com);
	//comparing with expected protocol form :D
	if(!(a.compare("IPK2018 PROTOCOL version 10.3\nHi mate, can I ask you something?"))){
		int b = send(r, "IPK2018 PROTOCOL version 10.3\nHi, yes, of course", 48, 0);
		if(b < 0){
			cerr<<"Send error\n";
			exit(EXIT_FAILURE);
		}
		memset(com, 0, 1024);				
		k = recv(r, com, 1024, 0);
		if(k < 0){
			cerr<<"Recv error\n";
			exit(EXIT_FAILURE);
		}

		string parse;
		parse.clear();
		parse.append(com);

		if(parse.substr(0, OPERATION)!= "&selected operation: "){
			cerr << "Protocol error\n";
			exit(EXIT_FAILURE);
		}
		parse.erase(0, OPERATION);
		//assignment [-n|-f|-l]
		param.operation = parse.substr(0, 2);
		parse.erase(0, 2);

		//if option -n
		if(param.operation == "-n"){
			if(parse.substr(0, FORUSER)!= "&for user: "){
				cerr << "Protocol error\n";
				exit(EXIT_FAILURE);
			}
			parse.erase(0, FORUSER);
			if((f = getpwnam((parse).c_str())) != NULL){
				send(r, f->pw_gecos, strlen(f->pw_gecos), 0);
			}
			else{
				send(r, "Can not find user\n", strlen("Can not find user\n"), 0);
			}
		}
		//if option -f
		else if(param.operation == "-f"){
			if(parse.substr(0, FORUSER)!= "&for user: "){
				cerr << "Protocol error\n";
				exit(EXIT_FAILURE);
			}
			parse.erase(0, FORUSER);
			if((f = getpwnam((parse).c_str())) != NULL){
				send(r, f->pw_dir, strlen(f->pw_dir), 0);
			}
			else{
				send(r, "Can not find user with this login\n", strlen("Can not find user with this login\n"), 0);
			}
		}
		//if option -l
		else if(param.operation == "-l"){
			if(parse.substr(0, FORUSER)!= "&for user: "){
				cerr << "Protocol error\n";
				exit(EXIT_FAILURE);
			}
			parse.erase(0, FORUSER);
			//prefix in parse		
			while(infile){	
				infile.getline(buffer, 1024);
				char result[1024];
				memset(result, 0, sizeof(result));
				for(unsigned int i = 0; i<strlen(buffer); i++){
					//write only login					
					if(buffer[i] != ':'){
						result[i] = buffer[i];
					}
					else{
						break;
					}
				}
			//if we have prefix
			if(parse != ""){
				int length = parse.length();
				//comparing logins with prefix
				if(!strncmp(result, parse.c_str(), length)){
					send(r, result, 1024, 0);
					memset(buffer, 0, sizeof(buffer));
				}
			}
			//without prefix, write whole list
			else{
				send(r, result, 1024, 0);
				memset(buffer, 0, sizeof(buffer));
			}
						
		}
		//back to the beginnig of file...due multiple reading from file
		infile.clear();
		infile.seekg(0, ios::beg);
		}
	
		
	}

	*socket = r;
	return 0;	
}

int main(int argc, char *argv[]){
	//get port number from arguments
	int port_number = arguments(argc, argv);	
	int server_socket;
	int comming_socket;
	socklen_t sa_len;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	//connecting function
	connection(port_number, &server_socket, &serv);
	sa_len = sizeof(serv);
	

	while(1){
		//accept
		if((comming_socket = accept(server_socket, (struct sockaddr *)&serv, &sa_len)) < 0 ){
			cerr<<"Acept error"<<endl;
			exit(EXIT_FAILURE);
		}
		//communication with server
		communication(&comming_socket);
		if(close(comming_socket) < 0){
			cerr<<"Closing error"<<endl;
			exit(EXIT_FAILURE);
		}
	}
	return 0;	
}

