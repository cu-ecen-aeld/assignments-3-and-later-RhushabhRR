#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/stat.h>

#define PORT "9000"
#define BACKLOG 10

#define BUF_SIZE 1024

char * buffer;
int sockfd, newfd;
bool terminate = false;
FILE *file;


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

bool communicate(int fd)
{
    int bytesRead = 0;
    int numBytes = 0;
    
    file = fopen("/var/tmp/aesdsocketdata", "a+");

    while (!terminate)
    {
        bool connectionClosed = false;
        bool error = false;

        do {
                buffer = (char *)realloc(buffer, numBytes + BUF_SIZE);
                if (!buffer) {
                    syslog(LOG_INFO, "Unable to allocate space on heap");
                    printf("Unable to allocate space on heap\n");
                    close(sockfd);
                    fclose(file);
                    closelog();
                    return false;
                }

                bytesRead = recv(newfd, buffer + numBytes, BUF_SIZE, 0);

                if(bytesRead == 0)
                {
                    connectionClosed = true;
                    break;
                }

                else if(bytesRead < 0)
                {
                    syslog(LOG_ERR, "Received error");
                    error = true;
                    break;
                }

                numBytes += bytesRead;
            } while (!memchr(buffer + numBytes - bytesRead, '\n', bytesRead));

            if(error == 1 || connectionClosed == 1) 
            {
                free(buffer);
                buffer = NULL;
                numBytes = 0; 
                break;
            }
            else
            {
                buffer[numBytes] = '\0';

                fputs(buffer, file);
                fflush(file);
                fseek(file, 0, SEEK_SET);

                size_t bufferSize = BUF_SIZE;
                char* writeBuf = (char *)malloc(bufferSize * sizeof(char));
                if(writeBuf == NULL) {
                    perror("Unable to allocate memory for writeBuf");
                    return -1;
                }

                while(fgets(writeBuf, bufferSize, file) != NULL)
                {
                    send(newfd, writeBuf, strlen(writeBuf), 0);
                }

                free(writeBuf);
                                

                free(buffer);
                buffer = NULL;
                numBytes = 0;
            }
    }

    return true;
}

void signal_handler(int signum)
{
    if ((signum == SIGINT) || (signum == SIGTERM)) {

        if(remove("/var/tmp/aesdsocketdata") == 0)
        {
            printf("Sucesfully deleted file /var/tmp/aesdsocket\n");
        }

        syslog(LOG_INFO, "Caught signal, exiting\n");
        printf("Caught signal, exiting\n");
        
        terminate = true;

        close(sockfd);
        close(newfd);
        fclose(file);
        closelog();
        
        exit(EXIT_SUCCESS);

    } else 
    {   // Unknown signal
        syslog(LOG_ERR, "Error: Caught unknown signal\n");
        return;
    }
    
    return;
}

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *servinfo;
    int yes = 1;
    socklen_t their_size;
    struct sockaddr_storage their_addr;



    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    char client[INET6_ADDRSTRLEN];

    bool deamonMode = false;

    //Check for daemon mode
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[1], "-d") == 0)
        {
            deamonMode = true;
        }
    }

    openlog("socket.c", LOG_CONS | LOG_PID, LOG_USER);

    if(getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
    {
        syslog(LOG_ERR, "getaddrinfo() failed with error");
        printf("getaddrinfo() failed with error\n");
        return -1;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    if(sockfd == -1)
    {
        syslog(LOG_ERR, "socket() failed with error");
        printf("socket() failed with error\n");
        return -1;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    if(bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) != 0)
    {
        syslog(LOG_ERR, "bind() failed with error");
        printf("bind() failed with error\n");
        return -1;
    }

    freeaddrinfo(servinfo);

    if(listen(sockfd, BACKLOG) != 0)
    {
        syslog(LOG_ERR, "listen() failed with error");
        printf("listen() failed with error\n");
        return -1;
    }

    //sig handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if(deamonMode == true)
    {
            pid_t pid = fork();

            if(pid < 0)
            {
                printf("failed to fork\n"); 
                close(sockfd);
                fclose(file);
                closelog();  
                exit(EXIT_FAILURE);      
            }   

            if(pid > 0 )
            {
                printf("Running as a deamon\n");
                exit(EXIT_SUCCESS);
            }

            umask(0);

            if(setsid() < 0)
            {
                printf("Failed to create SID for child\n");
                close(sockfd);
                fclose(file);
                closelog(); 
                exit(EXIT_FAILURE);
            }

            if(chdir("/") < 0)
            {
                printf("Unable to change directory to root\n");
                close(sockfd);
                fclose(file);
                closelog(); 
                exit(EXIT_FAILURE);
            }

            close(STDIN_FILENO);
            close(STDOUT_FILENO);
            close(STDERR_FILENO);        
    }

    their_size = sizeof(their_addr);

    while(1)
    {
        newfd = accept(sockfd, (struct sockaddr *)&their_addr, &their_size);
        if(!newfd)
        {
            printf("error connecting");
        }

        //Print IP of connected client
        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            client, sizeof(client));
        syslog(LOG_INFO, "Accepted connection from %s\n", client);
        printf("server: got connection from %s\n", client);

        if(!communicate(newfd))
        {
            close(newfd);
            fclose(file);
            return -1;
        }
    }

    close(sockfd);
    fclose(file);
    closelog();

    return 0;
}