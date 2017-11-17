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
#define REDUCLOCAL 304
#define REDUCGLOBAL 305
#define ALMACENADO 306
#define CIERRE 307
#define PASAREG 308

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/WorkerLog.log"
#define RUTA_TEMPS "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/temp/"

typedef struct {
	char ipFileSytem[20];
	char puertoFileSystem[20];
	char nombreNodo[10];
	char puertoWorker[20];
	char puertoMaster[20];
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

#define DIRSIZE sizeof(Dir)
#define INTSIZE sizeof(int32_t)
#define TEMPSIZE 12

String campos[6];
Socket listenerMaster;

Puntero punteroDataBin;
File dataBin;
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoWorker;
int dataBinBloques;
int dataBinTamanio;
Socket socketListenerWorker;
void socketAceptarConexion();
typedef void* BloqueWorker;

void workerIniciar();
int transformar(char*,int,int,int,char*);
int reduccionLocal(char*,int,char*,char*);
locOri* getOrigenesLocales(char*);
char* appendL(locOri*);
int reduccionGlobal(char*,int,char*,char*);
lGlobOri* getOrigenesGlobales(char*);
char* appendG(lGlobOri*);
datosReg* PasaRegistro(char*,int);
void dataBinAbrir();
void configuracionCalcularBloques();
Puntero dataBinMapear();
void dataBinConfigurar();
String transformacionBloqueTemporal(int , int );
String transformacionScriptTemporal(char* ,int , int );
BloqueWorker getBloque(Entero );
BloqueWorker bloqueBuscar(Entero );
