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

int main(int argc, char *argv[])
{
  printf("hi\n");
  if(argc != 2)
  {
    printf("Wrong number of arguments %d \n", argc);
    return -1;
  }

  int numberOfChars = atoi(argv[1]);
  printf("num - %d\n", numberOfChars);
  if(numberOfChars <= 0)
  {
    printf("parameter is not a valid number\n");
    return -1;
  }

  int sockfd = openSocket();
  if(sockfd == -1)
  {
    printf("unable to open a socket\n");
    return -1;
  }

  int filefd = openFile();
  if(filefd == -1)
  {
    printf("unable to open the file\n");
    close(sockfd);
    return -1;
  }
  printf("print!\n");
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

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT_num); // Note: htons for endiannes
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded... but why not. lets doing it hard 

  if( connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("Error : Connect Failed. %s \n", strerror(errno));
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

  while(size > 0) //while more bytes need to be processed
  {
    //reading from file
    ssize_t len;
    if(size<BUFFER_SIZE) //read the minimum between readingSize and bufferSize
      len = read(filefd, buf, size);
    else
      len = read(filefd, buf, BUFFER_SIZE);

    if(len < 0) //check that the read call succeeded 
    {
      printf("Error reading from input file: %s\n", strerror(errno));
      return -1;
    }
    printf("data - ");
    for(int i=0;i<len;i++)
      printf("%c",buf[i]);

    printf("\n");

    size -= len;
    char *bufPointer = buf;
    //write to socket
    while( len>0 )
    {
      ssize_t bytes_written = write(sockfd, bufPointer, len);
      if( bytes_written <= 0 )
      {
        printf("Error reading from input file: %s\n", strerror(errno));
        return -1;
      }
      len -= bytes_written;
      bufPointer += bytes_written;
    }
  }
  return 0;
}

int printResult(int sockfd)
{
  return 0;
}