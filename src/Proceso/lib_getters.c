#include "data.h"
#include "lib_getters.h"

/** Retrieves the port of a given id,
 *  -1 if failure
 */
int getPort(const char *id, PEER *peers, int n_peers){
  int i;
  bool found=false;
  for(i=0; i<n_peers && !found; i++){
      if(!strcmp(id, peers[i].id)){
	  found=true;
	  return peers[i].port;
	}
  }
  return -1;
}

/** Retrieves the index of a lock,
 *  -1 if failure
 */
int getLockIndex(const char *idLock, MUTEX *locks, int n_locks){
  int i;
  bool found=false;
  for (i=0; i < n_locks && !found; i++){
      if (!strcmp(locks[i].id, idLock)){
	  return i;
	}
  }
  return -1;
}

/** Retrieves the index of a process given id,
 *  -1 if failure
 */
int getPeerIndex(const char *idPeer, PEER *peers, int n_peers){
  int i;
  bool found=false;
  for (i=0; i < n_peers && !found; i++){
      if (!strcmp(peers[i].id, idPeer)){
	  return i;
	}
  }
  return -1;
}