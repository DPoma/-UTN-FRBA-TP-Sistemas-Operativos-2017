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
	punteroDataBin = dataBinMapear();
	//senialAsignarFuncion(SIGINT, funcionSenial);
	dataNodeActivar();
	dataBinCalcularBloques();
	imprimirMensajeDos(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_DATANODE);
	Puntero datos = memoriaAlocar(50);
	char ip[20] = "127.0.0.0";
	memcpy(datos, configuracion->nombreNodo ,10);
	memcpy(datos+10, ip , 20);
	memcpy(datos+30, configuracion->puertoFileSystem ,20);
	mensajeEnviar(socketFileSystem, 14, datos, 50);
	memoriaLiberar(datos);
}

void dataNodeAtenderFileSystem(){
	Mensaje* mensaje = mensajeRecibir(socketFileSystem);
	switch(mensaje->header.operacion){
		case DESCONEXION: dataNodeDesconectarFS(); break;
		case GET_BLOQUE: dataBinGetBloque(mensaje->datos); break;
		case SET_BLOQUE: dataBinSetBloque(mensaje->datos); break;
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
	dataNodeDesactivar();
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
}


//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir() {
	dataBin = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(dataBin == NULL){
		imprimirMensaje(archivoLog,"[ERROR] No se pudo abrir el archivo data.bin");
		exit(EXIT_FAILURE);
	}
}

Puntero dataBinUbicarPuntero(Entero numeroBloque) {
	Puntero puntero = punteroDataBin + (BLOQUE * numeroBloque);
	return puntero;
}

void dataBinGetBloque(Puntero datos) {
	Entero numeroBloque = *(Entero*)datos;
	if(numeroBloque < dataBinBloques) {
		Puntero puntero = getBloque(numeroBloque);
		mensajeEnviar(socketFileSystem, 14, puntero, BLOQUE);
	}
	else {
		imprimirMensaje(archivoLog,"[ERROR] El bloque no existe");
		//mensajeEnviar(socketFileSystem, -2, NULL, NULO);
	}
}

void dataBinSetBloque(Puntero datos) {
	Entero numeroBloque;
	memcpy(&numeroBloque, datos, sizeof(Entero));
	if(numeroBloque < dataBinBloques)
		setBloque(numeroBloque, datos+sizeof(Entero));
	else {
		imprimirMensaje(archivoLog,"[ERROR] El bloque no existe");
		//mensajeEnviar(socketFileSystem, -2, NULL, NULO);
	}

}


Puntero dataBinMapear() {
	Puntero Puntero;
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirMensaje(archivoLog, "[ERROR] Fallo el open()");
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirMensaje(archivoLog, "[ERROR] Fallo el fstat()");
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	dataBinTamanio = estadoArchivo.st_size;

	/* Segun lo que entendi (mi no saber mucho ingles)
	 * El mmap() sirve para mapear un archivo en memoria, lo cual es mucho mas rapido y comodo, tambien me evito utilizar los falgo()
	 * mmap(Direccion, Tamanio, Proteccion, Mapeo, Descriptor, Offset)
	 * Direccion: Elegis la direccion que queres, lo mejor es poner 0 asi lo elige el SO
	 * Tamanio: El tamanio del archivo
	 * Proteccion: Serian como los permisos leer, escribir, ejecutar
	 * Mapeo: Por lo que entendi si es compartido los cambios se reflejan el archivo, si es privado no
	 * Descriptor: El fd del archivo
	 * Offset: Desde donde quiero que empiece a apuntar el puntero, en este caso desde el principio (0)
	 */

	Puntero = mmap(0, dataBinTamanio, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, descriptorArchivo, 0);
	if (Puntero == MAP_FAILED) {
		imprimirMensaje(archivoLog, "[ERROR] Fallo el mmap(), corran por sus vidas");
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(descriptorArchivo);
	return Puntero;
}

//--------------------------------------- Funciones varias -------------------------------------

Puntero getBloque(Entero numeroBloque) {
	Puntero puntero = dataBinUbicarPuntero(numeroBloque);
	imprimirMensajeUno(archivoLog, "[DATABIN] El bloque N°%i fue leido", (int*)numeroBloque);
	return puntero;
}

void setBloque(Entero numeroBloque, Puntero datos) {
	Puntero puntero = dataBinUbicarPuntero(numeroBloque);
	memcpy(puntero, datos, BLOQUE); //TODO ver si es bloque o el largo del dato
	imprimirMensajeUno(archivoLog, "[DATABIN] El bloque N°%i fue escrito", (int*)numeroBloque);
}

void dataBinCalcularBloques() {
	dataBinBloques = (int)ceil((double)dataBinTamanio/(double)BLOQUE);
	imprimirMensajeUno(archivoLog, "[DATABIN] Cantidad de bloques disponibles %i", (int*)dataBinBloques);
}

void funcionSenial(int senial) {
	socketCerrar(socketFileSystem);
	dataNodeDesactivar();
	//dataNodeFinalizar();
	puts("");
	//exit(1);
}
