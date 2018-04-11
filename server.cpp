#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <iostream>
#include <string>

#define MAX_REQUEST_SIZE 5000

void * requestHandler(void * arg);
void parse_arguments(int argc, char* argv[], int *port, std::string *docroot, std::string *logfile);

/*----------------------------------------------------*/
/*TO CONNECT and issue a GET REQUEST: type into address bar:  http://localhost:8080/test.txt */
/*Or more generally: http://localhost:<port>/<requested_file>*/
/*----------------------------------------------------*/


int main(int argc, char * argv[]) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/*Set default values if the user chooses not to specify anything in argv[]*/
	int port = 8080;
	std::string docroot = ".";
	std::string logfile = "output.txt";

	parse_arguments(argc, argv, &port, &docroot, &logfile);

	std::cout << "Port:             " << port << std::endl;
	std::cout << "Root Directory:   " << docroot << std::endl;
	std::cout << "Log File:         " << logfile << std::endl;
	

	struct sockaddr_in serveraddr,clientaddr;
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(sockfd, 10);

	while(1) 
	{
		
		unsigned int len = sizeof(clientaddr);
		int clientSocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
		std::cout << "Client Accepted " << '\n';

		if (clientSocket > 0 ) 
		{
			// Create separate thread to handle this request.
			pthread_t requestHandlerThread;
			pthread_create(&requestHandlerThread, NULL, requestHandler, &clientSocket);
			pthread_detach(requestHandlerThread);
		}
	}

}

/*STUB: CODE TO HANDLE HTTP REQUEST*/
void * requestHandler(void * arg)
{
	int * sockfd_p = (int *) arg;
	int sockfd = *sockfd_p;

	
	while (1) 
	{
		char request[MAX_REQUEST_SIZE];
		int n;
		if ((n = recv(sockfd, request, MAX_REQUEST_SIZE, 0)) > 0) 
		{
			puts("---------------------------------------------------------");
			printf("Request: %s\n", request);	
			puts("---------------------------------------------------------");
		}
	}
	return 0;
}

/*Parses argv[]*/
void parse_arguments(int argc, char* argv[], int *port, std::string *docroot, std::string *logfile)
{
	for (int i = 1; i < argc; i++)
	{
		std::string temp = argv[i];
		
		/*If the user specified "-p [port]"*/
		if (temp.compare("-p") == 0 && (i + 1 < argc))
		{
			*port = atoi(argv[i + 1]);
		}

		/*If the user specified "-docroot [root_directory]"*/
		if (temp.compare("-docroot") == 0 && (i + 1 < argc))
		{
			*docroot = argv[i + 1];
		}

		/*If the user specified "-logfile [file_name]"*/
		if (temp.compare("-logfile") == 0 && (i + 1 < argc))
		{
			*logfile = argv[i + 1];
		}
	}
}


