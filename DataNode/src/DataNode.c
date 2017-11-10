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
	configuracionIniciar();
	dataNodeActivar();
	dataBinConfigurar();
	dataNodeConectarAFS();
	senialAsignarFuncion(SIGINT, configuracionSenial);
}

void dataNodeAtenderFileSystem(){
	Mensaje* mensaje = mensajeRecibir(socketFileSystem);
	switch(mensaje->header.operacion){
		case DESCONEXION: dataNodeDesconectarFS(); break;
		case ACEPTACION: dataNodeAceptado(); break;
		case LEER_BLOQUE: bloqueObtenerParaLeer(mensaje->datos); break;
		case ESCRIBIR_BLOQUE: bloqueEscribir(mensaje->datos); break;
		case COPIAR_BLOQUE: bloqueObtenerParaCopiar(mensaje->datos); break;
		case COPIAR_ARCHIVO: bloqueObtenerParaCopiarArchivo(mensaje->datos); break;
	}
	mensajeDestruir(mensaje);
}

void dataNodeFinalizar(){
	socketCerrar(socketFileSystem);
	memoriaLiberar(configuracion);
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Data Node finalizado");
	archivoLogDestruir(archivoLog);
}

void dataNodeDesconectarFS() {
	imprimirMensaje(archivoLog, ROJO"[CONEXION] Conexion finalizada con File System"BLANCO);
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

void dataNodeAceptado() {
	imprimirMensaje(archivoLog, "[CONEXION] Conexion establecida con el File System");
}

void dataNodeConectarAFS() {
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_DATANODE);
	Puntero puntero = malloc(100);
	String mensaje = string_from_format("/%s/%s/%s", configuracion->nombreNodo, configuracion->ipPropia, configuracion->puertoWorker);
	memcpy(puntero, &dataBinBloques, sizeof(Entero));
	memcpy(puntero+sizeof(Entero), mensaje, stringLongitud(mensaje)+1);
	mensajeEnviar(socketFileSystem, SOLICITAR_CONEXION, puntero, stringLongitud(mensaje)+1+sizeof(Entero));
	memoriaLiberar(mensaje);
	memoriaLiberar(puntero);
	imprimirMensaje2(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
}

void configuracionIniciarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "PUERTO_WORKER";
	campos[4] = "RUTA_DATABIN";
	campos[5] = "IP_PROPIA";
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(RUTA_LOG, "DataNode");
}

void configuracionIniciar() {
	configuracionIniciarLog();
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionImprimir(configuracion);
}

void configuracionSenial(int senial) {
	mensajeEnviar(socketFileSystem, DESCONEXION, VACIO, ACTIVADO);
	dataNodeDesactivar();
	puts("");
}

//--------------------------------------- Funciones de bloques -------------------------------------

BloqueWorker bloqueBuscar(Entero numeroBloque) {
	BloqueWorker bloque = punteroDataBin + (BLOQUE * numeroBloque);
	return bloque;
}

void bloqueObtenerParaLeer(Puntero datos) {
	Entero numeroBloque = *(Entero*)datos;
	BloqueWorker bloque = getBloque(numeroBloque);
	mensajeEnviar(socketFileSystem, LEER_BLOQUE, bloque, BLOQUE);
}

void bloqueObtenerParaCopiar(Puntero datos) {
	Entero numeroBloqueACopiar = *(Entero*)datos;
	BloqueWorker bloqueACopiar = getBloque(numeroBloqueACopiar);
	mensajeEnviar(socketFileSystem, COPIAR_BLOQUE, bloqueACopiar, BLOQUE);
}

void bloqueObtenerParaCopiarArchivo(Puntero datos) {
	Entero numeroBloque = *(Entero*)datos;
	BloqueWorker bloque = getBloque(numeroBloque);
	mensajeEnviar(socketFileSystem, COPIAR_ARCHIVO, bloque, BLOQUE);
}

void bloqueEscribir(Puntero datos) {
	Entero numeroBloque;
	memcpy(&numeroBloque, datos, sizeof(Entero));
	setBloque(numeroBloque, datos+sizeof(Entero));
}

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir() {
	dataBin = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(dataBin == NULL){
		imprimirMensaje(archivoLog,ROJO"[ERROR] No se pudo abrir el archivo data.bin"BLANCO);
		exit(EXIT_FAILURE);
	}
	fileCerrar(dataBin);
}

Puntero dataBinMapear() {
	Puntero Puntero;
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el open()"BLANCO);
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el fstat()"BLANCO);
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
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el mmap(), corran por sus vidas"BLANCO);
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(descriptorArchivo);
	return Puntero;
}

void configuracionCalcularBloques() {
	dataBinBloques = (Entero)ceil((double)dataBinTamanio/(double)BLOQUE);
	imprimirMensaje1(archivoLog, "[DATABIN] Cantidad de bloques %i", (int*)dataBinBloques);
}

void dataBinConfigurar() {
	dataBinAbrir();
	punteroDataBin = dataBinMapear();
	configuracionCalcularBloques();
}

//--------------------------------------- Interfaz con File System -------------------------------------

BloqueWorker getBloque(Entero numeroBloque) {
	BloqueWorker bloque = bloqueBuscar(numeroBloque);
	imprimirMensaje1(archivoLog, "[DATABIN] El bloque N°%i fue leido", (int*)numeroBloque);
	return bloque;
}

void setBloque(Entero numeroBloque, Puntero datos) {
	BloqueWorker bloque = bloqueBuscar(numeroBloque);
	memcpy(bloque, datos, BLOQUE);
	imprimirMensaje1(archivoLog, "[DATABIN] El bloque N°%i fue escrito", (int*)numeroBloque);
}
