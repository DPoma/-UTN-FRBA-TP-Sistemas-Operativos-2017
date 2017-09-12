/*
 * master.c
 *
 *  Created on: 11/9/2017
 *      Author: utnso
 */
#include "src/Biblioteca.h"

int main(){
//	int argc,char** argv
//	if(argc!=2){printf("uso: master archivo\n");return 1;}

	FILE* arch=fopen("texto.txt","r");

	Conexion* conexion;
	socketConfigurar(conexion,NULL,"3000"); //no se si lo necesito, esta para aceptar
	Socket listener = socketCrearListener(NULL,"3000"); //para que quiere el ip este?

	int clientes=3; //clientes esperados
	int i=0;
	while(i<clientes){
		Socket cliente=socketAceptar(conexion,listener);
		if(cliente==-1) continue;
		i++;
		printf("i=%d\n",i);

		if(!fork()){
			printf("arranca hijo\n");
			socketCerrar(listener);
			size_t lineSize=30;
			char* line=(char*) malloc(lineSize*sizeof(char));
			while(getline(&line,&lineSize,arch)!=EOF){
				printf("enviando: %s",line);
				socketEnviar(cliente,line,strlen(line));
			}
			free(line);
			socketCerrar(cliente);
			printf("fin\n");
			exit(0);
		}
	}
	printf("cool and good\n");
	return 0;
}
