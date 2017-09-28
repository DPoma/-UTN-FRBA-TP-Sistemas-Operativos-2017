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
	yamaIniciar();
	yamaConectarAFileSystem();
	yamaAtenderMasters();
	yamaFinalizar();
	return EXIT_SUCCESS;
}

void yamaIniciar() {
	pantallaLimpiar();
	estadoYama=ACTIVADO;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoLog = archivoLogCrear(RUTA_LOG, "YAMA");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivoConfig, campos);
}

void yamaConectarAFileSystem() {
	imprimirMensajeDos(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con File System");
}

void yamaAtenderMasters() {
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorInicializar(servidor);
	while(estadoYama==ACTIVADO)
		servidorAtenderPedidos(servidor);
	memoriaLiberar(servidor);
}

void yamaFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso YAMA finalizado");
	archivoLogDestruir(archivoLog);
	memoriaLiberar(configuracion);
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


void servidorInicializar(Servidor* servidor) {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);

	servidor->listenerMaster = socketCrearListener(configuracion->puertoMaster);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de un Master (Puerto %s)", configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	listaSocketsAgregar(socketFileSystem,&servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, socketFileSystem);
	servidorControlarMaximoSocket(servidor, servidor->listenerMaster);
}

void servidorAtenderPedidos(Servidor* servidor) {
	servidor->listaSelect = servidor->listaMaster;
	socketSelect(servidor->maximoSocket, &servidor->listaSelect);


	Socket socketI;
	Socket maximoSocket = servidor->maximoSocket;
	for(socketI = 0; socketI <= maximoSocket; socketI++){
		if (listaSocketsContiene(socketI, &servidor->listaSelect)){ //hay solicitud
			if(socketI==servidor->listenerMaster){
				Socket nuevoSocket;
				nuevoSocket = socketAceptar(socketI, ID_MASTER);
				if(nuevoSocket != ERROR) {
					imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
					listaSocketsAgregar(nuevoSocket, &servidor->listaMaster);
					servidorControlarMaximoSocket(servidor, nuevoSocket);
				}
			}else if(socketI==socketFileSystem){
				Mensaje* mensaje = mensajeRecibir(socketI);
				Socket masterid;
				memcpy(&masterid,mensaje->datos,sizeof(Socket));
				imprimirMensajeUno(archivoLog, "[RECEPCION] lista de bloques para master #%d recivida",&masterid);
				if(listaSocketsContiene(masterid,&servidor->listaMaster)){ //por si el master se desconecto
					//agarro la lista de bloques
					//segun el algoritmo hago lo que tenga que hacer
					//armo la tabla
					//le mando al master
				}
				mensajeDestruir(mensaje);
			}else{
				Mensaje* mensaje = mensajeRecibir(socketI);
				if(mensajeDesconexion(mensaje)){
					listaSocketsEliminar(socketI, &servidor->listaMaster);
					socketCerrar(socketI);
					if(socketI==servidor->maximoSocket)
						servidor->maximoSocket--; //no debería romper nada
					imprimirMensaje(archivoLog, "[CONEXION] Un proceso Master se ha desconectado");
				}
				else{//se recibio solicitud de master #socketI
					//el mensaje es el path del archivo, aca le acoplo el numero de master y se lo mando al fileSystem
					mensaje=realloc(mensaje,mensaje->header.tamanio+sizeof(Socket));
					memmove(mensaje+sizeof(Socket),mensaje,mensaje->header.tamanio);
					memcpy(mensaje,&socketI,sizeof(Socket));
					mensajeEnviar(socketFileSystem,1,mensaje->datos,mensaje->header.tamanio+sizeof(Socket));
					imprimirMensajeUno(archivoLog, "[ENVIO] path de master #%d enviado al fileSystem",&socketI);
				}
				//aca va a haber un bloque mas para el caso de que el master
				//me avise que termino algun proceso o tuvo errores
				mensajeDestruir(mensaje);
			}
		}
	}
}

void servidorControlarMaximoSocket(Servidor* servidor, Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->maximoSocket))
		servidor->maximoSocket = unSocket;
}











//wat is dis
//void notificadorInformar(Socket unSocket) {
//	char buffer[BUF_LEN];
//	int length = read(unSocket, buffer, BUF_LEN);
//	int offset = 0;
//	while (offset < length) {
//		struct inotify_event *event = (struct inotify_event *) &buffer[offset];
//		if (event->len) {
//		if (event->mask & IN_MODIFY) {
//		if (!(event->mask & IN_ISDIR)) {
//		if(strcmp(event->name, "ArchivoConfig.conf"))
//			break;
//		ArchivoConfig archivoConfig = config_create(RUTA_CONFIG);
//		if(archivoConfigTieneCampo(archivoConfig, "RETARDO_PLANIFICACION")){
//			int retardo = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
//				if(retardo != configuracion->retardoPlanificacion){
//					puts("");
//					log_warning(archivoLog, "[CONFIG]: SE MODIFICO EL ARCHIVO DE CONFIGURACION");
//					configuracion->retardoPlanificacion = retardo;
//					log_warning(archivoLog, "[CONFIG]: NUEVA RUTA METADATA: %s\n", configuracion->retardoPlanificacion);
//
//				}
//				archivoConfigDestruir(archivoConfig);
//		}
//		}
//		}
//		}
//		offset += sizeof (struct inotify_event) + event->len;
//		}
//
//	//Esto se haria en otro lado
//
//	//inotify_rm_watch(file_descriptor, watch_descriptor);
//		//close(file_descriptor);
//}
