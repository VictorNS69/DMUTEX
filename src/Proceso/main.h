/* DMUTEX (2019) Sistemas Operativos Distribuidos
 * Author: Víctor Nieves Sánchez
*/
#ifndef MAIN_H
#define MAIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>
////////////////////////// TYPEDEF'S
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

typedef struct info_sckt{
  int sckt;
  SOCKADDR_IN sckaddr;
}INFO_SCKT;

typedef struct peer_sckt{
  char id[80];
  int port;
}PEER;

typedef struct udp_msg{
  int op;
  char idLock[80];
  int *lclk;
}UDP_MSG;

typedef struct mutex{
  char id[80];
  bool req; 
  int *req_lclk;
  bool inside;
  int ok;
  int n_waiting;
  char (*waiting)[80];
}MUTEX;

#endif
