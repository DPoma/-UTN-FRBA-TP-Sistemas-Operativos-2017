/*
 * Worker.h

 *
 *  Created on: 18/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define FRACASO -800
#define EXITO 1
#define DESCONEXION 0
#define ABORTAR 301
#define TRANSFORMACION 303
#define REDUCCION_LOCAL 304
#define REDUCCION_GLOBAL 305
#define ALMACENADO 306
#define CIERRE 307
#define PASAREG 308

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/WorkerLog.log"
#define RUTA_ARCHDATA "/home/utnso/Escritorio/" //TODO borrrar y poner en config
#define RUTA_TEMPS "/home/utnso/Escritorio/temp/"

typedef struct {
	char ipFileSytem[20];
	char puertoFileSystem[20];
	char nombreNodo[10];
	char puertoWorker[20];
	char rutaDataBin[255];
	char ipPropia[20];
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
int bloquesArchData;
int dataBinTamanio;
Socket socketListenerWorker;
void socketAceptarConexion();


void workerIniciar();
int transformar(char*,int,char*);
int reduccionLocalEjecutar(char*,char*,char*);
locOri* getOrigenesLocales(char*);
char* appendL(locOri*);
int reduccionGlobalEjecutar(char*,char*,char*);
lGlobOri* getOrigenesGlobales(char*);
char* appendG(lGlobOri*);
datosReg* PasaRegistro(char*,int);
