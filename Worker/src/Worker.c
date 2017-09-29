/*
 ============================================================================
 Name        : Worker.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso Worker
 ============================================================================
 */

#include "Worker.h"

int main(void) {
	workerIniciar();
	socketListenerWorker = socketCrearListener(configuracion->puertoWorker);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto: %s)", configuracion->puertoWorker);
	while(estadoWorker)
		socketAceptarConexion();
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	return EXIT_SUCCESS;
}

void workerCrearHijo(Socket unSocket) {
	int pid = fork();
	if(pid == 0) {
	imprimirMensaje(archivoLog, "[CONEXION] Esperando mensajes de Master");
	Mensaje* mensaje = mensajeRecibir(unSocket);
	char* codigo;
	int sizeCodigo;
	char* origen;
	int sizeOrigen;
	char* destino;
	int sizeDestino;
	switch(mensaje->header.operacion){
			case -1:
				imprimirMensaje(archivoLog, "[EJECUCION] Murio el Master"); //revisar si deve morir por este caso
				estadoWorker=0;
				break;
			case 1: //Etapa Transformacion
			{
				int Origenb; //si el origen es un numero de bloque esto lo facilitaria, revisar
				memcpy(&sizeCodigo, mensaje->datos, sizeof(int));
				memcpy(&codigo,mensaje->datos + sizeof(int), sizeCodigo);
				memcpy(&origen,mensaje->datos + sizeof(int)+sizeCodigo, sizeof(int));
				memcpy(&sizeDestino, mensaje->datos + sizeof(int)*2 + sizeCodigo , sizeof(int));
				memcpy(&destino,mensaje->datos + sizeof(int)*3+sizeCodigo, sizeDestino);
				//transformar();
				/*char* buffer = leerArchivo(path,offset,size);
				log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
				if(buffer=="-1"){
					lSend(conexion, NULL, -4, 0);
					log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
					break;
				}
				//enviar el buffer
				lSend(conexion, buffer, 2, sizeof(char)*size);
				free(buffer);
				free(path);*/
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 2:{ //Etapa Reduccion Local

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int));
				memcpy(&codigo,mensaje->datos + sizeof(int), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int) +sizeCodigo, sizeof(int));
				memcpy(&origen,mensaje->datos + sizeof(int)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int)*2 + sizeCodigo + sizeOrigen, sizeof(int));
				memcpy(&destino,mensaje->datos + sizeof(int)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//reduccionLocal();
				/*char* buffer = leerArchivo(path,offset,size);
				log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
				if(buffer=="-1"){
					lSend(conexion, NULL, -4, 0);
					log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
					break;
				}
				//enviar el buffer
				lSend(conexion, buffer, 2, sizeof(char)*size);
				free(buffer);
				free(path);*/
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 3:{ //Etapa Reduccion Global

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int));
				memcpy(&codigo,mensaje->datos + sizeof(int), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int) +sizeCodigo, sizeof(int));
				memcpy(&origen,mensaje->datos + sizeof(int)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int)*2 + sizeCodigo + sizeOrigen, sizeof(int));
				memcpy(&destino,mensaje->datos + sizeof(int)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//reduccionGlobal();
				/*char* buffer = leerArchivo(path,offset,size);
				log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
				if(buffer=="-1"){
					lSend(conexion, NULL, -4, 0);
					log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
					break;
				}
				//enviar el buffer
				lSend(conexion, buffer, 2, sizeof(char)*size);
				free(buffer);
				free(path);*/
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 4:{ //Almacenamiento Definitivo

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int));
				memcpy(&codigo,mensaje->datos + sizeof(int), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int) +sizeCodigo, sizeof(int));
				memcpy(&origen,mensaje->datos + sizeof(int)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int)*2 + sizeCodigo + sizeOrigen, sizeof(int));
				memcpy(&destino,mensaje->datos + sizeof(int)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//almacenar();
				/*char* buffer = leerArchivo(path,offset,size);
				log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
				if(buffer=="-1"){
					lSend(conexion, NULL, -4, 0);
					log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
					break;
				}
				//enviar el buffer
				lSend(conexion, buffer, 2, sizeof(char)*size);
				free(buffer);
				free(path);*/
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
		}
	//printf("Mensaje: %s\n", (String)mensaje->datos);
	}
	else if(pid > 0)
		puts("PADRE ACEPTO UNA CONEXION");
	else
		puts("ERROR");
}

void socketAceptarConexion() {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(socketListenerWorker, ID_MASTER);
	if(nuevoSocket != ERROR) {
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
		workerCrearHijo(nuevoSocket);
	}
}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSytem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

char* agregarBarraCero(char* data, int tamanio)
{
	char* path = malloc(tamanio+1);
	memcpy(path, data, tamanio);
	path[tamanio] = '\0';
	return path;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);

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
	puts("");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	exit(0);
}

void workerIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	archivoConfigObtenerCampos();
	senialAsignarFuncion(SIGINT, funcionSenial);
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	estadoWorker = 1;
}
