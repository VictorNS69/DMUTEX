#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdbool.h>

////////////////////////// Constants
#define HOST "localhost" 
#define MSG 0
#define LOCK 1
#define OK 2

////////////////////////// TYPEDEF'S
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

typedef struct {
  int sckt;
  SOCKADDR_IN sckaddr;
}SCKT;

typedef struct {
  char id[100];
  int port;
}PEER;

typedef struct {
  int op;
  char idLock[100];
  int *CLK;
}MESSAGE;

typedef struct {
  char id[100];
  bool req; 
  int *req_CLK;
  bool inside;
  int ok;
  int n_waiting;
  char (*waiting)[100];
}MUTEX;

////////////////////////// GLOBAL VARIALBES
int PORT;
PEER *PEERS;
int NPEERS;
int *CLK;
int *PASTCLK;
int INDEX;
MUTEX *LOCKS;
int NLOCKS;

////////////////////////// AUXILIAR FUNCTIONS
int open_port(SCKT *skt){
  if ((skt->sckt = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1){
    return -1;
  }
  bzero((char *)&(skt->sckaddr), sizeof(SOCKADDR_IN));
  skt->sckaddr.sin_family = AF_INET;
  skt->sckaddr.sin_addr.s_addr = INADDR_ANY;
  skt->sckaddr.sin_port = htons(0);
  if (bind(skt->sckt, (struct sockaddr *)&(skt->sckaddr), sizeof(SOCKADDR_IN)) == -1){
    close(skt->sckt);
    return -1;
  }
  int tam_dir = sizeof(SOCKADDR_IN);
  if (getsockname(skt->sckt, (struct sockaddr *)&(skt->sckaddr), (socklen_t *)&tam_dir) == -1){
    close(skt->sckt);
    return -1;
  }
  return 0;
}

int store_peer(const char *proc, const int port){
  strcpy(PEERS[NPEERS].id, proc);
  PEERS[NPEERS++].port = port;
  if ((PEERS = realloc(PEERS, (1 + NPEERS) * sizeof(PEER))) == NULL){
    return -1;
  }
  return 0;
}

int init_clk(void){
  if ((CLK = calloc(NPEERS, sizeof(int))) == NULL){
    return -1;
  }
  if ((PASTCLK = calloc(NPEERS, sizeof(int))) == NULL) {
    return -1;
  }
  return 0;
}

void print_clk(void){
  int i;
  printf("%s: LC[", PEERS[INDEX].id);
  for (i = 0; i < NPEERS; i++){
    if (i == NPEERS - 1)
      printf("%i]\n", CLK[i]);
    else
      printf("%i,", CLK[i]);
  }
}

void update_clk(const int *r_CLK){
  int i;
  for (i = 0; i < NPEERS; i++)
    CLK[i] = CLK[i] > r_CLK[i] ? CLK[i] : r_CLK[i];
}

int serialize(const MESSAGE *msg, unsigned char **buf){
  int msg_sz = sizeof(uint32_t) + 100 + NPEERS * sizeof(uint32_t);
  if ((*buf = calloc(1, msg_sz)) == NULL){
    return -1;
  }
  size_t offset = 0;
  unsigned int tmp;
  int i;
  tmp = htonl(msg->op);
  memcpy(*buf + offset, &tmp, sizeof(tmp));
  offset += sizeof(tmp);
  memcpy(*buf + offset, msg->idLock, 100);
  offset += 100;
  for (i = 0; i < NPEERS; i++){
    tmp = htonl(CLK[i]);
    memcpy(*buf + offset, &tmp, sizeof(tmp));
    offset += sizeof(tmp);
  }
  return msg_sz;
}

MESSAGE *deserialize(const unsigned char *buf, const size_t bufSz){
  static const size_t MIN_BUF_SZ = 88;
  MESSAGE *msg;
  if (buf && bufSz < MIN_BUF_SZ){
    return NULL;
  }
  if ((msg = calloc(1, sizeof(MESSAGE))) == NULL){
    return NULL;
  }
  size_t tmp = 0, offset = 0;
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  memcpy(&msg->op, &tmp, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  memcpy(&msg->idLock, buf + offset, 100);
  offset += 100;
  int i;
  if ((msg->CLK = calloc(NPEERS, sizeof(uint32_t))) == NULL){
    free(msg);
    return NULL;
  }
  for (i = 0; i < NPEERS; i++){
    memcpy(&tmp, buf + offset, sizeof(tmp));
    tmp = ntohl(tmp);
    memcpy(&msg->CLK[i], &tmp, sizeof(uint32_t));
    offset += sizeof(uint32_t);
  }
  return msg;
}

int process_name(char *name, const int port){
  int i;
  bool found = false;
  for (i = 0; i < NPEERS && !found; i++){
    if (PEERS[i].port == port){
      found = true;
      strcpy(name, PEERS[i].id);
      return 0;
    }
  }
  return -1;
}

int send_message(SCKT *skt, const char *to){
  CLK[INDEX]++;
  printf("%s: TICK\n", PEERS[INDEX].id);
  MESSAGE msg;
  bzero((char *)&msg, sizeof(MESSAGE));
  msg.op = MSG;
  int port;
  if ((port = get_port(to)) == -1) {
    return -1;
  }
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  receiver.sin_port = htons(port);
  int msg_sz;
  unsigned char *buf = 0;
  if ((msg_sz = serialize(&msg, &buf)) == -1){
    return -1;
  }
  int tam_d = sizeof(SOCKADDR_IN);
  if ((sendto(skt->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1) {
    return -1;
  }
  printf("%s: SEND(MSG,%s)\n", PEERS[INDEX].id, to);
  return 0;
}

MESSAGE *receive_message(const SCKT *skt, char *pname){
  SOCKADDR_IN rec;
  unsigned char buff[256] = {0};
  MESSAGE *msg;
  int tam = sizeof(SOCKADDR_IN);
  int msg_sz;
  if ((msg_sz = recvfrom(skt->sckt, buff, 256, 0, (SOCKADDR *)&rec, (socklen_t *)&tam)) == -1){
    return NULL;
  }
  if ((msg = deserialize(buff, msg_sz)) == NULL){
    return NULL;
  }
  if ((process_name(pname, ntohs(rec.sin_port))) == -1){
    return NULL;
  }
  return msg;
}

int get_port(const char *id){
  int i;
  bool found = false;
  for (i = 0; i < NPEERS && !found; i++){
    if (!strcmp(id, PEERS[i].id)){
      found = true;
      return PEERS[i].port;
    }
  }
  return -1;
}

int get_lock_index(const char *idLock){
  int i;
  bool found = false;
  for (i = 0; i < NLOCKS && !found; i++){
    if (!strcmp(LOCKS[i].id, idLock)){
      return i;
    }
  }
  return -1;
}

int get_peer_index(const char *idPeer){
  int i;
  bool found = false;
  for (i = 0; i < NPEERS && !found; i++){
    if (!strcmp(PEERS[i].id, idPeer)){
      return i;
    }
  }
  return -1;
}

int add_to_queue(const char *idLock, const char *idPeer){
  int lockIndex;
  if ((lockIndex = get_lock_index(idLock)) == -1){
    return -1;
  }
  if (get_peer_index(idPeer) == -1){
    return -1;
  }
  strcpy(LOCKS[lockIndex].waiting[LOCKS[lockIndex].n_waiting++], idPeer);
  if ((LOCKS[lockIndex].waiting = realloc(LOCKS[lockIndex].waiting,
       (1 + LOCKS[lockIndex].n_waiting) * sizeof(char[100]))) == NULL){
    return -1;
  }
  return 0;
}

int priority(const int *reqLClk, const int *msgLclk, const char *id){
  int i;
  int less = 0;
  int leq = 0;
  int gr = 0;
  int geq = 0;
  for (i = 0; i < NPEERS; i++){
    if (reqLClk[i] < msgLclk[i]){
      less++;
    }
    else if (reqLClk[i] > msgLclk[i]){
      gr++;
    }
    if (reqLClk[i] <= msgLclk[i] || reqLClk[i] >= msgLclk[i]){
      geq++;
      leq++;
    }
  }
  if (less > 1 && leq == NPEERS){
    return 0;
  }
  else if (gr > 1 && geq == NPEERS){
    return 1;
  }
  else{
    int pIndex;
    if ((pIndex = get_peer_index(id)) == -1){
      return -1;
    }
    return INDEX < pIndex ? 0 : 1;
  }
}

int remove_lock(const char *id){
  int lockIndex;
  if ((lockIndex = get_lock_index(id)) == -1){
    return -1;
  }
  MUTEX *temp = malloc((NLOCKS) * sizeof(MUTEX));
  if (!lockIndex){
    memmove(temp, LOCKS + 1, (NLOCKS - 1) * sizeof(MUTEX));
  }
  else{
    memmove(temp, LOCKS, (lockIndex) * sizeof(MUTEX));
    memmove(temp + lockIndex, LOCKS + lockIndex + 1, (NLOCKS - lockIndex - 1) * sizeof(MUTEX));
  }
  free(LOCKS[lockIndex].req_CLK);
  free(LOCKS[lockIndex].waiting);
  free(LOCKS);
  NLOCKS--;
  LOCKS = temp;
  return 0;
}

int add_lock(const SCKT *skt, const char *id){
  if ((get_lock_index(id) != -1)){
    return -1;
  }
  strcpy(LOCKS[NLOCKS].id, id);
  LOCKS[NLOCKS].req = true;
  LOCKS[NLOCKS].inside = false;
  LOCKS[NLOCKS].n_waiting = 0;
  if ((LOCKS[NLOCKS].req_CLK = malloc(NPEERS * sizeof(int))) == NULL){
    return -1;
  }
  CLK[INDEX]++;
  printf("%s: TICK\n", PEERS[INDEX].id);
  int i;
  for (i = 0; i < NPEERS; i++)
    LOCKS[NLOCKS].req_CLK[i] = CLK[i];
  if ((LOCKS[NLOCKS++].waiting = malloc(sizeof(char[100]))) == NULL){
    return -1;
  }
  if ((LOCKS = realloc(LOCKS, (1 + NLOCKS) * sizeof(MUTEX))) == NULL){
    return -1;
  }
  MESSAGE msg;
  bzero((char *)&msg, sizeof(MESSAGE));
  msg.op = LOCK;
  strcpy(msg.idLock, id);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  for (i = 0; i < NPEERS; i++){
    if (i != INDEX){
      receiver.sin_port = htons(PEERS[i].port);
      int msg_sz;
      unsigned char *buf = 0;
      if ((msg_sz = serialize(&msg, &buf)) == -1){
        return -1;
      }
      int tam_d = sizeof(SOCKADDR_IN);
      if ((sendto(skt->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1){
        return -1;
      }
      printf("%s: SEND(LOCK,%s)\n", PEERS[INDEX].id, PEERS[i].id);
    }
  }
  return 0;
}

int unlock_lock(const SCKT *skt, const char *idLock){
  int lockIndex;
  if ((lockIndex = get_lock_index(idLock)) == -1){
    return -1;
  }
  MESSAGE msg;
  bzero((char *)&msg, sizeof(MESSAGE));
  msg.op = OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  if (LOCKS[lockIndex].n_waiting){
    int i, peerIndex;
    for (i = 0; i < LOCKS[lockIndex].n_waiting; i++){
      CLK[INDEX]++;
      printf("%s: TICK\n", PEERS[INDEX].id);
      if ((peerIndex = get_peer_index(LOCKS[lockIndex].waiting[i])) == -1){
        return -1;
      }
      receiver.sin_port = htons(PEERS[peerIndex].port);
      int msg_sz;
      unsigned char *buf = 0;
      if ((msg_sz = serialize(&msg, &buf)) == -1){
        return -1;
      }
      int tam_d = sizeof(SOCKADDR_IN);
      if ((sendto(skt->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1){
        return -1;
      }
      printf("%s: SEND(OK,%s)\n", PEERS[INDEX].id, LOCKS[lockIndex].waiting[i]);
    }
  }
  remove_lock(idLock);
  return 0;
}

int send_ok_req(const SCKT *skt, const char *idPeer, const char *idLock){
  CLK[INDEX]++;
  printf("%s: TICK\n", PEERS[INDEX].id);
  int peerIndex;
  if ((peerIndex = get_peer_index(idPeer)) == -1){
    return -1;
  }
  MESSAGE msg;
  bzero((char *)&msg, sizeof(MESSAGE));
  msg.op = OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  receiver.sin_port = htons(PEERS[peerIndex].port);
  int msg_sz;
  unsigned char *buf = 0;
  if ((msg_sz = serialize(&msg, &buf)) == -1){
    return -1;
  }
  int tam_d = sizeof(SOCKADDR_IN);
  if ((sendto(skt->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1){
    return -1;
  }
  printf("%s: SEND(OK,%s)\n", PEERS[INDEX].id, idPeer);
  return 0;
}

////////////////////////// MAIN
int main(int argc, char *argv[]){
  int port;
  char line[100], proc[100];
  if (argc < 2){
    fprintf(stderr, "Uso: proceso <ID>\n");
    return 1;
  }
  /* Establece el modo buffer de entrada/salida a línea */
  setvbuf(stdout, (char *)malloc(sizeof(char) * 100), _IOLBF, 100);
  setvbuf(stdin, (char *)malloc(sizeof(char) * 100), _IOLBF, 100);
  SCKT skt;
  if (open_port(&skt) == -1){
    return -1;
  }
  PORT = ntohs(skt.sckaddr.sin_port);
  fprintf(stdout, "%s: %d\n", argv[1], PORT);
  if ((PEERS = malloc(sizeof(PEER))) == NULL){
    return -1;
  }
  if ((LOCKS = malloc(sizeof(MUTEX))) == NULL){
    return -1;
  }
  if ((LOCKS->waiting = malloc(sizeof(PEER))) == NULL){
    return -1;
  }
  for (; fgets(line, 100, stdin);){
    if (!strcmp(line, "START\n"))
      break;
    sscanf(line, "%[^:]: %d", proc, &port);
    if (!strcmp(proc, argv[1])){
      INDEX = NPEERS;
    }
    if (store_peer(proc, port) == -1){
      free(PEERS);
      return -1;
    }
  }
  if ((PEERS = realloc(PEERS, (NPEERS) * sizeof(PEER))) == NULL){
    free(PEERS);
    return -1;
  }
  if (init_clk() == -1) {
    free(PEERS);
    return -1;
  }
  char id_sec[100];
  char action[100];
  for (; fgets(line, 100, stdin);){
    if (!strcmp(line, "EVENT\n")) {
      CLK[INDEX]++;
      printf("%s: TICK\n", PEERS[INDEX].id);
      continue;
    }
    if (!strcmp(line, "GETCLOCK\n")) {
      print_clk();
      continue;
    }
    if (!strcmp(line, "RECEIVE\n")){
      MESSAGE *msg;
      int lockIndex;
      char pname[100];
      if ((msg = receive_message(&skt, pname)) == NULL){
        free(msg);
        free(CLK);
        free(PEERS);
        return -1;
      }
      switch (msg->op){
        case MSG:
          printf("%s: RECEIVE(MSG,%s)\n", PEERS[INDEX].id, pname);
          update_clk(msg->CLK);
          CLK[INDEX]++;
          printf("%s: TICK\n", PEERS[INDEX].id);
          break;
        case LOCK:
          printf("%s: RECEIVE(LOCK,%s)\n", PEERS[INDEX].id, pname);
          update_clk(msg->CLK);
          CLK[INDEX]++;
          printf("%s: TICK\n", PEERS[INDEX].id);
          if ((lockIndex = get_lock_index(msg->idLock)) == -1) {
            if (send_ok_req(&skt, pname, msg->idLock) == -1){
              free(msg);
              free(CLK);
              free(PEERS);
              return -1;
            }
          }
          else{
            if (LOCKS[lockIndex].inside){
              if (add_to_queue(msg->idLock, pname) == -1){
                free(msg);
                free(CLK);
                free(PEERS);
                return -1;
              }
            }
            else {
              if (priority(LOCKS[lockIndex].req_CLK, msg->CLK, pname) == 1){
                if (send_ok_req(&skt, pname, msg->idLock) == -1){
                  free(msg);
                  free(CLK);
                  free(PEERS);
                  return -1;
                }
              }
              else{
                if (add_to_queue(msg->idLock, pname) == -1){
                  free(msg);
                  free(CLK);
                  free(PEERS);
                  return -1;
                }
              }
            }
          }
          break;
        case OK:
          printf("%s: RECEIVE(OK,%s)\n", PEERS[INDEX].id, pname);
          update_clk(msg->CLK);
          CLK[INDEX]++;
          printf("%s: TICK\n", PEERS[INDEX].id);
          if ((lockIndex = get_lock_index(msg->idLock)) == -1){
            free(msg);
            free(CLK);
            free(PEERS);
            return -1;
          }
          LOCKS[lockIndex].ok++;
          if (LOCKS[lockIndex].ok == NPEERS - 1){
            LOCKS[lockIndex].inside = true;
            printf("%s: MUTEX(%s)\n", PEERS[INDEX].id, msg->idLock);
          }
        }
      free(msg);
    }
    if (!strcmp(line, "FINISH\n")){
      printf("%s: FINISH[%i]\n", PEERS[INDEX].id, getpid());
      break;
    }
    sscanf(line, "%s %s", action, id_sec);
    if (!strcmp(action, "MESSAGETO")){
      if (send_message(&skt, id_sec) == -1){
        free(CLK);
        free(PEERS);
        return -1;
      }
    }
    if (!strcmp(action, "LOCK")){
      if (add_lock(&skt, id_sec) == -1){
        free(CLK);
        free(PEERS);
        return -1;
      }
    }
    if (!strcmp(action, "UNLOCK")){
      if (unlock_lock(&skt, id_sec) == -1){
        free(CLK);
        free(PEERS);
        return -1;
      }
    }
  }
  free(CLK);
  free(PEERS);
  return 0;
}