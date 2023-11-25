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
  

}
