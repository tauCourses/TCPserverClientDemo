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
#include <fcntl.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define PORT_num 2233
#define NUM_OF_PRINTABLE_CHARS 94
#define NUM_OF_THREADS 10

typedef struct {
  unsigned long printableArray[NUM_OF_PRINTABLE_CHARS];
  unsigned long counted;
  unsigned long printable;
} statistics;

int setSignalHandler();
void signalHandler(int signum, siginfo_t* info, void* ptr);
void initStat(statistics *stat);
void updateGlobalStatistics(statistics stat);
void printStat(statistics stat);
int openSocket();
void* clientHandler(void *connfd_ptr);
int processData(int connfd, int size, statistics *stat);
int readAll(int file, void * buffer, size_t size);
int writeAll(int file, void * buffer, size_t size);

statistics globalStat;
pthread_mutex_t statLock;

bool stillRunning = true;

int main(int argc, char *argv[])
{
  if(setSignalHandler() == -1)
    return -1;
  
  initStat(&globalStat);

  if(pthread_mutex_init( &statLock, NULL ))
  {
    printf("Mutex init failed\n");
    return -1;
  }


  int sockfd = openSocket();
  if (sockfd < 0)
    return -1;
  
  while(stillRunning)
  {
      int connfd = accept(sockfd, (struct sockaddr*) NULL, NULL); 
      if(connfd == -1)
      {
        if(errno == 4) //it's just a system call, maybe signal arrived!
          continue;
        printf("accept function failed - %s\n",strerror(errno));
        break;
      }
      pthread_t thread;
      if(pthread_create(&thread, NULL, clientHandler, (void*) &connfd)<0)
      {
        printf("pthread create function failed - %s\n",strerror(errno));
        break;
      }
     
  }

  pthread_mutex_destroy(&statLock);
  close(sockfd);
  printStat(globalStat);
}

int setSignalHandler()
{
  // register the signal handler
  struct sigaction new_action;
  sigfillset(&new_action.sa_mask); 
  new_action.sa_flags = SA_SIGINFO;
  new_action.sa_sigaction = signalHandler;
  if (sigaction(SIGINT, &new_action, NULL) != 0) 
  {
    printf("Signal handle registration failed. %s\n", strerror(errno));
    return -1;
  }

  return 1;
}

void signalHandler(int signum, siginfo_t* info, void* ptr)
{
  stillRunning = false;
}

void initStat(statistics *stat)
{
  stat->printable = 0;
  stat->counted = 0;
  for(int i=0; i<NUM_OF_PRINTABLE_CHARS; i++)
    stat->printableArray[i] = 0;
}

void updateGlobalStatistics(statistics stat)
{
  pthread_mutex_lock(&statLock);
  
  globalStat.counted += stat.counted;  
  globalStat.printable += stat.printable;
  for(int i=0;i<NUM_OF_PRINTABLE_CHARS;i++)
    globalStat.printableArray[i] += stat.printableArray[i];

  pthread_mutex_unlock(&statLock);
}

void printStat(statistics stat)
{
  printf("Statistics:\n");
  printf("Processed - %lu characters, %lu from them are readable\n", stat.counted, stat.printable);
  for(int i=0;i<NUM_OF_PRINTABLE_CHARS;i++)
    if(stat.printableArray[i] > 0)
      printf("%c : %lu\n",i+32, stat.printableArray[i]);
  
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

void* clientHandler(void *connfd_ptr) 
{
  int connfd = *(int*)connfd_ptr;
  statistics localStat;
  initStat(&localStat);

  off_t charToRead;
  if(readAll(connfd,&charToRead,sizeof(off_t)) == -1)
  {
    close(connfd);
    return 0;
  }

  printf("charToRead - %lu\n",charToRead);
  while(charToRead > 0)
  {
    int len = charToRead<BUFFER_SIZE? charToRead:BUFFER_SIZE; 
    if(processData(connfd,len, &localStat) == -1)
    {
      close(connfd);
      return 0;
    }
    charToRead -= len;
  }

  if(writeAll(connfd, &localStat.printable, sizeof(unsigned long)) == -1)
  {
    close(connfd);
    return 0;
  }
  close(connfd);

  updateGlobalStatistics(localStat);
  return 0;
}

int processData(int connfd, int size, statistics *stat)
{
  int printable = 0;
  if(size > BUFFER_SIZE)
    return -1;
  char buf[BUFFER_SIZE];
  if(readAll(connfd, buf, size) == -1)
      return -1;

  pthread_mutex_lock(&statLock);
  for(int i=0;i<size;i++)
  {
    stat->counted++;
    if(buf[i]>=32 && buf[i]<=126) //check char is printable
    {
      stat->printable++;
      stat->printableArray[buf[i]-32]++;
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