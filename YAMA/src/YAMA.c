/*
 ============================================================================
 Name        : YAMA.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "YAMA.h"

int main(void) {
	YAMAIniciar();
	YAMAConectarAFileSystem();
	YAMAAtenderMasters();
	return EXIT_SUCCESS;
}

void YAMAAtenderMasters() {
	Servidor* servidor = malloc(sizeof(Servidor));
	servidorInicializar(servidor);
	while(estadoYAMA)
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void YAMAConectarAFileSystem() {
	printf("[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	log_info(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	log_info(archivoLog, "[CONEXION] Conexion exitosa con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	printf("[CONEXION] Conexion exitosa con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
}

void YAMAIniciar() {
	pantallaLimpiar();
	estadoYAMA = 1;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoLog = archivoLogCrear(RUTA_LOG, "YAMA");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	strcpy(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	strcpy(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("Puerto Master: %s\n", configuracion->puertoMaster);
	printf("IP File System: %s\n", configuracion->ipFileSystem);
	printf("Puerto File System: %s\n", configuracion->puertoFileSystem);
	printf("Retardo de planificacion: %i\n", configuracion->retardoPlanificacion);
	printf("Algoritmo de balanceo: %s\n", configuracion->algoritmoBalanceo);
	puts("----------------------------------------------------------------");
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_PROPIO";
	campos[1] = "PUERTO_MASTER";
	campos[2] = "IP_FILESYSTEM";
	campos[3] = "PUERTO_FILESYSTEM";
	campos[4] = "RETARDO_PLANIFICACION";
	campos[5] = "ALGORITMO_BALANCEO";
}


void funcionSenial(int senial) {
	estadoYAMA = 0;
	puts("");
	imprimirMensajeProceso("# PROCESO YAMA FINALIZADO");
	puts("Aprete enter para salir");
	puts("----------------------------------------------------------------");
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
		if(archivoConfigTieneCampo(archivoConfig, "RETARDO_PLANIFICACION")){
			int retardo = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
				if(retardo != configuracion->retardoPlanificacion){
					puts("");
					log_warning(archivoLog, "[CONFIG]: SE MODIFICO EL ARCHIVO DE CONFIGURACION");
					configuracion->retardoPlanificacion = retardo;
					log_warning(archivoLog, "[CONFIG]: NUEVA RUTA METADATA: %s\n", configuracion->retardoPlanificacion);
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

void servidorFinalizarConexion(Servidor* servidor, Socket unSocket) {
	listaSocketsEliminar(unSocket, &servidor->listaMaster);
	socketCerrar(unSocket);
	puts("[CONEXION] Un proceso Master se ha desconectado");
	log_info(archivoLog, "[CONEXION] Un proceso Master se ha desconectado");
}

void servidorAceptarConexion(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_MASTER);
	if(nuevoSocket != ERROR) {
		puts("[CONEXION] Proceso Master conectado exitosamente");
		log_info(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
		listaSocketsAgregar(nuevoSocket, &servidor->listaMaster);
		servidorControlarMaximoSocket(servidor, unSocket);
	}
}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeOperacionErronea(mensaje))
		servidorFinalizarConexion(servidor, unSocket);
	else
		puts("[MENSAJE]");
	mensajeDestruir(mensaje);
}

bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaSelect);
}

bool socketEsListener(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerMaster, unSocket);
}

void servidorControlarSocket(Servidor* servidor, Socket unSocket) {
	if (socketRealizoSolicitud(servidor, unSocket)) {
		if(socketEsListener(servidor, unSocket))
			servidorAceptarConexion(servidor, unSocket);
		else
			servidorRecibirMensaje(servidor, unSocket);
	}
}

void servidorActivarListenerMaster(Servidor* servidor) {
	servidor->listenerMaster = socketCrearListener(configuracion->puertoMaster);
	printf("[CONEXION] Esperando conexiones de Master (Puerto %s)\n", configuracion->puertoMaster);
	log_info(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto %s)\n", configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerMaster);
}


void servidorInicializar(Servidor* servidor) {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	servidorActivarListenerMaster(servidor);
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


