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
#define TRANSFORMACION 303
#define REDUCCION_LOCAL 304
#define REDUCCION_GLOBAL 305
#define ALMACENADO_FINAL 306
#define ENVIAR_TEMPORAL 307
#define PEDIR_LINEA 308
#define CONEXION_WORKER 309

#define INTSIZE sizeof(Entero)
#define TEMPSIZE 12
#define DIRSIZE 40

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/NodoConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/Worker/WorkerLog.log"
#define RUTA_TEMP "/home/utnso/Escritorio/temp/"

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	char ipFileSystem[20];
	char puertoFileSystemDataNode[20];
	char puertoFileSystemWorker[20];
	char puertoMaster[20];
	char nombreNodo[10];
	char rutaDataBin[255];
	char ipPropia[20];
} Configuracion;

typedef struct {
	int scriptSize;
	String script;
	int numeroBloque;
	int bytesUtilizados;
	char nombreResultado[12];
} Transformacion;

typedef struct {
	int scriptSize;
	String script;
	int cantidadTemporales;
	String nombresTemporales;
	char nombreResultado[12];
} ReduccionLocal;

typedef struct {
	Dir nodo;
	char temporal[TEMPSIZE];
} ReduccionGlobalNodo;

typedef struct {
	String linea;
	Socket socketWorker;
} Apareo;

typedef struct {
	ReduccionGlobalNodo* nodos;
	int scriptSize;
	String script;
	int cantidadNodos;
	String rutaArchivoApareo;
	char nombreResultado[12];
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
void workerAceptarMaster();
void masterRealizarOperacion(Socket unSocket);
void workerAtenderProcesos();
void workerAtenderWorkers();
void workerAceptarWorker();
void reduccionGlobalConOtroWorker(Mensaje* mensaje, Socket socketWorker);

//--------------------------------------- Funciones de Configuracion  -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
void configuracionIniciarLog();
void configuracionIniciarCampos();
void configuracionCalcularBloques();
void configuracionSenial(int senial);
void configuracionIniciar();

//--------------------------------------- Funciones de Master -------------------------------------

void workerAceptarMaster();
void masterAtenderOperacion(Socket unSocket);
void masterRealizarOperacion(Socket unSocket);

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
String reduccionLocalCrearScript(ReduccionLocal reduccion);
String reduccionLocalObtenerTemporales(ReduccionLocal reduccion);

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobal(Mensaje* mensaje, Socket unSocket);
int reduccionGlobalEjecutar(ReduccionGlobal reduccion, String path);
void reduccionGlobalTerminar(int resultado, Socket unSocket);
void reduccionGlobalExito(Socket unSocket);
void reduccionGlobalFracaso(Socket unSocket);
String reduccionGlobalObtenerLinea(Socket unSocket);
String reduccionGlobalGenerarArchivo(ReduccionGlobal reduccion);

//--------------------------------------- Funciones de Almacenado Final -------------------------------------

void almacenadoFracaso(Socket unSocket);
void almacenadoExitoso(Socket unSocket);
void almacenadoTerminar(int resultado, Socket unSocket);
void almacenadoFinal(Mensaje* mensaje, Socket socketMaster);

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinAbrir();
void configuracionCalcularBloques();
Puntero dataBinMapear();
void dataBinConfigurar();
BloqueWorker bloqueBuscar(Entero numeroBloque);
BloqueWorker getBloque(Entero numeroBloque);
