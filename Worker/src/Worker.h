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

typedef struct {
	int cant;
	void** oris;
} lGlobOri;

typedef struct {
	int cant;
	char** ruta;
} locOri;

typedef struct {
	int sizebuffer;
	char* buffer;
	int NumReg;
} datosReg;

int pid;

const int MB = 1048576;
String campos[5];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoWorker;
int tamanioArchData;
Socket socketListenerWorker;
void socketAceptarConexion();


void workerIniciar();
int transformar(char*,int,char*);
int reduccionLocal(char*,char*,char*);
locOri* getOrigenesLocales(char*);
char* appendL(locOri*);
int reduccionGlobal(char*,char*,char*);
lGlobOri* getOrigenesGlobales(char*);
char* appendG(lGlobOri*);
datosReg* PasaRegistro(char*,int);
