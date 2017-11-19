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
	int sizeScript;
	String script;
	int cantidadTemporales;
	String temporales;
	char temporalReduccion[12];
} ReduccionGlobal;

typedef void* BloqueWorker;

//--------------------------------------- Globales -------------------------------------

Configuracion* configuracion;
ArchivoLog archivoLog;
String campos[7];
Socket listenerWorker;
Socket listenerMaster;
File dataBin;
Puntero punteroDataBin;
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
void transformacion(Mensaje* mensaje, Socket unSocket);
void transformacionDestruir(Transformacion* transformacion);
void transformacionExito(Entero numeroBloque, Socket unSocket);
void transformacionFracaso(Entero numeroBloque, Socket unSocket);
void transformacionRecibirScript(Transformacion* transformacion, Mensaje* mensaje);
void transformacionRecibirBloque(Transformacion* transformacion, Puntero datos);
void transformacionCrearNieto(Transformacion* transformacion, Socket unSocket);
String transformacionCrearBloqueTemporal(Transformacion* transformacion);
String transformacionCrearScriptTemporal(Transformacion* transformacion);

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket);
ReduccionLocal reduccionLocalRecibirTemporales(Puntero datos);
int reduccionLocalEjecutar(ReduccionLocal reduccion, String temporales);
void reduccionLocalTerminar(int resultado, Socket unSocket);
void reduccionLocalExito(Socket unSocket);
void reduccionLocalFracaso(Socket unSocket);
String reduccionLocalCrearScript(ReduccionLocal* reduccion);
String reduccionLocalObtenerTemporales(ReduccionLocal reduccion);

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionGlobalIniciar(Mensaje* mensaje, Socket unSocket);
void reduccionGlobalEjecutar();

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir();
void configuracionCalcularBloques();
Puntero dataBinMapear();
void dataBinConfigurar();
BloqueWorker bloqueBuscar(Entero numeroBloque);
BloqueWorker getBloque(Entero numeroBloque);














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

locOri* getOrigenesLocales(char*);
char* appendL(locOri*);
int reduccionGlobalEjecutar(char*,char*,char*);
lGlobOri* getOrigenesGlobales(char*);
char* appendG(lGlobOri*);
datosReg* PasaRegistro(char*,int);

