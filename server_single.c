#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string.h>

#define PORT 5000
#define PATH_MAX 4096

char *ROOT;
void readfile(char* url, char** file);
void geturl(char* msg, char** url);
void respond (int sock);
void getextension(char* url, char** extension);

void sendall(int sock, char* msg) {
  int length = strlen(msg);
  int bytes;
  while(length > 0) {
    /* printf("send bytes : %d\n", bytes); */
    bytes = send(sock, msg, length, 0);
    length = length - bytes;
  }
}



int main( int argc, char *argv[] ) {
  int newsockfd;
  int sockfd, portno = PORT;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  clilen = sizeof(cli_addr);
  ROOT = getenv("PWD");

  /* First call to socket() function */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("ERROR opening socket");
    exit(1);
  }

  // port reusable
  int tr = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &tr, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  /* Initialize socket structure */
  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* TODO : Now bind the host address using bind() call.*/
  if ( bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1 ){
    perror("bind error");
    exit(1);
  }

  /* TODO : listen on socket you created */

  if ( listen(sockfd, 20) == -1 ){
    perror("listen error");
    exit(1);
  }

  printf("Server is running on port %d\n", portno);
  while (1) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if ( newsockfd == -1 ){
      perror("accept error");
      exit(1);
    }
    respond(newsockfd);
    printf("close\n");
    shutdown(newsockfd, SHUT_RDWR);
    close(newsockfd);
  }

  return 0;
}

void respond(int sock) {
    int offset, bytes;
    char buffer[9000];
    bzero(buffer, 9000);
    offset = 0;
    bytes = 0;
    do{
      bytes = recv(sock, buffer+offset, 1500, 0);
      offset+=bytes;
      if(strncmp(buffer+offset-4, "\r\n\r\n", 4) == 0) break;
    }while(bytes > 0);
    char* url;
    char* file;
    char* extension;
    geturl(buffer, &url);
    readfile(url, &file);
    getextension(url, &extension);
    printf("%s", extension);
    if(file)
    {
      for(int i = 0; i < strlen(file); i++)
      {
        printf("%c", file[i]);
      }
    }
    char* message =  "HTTP/1.1 200 OK\r\nContent-Type: text/html;\r\n\r\n<html><body>Hello World!</body></html\r\n\r\n";
    sendall(sock, message);
}

// Function to read file from url
// If file not found, returns NULL

void readfile(char* url, char** file){
    char pwd[PATH_MAX];
    getcwd(pwd, sizeof(pwd));
    strcat(pwd, url);
    FILE* fileptr;
    fileptr = fopen(pwd, "r");
    if(!fileptr){
      *file = NULL;
    }else{
      fseek(fileptr, 0L, SEEK_END);
      int size = ftell(fileptr);
      *file = malloc((size+5)*sizeof(char));
      fseek(fileptr, 0L, SEEK_SET);
      fread(*file, 1, size, fileptr);
      strcpy(*file+size+1, "\r\n\r\n");
    }
}

// Function to get url of requested file
// If url is empty, returns "index.html"

void geturl(char* msg, char** url){
    int length = 0, i = 4;
    while(i < strlen(msg) && msg[i] != ' ')
    {
      ++length;
      ++i;
    }
    if(length == 1){
      *url = malloc(12*sizeof(char));
      strcpy(*url, "/index.html");
    }else{
      *url = malloc((length+1)*sizeof(char));
      strncpy(*url, msg+4, length);
    }
}

// Function to get extension of the requested file
// Iterates from end of url

void getextension(char* url, char** extension){
    int i = strlen(url);
    int length = 0;
    while(i >= 0 && url[i] != '.'){
      ++length;
      --i;
    }
    if(i < 0)
      *extension = NULL;
    else{
      *extension = malloc((length)*sizeof(char));
      strcpy(*extension, url+strlen(url)-length+1);
    }
}