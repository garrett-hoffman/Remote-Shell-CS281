#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <string>

//checks pass against the password
bool authenticate(char pass[]){
		char password[48] = {'A','b','h','i','s','h','e','k','R','u','l','e','s','\n','\0'};
		if (strcmp(pass, password) == 0){
				return true;
		}
		else {
				return false;
		}
}

//parses line into an array argv
//from http://www.csl.mtu.edu/cs4411.ck/www/NOTES/process/fork/exec.html
void  parse(char *line, char **argv)
{
		while (*line != '\0') {       			/* if not the end of line ....... */ 
				while (*line == ' ' || *line == '\t' || *line == '\n'){
						*line++ = '\0'; 		/* replace white spaces with 0    */
				}
				*argv++ = line;          		/* save the argument position     */
				while (*line != '\0' && *line != ' ' && *line != '\t' && *line != '\n'){
						line++; 				/* skip chars until...*/
				}	
		}
		*argv = NULL;                 			/* mark the end of argument list  */
}

//TODO: make this code better. Causes failure for a string that is only multiple spaces
//and the if statement is really hacky 
//trims trailing characters target off of str
//based on http://stackoverflow.com/questions/25829143/c-trim-whitespace-from-a-string
std::string trim(std::string str,char target)
{
		if(str.compare("\n") == 0 || str.compare(" ") == 0|| str.length() == 0){
				return "";
		}
		size_t first = str.find_first_not_of(target);
		size_t last = str.find_last_not_of(target);
		return str.substr(first, (last-first+1));
}

//entry point for the server
int main(int argc, char *argv[]){

		//variables necessary for forking
		pid_t w;
		int status;  

		//sets up client/server connection
		int sockfd, newsockfd, portno;
		socklen_t clilen;
		struct sockaddr_in serv_addr, cli_addr;
		if (argc < 2) {
				fprintf(stderr,"ERROR, no port provided\n");
				exit(1);
		}
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
				perror("ERROR opening socket");
		bzero((char *) &serv_addr, sizeof(serv_addr));
		portno = atoi(argv[1]);
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = INADDR_ANY;
		serv_addr.sin_port = htons(portno);
		if (bind(sockfd, (struct sockaddr *) &serv_addr,
								sizeof(serv_addr)) < 0)
				perror("ERROR on binding");	 

		std::cout<<"Listening for connections..."<<std::endl;
		//server listens for new connections
		listen(sockfd,5);

		//new connection established	
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
		if (newsockfd < 0){		
				perror("ERROR on accept");
		}
		std::cout<<"Connection received. Authenticating... "<<std::endl;


		//buffer for receiving from the socket
		char buffer[48];
		int n;

		//authenticates the client. Loop allows multiple password attempts
		bool authenticated = false;
		while (!authenticated) {
				bzero(buffer,48);

				//message to send back to client on successful authentication
				char pgood[] = {'y','\0'};

				//reads password from user
				n = read(newsockfd,buffer,47);
				if(n < 0)
						perror("ERROR reading from socket");

				//if authenication succeeds, set authenticated flag to true 
				if (authenticate(buffer)) {
						std::cout<<"client entered correct password"<<std::endl;
						authenticated = true;
				}
				//if authentication fails, clear pgood buffer to send negative reponse back to client
				else{
						bzero(pgood,2);
				}

				//sends authentication result back to client
				std::cout<<"sending response to client: "<<pgood<<std::endl;
				n = write(newsockfd,pgood,2);

				//clears buffer
				bzero(buffer,48);	
				int r = read(newsockfd,buffer,47);
		}
		std::cout<<"Client connected"<<std::endl;		

		//infinite loop to receive commands from client, forks a new process each time
		while(true){

				//clears buffer		
				bzero(buffer,48);
				std::cout<<"waiting on client to enter command..."<<std::endl;

				//blocking call to read user command from socket
				n = read(newsockfd,buffer,48);
				if (n < 0) {
						perror("ERROR reading from socket");
				}

				//creates new process
				std::cout<<"about to fork..."<<std::endl;
				w = fork ();
				if (w == 0){
						//in child
						//duplicates file descriptors to send stdout and stderr back to client
						dup2(newsockfd,STDOUT_FILENO);
						dup2(newsockfd,STDERR_FILENO);

						//initializes array to pass to exec
						char *args[48];
						//calls helper methods to trim whitespace and new line characters,
						//as well as parse the command into the args array
						std::string arg(buffer);
						std::string trimmedArg = trim(arg,'\n');
						trimmedArg = trim(trimmedArg,' ');
						bzero(buffer,48);
						strcpy(buffer,trimmedArg.c_str());
						parse(buffer,args);

						//exec to launch specified command
						int rc = execvp(args[0],args);
						if(rc == -1) {
								std::cerr << "Failure! execve error code=" << errno << std::endl;
						}						
						exit(-1);
				}

				else if(w > 0){
						//in parent
						//waits on child to finish
						int pid_2 = waitpid(w,&status,0);

						//sends the word 'end' to client to signify the end of the process
						char end[] = {'e','n','d','\0'};
						n = write(newsockfd,end,48);

						//handles errors from fork call
						if(WIFEXITED(status)){
								int retcode = WEXITSTATUS(status);
								if(retcode < 0){
										std::cerr << "Child Error: exited with error " << retcode << std::endl;
								}else{
										std::cout << "Child exited successfully" << std::endl;
								}       
						}else if(WIFSIGNALED(status)){
								int signal = WTERMSIG(status);
								std::cerr << "Child Error: signaled with signal :" << signal << std::endl;
						}else {
								std::cerr << "Child Error: unknown error occured"<<std::endl; 
						}               
				}else{
						std::cerr << "Child Error: could not fork correctly";
				}

		}
}
