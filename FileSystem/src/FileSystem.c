/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso File System
 ============================================================================
 */

#include "FileSystem.h"

int main(void) {
	fileSystemIniciar();
	fileSystemCrearConsola();
	fileSystemAtenderProcesos();
	fileSystemFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	archivoLog = archivoLogCrear(RUTA_LOG, "FileSystem");
	puts("[EJECUCION] Proceso File System inicializado");
	log_info(archivoLog, "[EJECUCION] Proceso File System inicializado");
	estadoFileSystem = 1;
	archivoConfigObtenerCampos();
	senialAsignarFuncion(SIGINT, funcionSenial);
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
}

void fileSystemCrearConsola() {
	Hilo hilo;
	hiloCrear(&hilo, (void*)consolaAtenderComandos, NULL);
}

void fileSystemAtenderProcesos() {
	Servidor* servidor = malloc(sizeof(Servidor));
	servidorInicializar(servidor);
	while(estadoFileSystem)
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void fileSystemFinalizar() {
	puts("[EJECUCION] Proceso File System finalizado");
	log_info(archivoLog, "[EJECUCION] Proceso File System finalizado");
}

//--------------------------------------- Funciones de Consola -------------------------------------


int consolaIdentificarComando(char* comando) {
	if(stringSonIguales(comando, C_FORMAT))
		return FORMAT;
	if(stringSonIguales(comando, C_INFO))
		return INFO;
	else if(stringSonIguales(comando, C_RM))
		return RM;
	else if(stringSonIguales(comando, C_RMB))
		return RMB;
	else if(stringSonIguales(comando, C_RMD))
		return RMD;
	else if(stringSonIguales(comando, C_RENAME))
		return RENAME;
	else if(stringSonIguales(comando, C_MV))
		return MV;
	else if(stringSonIguales(comando, C_CAT))
		return CAT;
	else if(stringSonIguales(comando, C_MKDIR))
		return MKDIR;
	else if(stringSonIguales(comando, C_PFROM))
		return PFROM;
	else if(stringSonIguales(comando, C_CPTO))
		return CPTO;
	else if(stringSonIguales(comando, C_CPTBLOCK))
		return CPTBLOCK;
	else if(stringSonIguales(comando, C_MD5))
		return MD5;
	else if(stringSonIguales(comando, C_LS))
		return LS;
	else if(stringSonIguales(comando, C_INFO))
		return INFO;
	else if(stringSonIguales(comando, C_RENAME))
		return RENAME;
	else
		return -1;
}

char* consolaLeerCaracteresEntrantes() {
	int i, caracterLeido;
	char* cadena = malloc(1000);
	for(i = 0; (caracterLeido= getchar()) != '\n'; i++)
		cadena[i] = caracterLeido;
	cadena[i] = '\0';
	return cadena;
}


Comando consolaObtenerComando() {
	Comando comando;
	char* mensaje = consolaLeerCaracteresEntrantes();
	comando.identificador = consolaIdentificarComando(mensaje);
	free(mensaje);
	return comando;
}


void consolaAtenderComandos() {
	Comando comando;
	while(estadoFileSystem) {
		comando = consolaObtenerComando();
		switch(comando.identificador) {
			case FORMAT: puts("COMANDO FORMAT"); break;
			case RM: puts("COMANDO RM"); break;
			case RMB: puts("COMANDO RM -B"); break;
			case RMD: puts("COMANDO RM -D"); break;
			case RENAME: puts("COMANDO RENAME"); break;
			case MV: puts("COMANDO  MV"); break;
			case CAT: puts("COMANDO CAT"); break;
			case MKDIR: puts("COMANDO MKDIR"); break;
			case PFROM: puts("COMANDO PFROM"); break;
			case CPTO: puts("COMANDO CPTO"); break;
			case CPTBLOCK:puts("COMANDO CPTBLOCK"); break;
			case MD5: puts("COMANDO MD5"); break;
			case LS: puts("COMANDO LS"); break;
			case INFO: puts("COMANDO INFO"); break;
			default: puts("COMANDO INVALIDO"); break;
		}
	}
}

//--------------------------------------- Funciones de Servidor -------------------------------------

bool servidorObtenerMaximoSocket(Servidor* servidor) {
	return servidor->maximoSocket;
}

void servidorSetearListaSelect(Servidor* servidor) {
	servidor->listaSelect = servidor->listaMaster;
}

void servidorControlarMaximoSocket(Servidor* servidor, Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->maximoSocket))
		servidor->maximoSocket = unSocket;
}

void servidorEsperarSolicitud(Servidor* servidor) {
	socketSelect(servidor->maximoSocket, &servidor->listaSelect);
}

bool socketEsDataNode(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaDataNodes);
}

bool socketEsYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->procesoYAMA, unSocket);
}

