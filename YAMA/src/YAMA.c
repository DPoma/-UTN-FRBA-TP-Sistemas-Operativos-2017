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
	servidor = memoriaAlocar(sizeof(Servidor));
	imprimirMensajeDos(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	servidor->fileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);
	imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con File System");

	Mensaje* cantNodos=mensajeRecibir(servidor->fileSystem);
	mensajeObtenerDatos(cantNodos,servidor->fileSystem); //medio al pedo para un int pero bueno
	int i=0;
	while(i<(*(int32_t*)cantNodos->datos)){
		Nodo nodo;
		nodo.conectado=true;
//		nodo.disponibilidad=###;
		nodo.numero=i;
		listaAgregarElemento(nodos,&nodo);
	}
}

void yamaAtenderMasters() {
	servidorInicializar(servidor);
	while(estadoYama==ACTIVADO)
		servidorAtenderPedidos(servidor);
	memoriaLiberar(servidor);
}

void yamaFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso YAMA finalizado");
	archivoLogDestruir(archivoLog);
	memoriaLiberar(servidor); //no es al pedo liberar memoria si el
	memoriaLiberar(configuracion);//programa esta a punto de terminar?
}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	configuracion->retardoPlanificacion = archivoConfigEnteroDe(archivoConfig, "RETARDO_PLANIFICACION");
	stringCopiar(configuracion->algoritmoBalanceo, archivoConfigStringDe(archivoConfig, "ALGORITMO_BALANCEO"));
	stringCopiar(configuracion->disponibilidadBase, archivoConfigStringDe(archivoConfig, "DISPONIBILIDAD_BASE"));
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
	campos[6] = "DISPONIBILIDAD_BASE";
}//donde se usan los campos estos?


void servidorInicializar() {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);

	servidor->listenerMaster = socketCrearListener(configuracion->puertoMaster);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de un Master (Puerto %s)", configuracion->puertoMaster);
	listaSocketsAgregar(servidor->listenerMaster, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor->fileSystem);
	servidorControlarMaximoSocket(servidor->listenerMaster);
}

void servidorAtenderPedidos() {
	servidor->listaSelect = servidor->listaMaster;
	socketSelect(servidor->maximoSocket, &servidor->listaSelect);


	Socket socketI;
	Socket maximoSocket = servidor->maximoSocket;
	for(socketI = 0; socketI <= maximoSocket; socketI++){
		if (listaSocketsContiene(socketI, &servidor->listaSelect)){ //hay solicitud
			//podría disparar el thread aca sino
			if(socketI==servidor->listenerMaster){
				Socket nuevoSocket;
				nuevoSocket = socketAceptar(socketI, ID_MASTER);
				if(nuevoSocket != ERROR) {
					imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
					listaSocketsAgregar(nuevoSocket, &servidor->listaMaster);
					servidorControlarMaximoSocket(nuevoSocket);
				}
			}else if(socketI==servidor->fileSystem){
				Mensaje* mensaje = mensajeRecibir(socketI);
				mensajeObtenerDatos(mensaje,socketI);
				//aca debería estar el caso de que el mensaje sea
				//para avisar que se desconectó un nodo
				//y otro para avisar que se reconectó. (si pueden hacer eso)
				//podría hacer switch(mensaje->header.operacion)

				Socket masterid;
				memcpy(&masterid,mensaje->datos,INTSIZE);
				imprimirMensajeUno(archivoLog, "[RECEPCION] lista de bloques para master #%d recibida",&masterid);
				void* listaBloques=malloc(mensaje->header.tamanio-INTSIZE);
				memcpy(listaBloques,mensaje->datos+INTSIZE,mensaje->header.tamanio-INTSIZE);
				if(listaSocketsContiene(masterid,&servidor->listaMaster)) //por si el master se desconecto
					yamaPlanificar(masterid,listaBloques,mensaje->header.tamanio-INTSIZE);
				//por ahi meto yamaPlanificar en un thread asi puedo seguir usando select
				mensajeDestruir(mensaje);
			}else{ //master
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
					int32_t masterid = socketI;
					mensaje=realloc(mensaje,mensaje->header.tamanio+sizeof(int32_t));
					memmove(mensaje+sizeof(int32_t),mensaje,mensaje->header.tamanio);
					memcpy(mensaje,&masterid,sizeof(int32_t));
					mensajeEnviar(socketFileSystem,1,mensaje->datos,mensaje->header.tamanio+sizeof(int32_t));
					imprimirMensajeUno(archivoLog, "[ENVIO] path de master #%d enviado al fileSystem",&socketI);
				}
				//aca va a haber un bloque mas para el caso de que el master
				//me avise que termino algun proceso o tuvo errores. Otro switch
				mensajeDestruir(mensaje);
			}
		}
	}
}

void servidorControlarMaximoSocket(Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->maximoSocket))
		servidor->maximoSocket = unSocket;
}

void yamaPlanificar(Socket master, void* listaBloques,int tamanio){
	int i=0;
	Lista bloques=listaCrear();
	while(sizeof(Bloque)*i<tamanio){
		listaAgregarElemento(bloques,(Bloque*)(listaBloques+sizeof(Bloque)*i));
		i++;
	}//por ahi esto es al pedo y me puedo manejar con la lista de void*


	if(!strcmp(configuracion->algoritmoBalanceo,"Clock")){

	}else if(!strcmp(configuracion->algoritmoBalanceo,"W-Clock")){

	}else{
		imprimirMensaje(archivoLog,"[] no se reconoce el algoritmo");
		abort();
	}


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
