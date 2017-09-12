/*
 * worker.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */

#include "src/Biblioteca.h"

int main(){
	char line[40];
	Socket emisor = socketCrearCliente(NULL,"3000");
	printf("recibiendo\n");
	while(socketRecibir(emisor,line,39)){
		line[39]='\0';
		printf("%s",line);
	}
	printf("fin\n");
	return 0;
}
