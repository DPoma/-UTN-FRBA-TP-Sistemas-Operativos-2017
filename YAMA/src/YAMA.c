/*
 ============================================================================
 Name        : YAMA.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "YAMA.h"

int main(void) {
	system("clear");
	imprimirMensajeProceso("# PROCESO YAMA");
	cargarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	printf("IP: %s | Puerto %s\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	Socket unSocket = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem);
	estado = 1;
	char message[1000];
	senialAsignarFuncion(SIGINT, funcionSenial);
	while(estado){
		fgets(message, 1024, stdin);
			if (estado)
				mensajeEnviar(unSocket, 4, message, strlen(message)+1);
	}
	close(unSocket);
	return 0;

}


Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	strcpy(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP File System: %s\n", configuracion->ipFileSystem);
	printf("Puerto File System: %s\n", configuracion->puertoFileSystem);
	printf("Retardo de planificacion: %i\n", configuracion->retardoPlanificacion);
	printf("Algoritmo de balanceo: %s\n", configuracion->algoritmoBalanceo);
	puts("----------------------------------------------------------------");
}

void cargarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "RETARDO_PLANIFICACION";
	campos[3] = "ALGORITMO_BALANCEO";
}


void funcionSenial(int senial) {
	estado = 0;
	puts("");
	imprimirMensajeProceso("# PROCESO YAMA FINALIZADO");
	puts("Aprete enter para salir");
	puts("----------------------------------------------------------------");
}
