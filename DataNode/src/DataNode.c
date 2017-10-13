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
	mensajeEnviar(unSocket, 14, configuracion->nombreNodo, stringLongitud(configuracion->nombreNodo)+1);

	dataBin=fopen(configuracion->rutaDataBin, "r+");


	while(dataNodeActivado()){
		//atenderFileSystem(unSocket);
	}

	finalizarDataNode();

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




void setBloque(){

}

void getBloque(){

}


void atenderFileSystem(Socket unSocket){
	Mensaje* peticion = mensajeRecibir(unSocket);

	switch(peticion->header.operacion){
		case -1:
		imprimirMensaje(archivoLog, "Error en el File System");
		finalizarDataNode();
		estadoDataNode = dataNodeDesactivado();
		break;

		case SETBLOQUE:
		imprimirMensaje(archivoLog, "Grabando en el bloque n"); //desp lo cambio
		setBloque();
		estadoDataNode = dataNodeActivado();
		break;

		case GETBLOQUE:
		imprimirMensaje(archivoLog, "Se Obtuvo el bloque n");
		getBloque();
		estadoDataNode = dataNodeActivado();
		break;
	}
	 free(peticion);
}

//mandar mensaje confirmacion


void deserializar(Mensaje* mensaje){

}



void finalizarDataNode(){
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Data Node finalizado");
	archivoLogDestruir(archivoLog);
	memoriaLiberar(configuracion);
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
	dataNodeDesactivar();
	puts("");
}

void dataNodeIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(RUTA_LOG, "DataNode");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	senialAsignarFuncion(SIGINT, funcionSenial);
	dataNodeActivar();
}


bool dataNodeActivado() {
	return estadoDataNode == ACTIVADO;
}

bool dataNodeDesactivado() {
	return estadoDataNode == DESACTIVADO;
}

void dataNodeActivar() {
	estadoDataNode = ACTIVADO;
}

void dataNodeDesactivar() {
	estadoDataNode = DESACTIVADO;



}

