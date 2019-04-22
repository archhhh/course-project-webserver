#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <string.h>
#include <assert.h>

#define PORT 5000
#define PATH_MAX 4096

char *ROOT;
int client_count = 0;
pthread_mutex_t lock;

void readfile(char* url, char** file, int* sizeoffile);
void geturl(char* msg, char** url);
void* respond (void* sock);
void getextension(char* url, char** extension);

void sendall(int sock, char* msg, int length) {
  int bytes;
  while(length > 0) {
    /* printf("send bytes : %d\n", bytes); */
    bytes = send(sock, msg, length, 0);
    length = length - bytes;
  }
}



int main( int argc, char *argv[] ) {
  int rc = pthread_mutex_init(&lock, NULL);
  assert(rc == 0);
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
    if(client_count < 50)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if ( newsockfd == -1 ){
          perror("accept error");
          exit(1);
        }
        pthread_t thread_id; 
        printf("Before Thread\n"); 
        rc = pthread_create(&thread_id, NULL, respond, (void*)newsockfd);
        if(rc == 0)
        {
          printf("After Thread\n");
          pthread_mutex_lock(&lock);
          client_count++;
          pthread_mutex_unlock(&lock);
        }
    }
  }

  return 0;
}

void * respond(void * sock) {
    const char* headers;
    char* url;
    char* file;
    char* extension;
    char* message;
    int offset, bytes, sizeoffile, length;
    char buffer[9000];
    bzero(buffer, 9000);
    offset = 0;
    bytes = 0;
    sizeoffile = 0;
    length = 0;
    do{
      bytes = recv((int)sock, buffer+offset, 1500, 0);
      offset+=bytes;
      if(strncmp(buffer+offset-4, "\r\n\r\n", 4) == 0) break;
    }while(bytes > 0);
    geturl(buffer, &url);
    // Generate messages
    // If incorrect format of GET Request
    // Send 400 Status Code
    if(url == NULL)
    {
      headers = "HTTP/1.1 400 Bad Request\r\n\r\n";
      length = strlen(headers)+1;
      message = malloc((length)*sizeof(char));
      strcpy(message, headers);
    }else{
      readfile(url, &file, &sizeoffile);
      // If file is found
      // Include Content-Type header depending on extension
      // If not
      // Send 404 Status Code
      if(file)
      {
        getextension(url, &extension);
        if(strcmp(extension, "html") == 0)
        {
          headers = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        }else if(strcmp(extension, "js") == 0)
        {
          headers = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n\r\n";
        }else if(strcmp(extension, "css") == 0)
        {
          headers = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\n\r\n";
        }else if(strcmp(extension, "jpg") == 0)
        {
          headers = "HTTP/1.1 200 OK\r\nContent-Type: image/jpg\r\n\r\n";
        } else
        {
          headers = "HTTP/1.1 200 OK\r\n\r\n";
        }
        length = strlen(headers)+sizeoffile+1;
        message = malloc((length)*sizeof(char));
        strcpy(message, headers);
        int i;
        for(i = strlen(headers); i < length-1; ++i){
          message[i] = file[i-strlen(headers)];
        }
        message[i] = '\0';
      }else{
        headers = "HTTP/1.1 404 Not Found\r\n\r\n";
        length = strlen(headers)+1;
        message = malloc((length)*sizeof(char));
        strcpy(message, headers);
      }
    }
    sendall((int)sock, message, length-1);
    printf("close\n");
    shutdown((int)sock, SHUT_RDWR);
    close((int)sock);
    pthread_mutex_lock(&lock);
    client_count--;
    pthread_mutex_unlock(&lock);
    return NULL;
}

// Function to read file from url
// If file not found, returns NULL

void readfile(char* url, char** file, int* sizeoffile){
    char pwd[PATH_MAX];
    getcwd(pwd, sizeof(pwd));
    strcat(pwd, url);
    FILE* fileptr;
    fileptr = fopen(pwd, "rb");
    if(!fileptr){
      *file = NULL;
    }else{
      fseek(fileptr, 0L, SEEK_END);
      int size = ftell(fileptr);
      *file = malloc((size+5)*sizeof(char));
      fseek(fileptr, 0L, SEEK_SET);
      fread(*file, 1, size, fileptr);
      strcpy(*file+size, "\r\n\r\n");
      *sizeoffile = size+4;
    }
}

// Function to get url of requested file
// If url is empty, returns "index.html"

void geturl(char* msg, char** url){
    int length = 0, i = 4;
    if(strlen(msg) < 4)
    {
      *url = NULL;
    }else{
      if(strncmp(msg, "GET", 3) != 0)
      {
        *url = NULL;
      }else{
        while(i < strlen(msg) && msg[i] != ' ')
        {
          ++length;
          ++i;
        }
        if(length <  1)
        {
          *url = NULL;
        }else if(length == 1){
          *url = malloc(12*sizeof(char));
          strcpy(*url, "/index.html");
        }else{
          *url = malloc((length+1)*sizeof(char));
          strncpy(*url, msg+4, length);
        }
      }
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