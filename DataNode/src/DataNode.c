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
	imprimirMensajeDos(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	Socket unSocket = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_DATANODE);
	estadoDataNode = 1;
	while(estadoDataNode);
	socketCerrar(unSocket);
	return EXIT_SUCCESS;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
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
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Data Node finalizado");
}

void dataNodeIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(RUTA_LOG, "DataNode");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	senialAsignarFuncion(SIGINT, funcionSenial);
}
