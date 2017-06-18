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
#define PORT_NUM 2233
#define NUM_OF_PRINTABLE_CHARS 95
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
pthread_mutex_t threadCounterLock;
pthread_cond_t noThreadCond;

bool stillRunning = true;
int threadCounter = 0;

int main(int argc, char *argv[])
{
  if(setSignalHandler() == -1)
    return -1;
  
  initStat(&globalStat); //not in a mutex because there is only one thread here!

  int lock_val = pthread_mutex_init( &statLock, NULL );
  if(lock_val != 0)
  {
    printf("Mutex init failed - %s\n", strerror(lock_val));
    return -1;
  }
  lock_val = pthread_mutex_init( &threadCounterLock, NULL );
  if(lock_val!=0)
  {
    printf("Mutex init failed - %s\n", strerror(lock_val));
    return -1;
  }

  lock_val = pthread_cond_init (&noThreadCond, NULL);
  if(lock_val!=0)
  {
    printf("Condition varible init failed - %s\n", strerror(lock_val));
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
        if(errno == EINTR) //it's just a system call, maybe signal arrived!
          continue;
        printf("accept function failed - %s\n",strerror(errno));
        close(sockfd);
        pthread_exit(NULL);
      }
      
      lock_val = pthread_mutex_lock(&threadCounterLock);
      if( lock_val != 0 )
      {
        printf("mutex lock falied - %s\n", strerror( lock_val ) );
        pthread_exit(NULL);
      }
      
        threadCounter++;
      
      lock_val = pthread_mutex_unlock(&threadCounterLock);
      if( lock_val != 0 )
      {
        printf("mutex lock falied - %s\n", strerror( lock_val ) );
        pthread_exit(NULL);
      }

      pthread_t thread;
      if(pthread_create(&thread, NULL, clientHandler, (void*) &connfd)<0)
      {
        printf("pthread create function failed - %s\n",strerror(errno));
        close(sockfd);
        pthread_exit(NULL);
      }
     
  }
  close(sockfd);
  lock_val = pthread_mutex_lock(&threadCounterLock);
  if( lock_val != 0 )
  {
    printf("mutex lock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }
    while(threadCounter > 0)
      pthread_cond_wait(&noThreadCond, &threadCounterLock);
  
  lock_val = pthread_mutex_unlock(&threadCounterLock);
  if( lock_val != 0 )
  {
    printf("mutex unlock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }

  pthread_mutex_destroy(&statLock);
  pthread_mutex_destroy(&threadCounterLock);
  pthread_cond_destroy(&noThreadCond);
  
  printStat(globalStat);
  pthread_exit(NULL);
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
  int lock_val = pthread_mutex_lock(&statLock);
  if( lock_val != 0 )
  {
    printf("mutex lock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }

  globalStat.counted += stat.counted;  
  globalStat.printable += stat.printable;
  for(int i=0;i<NUM_OF_PRINTABLE_CHARS;i++)
    globalStat.printableArray[i] += stat.printableArray[i];

  lock_val = pthread_mutex_unlock(&statLock);
  if( lock_val != 0 )
  {
    printf("mutex unlock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }
}

void printStat(statistics stat)
{
  printf("\nStatistics:\n");
  printf("Processed - %lu characters, %lu from them are printable\n", stat.counted, stat.printable);
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
  serv_addr.sin_port = htons(PORT_NUM); 

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

  int lock_val = pthread_mutex_lock(&threadCounterLock);
  if( lock_val != 0 )
  {
    printf("mutex lock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }

    threadCounter--;
    if(threadCounter == 0)
      pthread_cond_signal(&noThreadCond);

  lock_val = pthread_mutex_unlock(&threadCounterLock);
  if(lock_val != 0)
  {
    printf("mutex unlock falied - %s\n", strerror( lock_val ) );
    pthread_exit(NULL);
  }

  return 0;
}

int processData(int connfd, int size, statistics *stat)
{
  if(size > BUFFER_SIZE)
    return -1;
  char buf[BUFFER_SIZE];
  if(readAll(connfd, buf, size) == -1)
      return -1;
  
  for(int i=0;i<size;i++)
  {
    stat->counted++;
    if(buf[i]>=32 && buf[i]<=126) //check char is printable
    {
      stat->printable++;
      stat->printableArray[buf[i]-32]++;
    }
  }

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