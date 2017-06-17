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

#define BUFFER_SIZE 1024
#define PORT_num 2233

int main(int argc, char *argv[])
{
  int totalsent = -1;
  int nsent     = -1;
  int len       = -1;
  int n         =  0;
  int listenfd  = -1;
  int connfd    = -1;

  struct sockaddr_in serv_addr;
  struct sockaddr_in my_addr;
  struct sockaddr_in peer_addr;

  char data_buff[BUFFER_SIZE];

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  memset( &serv_addr, '0', sizeof(serv_addr));
  memset( data_buff, '0', sizeof(data_buff));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // INADDR_ANY = any local machine address
  serv_addr.sin_port = htons(PORT_num);

  if( 0 != bind( listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)))
  {
    printf("\n Error : Bind Failed. %s \n", strerror(errno));
    return 1;
  }

  if( 0 != listen(listenfd, 10) )
  {
    printf("\n Error : Listen Failed. %s \n", strerror(errno));
    return 1;
  }

  while(1)
  {
    // Prepare for a new connection
    socklen_t addrsize = sizeof(struct sockaddr_in );

    // Accept a connection.
    // Can use NULL in 2nd and 3rd arguments
    // but we want to print the client socket details
    connfd = accept( listenfd,
                     (struct sockaddr*) &peer_addr,
                     &addrsize);

    if( connfd < 0 )
    {
      printf("\n Error : Accept Failed. %s \n", strerror(errno));
      return 1;
    }


   

   
    while( notwritten > 0 )
    {
      // notwritten = how much we have left to write
      // totalsent  = how much we've written so far
      // nsent = how much we've written in last write() call */
      nsent = write(connfd,
                    data_buff + totalsent,
                    notwritten);
      // check if error occured (client closed connection?)
      assert( nsent >= 0);
      printf("Server: wrote %d bytes\n", nsent);

      totalsent  += nsent;
      notwritten -= nsent;
    }

    /* close socket  */
    close(connfd);
  }
}