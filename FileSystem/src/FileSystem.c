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

//Identificador de cada comando
#define FORMAT 1
#define RM 2
#define RMB 3
#define RMD 14
#define RENAME 4
#define MV 5
#define CAT 6
#define MKDIR 7
#define PFROM 8
#define CPTO 9
#define CPTBLOCK 10
#define MD5 11
#define LS 12
#define INFO 13

#define C_FORMAT "format"
#define C_RM "rm"
#define C_RMB "rm -b"
#define C_RMD "rm -d"
#define C_RENAME "rename"
#define C_MV "mv"
#define C_CAT "cat"
#define C_MKDIR "mkdir"
#define C_PFROM "pfrom"
#define C_CPTO "cpto"
#define C_CPTBLOCK "cptblock"
#define C_MD5 "md5"
#define C_LS "ls"
#define C_INFO "info"

int sonIguales(char* s1, char* s2) {
	if (strcmp(s1, s2) == 0)
		return 1;
	else
		return 0;
}

int identificarComando(char* comando) {
	if(sonIguales(comando, C_FORMAT))
		return FORMAT;
	if(sonIguales(comando, C_INFO))
		return INFO;
	else if(sonIguales(comando, C_RM))
		return RM;
	else if(sonIguales(comando, C_RMB))
		return RMB;
	else if(sonIguales(comando, C_RMD))
		return RMD;
	else if(sonIguales(comando, C_RENAME))
		return RENAME;
	else if(sonIguales(comando, C_MV))
		return MV;
	else if(sonIguales(comando, C_CAT))
		return CAT;
	else if(sonIguales(comando, C_MKDIR))
		return MKDIR;
	else if(sonIguales(comando, C_PFROM))
		return PFROM;
	else if(sonIguales(comando, C_CPTO))
		return CPTO;
	else if(sonIguales(comando, C_CPTBLOCK))
		return CPTBLOCK;
	else if(sonIguales(comando, C_MD5))
		return MD5;
	else if(sonIguales(comando, C_LS))
		return LS;
	else if(sonIguales(comando, C_INFO))
		return INFO;
	else if(sonIguales(comando, C_RENAME))
		return RENAME;
	else
		return -1;
}

char* leerCaracteresEntrantes() {
	int i, caracterLeido;
	char* cadena = malloc(1000);
	for(i = 0; (caracterLeido= getchar()) != '\n'; i++)
		cadena[i] = caracterLeido;
	cadena[i] = '\0';
	return cadena;
}


Instruccion obtenerInstruccion() {
	Instruccion instruccion;
	char* mensaje = leerCaracteresEntrantes();
	instruccion.comando = identificarComando(mensaje);
	free(mensaje);
	return instruccion;
}


