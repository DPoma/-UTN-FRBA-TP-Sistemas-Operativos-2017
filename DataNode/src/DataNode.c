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
	dataNodeIniciar();
	printf("[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	log_info(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	Socket unSocket = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_DATANODE);
	estadoDataNode = 1;
	while(estadoDataNode);
	socketCerrar(unSocket);
	return EXIT_SUCCESS;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	strcpy(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	strcpy(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	strcpy(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	printf("[CONFIGURACION] Nombre Nodo: %s\n", configuracion->nombreNodo);
	log_info(archivoLog, "[CONFIGURACION] Nombre Nodo: %s\n", configuracion->nombreNodo);
	printf("[CONFIGURACION] Ruta archivo data.bin: %s\n", configuracion->rutaDataBin);
	log_info(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s\n", configuracion->rutaDataBin);
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "PUERTO_WORKER";
	campos[4] = "RUTA_DATABIN";
	campos[5] = "IP_PROPIO";
}

void funcionSenial(int senial) {
	estadoDataNode = 0;
	puts("");
	puts("[EJECUCION] Proceso Data Node finalizado");
	log_info(archivoLog, "[EJECUCION] Proceso Data Node finalizado");
}

void dataNodeIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(RUTA_LOG, "DataNode");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	senialAsignarFuncion(SIGINT, funcionSenial);
}
