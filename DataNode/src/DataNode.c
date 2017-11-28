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
	senialAsignarFuncion(SIGINT, configuracionSenialHijo);
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
	imprimirAviso(archivoLog, "[AVISO] Conexion finalizada con el File System");
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
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystemDataNode, ID_DATANODE);
	Puntero puntero = malloc(100);
	String mensaje = string_from_format("/%s/%s/%s", configuracion->nombreNodo, configuracion->ipPropia, configuracion->puertoMaster);
	memcpy(puntero, &dataBinBloques, sizeof(Entero));
	memcpy(puntero+sizeof(Entero), mensaje, stringLongitud(mensaje)+1);
	mensajeEnviar(socketFileSystem, SOLICITAR_CONEXION, puntero, stringLongitud(mensaje)+1+sizeof(Entero));
	memoriaLiberar(mensaje);
	memoriaLiberar(puntero);
	imprimirMensaje2(archivoLog, "[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystemDataNode);
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystemDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM_DATANODE"));
	stringCopiar(configuracion->puertoFileSystemWorker, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM_WORKER"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	stringCopiar(configuracion->tamanioDataBin, archivoConfigStringDe(archivoConfig, "TAMANIO_DATABIN"));
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	stringCopiar(configuracion->rutaLogDataNode, archivoConfigStringDe(archivoConfig, "RUTA_LOG_DATANODE"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
}

void configuracionIniciarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM_DATANODE";
	campos[2] = "PUERTO_FILESYSTEM_WORKER";
	campos[3] = "PUERTO_MASTER";
	campos[4] = "NOMBRE_NODO";
	campos[5] = "RUTA_DATABIN";
	campos[6] = "IP_PROPIA";
	campos[7] = "TAMANIO_DATABIN";
	campos[8] = "RUTA_LOG_DATANODE";
	campos[9] = "RUTA_LOG_WORKER";
	campos[10] = "RUTA_TEMPORALES";
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO DATA NODE");
	archivoLog = archivoLogCrear(configuracion->rutaLogDataNode, "DataNode");
}

void configuracionIniciar() {
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionIniciarLog();
	configuracionImprimir(configuracion);
}

void configuracionSenialHijo(int senial) {
	mensajeEnviar(socketFileSystem, DESCONEXION, NULL, NULO);
	dataNodeDesactivar();
	puts("");
}

//--------------------------------------- Funciones de bloques -------------------------------------

Bloque bloqueBuscar(Entero numeroBloque) {
	Bloque bloque = punteroDataBin + (BLOQUE * numeroBloque);
	return bloque;
}

void bloqueObtenerParaLeer(Puntero datos) {
	Entero numeroBloque = *(Entero*)datos;
	Bloque bloque = getBloque(numeroBloque);
	mensajeEnviar(socketFileSystem, LEER_BLOQUE, bloque, BLOQUE);
}

void bloqueObtenerParaCopiar(Puntero datos) {
	Entero numeroBloqueACopiar = *(Entero*)datos;
	Bloque bloqueACopiar = getBloque(numeroBloqueACopiar);
	mensajeEnviar(socketFileSystem, COPIAR_BLOQUE, bloqueACopiar, BLOQUE);
}

void bloqueObtenerParaCopiarArchivo(Puntero datos) {
	Entero numeroBloque = *(Entero*)datos;
	Bloque bloque = getBloque(numeroBloque);
	mensajeEnviar(socketFileSystem, COPIAR_ARCHIVO, bloque, BLOQUE);
}

void bloqueEscribir(Puntero datos) {
	Entero numeroBloque;
	memcpy(&numeroBloque, datos, sizeof(Entero));
	if(bloqueValido(numeroBloque))
		setBloque(numeroBloque, datos+sizeof(Entero));
	else
		imprimirError(archivoLog, "[ERROR] El bloque no existe");
}

bool bloqueValido(Entero numeroBloque) {
	return numeroBloque >= 0 && numeroBloque < dataBinBloques;
}

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinCrear() {
	File archivo = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(archivo == NULL) {
		String comando = string_from_format("truncate -s %s %s", configuracion->tamanioDataBin, configuracion->rutaDataBin);
		system(comando);
		memoriaLiberar(comando);
	}
	else
		fileCerrar(archivo);
}

Puntero dataBinMapear() {
	Puntero Puntero;
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirError(archivoLog, "[ERROR] Fallo el open()");
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirError(archivoLog, "[ERROR] Fallo el fstat()");
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
		imprimirError(archivoLog, "[ERROR] Fallo el mmap(), corran por sus vidas");
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(descriptorArchivo);
	return Puntero;
}

void configuracionCalcularBloques() {
	dataBinBloques = (Entero)ceil((double)dataBinTamanio/(double)BLOQUE);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Cantidad de bloques %i", (int*)dataBinBloques);
}

void dataBinConfigurar() {
	dataBinCrear();
	punteroDataBin = dataBinMapear();
	configuracionCalcularBloques();
}

//--------------------------------------- Interfaz con File System -------------------------------------

Bloque getBloque(Entero numeroBloque) {
	Bloque bloque = bloqueBuscar(numeroBloque);
	imprimirMensaje2(archivoLog, "[DATABIN] El bloque N°%i del %s fue leido", (int*)numeroBloque, configuracion->nombreNodo);
	return bloque;
}

void setBloque(Entero numeroBloque, Puntero datos) {
	Bloque bloque = bloqueBuscar(numeroBloque);
	memcpy(bloque, datos, BLOQUE);
	imprimirMensaje2(archivoLog, "[DATABIN] El bloque N°%i del %s fue escrito", (int*)numeroBloque, configuracion->nombreNodo);
}
