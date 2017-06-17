#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define PORT_num 2233
#define NUM_OF_PRINTABLE_CHARS 94
#define NUM_OF_THREADS 10

void initStat();
int openSocket();
int processData(int sockfd, int size);
int readAll(int file, void * buffer, size_t size);
int writeAll(int file, void * buffer, size_t size);

typedef struct {
  unsigned long printableArray[NUM_OF_PRINTABLE_CHARS];
  unsigned long Counted;
  unsigned long printable;
} statistics;

statistics stat;
pthread_mutex_t statLock;

bool stillRunning = true;

int main(int argc, char *argv[])
{
  if(pthread_mutex_init( &statLock, NULL ))
  {
    printf("Mutex init failed\n");
    return -1;
  }
  initStat();
  //addSignalHandler();

  int sockfd = openSocket();
  if (sockfd < 0)
    return -1;
  
  while(stillRunning)
  {
      int connfd = accept(sockfd, (struct sockaddr*) NULL, NULL); 
      off_t charToRead;
      if(readAll(connfd,&charToRead,sizeof(off_t)) == -1)
      {
        close(sockfd);
        return -1;
      }

      printf("charToRead - %lu\n",charToRead);
      unsigned long printable = 0;
      while(charToRead > 0)
      {
        int len = charToRead<BUFFER_SIZE? charToRead:BUFFER_SIZE; 
        int printableTemp = processData(connfd,len);
        if(printableTemp == -1)
        {
          close(sockfd);
          return -1;
        }
        printable += printableTemp;
        charToRead -= len;
      }

      if(writeAll(connfd, &printable, sizeof(unsigned long)) == -1)
      {
        close(sockfd);
        return -1;
      }

      close(connfd);
  }
  close(sockfd);
}

void initStat()
{
  pthread_mutex_lock(&statLock);

  stat.printable = 0;
  stat.Counted = 0;
  for(int i=0; i<NUM_OF_PRINTABLE_CHARS; i++)
    stat.printableArray[i] = 0;

  pthread_mutex_unlock(&statLock);
}

int openSocket()
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    printf("Unable to open a socket\n");
    return -1;
  }


  struct sockaddr_in serv_addr; 
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(PORT_num); 

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0)
  {
    printf("Error: binding failed\n");
    close(sockfd);
    return -1;
  }

  if(listen(sockfd,NUM_OF_THREADS) < 0)
  {
    printf("Error: listen failed\n");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

int processData(int sockfd, int size)
{

  int printable = 0;
  char buf[size];
  if(readAll(sockfd, buf, size) == -1)
      return -1;

  pthread_mutex_lock(&statLock);
  for(int i=0;i<size;i++)
  {
    stat.Counted++;
    if(buf[i]>=32 && buf[i]<=126) //check char is printable
    {
      stat.printable++;
      stat.printableArray[buf[i]-32]++;
      printable++;
    }
  }
  pthread_mutex_unlock(&statLock);

  return printable;
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