#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <iostream>
#include <unistd.h>

#include <stdio.h>
#include <inttypes.h>
// Included to get the support library
#include "calcLib.h"

#include "protocol.h"

using namespace std;

int checkMsg(calcMessage *clcMsg)
{
  int ret = 0;
  if (ntohs(clcMsg->type) == 2 && ntohs(clcMsg->message) == 2 && ntohs(clcMsg->major_version) == 1 && ntohs(clcMsg->minor_version) == 0)
  {
    cout << "Abort";
    ret = -1;
  }
  return ret;
}

int main(int argc, char *argv[])
{
  int nrOfSent = 0;
  int bytesReceived = 0;
  //Check the initail inputs before and after ":"
  if (argc != 2)
  {
    printf("Invalid input detected\n");
    exit(1);
  }
  char delim[] = ":";
  char *Desthost = strtok(argv[1], delim);
  char *Destport =strtok(NULL, delim);
  if (Desthost == NULL || Destport == NULL)
  {
    cout << "Invalid input detected.\n";
    exit(1);
  }

  //int port = atoi(Destport);
  string ipAddr = Desthost;

  calcMessage msg;
  msg.type = htons(22);     //binary protocol for client-to-server
  msg.message = htonl(0);   //Not appliicable or available
  msg.protocol = htons(17); //UDP
  msg.major_version = htons(1):
  msg.minor_version = htons(0);

  struct addrinfo hint, *servinfo, *p;
  memset(&hint, 0, sizeof(hint));
  hint.ai_family = AF_INET;
  hint.ai_socktype = SOCK_DGRAM;

  int rv = getaddrinfo(Desthost, Destport, &hint, &servinfo);
  if (rv != 0)
  {
    fprintf(stderr, "get address info: %s\n", gai_strerror(rv));
    exit(1);
  };
  int sock = 0;
  for (p = servinfo; p != NULL; p = p->ai_next)
  {
    sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock == -1)
    {
      continue;
    }
    else
    {
      freeaddrinfo(servinfo);

      struct  timeval timeout;
      timeout.tv_sec = 2;   //2 sec
      timeout.tv_usec = 0;

      if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
      {
        perror("setsockopt failed\n");
      }
      if (sendto(sock, &msg, sizeof(msg), 0, p->ai_addr, p->ai_addrlen) == -1)
      {
        cout << "Unable to send message\n";
        close(sock);
        exit(1);
      }

      break; 
    }
  }

  //calcProtocol msgRcv;
  calcProtocol msgRcv;

}
