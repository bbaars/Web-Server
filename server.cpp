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
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/sendfile.h>
#include <ctime>
#include <locale>

#define MAX_REQUEST_SIZE 5000

void * requestHandler(void * arg);
void parse_arguments(int argc, char* argv[], int *port, std::string *docroot, std::string *logfile);
int isGetRequest(std::string firstLine, std::string * file);
std::vector<std::string> splitLine(std::string line, char delimitor);
int hasFileBeenModifiedSince(std::string file, std::string modifiedSinceDate);
void sendFile( int sockfd, std::string file);
void sendNotImplmented(int sock);
void logFile(char* logMessage);

/*----------------------------------------------------*/
/*TO CONNECT and issue a GET REQUEST: type into address bar:  http://localhost:8080/test.txt */
/*Or more generally: http://localhost:<port>/<requested_file>*/
/*----------------------------------------------------*/
/*Set default values if the user chooses not to specify anything in argv[]*/
int port = 8080;
std::string docroot = ".";
std::string logfile = "";
char notImplementedError[50] = "HTTP/1.1 501 Not Implemented\r\n";
char invalidRequest[40] = "HTTP/1.1 404 Not Found\r\n";
char notModified[40] = "HTTP/1.1 304 Not Modified\r\n";
char requestHeader[] = "----------------------REQUEST---------------------------";
char responseHeader[] = "----------------------RESPONSE---------------------------";
char bottomBorder[] = "-------------------------------------------------------";