void servidorFinalizarConexion(Servidor* servidor, Socket unSocket) {
	listaSocketsEliminar(unSocket, &servidor->listaMaster);
	if(socketEsDataNode(servidor, unSocket)) {
		listaSocketsEliminar(unSocket, &servidor->listaDataNodes);
		puts("[CONEXION] Un proceso Data Node se ha desconectado");
		log_info(archivoLog, "[CONEXION] Un proceso Data Node se ha desconectado");
	}
	else {
		puts("[CONEXION] El proceso YAMA se ha desconectado");
		log_info(archivoLog, "[CONEXION] El proceso YAMA se ha desconectado");
	}

	socketCerrar(unSocket);
}

void servidorEstablecerConexion(Servidor* servidor, Socket unSocket) {
	listaSocketsAgregar(unSocket, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, unSocket);
}

Socket servidorAceptarDataNode(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_DATANODE);
	if(nuevoSocket != ERROR) {
		listaSocketsAgregar(nuevoSocket, &servidor->listaDataNodes);
		puts("[CONEXION] Proceso Data Node conectado exitosamente.");
		log_info(archivoLog, "[CONEXION] Proceso Data Node conectado exitosamente");
	}
	return nuevoSocket;
}

Socket servidorAceptarYAMA(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_YAMA);
	if(nuevoSocket != ERROR) {
		servidor->procesoYAMA = nuevoSocket;
		puts("[CONEXION] Proceso YAMA conectado exitosamente.");
		log_info(archivoLog, "[CONEXION] Proceso YAMA conectado exitosamente");
	}
	return nuevoSocket;
}

bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerDataNode, unSocket);
}


void servidorAceptarConexion(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	if(socketEsListenerDataNode(servidor, unSocket))
		nuevoSocket = servidorAceptarDataNode(servidor, unSocket);
	else
		nuevoSocket = servidorAceptarYAMA(servidor, unSocket);
	if(nuevoSocket != ERROR)
		servidorEstablecerConexion(servidor, nuevoSocket);
}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeOperacionErronea(mensaje))
		servidorFinalizarConexion(servidor, unSocket);
	else
		puts("[MENSAJE]");
	mensajeDestruir(mensaje);
}


bool socketEsListenerYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerYAMA, unSocket);
}

bool socketEsListener(Servidor* servidor, Socket unSocket) {
	return socketEsListenerDataNode(servidor, unSocket) || socketEsListenerYAMA(servidor, unSocket);
}

bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaSelect);
}

void servidorControlarSocket(Servidor* servidor, Socket unSocket) {
	if (socketRealizoSolicitud(servidor, unSocket)) {
		if(socketEsListener(servidor, unSocket))
			servidorAceptarConexion(servidor, unSocket);
		else
			servidorRecibirMensaje(servidor, unSocket);
	}
}

void servidorActivarListenerYAMA(Servidor* servidor) {
	servidor->listenerYAMA = socketCrearListener(configuracion->puertoYAMA);
	listaSocketsAgregar(servidor->listenerYAMA, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerYAMA);
}

void servidorActivarListenerDataNode(Servidor* servidor) {
	servidor->listenerDataNode = socketCrearListener(configuracion->puertoDataNode);
	listaSocketsAgregar(servidor->listenerDataNode, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerDataNode);
}

void servidorActivarListeners(Servidor* servidor) {
	servidorActivarListenerDataNode(servidor);
	servidorActivarListenerYAMA(servidor);
}

void servidorInicializar(Servidor* servidor) {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	listaSocketsLimpiar(&servidor->listaDataNodes);
	servidorActivarListeners(servidor);
}

void servidorFinalizar(Servidor* servidor) {
	archivoLogDestruir(archivoLog);
	free(configuracion);
}

void servidorAtenderSolicitud(Servidor* servidor) {
	Socket unSocket;
	int maximoSocket = servidor->maximoSocket;
	for(unSocket = 0; unSocket <= maximoSocket; unSocket++)
		servidorControlarSocket(servidor, unSocket);
}

void servidorAtenderPedidos(Servidor* servidor) {
	servidorSetearListaSelect(servidor);
	servidorEsperarSolicitud(servidor);
	servidorAtenderSolicitud(servidor);
}



//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	strcpy(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	strcpy(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	printf("[CONEXION] Esperando conexion de YAMA (Puerto: %s)\n", configuracion->puertoYAMA);
	log_info(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)\n", configuracion->puertoYAMA);
	printf("[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)\n", configuracion->puertoDataNode);
	log_info(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)\n", configuracion->puertoDataNode);
	printf("[CONFIGURACION] Ruta Metadata: %s.\n", configuracion->rutaMetadata);
	log_info(archivoLog, "[CONFIGURACION] Ruta Metadata: %s.\n", configuracion->rutaMetadata);
}

void archivoConfigObtenerCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "RUTA_METADATA";
}

void funcionSenial(int senial) {
	estadoFileSystem = 0;
	puts("");
	puts("[EJECUCION] Proceso File System finalizado.");
	log_info(archivoLog, "[EJECUCION] Proceso File System finalizado.");
}

