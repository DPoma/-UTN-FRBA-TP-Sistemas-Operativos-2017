/*
 * worker.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include "src/Biblioteca.h"

#define blockSize 10

int main(){
	char buffer[blockSize+1];
	Socket emisor = socketCrearCliente(NULL,"3000");
	printf("recibiendo\n");
	while(socketRecibir(emisor,buffer,blockSize)){
		buffer[blockSize]='\0';
		printf("%s--\n",buffer);
	}
	socketCerrar(emisor);
	printf("\nfin\n");
	return 0;
}
