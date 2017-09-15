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
	system("clear");
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	estado = 1;
	cargarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	senialAsignarFuncion(SIGINT, funcionSenial);
	archivoConfigImprimir(configuracion);
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	log_info(archivoLog, "Probando imprimir mensaje en log...");
	log_warning(archivoLog, "Probando imprimir advertencia en log...");
	log_error(archivoLog, "Probando imprimir error en log...");
	puts("----------------------------------------------------------------");
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
		if(strcmp(event->name, "ArchivoConfig.conf"))
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
		puts("Un cliente se ha conectado");
	}
}

void puertoRecibirMensajeCliente(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeOperacionErronea(mensaje))
		puertoFinalizarConexionCon(servidor, unSocket);
	else {
		if(!strcmp(mensaje->dato,"quit\n"))
			servidor->controlServidor.estado = 0;
		else
			printf("Nuevo mensaje en socket %i: %s", unSocket, (char*)(mensaje->dato));
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
					else
						printf("Puerto 2: ");
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