/*Mutex to protect shared memory*/
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char * argv[])
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);



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
        std::string requestedFile;


        int n;
        if ((n = recv(sockfd, request, MAX_REQUEST_SIZE, 0)) > 0)
        {

            int modifiedByFlag=0;
            int fileWasModifiedSince=0;
            std::string modifiedBy;
            std::vector<std::string> linesInRequest;
            //Split up request line by line
            linesInRequest = splitLine(std::string(request), '\n');

            //TODO:parse any if-modified header out and handle
            //appropriate status code (304) if the page has not been modified
            //304 MUST include date header as well

            //TODO:chack for any connection: headers
            //if its connection: close, we must close connection

            //TODO:close connection if no messages in 20 seconds

            logFile(requestHeader);
            logFile(request);
            logFile(bottomBorder);

            //see if GET request
            if(isGetRequest(linesInRequest[0], &requestedFile))
            {
                if(requestedFile.compare("./favicon.ico")!=0)
                {
                    std::cout << "GET Request for file " << requestedFile << '\n';
                    printf("request: %s\n",request );
                }
                //TODO:check if this file is in our directory before we send
                //if its not send 404 response, status line is in invalidRequest[]
                std::size_t found;

                //see if if-modified-since header is in request
                found=std::string(request).find("If-Modified-Since:");
                //if it is, extract modified date
                if (found!=std::string::npos)
                {
                    modifiedBy = std::string(request).substr(std::string(request).find("If-Modified-Since:") + 19);
                    modifiedByFlag=1;
                }

                if(modifiedByFlag)
                {
                    //if file was not modified since the specified date, send 304 response
                    if(fileWasModifiedSince = hasFileBeenModifiedSince(requestedFile, modifiedBy))
                    {
                        //TODO:send 304 response
                    }
                }
                //else, send file
                if(!fileWasModifiedSince)
                {
                    if(requestedFile.compare("./favicon.ico")!=0)
                    {
                        puts(responseHeader);
                        sendFile(sockfd,requestedFile);
                        puts(bottomBorder);
                    }
                }
            }
            else
            {
                std::cout << "NOT A GET REQUEST" << '\n';
                sendNotImplmented(sockfd);
            }
            //TODO:open log file, log request to file, close log file
            //MUST BE THREAD SAFE

        }
    }
    return 0;
}
int hasFileBeenModifiedSince(std::string file, std::string modifiedSinceDate)
{
    std::tm modifiedSince;
    struct stat result;

    struct tm modifiedTime;
    strptime(modifiedSinceDate.c_str(), "%a, %d %b %Y %H:%M:%S %Z\r\n", &modifiedTime);
    time_t modifiedByDate = mktime(&modifiedTime);  // t is now your desired time_t

    stat(file.c_str(), &result);
    time_t fileModifiedDate = result.st_mtime;

    double seconds = difftime(fileModifiedDate, modifiedByDate);

    if(seconds>0)
        return 1;
    else
        return 0;
}
void logFile(char* logMessage)
{
	/*If a logfile was specified with the -logfile flag, write to the logfile*/
	if (logfile.compare("") != 0)
	{
		/*Use a mutex to prevent multiple threads writing at the same time*/
		pthread_mutex_lock(&mutex);
    	FILE* fp;
    	fp = fopen(logfile.c_str(),"a");
    	fprintf(fp, "%s\n", logMessage);
    	fclose(fp);
		pthread_mutex_unlock(&mutex);
	}
}
void sendCantFindError(int sockfd)
{
    char contentHeader[50];
    char contentLength[30];
    char currentTime[70];
    char messageHeader[200];
    time_t rawtime;
    FILE* rfd;
    struct stat result;
    int sendBytes=0;

    time ( &rawtime );

    strftime(currentTime, sizeof(currentTime), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n",  gmtime ( &rawtime ));

    stat("404.html", &result);
    strcpy(contentHeader, "Content-Type: text/html\r\n");
    sprintf(contentLength,"Content-Length: %d\r\n",result.st_size);
    sprintf(messageHeader,"%s%s%s%s\r\n",invalidRequest,currentTime,contentHeader,contentLength);

    logFile(responseHeader);
    logFile(messageHeader);
    logFile(bottomBorder);

    printf(messageHeader);
    send(sockfd,messageHeader, strlen(messageHeader),0);
    rfd = fopen("404.html","rb");

    sendBytes = sendfile (sockfd, fileno(rfd), 0, result.st_size);
    std::cout << "Bytes sent:" << sendBytes<< '\n';

}

void sendNotModified(int sock)
{
    char contentLength[30];
    char currentTime[70];
    char messageHeader[200];
    time_t rawtime;

    time ( &rawtime );

    strftime(currentTime, sizeof(currentTime), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n",  gmtime ( &rawtime ));
    sprintf(contentLength,"Content-Length: %d\r\n",0);
    sprintf(messageHeader,"%s%s%s\r\n",notImplementedError,currentTime,contentLength);

    logFile(responseHeader);
    logFile(messageHeader);
    logFile(bottomBorder);

    send(sock,messageHeader, strlen(messageHeader),0);
}

void sendNotImplmented(int sock)
{
    char contentHeader[50];
    char contentLength[30];
    char currentTime[70];
    char messageHeader[200];
    time_t rawtime;
    FILE* rfd;
    struct stat result;
    int sendBytes=0;

    time ( &rawtime );

    strftime(currentTime, sizeof(currentTime), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n",  gmtime ( &rawtime ));

    stat("501.html", &result);
    strcpy(contentHeader, "Content-Type: text/html\r\n");
    sprintf(contentLength,"Content-Length: %d\r\n",result.st_size);
    sprintf(messageHeader,"%s%s%s%s\r\n",notImplementedError,currentTime,contentHeader,contentLength);

    logFile(responseHeader);
    logFile(messageHeader);
    logFile(bottomBorder);

    printf(messageHeader);
    send(sock,messageHeader, strlen(messageHeader),0);
    rfd = fopen("501.html","rb");

    sendBytes = sendfile (sock, fileno(rfd), 0, result.st_size);
    std::cout << "Bytes sent:" << sendBytes<< '\n';
}
void sendFile( int sockfd, std::string file)
{
    char httpHeader[30] = "HTTP/1.1 200 OK\r\n";
    char currentTime[70];
    char modifiedTime[70];
    char contentHeader[50];
    char contentLength[30];
    char messageHeader[200];
    char line[1024];

    int sendBytes=0;
    std::string fileType;
    time_t rawtime;
    FILE* rfd;
    struct stat result;
    time ( &rawtime );

    //format current time in http response format
    strftime(currentTime, sizeof(currentTime), "Date: %a, %d %b %Y %H:%M:%S %Z\r\n",  gmtime ( &rawtime ));
    //	printf("%s", currentTime);

    //try and open file requested
    rfd = fopen(file.c_str(),"rb");
    //if not found, send 404 response
    if(rfd == NULL)
    {
        sendCantFindError(sockfd);
    }
    //if found
    else
    {
        //get file info
        stat(file.c_str(), &result);

        //format modified time in http response format
        strftime(modifiedTime, sizeof(modifiedTime), "Last-Modified: %a, %d %b %Y %H:%M:%S %Z\r\n", gmtime(&result.st_mtime));
        //	printf("%s",modifiedTime);

        //get file type and set content type
        fileType = file.substr(file.find_last_of(".") + 1);
        std::cout << "FileType:" << fileType << '\n';
        if(!fileType.compare("html"))
            strcpy(contentHeader, "Content-Type: text/html\r\n");
        if(!fileType.compare("txt"))
            strcpy(contentHeader, "Content-Type: text/plain; charset=UTF-8\r\n");
        if(!fileType.compare("jpg"))
            strcpy(contentHeader, "Content-Type: image/jpeg\r\n");
        if(!fileType.compare("pdf"))
            strcpy(contentHeader, "Content-Type: application/pdf\r\n");
        //printf("%s",contentHeader);

        sprintf(contentLength,"Content-Length: %d\r\n",result.st_size);
        //printf("%s", contentLength);

        sprintf(messageHeader,"%s%s%s%s%s\r\n",httpHeader,currentTime,contentHeader,contentLength,modifiedTime);
        printf(messageHeader);

        logFile(responseHeader);
        logFile(messageHeader);
        logFile(bottomBorder);

        //TODO:open log file, log messageHeader to file, close log file
        //MUST BE THREAD SAFE
        printf("Send header\n" );
        send(sockfd,messageHeader, strlen(messageHeader),0);

        printf("Open file\n" );
        rfd = fopen(file.c_str(),"rb");

        sendBytes = sendfile (sockfd, fileno(rfd), 0, result.st_size);
        std::cout << "Bytes sent:" << sendBytes<< '\n';
        fclose(rfd);
    }
}

int isGetRequest(std::string firstLine, std::string * file)
{

    std::vector<std::string> elements;

    elements = splitLine(firstLine,' ');

    if(!elements[0].compare("GET"))
    {
        *file = (docroot + elements[1]);
        return 1;
    }

    else
        return 0;

}

std::vector<std::string> splitLine(std::string line,  char delimitor)
{
    std::stringstream lineToSplit(line);
    std::string segment;
    std::vector<std::string> seglist;

    while(std::getline(lineToSplit, segment, delimitor))
    {
        seglist.push_back(segment);
    }

    return seglist;
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
