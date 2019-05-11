#include "data.h"

int init_lclk(int *lclk, int *past_lclk, int n_peers);
void print_lclk(PEER *peers, int n_peers, int myIndex, int *lclk);
void update_lclk(const int *r_lclk, int *lclk, int n_peers);
int priority(const int *reqLClk, const int * msgLclk, const char *id, PEER *peers, int n_peers, int myIndex);
