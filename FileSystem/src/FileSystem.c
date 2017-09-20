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
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System inicializado");
	estadoFileSystem = 1;
	archivoConfigObtenerCampos();
	senialAsignarFuncion(SIGINT, funcionSenial);
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
}

void fileSystemCrearConsola() {
	Hilo hilo;
	hiloCrear(&hilo, (Puntero)consolaAtenderComandos, NULL);
}

void fileSystemAtenderProcesos() {
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorInicializar(servidor);
	while(estadoFileSystem)
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void fileSystemFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
}

//--------------------------------------- Funciones de Consola -------------------------------------


int consolaIdentificarComando(String comando) {
	if(stringNulo(comando))
		return ERROR;
	if(stringIguales(comando, C_FORMAT))
		return FORMAT;
	if(stringIguales(comando, C_INFO))
		return INFO;
	else if(stringIguales(comando, C_RM))
		return RM;
	else if(stringIguales(comando, C_RMB))
		return RMB;
	else if(stringIguales(comando, C_RMD))
		return RMD;
	else if(stringIguales(comando, C_RENAME))
		return RENAME;
	else if(stringIguales(comando, C_MV))
		return MV;
	else if(stringIguales(comando, C_CAT))
		return CAT;
	else if(stringIguales(comando, C_MKDIR))
		return MKDIR;
	else if(stringIguales(comando, C_CPFROM))
		return CPFROM;
	else if(stringIguales(comando, C_CPTO))
		return CPTO;
	else if(stringIguales(comando, C_CPBLOCK))
		return CPBLOCK;
	else if(stringIguales(comando, C_MD5))
		return MD5;
	else if(stringIguales(comando, C_LS))
		return LS;
	else if(stringIguales(comando, C_INFO))
		return INFO;
	else if(stringIguales(comando, C_RENAME))
		return RENAME;
	else
		return ERROR;
}

String consolaLeerEntrada() {
	int indice;
	int caracterLeido;
	String cadena = memoriaAlocar(MAX);
	for(indice = 0; (caracterLeido= caracterObtener()) != ENTER; indice++)
		cadena[indice] = caracterLeido;
	cadena[indice] = FIN;
	return cadena;
}

bool consolaComandoTipoUno(String comando) {
	return stringIguales(comando, C_RM) || stringIguales(comando, C_CAT) ||
			stringIguales(comando, C_MKDIR) || stringIguales(comando, C_MD5) ||
			stringIguales(comando, C_LS) || stringIguales(comando, C_INFO) ||
			stringIguales(comando, C_RMD);
}

bool consolaComandoTipoDos(String comando) {
	return stringIguales(comando, C_RENAME) ||
			stringIguales(comando, C_MV) || stringIguales(comando, C_CPFROM) ||
			stringIguales(comando, C_CPTO);
}

bool consolaComandoTipoTres(String comando) {
	return stringIguales(comando, C_CPBLOCK) || stringIguales(comando, C_RMB);
}


int consolaComandoCantidadArgumentos(String comando) {
	if(consolaComandoTipoUno(comando))
		return 1;
	else if(consolaComandoTipoDos(comando))
		return 2;
	else if(consolaComandoTipoTres(comando))
		return 3;
	else
		return 0;
}

void consolaProcesarComandoSinTipo(Comando* comando, String* subcadenas) {
	comando->argumento1 = NULL;
	comando->argumento2 = NULL;
	comando->argumento3 = NULL;
}

void consolaProcesarComandoTipoUno(Comando* comando, String* subcadenas) {
	comando->argumento1 = subcadenas[1];
	comando->argumento2 = NULL;
	comando->argumento3 = NULL;
}

void consolaProcesarComandoTipoDos(Comando* comando, String* subcadenas) {
	comando->argumento1 = subcadenas[1];
	comando->argumento2 = subcadenas[2];
	comando->argumento3 = NULL;
}

void consolaProcesarComandoTipoTres(Comando* comando, String* subcadenas) {
	comando->argumento1 = subcadenas[1];
	comando->argumento2 = subcadenas[2];
	comando->argumento3 = subcadenas[3];
}

void consolaProcesarComando(Comando* comando, String* subcadenas) {
	int cantidadArgumentos = consolaComandoCantidadArgumentos(subcadenas[0]);
	switch(cantidadArgumentos) {
		case 1: consolaProcesarComandoTipoUno(comando, subcadenas); break;
		case 2: consolaProcesarComandoTipoDos(comando, subcadenas); break;
		case 3: consolaProcesarComandoTipoTres(comando, subcadenas); break;
		default: consolaProcesarComandoSinTipo(comando, subcadenas);
	}
}

