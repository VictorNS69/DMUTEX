#include "data.h"

int serialize(const MESSAGE *msg, unsigned char **buf, int *lclk, int n_peers);
MESSAGE *deserialize(const unsigned char *buf, const size_t bufSz, int n_peers);
int process_name(char *name, const int port, PEER *peers, int n_peers);
int send_message(SOCKET *info, const char *to, int *lclk, int myIndex, PEER *peers, int n_peers);
MESSAGE *receive_message(const SOCKET *info, char *pname, PEER *peers, int n_peers);
