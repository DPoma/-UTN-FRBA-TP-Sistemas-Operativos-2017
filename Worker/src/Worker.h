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
#define ALMACENAR_BLOQUE 307
#define ALMACENAR_PATH 308
#define ENVIAR_TEMPORAL 309
#define ENVIAR_LINEA 310
#define PEDIR_LINEA 311
#define CONEXION_WORKER 312

#define INTSIZE sizeof(Entero)
#define TEMPSIZE 12
#define DIRSIZE 40

#define RUTA_CONFIG "Nodo.config"

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	char ipPropia[20];
	char ipFileSystem[20];
	char puertoFileSystemDataNode[20];
	char puertoFileSystemWorker[20];
	char puertoMaster[20];
	char nombreNodo[10];
	char tamanioDataBin[10];
	char rutaDataBin[150];
	char rutaLogDataNode[150];
	char rutaLogWorker[150];
	char rutaTemporales[150];
} Configuracion;

typedef struct {
	int scriptSize;
	String script;
	Entero numeroBloque;
	Entero bytesUtilizados;
	char nombreResultado[TEMPSIZE];
	int idMaster;
} Transformacion;

typedef struct {
	int scriptSize;
	String script;
	int cantidadTemporales;
	String nombresTemporales;
	char nombreResultado[TEMPSIZE];
	int idMaster;
} ReduccionLocal;

typedef struct {
	Dir nodo;
	char temporal[TEMPSIZE];
} Nodo;

typedef struct {
	String linea;
	Socket socketWorker;
	char nombre[10];
} Apareo;

typedef struct {
	Nodo* nodos;
	int scriptSize;
	String script;
	int cantidadNodos;
	String pathApareo;
	char nombreResultado[TEMPSIZE];
	int idMaster;
} ReduccionGlobal;

typedef struct __attribute__((packed)) {
	Entero bytesUtilizados;
	char datos[BLOQUE];
} BloqueWorker;

typedef void* Bloque;

//--------------------------------------- Globales -------------------------------------

Configuracion* configuracion;
ArchivoLog archivoLog;
String campos[11];
Socket listenerWorker;
Socket listenerMaster;
Puntero punteroDataBin;
pid_t pidPadre;
int estadoWorker;
int dataBinBloques;
int dataBinTamanio;
Socket socketBuffer;
String salidaTemporal;

//--------------------------------------- Funciones de Worker -------------------------------------

void workerIniciar();
void workerAtenderMasters();
void masterDesconectar();
void workerAceptarMaster();
void masterRealizarOperacion(Socket unSocket);
void workerAtenderProcesos();
void workerAtenderWorkers();
void workerAceptarWorker();
void workerFinalizar();
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

int transformacionEjecutar(Transformacion* transformacion, String pathScript);
void transformacion(Mensaje* mensaje, Socket unSocket);
void transformacionDestruir(Transformacion* transformacion);
void transformacionExito(Transformacion* transformacion, Socket unSocket);
void transformacionFracaso(Transformacion* transformacion, Socket unSocket);
void transformacionObtenerScript(Transformacion* transformacion, Mensaje* mensaje);
void transformacionFinalizar(Transformacion* transformacion, Socket unSocket, int* estado);
void transformacionProcesarBloque(Transformacion* transformacion, Mensaje* mensaje, Mensaje* otroMensaje, Socket unSocket, String pathScript);
String transformacionCrearBloque(Transformacion* transformacion);
String transformacionCrearScript(Transformacion* transformacion);
void transformacionFinalizarBloque(int resultado, Socket unSocket, Transformacion* transformacion);

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket);
ReduccionLocal* reduccionLocalRecibirDatos(Puntero datos);
int reduccionLocalEjecutar(ReduccionLocal* reduccion, String temporales);
String reduccionLocalCrearScript(ReduccionLocal* reduccion);
String reduccionLocalObtenerTemporales(ReduccionLocal* reduccion);
void reduccionLocalDestruir(ReduccionLocal* reduccion);
void reduccionLocalFinalizar(int resultado, Socket unSocket, int idMaster);
void reduccionLocalFracaso(Socket unSocket, int idMaster);
void reduccionLocalExito(Socket unSocket, int idMaster);

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobal(Mensaje* mensaje, Socket unSocket);
ReduccionGlobal* reduccionGlobalRecibirDatos(Puntero datos);
int reduccionGlobalEjecutar(ReduccionGlobal* reduccion);
int reduccionGlobalAparearTemporales(ReduccionGlobal* reduccion);
int reduccionGlobalAlgoritmoApareo(ReduccionGlobal* reduccion, Lista listaApareados);
String reduccionGlobalCrearScript(ReduccionGlobal* reduccion);
int reduccionGlobalRealizarConexiones(ReduccionGlobal* reduccion, Lista listaApareados);
void reduccionGlobalDestruir(ReduccionGlobal* reduccion);
void reduccionGlobalFinalizar(int resultado, Socket unSocket, int idMaster);
void reduccionGlobalFracaso(Socket unSocket, int idMaster);
void reduccionGlobalExito(Socket unSocket, int idMaster);
String reduccionGlobalEncargadoPedirLinea(Apareo* apareo);
Apareo* reduccionGlobalLineaMasCorta(Apareo* unApareo, Apareo* otroApareo);
void reduccionGlobalDestruirLineaNula(Lista listaApareados);
void reduccionGlobalEnviarLinea(Mensaje* mensaje, Socket socketWorker);
int reduccionGlobalEscribirLinea(Apareo* apareo, Lista listaApareados, File archivoResultado);

//--------------------------------------- Funciones de Almacenado Final -------------------------------------

void almacenadoFinal(Mensaje* mensaje, Socket socketMaster);
int almacenadoFinalEjecutar(String pathArchivo, String pathYama);
int almacenadoFinalEnviarArchivo(String pathArchivo, String pathYama, Socket socketFileSystem);
void almacenadoFinalFinalizar(int resultado, Socket unSocket, int idMaster);
void almacenadoFinalExito(Socket unSocket, int idMaster);
void almacenadoFinalFracaso(Socket unSocket, int idMaster);
Socket almacenadoFinalConectarAFileSystem();

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinCrear();
void configuracionCalcularBloques();
Puntero dataBinMapear();
void dataBinConfigurar();
Bloque bloqueBuscar(Entero numeroBloque);
Bloque getBloque(Entero numeroBloque);
void zombieDetector();
void zombieLimpiar();
