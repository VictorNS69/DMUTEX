/* DMUTEX (2019) Sistemas Operativos Distribuidos
 * Author: Víctor Nieves Sánchez
*/
#include "puerto.h"
struct listaProcesos *primero, *ultimo;
char *nombre;
int puerto_udp, nProceso;	

int main(int argc, char* argv[]) {
	struct listaProcesos *nuevo;
	struct sockaddr_in direccion;
	struct sockaddr_in sin;
	socklen_t addrlen = sizeof(sin);
	int port, descriptor, nProcesos;
      	char line[80],proc[80];
		
      	if (argc < 2 || argc > 2) {
	  	fprintf(stderr,"Uso: proceso <ID>\n");
	  	return 1;
    	}

	nombre = argv[1];

	/* Establece el modo buffer de entrada/salida a línea */
	setvbuf(stdout,(char*)malloc(sizeof(char)*80),_IOLBF,80);  
       	setvbuf(stdin,(char*)malloc(sizeof(char)*80),_IOLBF,80); 

	if ((descriptor = socket (AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "Error en el socket\n");
		close(descriptor);
		exit(-1);		
	}

	direccion.sin_family = AF_INET;	
	direccion.sin_port = htons(0);	/* Que el sistema elija el puerto.*/
	direccion.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind (descriptor, (struct sockaddr *)&direccion, sizeof(direccion))) {
		fprintf(stderr, "Error en el bind\n");
		close(descriptor);
		exit(-1);
	} 

	if (getsockname(descriptor, (struct sockaddr *)&sin, &addrlen) == 0 && 
			sin.sin_family == AF_INET && addrlen == sizeof(sin)) {
		puerto_udp = ntohs(sin.sin_port);
	} else {
		fprintf(stderr, "Error en el getsockname\n");
		close(descriptor);
		exit(-1);
	}

      	fprintf(stdout, "%s: %d\n", argv[1], puerto_udp);

	nProcesos = 0;
	nProceso = nProcesos;

      	for(;fgets(line,80,stdin);) {
		if(!strcmp(line,"START\n"))
			break;

		sscanf(line,"%[^:]: %d",proc,&port);
		nProcesos++;

	  	/* Habra que guardarlo en algun sitio */
		nuevo = (struct listaProcesos *) malloc(sizeof(struct listaProcesos));
		if (nuevo == NULL) {
			fprintf(stderr, "Error en el malloc\n");
			exit(-1);
		}

		nuevo->puerto = port;
		nuevo->nombre = proc;
		if (primero == NULL) {
			primero = nuevo;
			ultimo = nuevo;
		} else {
			ultimo->next = nuevo;
			ultimo = nuevo;
		}

	  	if(!strcmp(proc,argv[1])) {
			nProceso = nProcesos - 1;/* Este proceso soy yo */
		}
    	}

      	/* Inicializar Reloj */
	int vector[nProcesos];
	int i;
	for (i = 0; i < nProcesos; i++) {
		vector[i] = 0;
	}

      	/* Procesar Acciones */
	for(;fgets(line,80,stdin);) {	
		if (!strcmp(line, "EVENT\n")) {
			vector[nProceso]++;
			fprintf(stdout,"%s: TICK\n", nombre);
		} else if (!strcmp(line, "GETCLOCK\n")) {
			int j;
			fprintf(stdout,"%s: LC[", argv[1]);
			for(j = 0; j < nProcesos; j++) {
				fprintf(stdout, "%d", vector[j]);
				if (j != nProcesos - 1) {
					fprintf(stdout, ",");
				}
			}
			fprintf(stdout, "]\n");
		} else if (!strcmp(line, "RECEIVE\n")) {
			struct cabecera mnsj;
			int reloj[nProcesos];
			char emisor[512];
			char *tipo;

			if  (read(descriptor, (void *) &mnsj, sizeof(mnsj)) < 0) {
				fprintf(stderr, "Error en el read\n");
				close(descriptor);
				exit(-1);
			}
			if (mnsj.tipoM == 0) {
				tipo = "MSG";
				if  (read(descriptor, &emisor, mnsj.tamEmisor) < 0) {
					fprintf(stderr, "Error en el read\n");
					close(descriptor);
					exit(-1);
				}
				emisor[mnsj.tamEmisor] = '\0';

				if  (read(descriptor, &reloj, nProcesos) < 0) {
					fprintf(stderr, "Error en el read\n");
					close(descriptor);
					exit(-1);
				}

				fprintf(stdout, "%s: RECEIVE(%s,%s)\n", nombre, tipo, emisor);

				for (i = 0; i < nProcesos; i++) {
					if (i == nProceso) {
						vector[nProceso]++;
					} else {
						if (vector[i] < reloj[i]) {
							vector[i] = reloj[i];
						}
					}
				}

				fprintf(stdout, "%s: TICK\n", nombre);

			}
		} else if (!strncmp(line, "MESSAGETO", strlen("MESSAGETO"))) {
			int puertoD;
			struct cabecera mnsj;
			struct contenido cnt;
			struct sockaddr_in dest;
			char idLeido[(int)strlen(line) - 11];
			memcpy(idLeido, &line[10], (int)strlen(line) - 10);
			idLeido[strlen(idLeido) - 1] = '\0';

			puertoD = buscarPuerto(idLeido, primero, ultimo);

			dest.sin_family = AF_INET;	
			dest.sin_port = htons(puertoD);
			dest.sin_addr.s_addr = htonl(INADDR_ANY);
			addrlen = sizeof(dest);

			vector[nProceso]++;
			fprintf(stdout, "%s: TICK\n", nombre);

			mnsj.tipoM = 0;
			mnsj.tamEmisor = strlen(nombre);
			mnsj.tamSec = 0;

			cnt.emisor = nombre;
			cnt.reloj = vector;
			cnt.sec = 0;

			if (sendto(descriptor, &mnsj, sizeof(mnsj), 0, (struct sockaddr *) &dest, addrlen) < 0) {
				fprintf(stderr, "Error en el sendto\n");
				close(descriptor);
				exit(-1);
			}

			if (sendto(descriptor, cnt.emisor, mnsj.tamEmisor, 0, (struct sockaddr *) &dest, addrlen) < 0) {
				fprintf(stderr, "Error en el sendto\n");
				close(descriptor);
				exit(-1);
			}


			if (sendto(descriptor, cnt.reloj, nProcesos, 0, (struct sockaddr *) &dest, addrlen) < 0) {
				fprintf(stderr, "Error en el sendto\n");
				close(descriptor);
				exit(-1);
			}

			fprintf(stdout, "%s: SEND(MSG,%s)\n", nombre, idLeido);

		} else if (!strcmp(line, "LOCK sec\n")) {
		} else if (!strcmp(line, "UNLOCK sec\n")) {
		} else if (!strcmp(line, "FINISH\n")) {
			break;
		}
	}

      	return 0;
}

