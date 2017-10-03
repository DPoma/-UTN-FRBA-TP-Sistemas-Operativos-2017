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
				int origenB; //si el origen es un numero de bloque esto lo facilitaria, revisar
				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&origen,mensaje->datos + sizeof(int32_t)+sizeCodigo, sizeof(int32_t));
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo , sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo, sizeDestino);
				transformar(codigo,origenB,destino);
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

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
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

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//reduccionGlobal();
				/*char* buffer = leerArchivo(path,offset,size);
				log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
				if(buffer=="-1"){
					lSend(conexion, NULL, -4, 0);
					log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
					break;32_t
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

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
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

//1er etapa
int transformar(char* codigo,int origen,char* destino){
	if (origen>tamanioArchData){
		return -1;
	}
	char* buffer=NULL;
	FILE* arch;
	arch = fopen(configuracion->rutaDataBin,"r");
	fseek(arch,MB*origen,SEEK_SET);
	int i;
	char c;
	int cc = 0;
	for(i=0;cc<=MB;i++){
		c = fgetc(arch);
			buffer = realloc(buffer,sizeof(char)*(cc+1));
			buffer[cc]=c;
			cc++;
	}
	/*int j;
	for(j=0;j<=size;j++){
		c = fgetc(arch);
		srtcat(buffer,c);
	}*/
	fclose(arch);
	//doy privilegios a script
	char*commando=NULL;
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	//paso buffer a script y resultado script a sort
	char*command=NULL;
	strcat(command,codigo);
	strcat(command,buffer);//suponiendo que el script requiere un buffer como parametro
	strcat(command,"| sort >");
	strcat(command,destino);
	system(command);
	return 0;
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
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] tamanio data.bin: %dMB", tamanioArchData);

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
	char* texto;
	strcat(texto,"stat -c%s ");
	strcat(texto, configuracion->rutaDataBin);
	tamanioArchData = system(texto)/MB;
	configuracionImprimir(configuracion);
	estadoWorker = 1;
	free(texto);
}
