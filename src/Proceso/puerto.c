#include "puerto.h"

int buscarPuerto(char *id, struct listaProcesos *primero, struct listaProcesos *ultimo) {
	struct listaProcesos *aux = primero;
	int puerto;

	while (1) {
		if (!strcmp(aux->nombre, id)) {
			return aux->puerto;
		} else {
			if (ultimo == aux) {
			return -1;
			} else {
			aux = aux->next;
			}
		}
	}
	return 0;
}