void atenderInstrucciones() {
	Instruccion instruccion;
	while(1) {
		printf("Ingrese un comando: ");
		instruccion = obtenerInstruccion();
		switch(instruccion.comando) {
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

int main(void) {
	system("clear");
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	estado = 1;
	cargarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	senialAsignarFuncion(SIGINT, funcionSenial);
	archivoConfigImprimir(configuracion);
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	log_info(archivoLog, "Probando mensaje en log...");
	log_warning(archivoLog, "Probando advertencia en log...");
	log_error(archivoLog, "Probando error en log...");
	puts("----------------------------------------------------------------");
	Hilo hilo;
	hiloCrear(&hilo, (void*)atenderInstrucciones, NULL);
	cargarDatos();
	Servidor servidor = servidorCrear(puertos, CANTIDAD_PUERTOS);
	while(estado)
		servidorAtenderClientes(&servidor);
	servidorFinalizar(&servidor);
	imprimirMensajeProceso("Proceso File Sytstem finalizado finalizado");


	return 0;
}

//--------------------------------------- Funciones para ControlServidor -------------------------------------

void controlServidorInicializar(ControlServidor* controlServidor) {
	controlServidor->conexion.tamanioAddress = sizeof(controlServidor->conexion.address);
	controlServidor->maximoSocket = 0;
	listaSocketsLimpiar(&controlServidor->listaSocketsMaster);
	listaSocketsLimpiar(&controlServidor->listaSocketsSelect);
	notificadorInicializar(controlServidor);
}

int controlServidorCantidadSockets(ControlServidor* controlServidor) {
	return controlServidor->maximoSocket;
}

void controlServidorSetearListaSelect(ControlServidor* controlServidor) {
	controlServidor->listaSocketsSelect = controlServidor->listaSocketsMaster;
}

void controlServidorActualizarMaximoSocket(ControlServidor* controlServidor, Socket unSocket) {
	if(socketEsMayor(unSocket, controlServidor->maximoSocket))
		controlServidor->maximoSocket = unSocket;
}

void controlServidorEjecutarSelect(ControlServidor* controlServidor) {
	socketSelect(controlServidor->maximoSocket, &controlServidor->listaSocketsSelect);
}


void notificadorInformar(Socket unSocket) {
	char buffer[BUF_LEN];
	int length = read(unSocket, buffer, BUF_LEN);
	int offset = 0;
	while (offset < length) {
		struct inotify_event *event = (struct inotify_event *) &buffer[offset];
		if (event->len) {
		if (event->mask & IN_MODIFY) {
		if (!(event->mask & IN_ISDIR)) {
		if(strcmp(event->name, "FileSystemConfig.conf"))
			break;
		ArchivoConfig archivoConfig = config_create(RUTA_CONFIG);
		if(archivoConfigTieneCampo(archivoConfig, "RUTA_METADATA")){
			char* ruta = archivoConfigStringDe(archivoConfig, "RUTA_METADATA");
				if(ruta != configuracion->rutaMetadata){
					puts("");
					log_warning(archivoLog, "[CONFIG]: SE MODIFICO EL ARCHIVO DE CONFIGURACION");
					strcpy(configuracion->rutaMetadata,ruta);
					log_warning(archivoLog, "[CONFIG]: NUEVA RUTA METADATA: %s\n", configuracion->rutaMetadata);
				}
				archivoConfigDestruir(archivoConfig);
		}
		}
		}
		}
		offset += sizeof (struct inotify_event) + event->len;
		}

	//Esto se haria en otro lado

	//inotify_rm_watch(file_descriptor, watch_descriptor);
		//close(file_descriptor);
}


//--------------------------------------- Funciones para Puerto -------------------------------------

void puertoActivarListener(Puerto* puerto, Conexion* conexion) {
	puerto->listener = socketCrearListener(conexion->ip, conexion->puerto);
	printf("Esperando conexiones en IP: %s | Puerto: %s\n", conexion->ip, conexion->puerto);
}

void puertoFinalizarConexionCon(Servidor* servidor, Socket unSocket) {
	socketCerrar(unSocket);
	listaSocketsEliminar(unSocket, &servidor->controlServidor.listaSocketsMaster);
	int indice;
	for(indice = 0; !listaSocketsContiene(unSocket,&servidor->listaPuertos[indice].clientesConectados); indice++);
	listaSocketsEliminar(unSocket, &servidor->listaPuertos[indice].clientesConectados);
	printf("El socket %d finalizo la conexion\n", unSocket);
}

int puertoBuscarListener(Servidor* servidor, Socket unSocket) {
	int indice;
	for(indice = 0; socketSonDistintos(unSocket, servidor->listaPuertos[indice].listener) && indice < CANTIDAD_PUERTOS; indice++);
	if(indice >= CANTIDAD_PUERTOS)
		indice = -1;
	return indice;
}

void puertoAceptarCliente(Socket unSocket, Servidor* servidor) {
	int nuevoSocket = socketAceptar(&servidor->controlServidor.conexion, unSocket);
	if(nuevoSocket != -1) {
		int indice =  puertoBuscarListener(servidor, unSocket);
		if(indice != -1)
			listaSocketsAgregar(nuevoSocket, &servidor->listaPuertos[indice].clientesConectados);
		listaSocketsAgregar(nuevoSocket, &servidor->controlServidor.listaSocketsMaster);
		controlServidorActualizarMaximoSocket(&servidor->controlServidor, nuevoSocket);
		puts("Un proceso se ha conectado");
	}
}

void puertoRecibirMensajeCliente(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeOperacionErronea(mensaje))
		puertoFinalizarConexionCon(servidor, unSocket);
	else {
		if(!strcmp(mensaje->dato,"quit\n"))
			servidor->controlServidor.estado = 0;
		else {
			printf("Nuevo mensaje en socket %i: %s", unSocket, (char*)(mensaje->dato));
			mensajeEnviar(8, 4, mensaje->dato, stringLongitud(mensaje->dato)+1);
		}

	}
	mensajeDestruir(mensaje);
}


void puertoActualizarSocket(Servidor* servidor, Socket unSocket) {
	if (listaSocketsContiene(unSocket, &servidor->controlServidor.listaSocketsSelect)) {
		if (socketEsListenerDe(servidor, unSocket))
			puertoAceptarCliente(unSocket, servidor);
		else
			if(!socketEsNotificador(servidor, unSocket)) {
				if(listaSocketsContiene(unSocket, &servidor->listaPuertos[0].clientesConectados))
					printf("Puerto 0: ");
				else if(listaSocketsContiene(unSocket, &servidor->listaPuertos[1].clientesConectados))
					printf("Puerto 1: ");
				puertoRecibirMensajeCliente(servidor, unSocket);
			}
			else
				notificadorInformar(unSocket);

	}
}

//--------------------------------------- Funciones para Servidor -------------------------------------
void notificadorInicializar(ControlServidor* controlServidor) {
	controlServidor->notificador = inotify_init();
	controlServidor->observadorNotifcador= inotify_add_watch(controlServidor->notificador, RUTA_NOTIFY, IN_MODIFY);
	listaSocketsAgregar(controlServidor->notificador, &controlServidor->listaSocketsMaster);
	controlServidorActualizarMaximoSocket(controlServidor, controlServidor->notificador);
}


void servidorActivarPuertos(Servidor* servidor, String* puertos) {
	controlServidorInicializar(&servidor->controlServidor);
	servidor->listaPuertos = malloc(sizeof(Puerto) *  servidor->controlServidor.cantidadPuertos);
	int indice;
	for(indice = 0; indice < servidor->controlServidor.cantidadPuertos; indice++) {
		servidor->controlServidor.conexion.ip = ipFileSystem;
		servidor->controlServidor.conexion.puerto = puertos[indice];
		puertoActivarListener(&servidor->listaPuertos[indice], &servidor->controlServidor.conexion);
		listaSocketsAgregar(servidor->listaPuertos[indice].listener, &servidor->controlServidor.listaSocketsMaster);
		controlServidorActualizarMaximoSocket(&servidor->controlServidor, servidor->listaPuertos[indice].listener);
	}
}

void servidorSetearEstado(Servidor* servidor, int estado) {
	servidor->controlServidor.estado = estado;
}

Servidor servidorCrear(String* puertos, int cantidadPuertos) {
	Servidor servidor;
	servidor.controlServidor.estado = 1;
	servidor.controlServidor.cantidadPuertos = cantidadPuertos;
	servidorActivarPuertos(&servidor, puertos);
	return servidor;
}

void servidorFinalizar(Servidor* servidor) {
	archivoLogDestruir(archivoLog);
	free(servidor->listaPuertos);
	free(configuracion);
}

bool servidorEstaActivo(Servidor servidor) {
	return servidor.controlServidor.estado == 1;
}

void servidorAtenderClientes(Servidor* servidor) {
	controlServidorSetearListaSelect(&servidor->controlServidor);
	controlServidorEjecutarSelect(&servidor->controlServidor);
	servidorActualizarPuertos(servidor);
}

void servidorActualizarPuertos(Servidor* servidor) {
	Socket unSocket;
	int maximoSocket = servidor->controlServidor.maximoSocket;
	for(unSocket = 0; unSocket <= maximoSocket; unSocket++)
		puertoActualizarSocket(servidor, unSocket);
}

bool socketEsListenerDe(Servidor* servidor, Socket unSocket) {
	int indice;
	for(indice = 0; indice < servidor->controlServidor.cantidadPuertos; indice++)
		if(socketSonIguales(unSocket, servidor->listaPuertos[indice].listener))
			return 1;
	return 0;
}


bool socketEsNotificador(Servidor* servidor, Socket unSocket) {
	return servidor->controlServidor.notificador == unSocket;
}


Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	strcpy(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	strcpy(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP File System: %s\n", configuracion->ipFileSystem);
	printf("Puerto YAMA: %s\n", configuracion->puertoYAMA);
	printf("Puerto Data Node: %s\n", configuracion->puertoDataNode);
	printf("Ruta Metadata: %s\n", configuracion->rutaMetadata);
	puts("----------------------------------------------------------------");
}

void cargarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_YAMA";
	campos[2] = "PUERTO_DATANODE";
	campos[3] = "RUTA_METADATA";
}

void cargarDatos() {
	ipFileSystem = configuracion->ipFileSystem;
	puertos[0] = configuracion->puertoYAMA;
	puertos[1] = configuracion->puertoDataNode;
}

void funcionSenial(int senial) {
	estado = 0;
	puts("");
	imprimirMensajeProceso("PROCESO FILE SYSTEM FINALIZADO");
}




