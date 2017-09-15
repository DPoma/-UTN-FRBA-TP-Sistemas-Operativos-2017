/*
 * master.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */
#include "src/Biblioteca.h"
#include "commons/collections/queue.h"

#define blockSize 10 //el tamaño de los bloques es fijo

void limpiar(char* buffer,size_t size){
	int i;
	for(i=0;i<size;i++){
		buffer[i]='\0'; //hacer el byte 0
	}
}

int main(){
	t_queue* bloques=queue_create();
	FILE* arch=fopen("datos.txt","rb");
	char buffer[blockSize];
	limpiar(buffer, sizeof buffer);
	while(fread(buffer,sizeof buffer,1,arch)){ //todos los bloques que se completan
		queue_push(bloques,buffer);
		limpiar(buffer, sizeof buffer);
		//hay que limpiar el buffer porque sino el ultimo se solapa con el anteultimo
	}
	if(buffer[0]!='\0'){
		queue_push(bloques,buffer);//el bloque con lo que queda
	}

	Conexion* conexion;
	socketConfigurar(conexion,NULL,"3000"); //no se si lo necesito, esta para aceptar
	Socket listener = socketCrearListener(NULL,"3000"); //para que quiere el ip este?

	int clientesEsperados=3,clientesAtendidos=0;
	while(clientesAtendidos<clientesEsperados){
		Socket cliente=socketAceptar(conexion,listener);
		if(cliente==-1) continue;
		clientesAtendidos++;
		printf("clientesAtenidos=%d\n",clientesAtendidos);
		if(!fork()){
			printf("arranca hijo\n");
			socketCerrar(listener);
			while(!queue_is_empty(bloques)){
				socketEnviar(cliente,queue_pop(bloques),blockSize);
			}
			socketCerrar(cliente);
			printf("fin\n");
			exit(0);
		}
	}
	printf("cool and good\n");
	return 0;
}
