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
  //Check the initial inputs before and after ":"
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

  while (nrOfSent < 3)
  {
    memset(&msgRcv, 0, sizeof(msgRcv));
    bytesReceived = recvfrom(sock, &msgRcv, sizeof(msgRcv), 0, p->ai_addr, p->ai_addrlen == -1);
    if (bytesReceived < 0)
    {
      cout << "Receive timeout, sending again.\n";
      nrOfSent++;
      if (sendto(sock, &msg, sizeof(msg), 0, p->ai_addr, p->ai_addrlen) == -1)
      {
        cout << "Unable to send message.\n";
        close(sock);
        exit(1);
      }
      if (nrOfSent < 3)
      {
        continue;
      }
      else
      {
        cout << "Unable to send message.\n";
        close(sock);
        exit(1);
      }
    }
    else
     {
        nrOfSent = 0;
        break;
      }
  }

  if (bytesReceived < (int)sizeof(msgRcv))
  {
    //Receive a valculated Message
    calcMessage *clcMsg = (calcMessage *)&msgRcv;
    if (checkMsg(clcMsg))
    {
      cout << "A NOT OK message is gotten.\nAbort!\n";
      close(sock);
    }
  }
 
   //Receive a calcProtocol
  int i1 = ntohl(msgRcv.inValue1), i2 = ntohl(msgRcv.inValue2), iRes = ntohl(msgRcv.inResult);    float fi = msgRcv.flValue1, f2 = msgRcv.flValue2, fRes = msgRcv.flResult;
  cout << "Task:";
  string op = "";

  switch (ntohl(msgRcv.arith))
  {
  case 1: //addition
    cout << "Add "<< i1 <<" "<<i2 << endl;
    iRes = i1 + i2;
    msgRcv.inResult = htonl(iRes);
    op = "i";
    break;
  case 2: //subtraction
    cout << "Sub "<< i1 << " "<<i2 << endl;
    iRes = i1 - i2;
    msgRcv.inResult = htonl(iRes);
    op = "i";
    break;
  case 3: //multiplication
    cout << "Mul "<< i1 << " "<<i2 << endl;
    iRes = i1 * i2;
    msgRcv.inResult = htonl(iRes);
    op = "i";
    break;
  case 4: //division
    cout << "Div "<< i1 << " "<<i2 << endl;
    iRes = i1 / i2;
    msgRcv.inResult = htonl(iRes);
    op = "i";
    break;
  case 5: //float add
    cout << "Fadd "<< f1 << " "<<f2 << endl;
    fRes = f1 + f2;
    msgRcv.flResult = fRes;
    op = "f";
    break;
  case 6: //float sub
    cout << "Fsub "<< f1 << " "<<f2 << endl;
    fRes = f1 - f2;
    msgRcv.flResult = fRes;
    op = "f";
    break;
  case 7: //float mul
    cout << "Fmul "<< f1 << " "<<f2 << endl;
    fRes = f1 * f2;
    msgRcv.flResult = fRes;
    op = "f";
    break;
  case 8: //float div
    cout << "Fdiv "<< f1 << " "<<f2 << endl;
    fRes = f1 / f2;
    msgRcv.flResult = fRes;
    op = "f";
    break;
  default:
    cout << "Cant do that operation.\n";
    break;
  }

  if (sendto(sock, &msgRcv, sizeof(msgRcv), 0, p->ai_addr, p->ai_addrlen) == -1)
  {
    cout << "Unable to send message\n";
    close(sock);
    exit(1);
  }
  if(op == "1"){
    cout << "Sent result: "<< ntohl(msgRcv.inResult)<<"\n";
  }
  else{
    cout << "Sent result: "<< msgRcv.flResult<<"\n";
  }

  calcMessage resp;

  while (nrOfSent < 3)
  {
    memset(&resp, 0, sizeof(resp));
    bytesReceived = recvfrom(sock, &resp, sizeof(resp), 0,p->ai_addr, &p->ai_addrlen);
    if (bytesReceived < 0)
    {
      cout << "Receive timeout, sending again.\n";
      nrOfSent++;
      if (sendto(sock, &msgRcv, sizeof(msgRcv), 0, p->ai_addr, p->ai_addrlen) == -1)
      {
        cout << "Unable to send message.\n";
        close(sock);
        exit(1);
      }
      if (nrOfSent < 3)
      {
        continue;
      }
      else
      {
        cout << "unable to send message.\n";
        close(sock);
        exit(1);
      }
    }
    else
    {
      nrOfSent = 0;
      break;
    }    
  }
  if (checkMsg(&resp) == -1)
  {
    cout << "A NOT OK message is gotten.\nAbort!\n";
    close(sock);
    exit(1);
  }

  switch (ntohl(resp.message))
  {
  case 0:
    cout << "Not available\n";
    break;
  case 1:
    cout << "Ok\n";
    break;
  case 2:
    cout << "NOT OK";
    break;
  }

  close(sock);
  return 0;
}
  


