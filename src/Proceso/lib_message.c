#include "data.h"
#include "lib_getters.h"
#include "lib_message.h"

/** Serializes a UDP message
 *  Retrieves: the size of the message if success,
 *  -1 if failure
 */
int serialize(const MESSAGE *msg, unsigned char **buf, int *lclk, int n_peers){
  int msg_sz = sizeof(uint32_t) + 80 + n_peers * sizeof(uint32_t);
  if((*buf = calloc(1, msg_sz))==NULL){
      //perror("Error");
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

  for(i=0; i<n_peers; i++){
      tmp = htonl(lclk[i]);
      memcpy(*buf + offset, &tmp, sizeof(tmp));
      offset += sizeof(tmp);  
    }
  return msg_sz;
}

/** Deserializes a UDP message
 *  Retrieves: the message if succes,
 *  NULL message if failure
 */
MESSAGE *deserialize(const unsigned char *buf, const size_t bufSz, int n_peers){
  static const size_t MIN_BUF_SZ = 88;
  MESSAGE  *msg;
  if (buf && bufSz < MIN_BUF_SZ){
      //fprintf(stderr, "El tamaño del buffer es menor que el mínimo\n");
      return NULL;
    }
  if((msg = calloc(1, sizeof(MESSAGE)))==NULL){
      //perror("Error");
      return NULL;
    }
  size_t tmp = 0, offset = 0;
  memcpy(&tmp, buf + offset, sizeof(tmp));
  tmp = ntohl(tmp);
  memcpy(&msg->op, &tmp, sizeof(uint32_t));
  offset  += sizeof(uint32_t);
  memcpy(&msg->idLock, buf + offset, 80);
  offset  += 80;
  int i;
  if((msg->lclk=calloc(n_peers, sizeof(uint32_t)))==NULL){
      perror("Error");
      free(msg);
      return NULL;
    }
  for(i=0; i<n_peers; i++){
      memcpy(&tmp, buf + offset, sizeof(tmp));
      tmp = ntohl(tmp);
      memcpy(&msg->lclk[i], &tmp, sizeof(uint32_t));
      offset  += sizeof(uint32_t);
    }
  return msg;
}

/** Retrieves the name of a procees given a port
 *  Retrieves: 0 if success, -1 if failure
 */
int process_name(char *name, const int port, PEER *peers, int n_peers){
  int i;
  bool found=false;
  for(i=0; i<n_peers && !found; i++){
      if(peers[i].port == port){
	  found=true;
	  strcpy(name, peers[i].id);
	  return 0;
	}
  }
  return -1;
}

/** Sends a message
 *  Retrieves: 0 if success, -1 if failure
 */
int send_message(SOCKET *info, const char *to, int *lclk, int myIndex, PEER *peers, int n_peers){
  lclk[myIndex]++;
  printf("%s: TICK\n", peers[myIndex].id);
  MESSAGE msg;
  bzero((char*)&msg, sizeof(MESSAGE));
  msg.op=MSG;
  int port;
  if((port=getPort(to, peers, n_peers))==-1){
      //fprintf(stderr,"No se ha encontrado el destinatario indicado\n");
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
  if((msg_sz = serialize(&msg, &buf, lclk, n_peers))==-1){
      //fprintf(stderr, "Error al serializar mensaje(MSG)\n");
      return -1;
    }
  int tam_d=sizeof(SOCKADDR_IN);
  if((sendto(info->sckt, buf, msg_sz,0, (struct sockaddr*)&receiver,tam_d))==-1){
      //fprintf(stderr, "Error al enviar mensaje(MSG) a %s\n", to);
      return -1;
    }
  printf("%s: SEND(MSG,%s)\n",peers[myIndex].id, to);
  return 0;
}

/** Recives an UDP message
 *  Retrieves: the message if succes,
 *  NULL message if failure
 */
MESSAGE *receive_message(const SOCKET *info, char *pname, PEER *peers, int n_peers){
  SOCKADDR_IN rec;
  unsigned char buff[256] = {0};
  MESSAGE *msg;
  int tam = sizeof(SOCKADDR_IN);
  int msg_sz;
  if((msg_sz=recvfrom(info->sckt, buff, 256, 0, (SOCKADDR*)&rec, (socklen_t*)&tam))==-1){
      //perror("Error");
      return NULL;
    }
  if((msg=deserialize(buff, msg_sz, n_peers))==NULL){
      //fprintf(stderr, "Error al deserializar el mensaje\n");
      return NULL;
    }	  
  if((process_name(pname, ntohs(rec.sin_port), peers, n_peers))==-1){
      //fprintf(stderr, "No ha sido posible identificar el emisor del mensaje\n");
      return NULL;
    }
  return msg;
}