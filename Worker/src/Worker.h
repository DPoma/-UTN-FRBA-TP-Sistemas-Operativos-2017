/*
 * Worker.h

 *
 *  Created on: 18/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/WorkerLog.log"
#define RUTA_ARCHDATA ""

typedef struct {
	char ipFileSytem[50];
	char puertoFileSystem[50];
	char nombreNodo[50];
	char puertoWorker[50];
	char rutaDataBin[100];
} Configuracion;

typedef struct {
	char* ruta;
	char* ip;
	int puerto;
} globOri;

const MB = 1048576;
String campos[5];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoWorker;
int tamanioArchData;
Socket socketListenerWorker;
void socketAceptarConexion();


void workerIniciar();
int transformar(char*,int,char*);