bool consolaComandoExiste(String comando) {
	return consolaComandoTipoUno(comando) || consolaComandoTipoDos(comando) ||
			consolaComandoTipoTres(comando) || stringIguales(comando, C_FORMAT);
}

void consolaNormalizarComando(String* subcadenas) {
	if(stringIguales(subcadenas[1], FLAG_B)) {
		subcadenas[0] = C_RMB;
		subcadenas[1] = subcadenas[2];
		subcadenas[2] = subcadenas[3];
		subcadenas[3] = subcadenas[4];
		subcadenas[4] = NULL;
	}
	else {
		subcadenas[0] = C_RMD;
		subcadenas[1] = subcadenas[2];
		subcadenas[2] = NULL;
	}
}

bool consolaValidarComandoSinTipo(String* subcadenas) {
	return stringNulo(subcadenas[1]);
}

bool consolaValidarComandoTipoUno(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) && stringNulo(subcadenas[2]);
}

bool consolaValidarComandoTipoDos(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) && stringNoNulo(subcadenas[2]) &&
		   stringNulo(subcadenas[3]);
}

bool consolaValidarComandoTipoTres(String* subcadenas) {
	return stringNoNulo(subcadenas[1]) && stringNoNulo(subcadenas[2]) &&
		   stringNoNulo(subcadenas[3]) && stringNulo(subcadenas[4]);
}

bool consolaComandoConArgumentosValidos(String* subcadenas) {
	if(consolaComandoTipoUno(subcadenas[0]))
		return consolaValidarComandoTipoUno(subcadenas);
	else if(consolaComandoTipoDos(subcadenas[0]))
		return consolaValidarComandoTipoDos(subcadenas);
	else if(consolaComandoTipoTres(subcadenas[0]))
		return consolaValidarComandoTipoTres(subcadenas);
	else
		return consolaValidarComandoSinTipo(subcadenas);
}

bool consolaComandoInvalido(String* subcadenas) {
	return consolaComandoExiste(subcadenas[0]) == false;
}

Comando consolaObtenerComando() {
	Comando comando;
	char* cadena = consolaLeerEntrada();

	String* subcadenas = stringSeparar(cadena, ESPACIO);
	if(stringIguales(subcadenas[0], C_RM) && stringNoNulo(subcadenas[1]) && ((stringIguales(subcadenas[1], FLAG_B) && stringNulo(subcadenas[5])) || ((stringIguales(subcadenas[1], FLAG_D) && stringNulo(subcadenas[3])))))
		consolaNormalizarComando(subcadenas);
	if(consolaComandoInvalido(subcadenas)) {
		comando.identificador = ERROR;
		return comando;
	}

	if(!consolaComandoConArgumentosValidos(subcadenas)) {
		comando.identificador = ERROR;
		return comando;
	}
	comando.identificador = consolaIdentificarComando(subcadenas[0]);
	consolaProcesarComando(&comando, subcadenas);
	printf("arg 1 %s\n", comando.argumento1);
	printf("arg 2 %s\n", comando.argumento2);
	printf("arg 3 %s\n", comando.argumento3);
	memoriaLiberar(cadena);
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
			case CPFROM: puts("COMANDO CPFROM"); break;
			case CPTO: puts("COMANDO CPTO"); break;
			case CPBLOCK:puts("COMANDO CPBLOCK"); break;
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
		imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha desconectado");
	}
	else {
		imprimirMensaje(archivoLog, "[CONEXION] El proceso YAMA se ha desconectado");
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
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Data Node conectado exitosamente");
	}
	return nuevoSocket;
}

Socket servidorAceptarYAMA(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_YAMA);
	if(nuevoSocket != ERROR) {
		servidor->procesoYAMA = nuevoSocket;
		imprimirMensaje(archivoLog, "[CONEXION] Proceso YAMA conectado exitosamente");
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
	if(mensajeDesconexion(mensaje))
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
	memoriaLiberar(configuracion);
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
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	stringCopiar(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	stringCopiar(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYAMA);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Ruta Metadata: %s", configuracion->rutaMetadata);
}

void archivoConfigObtenerCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "RUTA_METADATA";
}

void funcionSenial(int senial) {
	estadoFileSystem = 0;
	puts("");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
}

