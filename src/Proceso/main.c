/* DMUTEX (2019) Sistemas Operativos Distribuidos
 * Author: Víctor Nieves Sánchez
*/
#include "data.h"
#include "lib_clock.h"
#include "lib_lock.h"
#include "lib_getters.h"
#include "lib_message.h"
#include "lib_socket.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

int main(int argc, char *argv[]){
  int puerto_udp;
  int port;
  char line[80], proc[80];
  PEER *peers;
  int n_peers = 0;
  int *lclk;
  int *past_lclk;
  int myIndex;
  MUTEX *locks;
  int n_locks;
  SOCKET info;

  if (argc < 2)  {
    fprintf(stderr, "Uso: proceso <ID>\n");
    return 1;
  }
  /* Establece el modo buffer de entrada/salida a línea */
  setvbuf(stdout, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);
  setvbuf(stdin, (char *)malloc(sizeof(char) * 80), _IOLBF, 80);
  /* Se determina el puerto UDP que corresponda.
  Dicho puerto debe estar libre y quedará
  reservado por dicho proceso. */
  if (open_udp(&info) == -1)  {
    //fprintf(stderr, "Error al crear el socket UDP\n");
    return -1;
  }
  puerto_udp = ntohs(info.sckaddr.sin_port);
  fprintf(stdout, "%s: %d\n", argv[1], puerto_udp);
  if ((peers = malloc(sizeof(PEER))) == NULL){
    //fprintf(stderr, "Error al crear la lista de procesos\n");
    return -1;
  }
  if ((locks = malloc(sizeof(MUTEX))) == NULL){
    //fprintf(stderr, "Error al crear la lista de cerrojos\n");
    return -1;
  }
  if ((locks->waiting = malloc(sizeof(PEER))) == NULL){
    //fprintf(stderr, "Error al crear la lista de espera en cerrojo\n");
    return -1;
  }
  for (; fgets(line, 80, stdin);){
    if (!strcmp(line, "START\n")){
      break;
    }
    sscanf(line, "%[^:]: %d", proc, &port);
    /* Habra que guardarlo en algun sitio */

    if (!strcmp(proc, argv[1])){
      /* Este proceso soy yo */
      myIndex = n_peers;
    }
    if (store_peer_sckt(proc, port, peers, n_peers) == -1){
      //fprintf(stderr, "Error al almacenar información de procesos\n");
      free(peers);
      return -1;
    }
  }
  /* Tamaño final */
  if ((peers = realloc(peers, (n_peers) * sizeof(PEER))) == NULL){
    //fprintf(stderr, "Error al dejar el tamaño final de la lista de procesos\n");
    free(peers);
    return -1;
  }
  /* Inicializar Reloj */
  if (init_lclk(lclk, past_lclk, n_peers) == -1){
    //fprintf(stderr, "Error al inicializar los relojes lógicos\n");
    free(peers);
    return -1;
  }
  /* Procesar Acciones */
  char id_sec[80];
  char action[80];
  for (; fgets(line, 80, stdin);){
    if (!strcmp(line, "EVENT\n")){
      lclk[myIndex]++;
      printf("%s: TICK\n", peers[myIndex].id);
      continue;
    }
    if (!strcmp(line, "GETCLOCK\n")){
      print_lclk(peers, n_peers, myIndex, lclk);
      continue;
    }
    if (!strcmp(line, "RECEIVE\n")){
      MESSAGE *msg;
      int lockIndex;
      char pname[80];
      if ((msg = receive_message(&info, pname, peers, n_peers)) == NULL){
        //fprintf(stderr, "Error al recibir mensaje\n");
        free(msg);
        free(lclk);
        free(peers);
        return -1;
      }
      switch (msg->op){
        case MSG:
          printf("%s: RECEIVE(MSG,%s)\n", peers[myIndex].id, pname);
          update_lclk(msg->lclk, lclk, n_peers);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          break;
        case LOCK:
          printf("%s: RECEIVE(LOCK,%s)\n", peers[myIndex].id, pname);
          update_lclk(msg->lclk, lclk, n_peers);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          if ((lockIndex = getLockIndex(msg->idLock, locks, n_locks)) == -1){
            if (sendOkLockRequest(&info, pname, msg->idLock, lclk, myIndex, peers, n_peers) == -1) {
              //fprintf(stderr, "No se ha podido enviar OK a \"%s\"", pname);
              free(msg);
              free(lclk);
              free(peers);
              return -1;
            }
          }
          else{
            if (locks[lockIndex].inside){
              if (addToQueue(msg->idLock, pname, locks, n_locks, peers, n_peers) == -1){
                //fprintf(stderr, "No se ha podido añadir a \"%s\" a la cola del lock \"%s\"", pname, msg->idLock);
                free(msg);
                free(lclk);
                free(peers);
                return -1;
              }
            }
            else{
              if (priority(locks[lockIndex].req_lclk, msg->lclk, pname, peers, n_peers, myIndex) == 1){
                if (sendOkLockRequest(&info, pname, msg->idLock, lclk, myIndex, peers, n_peers) == -1){
                  //fprintf(stderr, "No se ha podido enviar OK a \"%s\"", pname);
                  free(msg);
                  free(lclk);
                  free(peers);
                  return -1;
                }
              }
              else{
                if (addToQueue(msg->idLock, pname, locks, n_locks, peers, n_peers) == -1){
                  //fprintf(stderr, "No se ha podido añadir a \"%s\" a la cola del lock \"%s\"", pname, msg->idLock);
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
          update_lclk(msg->lclk, lclk, n_peers);
          lclk[myIndex]++;
          printf("%s: TICK\n", peers[myIndex].id);
          if ((lockIndex = getLockIndex(msg->idLock, locks, n_locks)) == -1){
            //fprintf(stderr, "No se ha encontrado el lock con id \"%s\"", msg->idLock);
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
          //break;
      }
      free(msg);
    }
    if (!strcmp(line, "FINISH\n")){
      printf("%s: FINISH[%i]\n", peers[myIndex].id, getpid());
      break;
    }
    sscanf(line, "%s %s", action, id_sec);
    if (!strcmp(action, "MESSAGETO")){
      if (send_message(&info, id_sec, lclk, myIndex, peers, n_peers) == -1){
        free(lclk);
        free(peers);
        return -1;
      }
    }
    if (!strcmp(action, "LOCK")){
      if (add_lock(&info, id_sec, peers, n_peers, myIndex, locks, n_locks, lclk) == -1){
        free(lclk);
        free(peers);
        return -1;
      }
    }
    if (!strcmp(action, "UNLOCK")){
      if (unlock(&info, id_sec, peers, n_peers, locks, n_locks, myIndex, lclk) == -1){
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
