#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
struct listaProcesos {
	int puerto;
	char *nombre;
	struct listaProcesos *next;
} listaProcesos;

typedef struct cabecera {
	int tipoM;
	int tamEmisor;
	int tamSec;
} cabecera;

typedef struct contenido {
	char *emisor;
	int *reloj;
	char *sec;	
} contenido;

int buscarPuerto(char *id, struct listaProcesos *primero, struct listaProcesos *ultimo);