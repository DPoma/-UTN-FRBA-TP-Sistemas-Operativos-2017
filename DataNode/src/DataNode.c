/*
 ============================================================================
 Name        : DataNode.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso DataNode
 ============================================================================
 */

#include "DataNode.h"

int main(void) {
	system("clear");
	imprimirMensajeProceso("# PROCESO DATA NODE");
	cargarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	printf("IP: %s | Puerto %s\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	Socket unSocket = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem);
	estado = 1;
	//senialAsignarFuncion(SIGINT, funcionSenial);
	while(estado){
		Mensaje* mensaje = mensajeRecibir(unSocket);
		if(mensajeOperacionErronea(mensaje))
			socketCerrar(unSocket);
		else
			printf("Nuevo mensaje de Master %i: %s", unSocket, (char*)(mensaje->dato));
		mensajeDestruir(mensaje);
	}
	socketCerrar(unSocket);
	return 0;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	strcpy(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	strcpy(configuracion->ipPropio, archivoConfigStringDe(archivoConfig, "IP_PROPIO"));
	strcpy(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	strcpy(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP Propio: %s\n", configuracion->ipPropio);
	printf("IP File System: %s\n", configuracion->ipFileSystem);
	printf("Puerto File System: %s\n", configuracion->puertoFileSystem);
	printf("Nombre Nodo: %s\n", configuracion->nombreNodo);
	printf("Puerto Worker: %s\n", configuracion->puertoWorker);
	printf("Ruta archivo data.bin: %s\n", configuracion->rutaDataBin);
	puts("----------------------------------------------------------------");
}

void cargarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "PUERTO_WORKER";
	campos[4] = "RUTA_DATABIN";
	campos[5] = "IP_PROPIO";
}

void funcionSenial(int senial) {
	estado = 0;
	puts("");
	imprimirMensajeProceso("PROCESO DATA NODE FINALIZADO");
	puts("Aprete enter para finalizar");
	puts("----------------------------------------------------------------");
}
