#include "main.h"

////////////////////////// Constants
#define HOST "localhost"
#define MSG 0
#define LOCK 1
#define OK 2

////////////////////////// GLOBAL VARIALBES

int udp_port;
PEER *peers;
int n_peers;
int *lclk;
int *past_lclk;
int myIndex;
MUTEX *locks;
int n_locks;

////////////////////////// AUXILIAR FUNCTIONS
int open_udp(INFO_SCKT *info)
{
  if ((info->sckt = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
  {
    return -1;
  }
  bzero((char *)&(info->sckaddr), sizeof(SOCKADDR_IN));
  info->sckaddr.sin_family = AF_INET;
  info->sckaddr.sin_addr.s_addr = INADDR_ANY;
  info->sckaddr.sin_port = htons(0);
  if (bind(info->sckt, (struct sockaddr *)&(info->sckaddr), sizeof(SOCKADDR_IN)) == -1)
  {
    close(info->sckt);
    return -1;
  }
  int tam_dir = sizeof(SOCKADDR_IN);
  if (getsockname(info->sckt, (struct sockaddr *)&(info->sckaddr), (socklen_t *)&tam_dir) == -1)
  {
    close(info->sckt);
    return -1;
  }
  return 0;
}

int store_peer_sckt(const char *proc, const int port)
{
  strcpy(peers[n_peers].id, proc);
  peers[n_peers++].port = port;
  if ((peers = realloc(peers, (1 + n_peers) * sizeof(PEER))) == NULL)
  {
    return -1;
  }
  return 0;
}

int init_lclk(void)
{
  if ((lclk = calloc(n_peers, sizeof(int))) == NULL)
  {
    return -1;
  }
  if ((past_lclk = calloc(n_peers, sizeof(int))) == NULL)
  {
    return -1;
  }
  return 0;
}

void print_lclk(void)
{
  int i;
  printf("%s: LC[", peers[myIndex].id);
  for (i = 0; i < n_peers; i++)
  {
    if (i == n_peers - 1)
      printf("%i]\n", lclk[i]);
    else
      printf("%i,", lclk[i]);
  }
}

void update_lclk(const int *r_lclk)
{
  int i;
  for (i = 0; i < n_peers; i++)
    lclk[i] = lclk[i] > r_lclk[i] ? lclk[i] : r_lclk[i];
}

int serialize(const UDP_MSG *msg, unsigned char **buf)
{
  int msg_sz = sizeof(uint32_t) + 80 + n_peers * sizeof(uint32_t);
  if ((*buf = calloc(1, msg_sz)) == NULL)
  {
    return -1;
  }
  size_t offset = 0;
  unsigned int tmp;
  int i;
  tmp = htonl(msg->op);
  memcpy(*buf + offset, &tmp, sizeof(tmp));
  offset += sizeof(tmp);
  memcpy(*buf + offset, msg->idLock, 80);
  offset += 80;
  for (i = 0; i < n_peers; i++)
  {
    tmp = htonl(lclk[i]);
    memcpy(*buf + offset, &tmp, sizeof(tmp));
    offset += sizeof(tmp);
  }
  return msg_sz;
}

UDP_MSG *deserialize(const unsigned char *buf, const size_t bufSz)
{
  static const size_t MIN_BUF_SZ = 88;
  UDP_MSG *msg;
  if (buf && bufSz < MIN_BUF_SZ)
  {
    return NULL;
  }
  if ((msg = calloc(1, sizeof(UDP_MSG))) == NULL)
  {
    return NULL;
  }
  size_t tmp = 0, offset = 0;
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  memcpy(&msg->op, &tmp, sizeof(uint32_t));
  offset += sizeof(uint32_t);
  memcpy(&msg->idLock, buf + offset, 80);
  offset += 80;
  int i;
  if ((msg->lclk = calloc(n_peers, sizeof(uint32_t))) == NULL)
  {
    free(msg);
    return NULL;
  }
  for (i = 0; i < n_peers; i++)
  {
    memcpy(&tmp, buf + offset, sizeof(tmp));
    tmp = ntohl(tmp);
    memcpy(&msg->lclk[i], &tmp, sizeof(uint32_t));
    offset += sizeof(uint32_t);
  }
  return msg;
}

int process_name(char *name, const int port)
{
  int i;
  bool found = false;
  for (i = 0; i < n_peers && !found; i++)
  {
    if (peers[i].port == port)
    {
      found = true;
      strcpy(name, peers[i].id);
      return 0;
    }
  }
  return -1;
}

int send_message(INFO_SCKT *info, const char *to)
{
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);
  UDP_MSG msg;
  bzero((char *)&msg, sizeof(UDP_MSG));
  msg.op = MSG;
  int port;
  if ((port = getPort(to)) == -1)
  {
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
  if ((msg_sz = serialize(&msg, &buf)) == -1)
  {
    return -1;
  }
  int tam_d = sizeof(SOCKADDR_IN);
  if ((sendto(info->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1)
  {
    return -1;
  }
  printf("%s: SEND(MSG,%s)\n", peers[myIndex].id, to);
  return 0;
}

UDP_MSG *receive_message(const INFO_SCKT *info, char *pname)
{
  SOCKADDR_IN rec;
  unsigned char buff[256] = {0};
  UDP_MSG *msg;
  int tam = sizeof(SOCKADDR_IN);
  int msg_sz;
  if ((msg_sz = recvfrom(info->sckt, buff, 256, 0, (SOCKADDR *)&rec, (socklen_t *)&tam)) == -1)
  {
    return NULL;
  }
  if ((msg = deserialize(buff, msg_sz)) == NULL)
  {
    return NULL;
  }
  if ((process_name(pname, ntohs(rec.sin_port))) == -1)
  {
    return NULL;
  }
  return msg;
}

int getPort(const char *id)
{
  int i;
  bool found = false;
  for (i = 0; i < n_peers && !found; i++)
  {
    if (!strcmp(id, peers[i].id))
    {
      found = true;
      return peers[i].port;
    }
  }
  return -1;
}

int getLockIndex(const char *idLock)
{
  int i;
  bool found = false;
  for (i = 0; i < n_locks && !found; i++)
  {
    if (!strcmp(locks[i].id, idLock))
    {
      return i;
    }
  }
  return -1;
}

int getPeerIndex(const char *idPeer)
{
  int i;
  bool found = false;
  for (i = 0; i < n_peers && !found; i++)
  {
    if (!strcmp(peers[i].id, idPeer))
    {
      return i;
    }
  }
  return -1;
}

int addToQueue(const char *idLock, const char *idPeer)
{
  int lockIndex;
  if ((lockIndex = getLockIndex(idLock)) == -1)
  {
    return -1;
  }
  if (getPeerIndex(idPeer) == -1)
  {
    return -1;
  }
  strcpy(locks[lockIndex].waiting[locks[lockIndex].n_waiting++], idPeer);
  if ((locks[lockIndex].waiting = realloc(locks[lockIndex].waiting,
                                          (1 + locks[lockIndex].n_waiting) * sizeof(char[80]))) == NULL)
  {
    return -1;
  }
  return 0;
}

int prioridad(const int *reqLClk, const int *msgLclk, const char *id)
{
  int i;
  int less = 0;
  int leq = 0;
  int gr = 0;
  int geq = 0;
  for (i = 0; i < n_peers; i++)
  {
    if (reqLClk[i] < msgLclk[i])
    {
      less++;
    }
    else if (reqLClk[i] > msgLclk[i])
    {
      gr++;
    }
    if (reqLClk[i] <= msgLclk[i] || reqLClk[i] >= msgLclk[i])
    {
      geq++;
      leq++;
    }
  }
  if (less > 1 && leq == n_peers)
  {
    return 0;
  }
  else if (gr > 1 && geq == n_peers)
  {
    return 1;
  }
  else
  {
    int pIndex;
    if ((pIndex = getPeerIndex(id)) == -1)
    {
      return -1;
    }
    return myIndex < pIndex ? 0 : 1;
  }
}

int remove_lock(const char *id)
{
  int lockIndex;
  if ((lockIndex = getLockIndex(id)) == -1)
  {
    return -1;
  }
  MUTEX *temp = malloc((n_locks) * sizeof(MUTEX));
  if (!lockIndex)
  {
    memmove(temp, locks + 1, (n_locks - 1) * sizeof(MUTEX));
  }
  else
  {
    memmove(temp, locks, (lockIndex) * sizeof(MUTEX));
    memmove(temp + lockIndex, locks + lockIndex + 1, (n_locks - lockIndex - 1) * sizeof(MUTEX));
  }
  free(locks[lockIndex].req_lclk);
  free(locks[lockIndex].waiting);
  free(locks);
  n_locks--;
  locks = temp;
  return 0;
}

int add_lock(const INFO_SCKT *info, const char *id)
{
  if ((getLockIndex(id) != -1))
  {
    return -1;
  }
  strcpy(locks[n_locks].id, id);
  locks[n_locks].req = true;
  locks[n_locks].inside = false;
  locks[n_locks].n_waiting = 0;
  if ((locks[n_locks].req_lclk = malloc(n_peers * sizeof(int))) == NULL)
  {
    return -1;
  }
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);
  int i;
  for (i = 0; i < n_peers; i++)
    locks[n_locks].req_lclk[i] = lclk[i];
  if ((locks[n_locks++].waiting = malloc(sizeof(char[80]))) == NULL)
  {
    return -1;
  }
  if ((locks = realloc(locks, (1 + n_locks) * sizeof(MUTEX))) == NULL)
  {
    return -1;
  }
  UDP_MSG msg;
  bzero((char *)&msg, sizeof(UDP_MSG));
  msg.op = LOCK;
  strcpy(msg.idLock, id);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  for (i = 0; i < n_peers; i++)
  {
    if (i != myIndex)
    {
      receiver.sin_port = htons(peers[i].port);
      int msg_sz;
      unsigned char *buf = 0;
      if ((msg_sz = serialize(&msg, &buf)) == -1)
      {
        return -1;
      }
      int tam_d = sizeof(SOCKADDR_IN);
      if ((sendto(info->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1)
      {
        return -1;
      }
      printf("%s: SEND(LOCK,%s)\n", peers[myIndex].id, peers[i].id);
    }
  }
  return 0;
}

int unlock(const INFO_SCKT *info, const char *idLock)
{
  int lockIndex;
  if ((lockIndex = getLockIndex(idLock)) == -1)
  {
    return -1;
  }
  UDP_MSG msg;
  bzero((char *)&msg, sizeof(UDP_MSG));
  msg.op = OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  if (locks[lockIndex].n_waiting)
  {
    int i, peerIndex;
    for (i = 0; i < locks[lockIndex].n_waiting; i++)
    {
      lclk[myIndex]++;
      printf("%s: TICK\n", peers[myIndex].id);
      if ((peerIndex = getPeerIndex(locks[lockIndex].waiting[i])) == -1)
      {
        return -1;
      }
      receiver.sin_port = htons(peers[peerIndex].port);
      int msg_sz;
      unsigned char *buf = 0;
      if ((msg_sz = serialize(&msg, &buf)) == -1)
      {
        return -1;
      }
      int tam_d = sizeof(SOCKADDR_IN);
      if ((sendto(info->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1)
      {
        return -1;
      }
      printf("%s: SEND(OK,%s)\n", peers[myIndex].id, locks[lockIndex].waiting[i]);
    }
  }
  remove_lock(idLock);
  return 0;
}

int sendOkLockRequest(const INFO_SCKT *info, const char *idPeer, const char *idLock)
{
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);
  int peerIndex;
  if ((peerIndex = getPeerIndex(idPeer)) == -1)
  {
    return -1;
  }
  UDP_MSG msg;
  bzero((char *)&msg, sizeof(UDP_MSG));
  msg.op = OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  receiver.sin_port = htons(peers[peerIndex].port);
  int msg_sz;
  unsigned char *buf = 0;
  if ((msg_sz = serialize(&msg, &buf)) == -1)
  {
    return -1;
  }
  int tam_d = sizeof(SOCKADDR_IN);
  if ((sendto(info->sckt, buf, msg_sz, 0, (struct sockaddr *)&receiver, tam_d)) == -1)
  {
    return -1;
  }
  printf("%s: SEND(OK,%s)\n", peers[myIndex].id, idPeer);
  return 0;
}

////////////////////////// MAIN
int main(int argc, char *argv[]){
  int port;
  char line[80], proc[80];
  if (argc < 2){
    fprintf(stderr, "Uso: proceso <ID>\n");
    return 1;
  }
  /* Establece el modo buffer de entrada/salida a línea */
  setvbuf(stdout, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);
  setvbuf(stdin, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);
  INFO_SCKT info;
  if (open_udp(&info) == -1){
    return -1;
  }
  udp_port = ntohs(info.sckaddr.sin_port);
  fprintf(stdout, "%s: %d\n", argv[1], udp_port);
  if ((peers = malloc(sizeof(PEER))) == NULL){
    return -1;
  }
  if ((locks = malloc(sizeof(MUTEX))) == NULL){
    return -1;
  }
  if ((locks->waiting = malloc(sizeof(PEER))) == NULL){
    return -1;
  }
  for (; fgets(line, 80, stdin);){
    if (!strcmp(line, "START\n"))
      break;
    sscanf(line, "%[^:]: %d", proc, &port);
    if (!strcmp(proc, argv[1])){
      myIndex = n_peers;
    }
    if (store_peer_sckt(proc, port) == -1){
      free(peers);
      return -1;
    }
  }
  if ((peers = realloc(peers, (n_peers) * sizeof(PEER))) == NULL){
    free(peers);
    return -1;
  }
  if (init_lclk() == -1) {
    free(peers);
    return -1;
  }
  char id_sec[80];
  char action[80];
  for (; fgets(line, 80, stdin);){
    if (!strcmp(line, "EVENT\n")) {
      lclk[myIndex]++;
      printf("%s: TICK\n", peers[myIndex].id);
      continue;
    }
    if (!strcmp(line, "GETCLOCK\n")) {
      print_lclk();
      continue;
    }
    if (!strcmp(line, "RECEIVE\n")){
      UDP_MSG *msg;
      int lockIndex;
      char pname[80];
      if ((msg = receive_message(&info, pname)) == NULL){
        free(msg);
        free(lclk);
        free(peers);
        return -1;
      }
      switch (msg->op){
        case MSG:
          printf("%s: RECEIVE(MSG,%s)\n", peers[myIndex].id, pname);
          update_lclk(msg->lclk);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          break;
        case LOCK:
          printf("%s: RECEIVE(LOCK,%s)\n", peers[myIndex].id, pname);
          update_lclk(msg->lclk);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          if ((lockIndex = getLockIndex(msg->idLock)) == -1) {
            if (sendOkLockRequest(&info, pname, msg->idLock) == -1){
              free(msg);
              free(lclk);
              free(peers);
              return -1;
            }
          }
          else{
            if (locks[lockIndex].inside){
              if (addToQueue(msg->idLock, pname) == -1){
                free(msg);
                free(lclk);
                free(peers);
                return -1;
              }
            }
            else {
              if (prioridad(locks[lockIndex].req_lclk, msg->lclk, pname) == 1){
                if (sendOkLockRequest(&info, pname, msg->idLock) == -1){
                  free(msg);
                  free(lclk);
                  free(peers);
                  return -1;
                }
              }
              else{
                if (addToQueue(msg->idLock, pname) == -1){
                  free(msg);
                  free(lclk);
                  free(peers);
                  return -1;
                }
              }
            }
          }
          break;
        case OK:
          printf("%s: RECEIVE(OK,%s)\n", peers[myIndex].id, pname);
          update_lclk(msg->lclk);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          if ((lockIndex = getLockIndex(msg->idLock)) == -1){
            free(msg);
            free(lclk);
            free(peers);
            return -1;
          }
          locks[lockIndex].ok++;
          if (locks[lockIndex].ok == n_peers - 1){
            locks[lockIndex].inside = true;
            printf("%s: MUTEX(%s)\n", peers[myIndex].id, msg->idLock);
          }
        }
      free(msg);
    }
    if (!strcmp(line, "FINISH\n")){
      printf("%s: FINISH[%i]\n", peers[myIndex].id, getpid());
      break;
    }
    sscanf(line, "%s %s", action, id_sec);
    if (!strcmp(action, "MESSAGETO")){
      if (send_message(&info, id_sec) == -1){
        free(lclk);
        free(peers);
        return -1;
      }
    }
    if (!strcmp(action, "LOCK")){
      if (add_lock(&info, id_sec) == -1){
        free(lclk);
        free(peers);
        return -1;
      }
    }
    if (!strcmp(action, "UNLOCK")){
      if (unlock(&info, id_sec) == -1){
        free(lclk);
        free(peers);
        return -1;
      }
    }
  }
  free(lclk);
  free(peers);
  return 0;
}