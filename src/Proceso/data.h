/* Definition of the objects
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>


#define HOST "localhost"

#define MSG 0
#define LOCK 1
#define OK 2

#define MAX_SIZE 256

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

typedef struct socket{
  int sckt;
  SOCKADDR_IN sckaddr;
}SOCKET;

typedef struct peer{
  char id[80];
  int port;
}PEER;

typedef struct message{
  int op;
  char idLock[80];
  int *lclk;
}MESSAGE;

typedef struct mutex{
  char id[80];
  bool req; 
  int *req_lclk;
  bool inside;
  int ok;
  int n_waiting;
  char (*waiting)[80];
}MUTEX;
