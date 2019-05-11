#include "data.h"
#include "lib_getters.h"
#include "lib_clock.h"

/** Initializes the clock
 *  Retrieves: 0 if succes, -1 if failure
 */
int init_lclk(int *lclk, int *past_lclk, int n_peers){
  if((lclk=calloc(n_peers, sizeof(int)))==NULL){
      perror("Error");
      return -1;
    }
  if((past_lclk=calloc(n_peers, sizeof(int)))==NULL){
      perror("Error");
      return -1;
    }
  return 0;
}

/** Prints the clock
 */
void print_lclk(PEER *peers, int n_peers, int myIndex, int *lclk){
  int i;
  printf("%s: LC[", peers[myIndex].id);
  for(i=0; i<n_peers; i++){
      if(i==n_peers-1)
	printf("%i]\n", lclk[i]);
      else
	printf("%i,", lclk[i]);
    }
}

/** Updates the local and the message clock
 */
void update_lclk(const int *r_lclk, int *lclk, int n_peers){
  int i;
  for(i=0; i<n_peers; i++)
    lclk[i] = lclk[i]>r_lclk[i]? lclk[i]:r_lclk[i];
}

/** Retrieves 0 if the clock is olther than the actual clock,
 *  1 if it is younghter or -1 if failure
 */
int priority(const int *reqLClk, const int * msgLclk, const char *id, PEER *peers, int n_peers, int myIndex){
  int i;
  int less=0;
  int leq=0;
  int gr=0;
  int geq=0;
  for (i = 0; i<n_peers; i++){
      if (reqLClk[i] < msgLclk[i]) {
	    less++;
      }
      else if (reqLClk[i] > msgLclk[i]) {
	    gr++;
      } 
      if (reqLClk[i] <= msgLclk[i] || reqLClk[i] >= msgLclk[i] ) {
	    geq++;
	    leq++;
      }
  }
  if (less>1 && leq==n_peers) {
    return 0;
  }
  else if (gr>1 && geq==n_peers) {
    return 1;
  }
  else {
    int pIndex;
    if((pIndex=getPeerIndex(id, peers, n_peers))==-1){
	//fprintf(stderr, "El proceso con id \"%s\" no se ha encontrado\n", id);
	    return -1;
      }
    //printf("mi index es %d y el del otro es %d\n", myIndex, pIndex);
    return myIndex<pIndex? 0:1; 
  }
}