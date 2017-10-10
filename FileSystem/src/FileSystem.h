/*
 * FileSystem.h

 *
 *  Created on: 14/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem/FileSystemConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/FileSystemLog.log"
#define RUTA_DIRECTORIO "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem/Directorios.dat"

//Identificador de cada comando
#define FORMAT 1
#define RM 2
#define RMB 3
#define RMD 4
#define RENAME 5
#define MV 6
#define CAT 7
#define MKDIR 8
#define CPFROM 9
#define CPTO 10
#define CPBLOCK 11
#define MD5 12
#define LS 13
#define INFO 14
#define EXIT 15

#define C_FORMAT "format"
#define C_RM "rm"
#define C_RMB "rm -b"
#define C_RMD "rm -d"
#define C_RENAME "rename"
#define C_MV "mv"
#define C_CAT "cat"
#define C_MKDIR "mkdir"
#define C_CPFROM "cpfrom"
#define C_CPTO "cpto"
#define C_CPBLOCK "cpblock"
#define C_MD5 "md5"
#define C_LS "ls"
#define C_INFO "info"
#define C_EXIT "exit"

#define FLAG_B "-b"
#define FLAG_D "-d"
#define MAX_STRING 255
#define LIMITE_DIRECTORIOS 100

typedef struct {
	int identificador;
	String argumentos[5];
} Comando;

typedef struct {
	ListaSockets listaSelect;
	ListaSockets listaMaster;
	ListaSockets listaDataNodes;
	Socket maximoSocket;
	Socket listenerYAMA;
	Socket listenerDataNode;
	Socket procesoYAMA;
} Servidor;


typedef struct {
	char puertoYAMA[50];
	char puertoDataNode[50];
	char rutaMetadata[100];
} Configuracion;


typedef struct {
	int identificador;
	int identificadorPadre;
	char nombre[MAX_STRING];
} Directorio;

typedef struct {
	int identificadorPadre;
	int identificadorDirectorio;
	int indiceNombresDirectorios;
	String nombreDirectorio;
	String* nombresDirectorios;
} ControlDirectorio;

typedef struct {
	int identificadorPadre;
	char tipo[8];
	char nombre[MAX_STRING];
	Lista listaBloques;
} ArchivoMetadata;

typedef struct {
	int bytes;
	Lista listaCopias;
} Bloque;

typedef struct {
	char nombreNodo[6];
	int  bloqueNodo;
} CopiaBloque;

typedef struct estructura_manejo_nodos {
	Socket socket;
	char nombre[6];
	int estado;
	char ip[20];
	char puerto[20];
	char puertoListener[20];
	String bloquesBitArray;
	BitArray* bitmap;
	int bloquesLibres;
	int bloquesTotales;
} Nodo;


String campos[3];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoFileSystem;
Hilo hiloConsola;
Lista listaDirectorios;
Lista listaArchivos;
Lista listaNodos;
char bitmapDirectorios[LIMITE_DIRECTORIOS];
int directoriosDisponibles;

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar();
void fileSystemCrearConsola();
void fileSystemAtenderProcesos();
void fileSystemFinalizar();

//--------------------------------------- Funciones de Socket-------------------------------------

bool socketEsListenerYAMA(Servidor* servidor, Socket unSocket);
bool socketEsListener(Servidor* servidor, Socket unSocket);
bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket);
bool socketEsDataNode(Servidor* servidor, Socket unSocket);
bool socketEsYAMA(Servidor* servidor, Socket unSocket);
bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket);

//--------------------------------------- Funciones de Servidor -------------------------------------

bool servidorObtenerMaximoSocket(Servidor* servidor);
void servidorSetearListaSelect(Servidor* servidor);
void servidorControlarMaximoSocket(Servidor* servidor, Socket unSocket);
void servidorEsperarSolicitud(Servidor* servidor);
void servidorFinalizarConexion(Servidor* servidor, Socket unSocket);
void servidorEstablecerConexion(Servidor* servidor, Socket unSocket);
Socket servidorAceptarDataNode(Servidor* servidor, Socket unSocket);
Socket servidorAceptarYAMA(Servidor* servidor, Socket unSocket);
void servidorAceptarConexion(Servidor* servidor, Socket unSocket);
void servidorRecibirMensaje(Servidor* servidor, Socket unSocket);
void servidorControlarSocket(Servidor* servidor, Socket unSocket);
void servidorActivarListenerYAMA(Servidor* servidor);
void servidorActivarListenerDataNode(Servidor* servidor);
void servidorActivarListeners(Servidor* servidor);
void servidorInicializar(Servidor* servidor);
void servidorFinalizar(Servidor* servidor);
void servidorAtenderSolicitud(Servidor* servidor);
void servidorAtenderPedidos(Servidor* servidor);

//--------------------------------------- Funciones de Consola -------------------------------------

void consolaAtenderComandos();
int consolaIdentificarComando(char* comando);
char* consolaLeerCaracteresEntrantes();
Comando consolaObtenerComando();

//--------------------------------------- Funciones de Configuracion -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);


void archivoConfigObtenerCampos();
void funcionSenial(int senial);
bool fileSystemActivado();
bool fileSystemDesactivado();
void fileSystemActivar();
void fileSystemDesactivar();

Directorio* directorioCrear(int indice, String nombre, int padre);
void directorioGuardarEnArchivo(Directorio unDirectorio);
Directorio directorioLeerDeArchivo(Archivo unArchivo);
void directorioPosicionarEnRegistro(Archivo archivo, int posicion);
long directorioCantidadRegistros(Archivo archivo);
long directorioObtenerPosicionActualArchivo(Archivo archivo);
void consolaDirectorioCrear(String path);
