#include "data.h"
#include "lib_socket.h"

/** Opens a socket and store its info
 *  in socket ("s") object 
 *  Returns: 0 if success, -1 if failure
 */
int open_udp(SOCKET *s){
  if((s->sckt=socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP))==-1){
    //perror("Error");
    return -1;
  }
  bzero((char*)&(s->sckaddr), sizeof(SOCKADDR_IN));                                            
    s->sckaddr.sin_family = AF_INET;                                
  s->sckaddr.sin_addr.s_addr=INADDR_ANY;
  s->sckaddr.sin_port = htons(0);
  if(bind(s->sckt, (struct sockaddr*)&(s->sckaddr), sizeof(SOCKADDR_IN))==-1){
    //perror("Error");
    close(s->sckt);
    return -1;
  }
  int tam_dir=sizeof(SOCKADDR_IN);
  if(getsockname(s->sckt, (struct sockaddr*)&(s->sckaddr), (socklen_t*) &tam_dir)==-1){
    //perror("Error");
    close(s->sckt);
    return -1;
  }
  return 0;
}

/** Stores the process and the port in a object
 *  Retrieves: 0 if success, -1 if failure
 */
int store_peer_sckt(const char *proc, const int port, PEER *peers, int n_peers){
  strcpy(peers[n_peers].id, proc);
  peers[n_peers++].port = port;
  if((peers=realloc(peers, (1+n_peers)*sizeof(PEER)))==NULL){
      //perror("Error");
      return -1;
    }
  return 0;
}