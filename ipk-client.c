#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <pwd.h>
#include <arpa/inet.h>
#include <netinet/in.h>


using namespace std;

#define BUFF_SIZE 1024

/**
*	structure for query
*/
typedef struct parametres {
		int port_number;
		 //1 == -n	2 == -f		3 == -l
		string operation;
		string server_hostname;
		string login;
		string prefix;
}Tparametres;

/**
*	function for parse arguments
*	@return Tparametres arguments, structure for query
*/
Tparametres arguments(int argc, char *argv[]){
	Tparametres param = {0,"", "", "", ""};
	if(argc < 6){
		fprintf(stderr, "Invalid arguments\n");
		exit(EXIT_FAILURE);
	}
	//hostname
	if(!strcmp(argv[1], "-h")){
		param.server_hostname = argv[2];
		//portnumber
		if(!strcmp(argv[3], "-p")){
			param.port_number = atoi(argv[4]);
			/*
				options			
			*/
			//Full name		
			if(!strcmp(argv[5], "-n")){
				param.operation = argv[5];
				//login
				if(argc == 7){
					param.login = argv[6];
				}
				else{
					fprintf(stderr, "Invalid arguments\nMissing login\n");
					exit(EXIT_FAILURE);
				}
			}
			//Home directory
			else if(!strcmp(argv[5], "-f")){
				param.operation = argv[5];
				//login
				if(argc == 7){
					param.login = argv[6];
				}
				else{
					fprintf(stderr, "Invalid arguments\nMissing login\n");
					exit(EXIT_FAILURE);
				}
			}
			//List of all users
			else if(!strcmp(argv[5], "-l")){
				param.operation = argv[5];
				if(argc == 6){
					param.prefix = "";
				}
				else if (argc == 7){
					param.prefix = argv[6];
				}
				else{
					fprintf(stderr, "Invalid arguments\nToo many for -l option\n");
					exit(EXIT_FAILURE);
				}
			}
			else{
				fprintf(stderr, "Invalid arguments\nOption[-n|-f|-l] missing\n");
				exit(EXIT_FAILURE);
			}
		
		}
		else{
			fprintf(stderr, "Invalid arguments\n-p missing\n");
			exit(EXIT_FAILURE);
		}
	}
	else{
		fprintf(stderr, "Invalid arguments\n-h missing\n");
		exit(EXIT_FAILURE);
	}
	//return structure
	return param;	
}


/**
*	Function for connect
*	@param Tparametres param, structure of query
*	@param int *sock, socket descriptor
*	@return 0, if succesfull
**/
int connection(Tparametres param, int *sock){
	struct hostent *server;
	struct sockaddr_in server_address;
	int client_socket;

	if((server = gethostbyname((param.server_hostname).c_str())) == NULL){
		fprintf(stderr, "Gethostbyname error\n");
		exit(EXIT_FAILURE);
	}

	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
		fprintf(stderr, "Error by creating socket\n");
		exit(EXIT_FAILURE);
	}

	//initialization of structure server_adress
	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
	server_address.sin_port = htons(param.port_number);

	if(connect(client_socket, (const struct sockaddr *) &server_address, sizeof(server_address)) != 0){
		fprintf(stderr, "Error by connecting\n");
		exit(EXIT_FAILURE);
	}
	
	//access to socket descriptor
	*sock = client_socket;
	return 0;
}
/**
*	Function for communication with server
*	@param Tparametres param, structure for query
*	@param int *sock, socket descriptor
**/

int chat(Tparametres param, int *sock){
	//send
	int sendb;
	//recv
	int recvb;
	char message[BUFF_SIZE];
	memset(message, 0, BUFF_SIZE);

	//greeting message :D
	string greeting("IPK2018 PROTOCOL version 10.3\nHi mate, can I ask you something?");

	//sending to server
	sendb = send(*sock, (greeting).c_str(), greeting.size(), 0);
	if(sendb < 0){
		cerr<<"Error by sending message"<<endl;
		exit(EXIT_FAILURE);
	}
	//recieving from server
	recvb = recv(*sock, message, BUFF_SIZE, 0);
	if(recvb < 0){
		cerr<<"Error by recieving message"<<endl;
		exit(EXIT_FAILURE);
	}

	//comparing expected answer
	if(!strcmp(message, "IPK2018 PROTOCOL version 10.3\nHi, yes, of course")){
		memset(message, 0, BUFF_SIZE);
		//sending query -n or -f
		if(param.operation == "-n" || param.operation == "-f"){
			string send_data("&selected operation: "+param.operation+"&for user: "+
			param.login);
			sendb = send(*sock, (send_data).c_str(), send_data.size(), 0);
			if(sendb < 0){
				cerr<<"Error by sending message"<<endl;
				exit(EXIT_FAILURE);
			}
			recvb = recv(*sock, message, BUFF_SIZE, 0);
			if(recvb < 0){
				cerr<<"Error by recieving message"<<endl;
				exit(EXIT_FAILURE);	
			}
			//write message to STDOUT
			cout << message<<"\n";
		}
		//sending query -l
		else if(param.operation == "-l"){
			string send_data("&selected operation: "+param.operation+"&for user: "+param.prefix);
			sendb = send(*sock, (send_data).c_str(), send_data.size(), 0);
			if(sendb < 0){
				cerr<<"Error by sending message"<<endl;
				exit(EXIT_FAILURE);
			}
			memset(message, 0, BUFF_SIZE);
			while((recvb = recv(*sock, message, BUFF_SIZE, 0))>0){
				if(recvb < 0){
					cerr<<"Error by recieving message"<<endl;
					exit(EXIT_FAILURE);	
				}
				cout << message<<"\n";
				memset(message, 0, strlen(message));
			}
		}
		else{
			cerr<<"Protocol error\n";
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}

int main(int argc, char *argv[]){
	int client_socket;
	Tparametres param;
	param = arguments(argc, argv);
	connection(param, &client_socket);
	
	//communication with server
	chat( param, &client_socket);
	close(client_socket);
	return 0;
}
