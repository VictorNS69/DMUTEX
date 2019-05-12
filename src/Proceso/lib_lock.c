#include "data.h"
#include "lib_getters.h"
#include "lib_message.h"
#include "lib_lock.h"

/** Adds a new lock
 * Retrieves: 0 if success, -1 if failure
 */
int add_lock(const SOCKET *s, const char *id, PEER *peers, int n_peers, int myIndex, MUTEX *locks, int n_locks, int *lclk){
  if ((getLockIndex(id, locks, n_locks)!=-1)){
      //fprintf(stderr, "Ya se ha hecho una petición del lock %s anteriormente\n", id);
      return -1;
  }
  strcpy(locks[n_locks].id, id);
  locks[n_locks].req = true;
  locks[n_locks].inside = false;
  locks[n_locks].n_waiting = 0;
  if ((locks[n_locks].req_lclk = malloc(n_peers*sizeof(int)))==NULL){
      //fprintf(stderr, "Error al reservar memoria para reloj lógico antiguo de lock \"%s\"\n", id);
      return -1;
    }
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);

  int i;
  for(i=0; i<n_peers;i++){
    locks[n_locks].req_lclk[i] = lclk[i];
  }
  if ((locks[n_locks++].waiting = malloc(sizeof(char[80])))==NULL){
      //fprintf(stderr, "Error al reservar memoria para lista de espera de lock \"%s\"\n", id);
      return -1;
    }
  if((locks = realloc(locks, (1+n_locks)*sizeof(MUTEX)))==NULL){
      //fprintf(stderr, "Error al reservar espacio extra para nuevo lock\n");
      return -1;
    }
  MESSAGE msg;
  bzero((char*)&msg, sizeof(MESSAGE));
  msg.op=LOCK;
  strcpy(msg.idLock, id);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  for(i=0; i<n_peers;i++){
      if(i!=myIndex){
        receiver.sin_port = htons(peers[i].port);
        int msg_sz;
        unsigned char *buf = 0;
        if((msg_sz = serialize(&msg, &buf, lclk, n_peers))==-1){
            //fprintf(stderr, "Error al serializar mensaje(LOCK)\n");
            return -1;
            }
        int tam_d=sizeof(SOCKADDR_IN);
        if((sendto(s->sckt, buf, msg_sz,0, (struct sockaddr*)&receiver,tam_d))==-1) {
            //fprintf(stderr, "Error al enviar mensaje(LOCK) a %s\n", peers[i].id);
            return -1;
            }
        printf("%s: SEND(LOCK,%s)\n",peers[myIndex].id, peers[i].id);
	   }
    }
  return 0;
}

/** Unlocks a lock
 * Retrieves: 0 if success, -1 if failure
 */
int unlock(const SOCKET *s, const char *idLock, PEER *peers, int n_peers, MUTEX *locks, int n_locks, int myIndex, int *lclk){
  int lockIndex;
  if ((lockIndex=getLockIndex(idLock, locks, n_locks))==-1){
      //fprintf(stderr, "No se ha encontrado el cerrojo con el id \"%s\"\n", idLock);
      return -1;
  }
  MESSAGE msg;
  bzero((char*)&msg, sizeof(MESSAGE));
  msg.op=OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  if(locks[lockIndex].n_waiting){      
      int i, peerIndex;
      for(i=0; i<locks[lockIndex].n_waiting;i++){
        lclk[myIndex]++;
        printf("%s: TICK\n", peers[myIndex].id);
        if ((peerIndex=getPeerIndex(locks[lockIndex].waiting[i], peers, n_peers))==-1){
            //fprintf(stderr, "No se ha encontrado el destinatario con id \"%s\"\n", locks[lockIndex].waiting[i]);
            return -1;
        }
        receiver.sin_port = htons(peers[peerIndex].port);
        int msg_sz;
        unsigned char *buf = 0;
        if((msg_sz = serialize(&msg, &buf, lclk, n_peers))==-1){
            //fprintf(stderr, "Error al serializar mensaje(OK)\n");
            return -1;
        }
        int tam_d=sizeof(SOCKADDR_IN);
        if((sendto(s->sckt, buf, msg_sz,0, (struct sockaddr*)&receiver,tam_d))==-1){
            //fprintf(stderr, "Error al enviar mensaje(OK) a %s\n", locks[lockIndex].waiting[i]);
            return -1;
        }
        printf("%s: SEND(OK,%s)\n",peers[myIndex].id, locks[lockIndex].waiting[i]);	  
	  }
    }
  //remove_lock(idLock);
  return 0;
}

