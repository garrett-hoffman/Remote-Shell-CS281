#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>

//main entry point for client
int main(int argc,char *argv[]){

		//set up server connection
		int sockfd, portno, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;

		char buffer[48];
		if (argc < 3) {
				fprintf(stderr,"usage %s hostname port\n", argv[0]);
				exit(0);
		}
		portno = atoi(argv[2]);
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
				perror("ERROR opening socket");
		server = gethostbyname(argv[1]);
		if (server == NULL) {
				fprintf(stderr,"ERROR, no such host\n");
				exit(0);
		}
		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr,
						(char *)&serv_addr.sin_addr.s_addr,
						server->h_length);
		serv_addr.sin_port = htons(portno);

		std::cout << "trying to connect..." << std::endl;

		//exits if it cannot connect to server
		if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
				perror("ERROR connecting, exiting...");
				sleep(1);
				exit(-1);
		}
		std::cout<<"connection successful"<<std::endl;

		//outputs sketchy welcome message
		std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<std::endl;
		std::cout<<"$$              WELCOME TO                 $$"<<std::endl;
		std::cout<<"$$          GARRETT AND NICK'S             $$"<<std::endl;
		std::cout<<"$$            REMOTE SHELL!!!	           $$"<<std::endl;
		std::cout<<"$$           host: " << argv[1] << "               $$"<<std::endl;
		std::cout<<"$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$"<<std::endl;

		//authenticates the client. 
		//promts for a password, sends password to server. If it receives a 'y' back, continues.
		bool authenticated;
		while(!authenticated) {
				//prompts for password
				std::cout<<"Password: ";
				bzero(buffer,48);
				fgets(buffer,47,stdin);

				//sends password to server
				n = write(sockfd,buffer,48);
				if(n < 0){
						std::cerr<<"error writing to socket"<<std::endl;
				}
				std::cout<<"Password sent, waiting on response from server..."<<std::endl<<std::endl;

				//reads response from server
				bzero(buffer,48);
				n = read(sockfd,buffer,47);
				if(n < 0){
						std::cerr<<"error reading from socket"<<std::endl;
				}

				//if the server's response is 'y', proceeds
				if(strcmp(buffer, "y") == 0){
						std::cout<<"Correct password. Proceeding..."<<std::endl;
						authenticated = true;
				}

				//else, prints incorrect message and returns to beginning of loop
				else{
						std::cout<<"Password incorrect. Please try again."<<std::endl;
				}
		}

		//main while loop for receiving commands and sending to server
		while (true){
				//prompt user for input, listen for response from server
				std::cout<<argv[1] << " >>> ";
				bzero(buffer,48);
				fgets(buffer,47,stdin);

				//if the command entered is 'exit', exit program
				if(strcmp(buffer,"exit\n") == 0){
						std::cout<<"exiting..."<<std::endl;
						sleep(1);
						exit(1);
				}

				//sends command to server
				n = write(sockfd,buffer,48);
				if(n < 0){
						std::cerr<<"error writing to socket"<<std::endl;
				}

				std::cout<<"command sent, waiting on response from server..."<<std::endl<<std::endl;

				//while loop to receive command output from server
				//repeats until it receives 'end'
				while(true){
						bzero(buffer,48);
						n = read(sockfd,buffer,47);                                     
						if(n < 0){
								std::cerr<<"error reading from socket"<<std::endl; 
						}

						//if the input from server is 'end', break out of loop						
						if(strcmp(buffer, "end") == 0){
								std::cout<<std::endl<<buffer<<std::endl;
								break;
						}
						//prints the output to stdout
						std::cout<<buffer;
				}
		}

}
