#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define FILE_NAME "/dev/urandom"
#define BUFFER_SIZE 1024
#define PORT_num 2233

int openSocket();
int openFile();
int sendFileToSockect(int sockfd, int filefd, off_t size);
int printResult(int sockfd);
int readAll(int file, void * buffer, size_t size);
int writeAll(int file, void * buffer, size_t size);

int main(int argc, char *argv[])
{
  if(argc != 2)
  {
    printf("Wrong number of arguments %d \n", argc);
    return -1;
  }

  int numberOfChars = atoi(argv[1]);
  if(numberOfChars <= 0)
  {
    printf("parameter is not a valid number\n");
    return -1;
  }

  int sockfd = openSocket();
  if(sockfd == -1)
    return -1;
  

  int filefd = openFile();
  if(filefd == -1)
  {
    printf("unable to open the file\n");
    close(sockfd);
    return -1;
  }

  if(sendFileToSockect(sockfd,filefd,numberOfChars) == -1)
  {
    close(sockfd);
    close(filefd);
    return -1;
  }

  printResult(sockfd);

  close(sockfd);
  close(filefd);
  return 0;
}

int openSocket()
{
  int  sockfd = -1;

  struct sockaddr_in serv_addr;
  struct sockaddr_in my_addr;
  struct sockaddr_in peer_addr;
  socklen_t addrsize = sizeof(struct sockaddr_in );
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd == -1)
  {
    printf("Unable to open a socket\n");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT_num); // Note: htons for endiannes
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded... but why not. lets doing it hard 

  if( connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("Connect to server failed. %s \n", strerror(errno));
    return -1;
  }

  return sockfd;

}

int openFile()
{
  int file = open(FILE_NAME, O_RDONLY);
  if(file == -1)
  {
    printf("unable to open the file - %s\n", strerror(errno));
    return -1;
  }
  return file;
}
  

int sendFileToSockect(int sockfd, int filefd, off_t size)
{ 
  long counter = 0;
  char buf[BUFFER_SIZE];

  if(writeAll(sockfd, &size, sizeof(off_t)) == -1)
      return -1;
  

  while(size > 0) 
  {
    ssize_t len = size<BUFFER_SIZE? size:BUFFER_SIZE;

    if(readAll(filefd, buf, len) == -1)
      return -1;

    if(writeAll(sockfd, buf, len) == -1)
     return -1;

    size -= len;
  }
  return 0;
}

int printResult(int sockfd)
{
  unsigned long result;

  
  if(readAll(sockfd, (void *)  &result, sizeof(unsigned long)) == -1)
    return -1;
  
  printf("the number of printable chars is %lu\n", result);

  return 0;
}

int readAll(int file, void * buffer, size_t size)
{
  void *location = buffer;
  ssize_t len;
  if(size == 0)
    return 1;
  do
  {
    len = read(file, location, size); 
    if(len == -1)
    {
      printf("len - %zu\n", len);
      printf("Error reading from file: %s\n", strerror(errno));
      return -1;  
    }
    else if(len == 0)
      break;
    size-=len;

    location += len;
  } while(size>0 || len == 0);
  return 1;
}

int writeAll(int file, void * buffer, size_t size)
{
  void *location = buffer;
  ssize_t len;
  if(size == 0)
    return 1;

  do
  {
    len = write(file, location, size);
    if(len <= 0)
    {
      printf("Error Writing to file: %s\n", strerror(errno));
      return -1;  
    }
    size-=len;
    location += len;
  } while (size>0); 
  return 1;
}