/** Removes a lock
 *  Retrieves: 0 if succes, -1 if failure
 */
int remove_lock(const char *id, MUTEX *locks, int n_locks){
  int lockIndex;
  if((lockIndex=getLockIndex(id, locks, n_locks))==-1){
      //fprintf(stderr, "No se ha encontrado el lock con id \"%s\"\n", id);
      return -1;
    }
  MUTEX *temp = malloc((n_locks)*sizeof(MUTEX));
  if(!lockIndex){
      memmove(temp, locks+1, (n_locks-1)*sizeof(MUTEX));
    }
  else{
      memmove(temp, locks, (lockIndex)*sizeof(MUTEX));
      memmove(temp+lockIndex, locks+lockIndex+1, (n_locks-lockIndex-1)*sizeof(MUTEX));
    }
  free(locks[lockIndex].req_lclk);
  free(locks[lockIndex].waiting);
  free(locks);
  n_locks--;
  locks = temp;
  return 0;
}

/** Adds a process to a lock's queue
 *  Retrieves: 0 if succes, -1 if failure 
 */
int addToQueue(const char *idLock, const char* idPeer, MUTEX *locks, int n_locks, PEER *peers, int n_peers){
  int lockIndex;
  if ((lockIndex=getLockIndex(idLock, locks, n_locks))==-1){
      //fprintf(stderr, "No se ha encontrado el cerrojo con el id \"%s\"\n", idLock);
      return -1;
    }

  if (getPeerIndex(idPeer, peers, n_peers)==-1){
      //fprintf(stderr, "No se ha encontrado el destinatario con id \"%s\"\n", idPeer);
      return -1;
    }
  
  strcpy(locks[lockIndex].waiting[locks[lockIndex].n_waiting++], idPeer);
  if((locks[lockIndex].waiting = realloc(locks[lockIndex].waiting, (1+locks[lockIndex].n_waiting)*sizeof(char[80])))==NULL){
      //fprintf(stderr, "Error al reservar memoria en cola para cerrojo \"%s\"\n", idLock);
      return -1;
    }
  return 0;
}

/** Sends the request
 *  Retrieves: 0 if success, -1 if failure
 */
int sendOkLockRequest(const SOCKET *s, const char *idPeer, const char *idLock, int *lclk, int myIndex, PEER *peers, int n_peers){
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);
  int peerIndex;
  if ((peerIndex=getPeerIndex(idPeer, peers, n_peers))==-1){
      //fprintf(stderr, "No se ha encontrado el destinatario con id \"%s\"\n", idPeer);
      return -1;
  }
  MESSAGE msg;
  bzero((char*)&msg, sizeof(MESSAGE));
  msg.op=OK;
  strcpy(msg.idLock, idLock);
  SOCKADDR_IN receiver;
  struct hostent *netdb;
  netdb = gethostbyname(HOST);
  receiver.sin_family = AF_INET;
  memcpy(&(receiver.sin_addr), netdb->h_addr, netdb->h_length);
  receiver.sin_port = htons(peers[peerIndex].port);
  int msg_sz;
  unsigned char *buf = 0;
  if((msg_sz = serialize(&msg, &buf, lclk, n_peers))==-1){
      //fprintf(stderr, "Error al serializar mensaje(OK)\n");
      return -1;
  }     
  int tam_d=sizeof(SOCKADDR_IN);
  if((sendto(s->sckt, buf, msg_sz,0, (struct sockaddr*)&receiver,tam_d))==-1){
      //fprintf(stderr, "Error al enviar mensaje(OK) a %s\n", idPeer);
      return -1;
  }
  printf("%s: SEND(OK,%s)\n",peers[myIndex].id, idPeer);
  return 0;
}
