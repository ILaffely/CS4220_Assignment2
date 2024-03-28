//As mentioned in the client server I used the book code from section 6.1.4
//I have annotated it with my explanation of what it does to the best of my ability
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>

//this was the pid of the first server I ran. 
#define SERVER_PORT 28157
#define BUF_SIZE 4096
#define QUEUE_SIZE 10

//same as client 
void fatal(char *string);

int main(int argc, char *argv[]) {

  //lot more ints this time
  //s is still the socket
  //b checks the bind
  //l checks if the server can listen
  //sa checks if the server can accept a socket connection
  //fd checks if the file can be opened
  //and on is set to one to be used in opening the socket
  int s, b, l, fd, sa, bytes, on = 1;
  char buff[BUF_SIZE];
  struct sockaddr_in channel;

  // build address structure to bind to socket, initially similar to client
  //but this time we are not coppying the information as we are the host
  memset(&channel, 0, sizeof(channel));
  channel.sin_family = AF_INET;
  channel.sin_addr.s_addr = htonl(INADDR_ANY);
  channel.sin_port = htons(SERVER_PORT);

  // passive open wait for connection
  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  //tests socket failure 
  if (s < 0)
    fatal("socket failed");
  //trys to bind
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  b = bind(s, (struct sockaddr *)&channel, sizeof(channel));
  //tests bind failure
  if (b < 0)
    fatal("bind failed");
  
  //listens 
  l = listen(s, QUEUE_SIZE);
  if (l < 0)
    fatal("listen failed");

  // accept connection
  while (1) {
    sa = accept(s, 0, 0);
    if (sa < 0)
      fatal("accept failed");

    // read from socket 

    read(sa, buff, BUF_SIZE);

    fd = open(buff, O_RDONLY);
    if (fd < 0)
      fatal("open failed");
    while (1) {
      bytes = read(fd, buff, BUF_SIZE);
      if (bytes <= 0)
        break;
      //writes info from requested file
      write(sa, buff, bytes);
    }
    close(fd);
    close(sa);
  }
}

void fatal(char *string) {
  printf("%s\n", string);
  exit(1);
}