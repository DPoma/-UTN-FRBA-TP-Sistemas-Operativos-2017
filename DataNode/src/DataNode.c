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

	if(dataBin==NULL){
		perror("No se pudo abrir databin");
		imprimirMensaje(archivoLog,"[DATABIN] Error al abrir DataBin");
		exit(-1);
	}


	while(dataNodeActivado()){
		atenderFileSystem(unSocket);
	}

	finalizarDataNode();
	fclose(dataBin);
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


int setBloque(int nroBloque, char* datos, int size){ //el prumer bloque es 0

	if(fseek(dataBin,nroBloque*MB,SEEK_SET)){
		perror("Error en el posicionamiento del puntero");
		imprimirMensaje(archivoLog, "[SETBLOQUE] No se pudo posicionar el puntero");
		return -1;
	}
	else{
		if(fwrite(datos,sizeof(char),size,dataBin)!= size){
			perror("Error en la escritura");
			imprimirMensajeUno(archivoLog,"[SETBLOQUE] No se pudo escribir en el bloque numero: %d", &nroBloque);
			rewind(dataBin); //posiciona el puntero al principio del archivo
			return FRACASOSETBLOQUE;
		}
		else{
			imprimirMensajeUno(archivoLog, "[SETBLOQUE] Se escribio en el bloque numero: %d", &nroBloque);
			rewind(dataBin); //posiciona el puntero al principio del archivo
			return EXITOSETBLOQUE;
		}
	}



}

char* getBloque(int nroBloque){
	char* data = malloc(sizeof(char)*MB);

	if(fseek(dataBin,nroBloque*MB,SEEK_SET)){
		perror("Error en el posicionamiento del puntero");
		imprimirMensaje(archivoLog, "[GETBLOQUE] No se pudo posicionar el puntero");
	}

	fread(data,sizeof(char),MB,dataBin);
	imprimirMensajeUno(archivoLog,"[GETBLOQUE] se leyo el bloque numero: %d", &nroBloque);

	rewind(dataBin);

	return data;
	free(data);

}

Operacion* deserializar(Mensaje* mensaje){
	int numeroBloque;
	int tamaniodatos;
	char* data;
	int size = mensaje->header.tamanio;

	Operacion* op = malloc(size);

	memcpy(&numeroBloque, mensaje->datos, sizeof(int));
	memcpy(&tamaniodatos, mensaje->datos + sizeof(int), sizeof(int));
	memcpy(&data, mensaje->datos + sizeof(int)+ tamaniodatos, tamaniodatos);

	op->nroBloque = numeroBloque;
	op->size_datos = tamaniodatos;
	op->datos = data;

	return op;


}



void atenderFileSystem(Socket unSocket){
	Mensaje* mensaje = mensajeRecibir(unSocket);
	int status;
	char* data;
	int len;
	Operacion* op = deserializar(mensaje);
	switch(mensaje->header.operacion){
		case -1:
		imprimirMensaje(archivoLog, "Error en el File System");
		finalizarDataNode();
		estadoDataNode = dataNodeDesactivado();
		status = -1;
		mensajeEnviar(unSocket, status, NULL, sizeof(int));
		break;

		case SETBLOQUE:
		status = setBloque(op->nroBloque,op->datos, op->size_datos);
		imprimirMensajeUno(archivoLog, "[SETBLOQUE]Grabando en el bloque: %d", &op->nroBloque);

		mensajeEnviar(unSocket, status, NULL, sizeof(int));

		estadoDataNode = dataNodeActivado();
		break;

		case GETBLOQUE:

		data = malloc(MB + sizeof(int));
		data = getBloque(op->nroBloque);
		imprimirMensajeUno(archivoLog, "[GETBLOQUE]Se Obtuvo el bloque numero: %d", &op->nroBloque);

		if(data==NULL){
			imprimirMensajeUno(archivoLog, "El bloque numero: %d esta vacio o se produjo un error", &op->nroBloque);
			status = FRACASOGETBLOQUE;
			mensajeEnviar(unSocket, status, NULL, sizeof(int));
		}

		else{

			status = EXITOGETBLOQUE;
			len = strlen(data) + 1;
			mensajeEnviar(unSocket, status, data, len);
		}

		estadoDataNode = dataNodeActivado();
		break;
	}
	 free(op);
	 free(data);
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
	//senialAsignarFuncion(SIGINT, funcionSenial);
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

