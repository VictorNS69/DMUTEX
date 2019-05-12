#include "data.h"

int add_lock(const SOCKET *s, const char *id, PEER *peers, int n_peers, int myIndex, MUTEX *locks, int n_locks, int *lclk);
int unlock(const SOCKET *s, const char *idLock, PEER *peers, int n_peers, MUTEX *locks, int n_locks, int myIndex, int *lclk);
int remove_lock(const char *id, MUTEX *locks, int n_locks);
int addToQueue(const char *idLock, const char* idPeer, MUTEX *locks, int n_locks, PEER *peers, int n_peers);
int sendOkLockRequest(const SOCKET *s, const char *idPeer, const char *idLock, int *lclk, int myIndex, PEER *peers, int n_peers);