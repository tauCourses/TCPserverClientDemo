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

#define fileName "/dev/urandom"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[])
{
  if(argc != 1)
  {
    printf(" \n");
    return -1;
  }

  int numberOfChars = atoi(argc);
  if(numberOfChars <= 0)
  {
    printf("parameter is not a valid number\n");
    return -1;
  }

  int sockfd = openSocket();
  if(sockfd == -1)
  {
    pritnf("unable to open a socket\n");
    return -1;
  }

  int filefd = openFile(fileName);
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

  printResult();

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
  serv_addr.sin_port = htons(2233); // Note: htons for endiannes
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // hardcoded... but why not. lets doing it hard 

  if( connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
  {
    printf("Error : Connect Failed. %s \n", strerror(errno));
    return -1;
  }

  return sockfd;

}

int openFile(char* fileName)
{
  int file = open(fileName, O_RDONLY);
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

    size -= len;
    //write to socket
    while( len>0 )
    {
      ssize_t bytes_written = write(sockfd, recv_buff, sizeof(recv_buff) - 1);
      if( bytes_written <= 0 )
      {
        printf("Error reading from input file: %s\n", strerror(errno));
        return -1;
      }
      len -= bytes_written;
    }
  }
  return 0;
}