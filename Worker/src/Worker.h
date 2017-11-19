/*
 * Worker.h

 *
 *  Created on: 18/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

//--------------------------------------- Constantes -------------------------------------

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
#define INTSIZE sizeof(Entero)
#define TEMPSIZE 12

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/WorkerLog.log"
#define RUTA_TEMPS "/home/utnso/Escritorio/temp/"

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	char ipFileSytem[20];
	char puertoFileSystem[20];
	char nombreNodo[10];
	char puertoMaster[20];
	char puertoWorker[20];
	char rutaDataBin[255];
	char ipPropia[20];
} Configuracion;

typedef struct {
	int sizeScript;
	String script;
	int numeroBloque;
	int bytesUtilizados;
	char archivoTemporal[12];
} Transformacion;

typedef struct {
	int sizeScript;
	String script;
	int cantidadTemporales;
	String temporales;
	char temporalReduccion[12];
} ReduccionLocal;

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

typedef void* BloqueWorker;

//--------------------------------------- Globales -------------------------------------

Configuracion* configuracion;
ArchivoLog archivoLog;
String campos[7];
Socket listenerWorker;
Socket listenerMaster;
File dataBin;
Puntero punteroDataBin;
int pid;
int estadoWorker;
int dataBinBloques;
int dataBinTamanio;

//--------------------------------------- Funciones de Worker -------------------------------------

void workerIniciar();
void workerAtenderMasters();
void workerFinalizar();
void masterAceptarConexion();
void masterEjecutarOperacion(Socket unSocket);

//--------------------------------------- Funciones de Configuracion  -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
void configuracionIniciarLog();
void configuracionIniciarCampos();
void configuracionCalcularBloques();
void configuracionSenial(int senial);
void configuracionIniciar();

//--------------------------------------- Funciones de Master -------------------------------------
void masterAceptarConexion();
void masterAtenderOperacion(Socket unSocket);
void masterEjecutarOperacion(Socket unSocket);

//--------------------------------------- Funciones de Transformacion -------------------------------------

int transformacionEjecutar(Transformacion* transformacion);
void transformacionIniciar(Mensaje* mensaje, Socket unSocket);
int transformacionEjecutar(Transformacion* transformacion);
void transformacionExito(Entero numeroBloque, Socket unSocket);
void transformacionFracaso(Entero numeroBloque, Socket unSocket);
String transformacionBloqueTemporal(Transformacion* transformacion);
String transformacionScriptTemporal(Transformacion* transformacion);

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir();
void configuracionCalcularBloques();
Puntero dataBinMapear();
void dataBinConfigurar();
BloqueWorker bloqueBuscar(Entero numeroBloque);
BloqueWorker getBloque(Entero numeroBloque);





int reduccionLocalEjecutar(char*,char*,char*);
locOri* getOrigenesLocales(char*);
char* appendL(locOri*);
int reduccionGlobalEjecutar(char*,char*,char*);
lGlobOri* getOrigenesGlobales(char*);
char* appendG(lGlobOri*);
datosReg* PasaRegistro(char*,int);

