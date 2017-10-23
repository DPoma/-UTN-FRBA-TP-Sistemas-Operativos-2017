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
	while(dataNodeActivado())
		dataNodeAtenderFileSystem();
	dataNodeFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de DataNode -------------------------------------

void dataNodeIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(RUTA_LOG, "DataNode");
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionImprimir(configuracion);
	dataBinAbrir();
	//senialAsignarFuncion(SIGINT, funcionSenial);
	dataNodeActivar();
	imprimirMensajeDos(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	Socket socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_DATANODE);
	mensajeEnviar(socketFileSystem, 14, configuracion->nombreNodo, stringLongitud(configuracion->nombreNodo)+1);
}

void dataNodeAtenderFileSystem(){
	Mensaje* mensaje = mensajeRecibir(socketFileSystem);
	Bloque* bloque = dataBinCrearBloque(mensaje->datos); //Ponerlo en set y get ya que sino cuando reciba error copia cualquier cosa
	switch(mensaje->header.operacion){
		case ERROR: dataNodeDesconectarFS(); break;
		case SET_BLOQUE: dataBinSetBloque(bloque->numeroBloque, bloque->datos); break;
		case GET_BLOQUE: dataBinGetBloque(bloque->numeroBloque); break;
	}
	mensajeDestruir(mensaje);
}

void dataNodeFinalizar(){
	socketCerrar(socketFileSystem);
	memoriaLiberar(configuracion);
	fileCerrar(dataBin);
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Data Node finalizado");
	archivoLogDestruir(archivoLog);
}

void dataNodeDesconectarFS() {
	imprimirMensaje(archivoLog, "[CONEXION] El File System se desconecto");
	estadoDataNode = dataNodeDesactivado();
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

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
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

void configuracionIniciarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "PUERTO_WORKER";
	campos[4] = "RUTA_DATABIN";
	campos[5] = "IP_PROPIO";
}


//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir() {
	dataBin = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(dataBin == NULL){
		imprimirMensaje(archivoLog,"[ERROR] No se pudo abrir el archivo data.bin");
		exit(EXIT_FAILURE);
	}
}

String dataBinUbicarPuntero(Entero numeroBloque) {
	String puntero = punteroDatabin + (BLOQUE * numeroBloque);
	return puntero;
}

void dataBinSetBloque(Entero numeroBloque, String datos) {
	String puntero = dataBinUbicarPuntero(numeroBloque);
	memcpy(puntero, datos, BLOQUE);
	imprimirMensajeUno(archivoLog, "[DATABIN] El bloque N°%i fue escrito", (int*)numeroBloque);
	return;
}

String dataBinGetBloque(Entero numeroBloque) {
	String puntero = dataBinUbicarPuntero(numeroBloque);
	imprimirMensajeUno(archivoLog, "[DATABIN] El bloque N°%i fue leido", (int*)numeroBloque);
	return puntero;
}

Bloque* dataBinCrearBloque(Puntero puntero) {
	Bloque* bloque = memoriaAlocar(sizeof(Bloque));
	memcpy(&bloque->numeroBloque, puntero, sizeof(Entero));
	memcpy(&bloque->tamanioDatos, puntero+sizeof(Entero), sizeof(Entero));
	memcpy(bloque->datos, puntero+(2*sizeof(Entero)), bloque->tamanioDatos);
	return bloque;
}

//--------------------------------------- Funciones varias -------------------------------------

void funcionSenial(int senial) {
	dataNodeDesactivar();
	puts("");
}
