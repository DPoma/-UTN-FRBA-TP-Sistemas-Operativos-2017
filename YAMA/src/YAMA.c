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
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorInicializar(servidor);
	while(estadoYAMA)
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void YAMAConectarAFileSystem() {
	imprimirMensajeDos(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con File System");
}

void YAMAIniciar() {
	pantallaLimpiar();
	estadoYAMA = 1;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoLog = archivoLogCrear(RUTA_LOG, "YAMA");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	stringCopiar(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
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
	imprimirMensaje(archivoLog, "[CONEXION] Un proceso Master se ha desconectado");
}

void servidorAceptarConexion(Servidor* servidor, Socket socketListener) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(socketListener, ID_MASTER);
	if(nuevoSocket != ERROR) {
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
		listaSocketsAgregar(nuevoSocket, &servidor->listaMaster);
		servidorControlarMaximoSocket(servidor, nuevoSocket);
	}
}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeDesconexion(mensaje))
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
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto %s)", configuracion->puertoMaster);
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
	memoriaLiberar(configuracion);
}

void servidorAtenderSolicitud(Servidor* servidor) {
	Socket unSocket;
	Socket maximoSocket = servidor->maximoSocket;
	for(unSocket = 0; unSocket <= maximoSocket; unSocket++)
		servidorControlarSocket(servidor, unSocket);
}

void servidorAtenderPedidos(Servidor* servidor) {
	servidorSetearListaSelect(servidor);
	servidorEsperarSolicitud(servidor);
	servidorAtenderSolicitud(servidor);
}


