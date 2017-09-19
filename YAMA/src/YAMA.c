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

	pantallaLimpiar();
	int estado = 1;
	imprimirMensajeProceso("# PROCESO YAMA");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	archivoLog = archivoLogCrear(RUTA_LOG, "YAMA");

	printf("[CONEXION] Estableciendo conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	log_info(archivoLog, "[CONEXION] Realizando conexion con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);

	Socket socketFileSystem = socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystem, ID_YAMA);

	log_info(archivoLog, "[CONEXION] Conexion exitosa con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);
	printf("[CONEXION] Conexion exitosa con File System (IP: %s | Puerto %s)\n", configuracion->ipFileSystem, configuracion->puertoFileSystem);

	estado = 1;
	Socket socketListenerMaster = socketCrearListener(configuracion->puertoMaster);

	printf("[CONEXION] Esperando conexiones de Master (Puerto %s)\n", configuracion->puertoMaster);
	log_info(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto %s)\n", configuracion->puertoMaster);

	Socket socketMaster = socketAceptar(socketListenerMaster, ID_MASTER);

	printf("[CONEXION] Conexion exitosa con Master\n");
	log_info(archivoLog, "[CONEXION] Conexion exitosa con Master\n");

	while(estado) {
		Mensaje* mensaje = mensajeRecibir(socketMaster);
		if(mensajeOperacionErronea(mensaje))
			socketCerrar(socketMaster);
		else {
			printf("Nuevo mensaje de Master %i: %s", socketMaster, (char*)(mensaje->dato));
			mensajeEnviar(socketFileSystem, 4, mensaje->dato, strlen(mensaje->dato)+1);
		}
	}
	close(socketMaster);
	return 0;
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
	estado = 0;
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
