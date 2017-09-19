/*
 ============================================================================
 Name        : Worker.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso Worker
 ============================================================================
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define TAMANIO_DATO_MAXIMO 1024
#define CLIENTES_ESPERANDO 5
#define EVENT_SIZE (sizeof(struct inotify_event)+24)
#define BUF_LEN (1024*EVENT_SIZE)
#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_NOTIFY "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker"
#define RUTA_LOG "/home/utnso/Escritorio/WorkerLog.log"
#define CANTIDAD_PUERTOS 2

typedef struct {
	char ipFileSytem[50];
	char puertoFileSystem[50];
	char nombreNodo[50];
	char ipPropio[50];
	char puertoWorker[50];
	char rutaDataBin[100];
} Configuracion;

String campos[6];
Configuracion* configuracion;
ArchivoLog archivoLog;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipFileSytem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	strcpy(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	strcpy(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	strcpy(configuracion->ipPropio, archivoConfigStringDe(archivoConfig, "IP_PROPIO"));
	strcpy(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	strcpy(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP File System: %s\n", configuracion->ipFileSytem);
	printf("Puerto File System: %s\n", configuracion->puertoFileSystem);
	printf("Nombre Nodo: %s\n", configuracion->nombreNodo);
	printf("IP Nodo: %s\n", configuracion->ipPropio);
	printf("Puerto Worker: %s\n", configuracion->puertoWorker);
	printf("Ruta archivo data.bin: %s\n", configuracion->rutaDataBin);
	puts("----------------------------------------------------------------");
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "IP_PROPIO";
	campos[4] = "PUERTO_WORKER";
	campos[5] = "RUTA_DATABIN";
}

void funcionSenial(int senial) {
	//estado = 0;
	puts("");
	imprimirMensajeProceso("# PROCESO WORKER FINALIZADO");
	exit(0);
}


void atenderConexion(Socket unSocket) {
	puts("Conexion con Master realizada. Esperando mensajes...");

	while(1) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeOperacionErronea(mensaje))
			socketCerrar(unSocket);
		else
			printf("Nuevo mensaje de Master %i: %s", unSocket, (char*)(mensaje->dato));
	}
}

int main(void) {
	system("clear");
	int estado = 1;
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	senialAsignarFuncion(SIGINT, funcionSenial);
	archivoConfigImprimir(configuracion);
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	log_info(archivoLog, "Probando mensaje en log...");
	log_warning(archivoLog, "Probando advertencia en log...");
	log_error(archivoLog, "Probando error en log...");
	puts("----------------------------------------------------------------");
	Socket unSocket = socketCrearListener(configuracion->ipPropio,configuracion->puertoWorker);
	Socket otroSocket;
	puts("Esperando conexiones de Master...");
	Conexion conexion;
	conexion.tamanioAddress = sizeof(conexion.address);
	while(estado) {
	otroSocket = socketAceptar(&conexion, unSocket);
		if(otroSocket == -1)
			break;
		int pid = fork();
		if(pid == 0)
			atenderConexion(otroSocket);
	}
}
