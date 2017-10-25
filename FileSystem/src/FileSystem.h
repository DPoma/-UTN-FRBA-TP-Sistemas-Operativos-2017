/*
 * FileSystem.h

 *
 *  Created on: 14/8/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

//--------------------------------------- Constantes -------------------------------------

#define RUTA_CONFIG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem/FileSystemConfig.conf"
#define RUTA_LOG "/home/utnso/Escritorio/tp-2017-2c-El-legado-del-Esqui/FileSystem/FileSystemLog.log"

#define ID_FORMAT 1
#define ID_RM 2
#define ID_RENAME 5
#define ID_MV 6
#define ID_CAT 7
#define ID_MKDIR 8
#define ID_CPFROM 9
#define ID_CPTO 10
#define ID_CPBLOCK 11
#define ID_MD5 12
#define ID_LS 13
#define ID_INFO 14
#define ID_EXIT 15

#define FORMAT "format"
#define RM "rm"
#define RMB "rm -b"
#define RMD "rm -d"
#define RENAME "rename"
#define MV "mv"
#define CAT "cat"
#define MKDIR "mkdir"
#define CPFROM "cpfrom"
#define CPTO "cpto"
#define CPBLOCK "cpblock"
#define MD5 "md5"
#define LS "ls"
#define INFO "info"
#define EXIT "exit"

#define FLAG_C "--clean"
#define FLAG_B "-b"
#define FLAG_D "-d"
#define FLAG_T "-t"

#define MAX_STRING 300
#define MAX_NOMBRE 255
#define MAX_DIR 100
#define MAX_COPIAS 2

#define ESCRIBIR 102

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	int identificador;
	String argumentos[5];
} Comando;

typedef struct {
	ListaSockets listaSelect;
	ListaSockets listaMaster;
	ListaSockets listaWorkers;
	ListaSockets listaDataNodes;
	Socket maximoSocket;
	Socket listenerYAMA;
	Socket listenerDataNode;
	Socket procesoYAMA;
} Servidor;


typedef struct {
	char puertoYAMA[20];
	char puertoDataNode[20];
	char rutaMetadata[MAX_NOMBRE];
} Configuracion;


typedef struct {
	int identificador;
	int identificadorPadre;
	char nombre[MAX_NOMBRE];
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
	char tipo[10];
	char nombre[MAX_NOMBRE];
	Lista listaBloques;
} Archivo;

typedef struct {
	int bytesUtilizados;
	int numeroBloque;
	Lista listaCopias;
} Bloque;

typedef struct {
	char nombreNodo[10];
	int  bloqueNodo;
} CopiaBloque;

typedef struct {
	char puerto[20];
	char ip[20];
	char nombre[10];
	Bitmap* bitmap;
	int bloquesLibres;
	int bloquesTotales;
	Socket socket;
} Nodo;

typedef struct __attribute__((packed)) {
	Entero numeroBloque;
	char bloque[BLOQUE];
} BloqueNodo;

typedef struct __attribute__((packed)) {
	char ip[20];
	char puerto[20];
} ConexionNodo;

//--------------------------------------- Variables globales -------------------------------------

String campos[3];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoFileSystem;
int directoriosDisponibles;
int estadoSeguro;
Hilo hiloConsola;
Lista listaDirectorios;
Lista listaArchivos;
Lista listaNodos;
Lista listaNodosDisponibles;
Bitmap* bitmapDirectorios;
Socket socketYama;
Socket socketDataNode;
Socket socketWorker;
String rutaDirectorioArchivos;
String rutaDirectorioBitmaps;
String rutaDirectorios;
String rutaArchivos;
String rutaNodos;
String rutaBuffer;

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar();
void fileSystemCrearConsola();
void fileSystemAtenderProcesos();
void fileSystemFinalizar();
bool fileSystemActivado();
bool fileSystemDesactivado();
void fileSystemActivar();
void fileSystemDesactivar();

//--------------------------------------- Funciones de Configuracion -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionIniciarRutas();
void configuracionDestruirRutas();
void configuracionIniciarLog();
void configuracionIniciar();

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

//--------------------------------------- Funciones de Socket-------------------------------------

bool socketEsListenerYAMA(Servidor* servidor, Socket unSocket);
bool socketEsListener(Servidor* servidor, Socket unSocket);
bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket);
bool socketEsDataNode(Servidor* servidor, Socket unSocket);
bool socketEsYAMA(Servidor* servidor, Socket unSocket);
bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket);

//--------------------------------------- Funciones de Consola -------------------------------------

bool consolaEntradaRespetaLimiteEspacios(String entrada);
bool consolaEntradaSinEspacioEnExtremos(String entrada);
bool consolaEntradaEspaciosSeparados(String entrada);
bool consolaEntradaEspaciosBienUtilizados(String entrada);
bool consolaEntradaSinTabs(String entrada);
bool consolaEntradaTieneEspaciosNecesarios(String entrada);
bool consolaEntradaLlena(String entrada);
bool consolaEntradaDecente(String entrada);
bool consolaComandoTipoUno(String comando);
bool consolaComandoTipoDos(String comando);
bool consolaComandoTipoTres(String comando);
bool consolaComandoExiste(String comando);
bool consolaValidarComandoSinTipo(String* subcadenas);
bool consolaValidarComandoTipoUno(String* subcadenas);
bool consolaValidarComandoTipoDos(String* subcadenas);
bool consolaValidarComandoTipoTres(String* subcadenas);
bool consolaComandoEliminarFlagB(String* subcadenas);
bool consolaComandoEliminarFlagD(String* subcadenas);
bool consolaValidarComandoFlagB(String* subcadenas);
bool consolaValidarComandoFlagD(String* subcadenas);
bool consolaComandoControlarArgumentos(String* subcadenas);
bool consolaValidarComando(String* buffer);
bool consolaflagInvalido(String flag);
void consolaIniciarArgumentos(String* argumentos);
void consolaLiberarArgumentos(String* argumentos);
void consolaObtenerArgumentos(String* buffer, String entrada);
void consolaSetearArgumentos(String* argumentos, String* buffer);
void consolaConfigurarComando(Comando* comando, String entrada);
void consolaEjecutarComando(Comando* comando);
void consolaDestruirComando(Comando* comando, String entrada);
void consolaAtenderComandos();
int consolaIdentificarComando(String comando);
String consolaLeerEntrada();

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem();
void comandoEliminar(Comando* comando);
void comandoEliminarBloque(Comando* comando);
void comandoEliminarDirectorio(Comando* comando);
void comandoEliminarArchivo(Comando* comando);
void comandoRenombrar(Comando* comando);
void comandoMover(Comando* comando);
void comandoMostrarArchivo(Comando* comando);
void comandoCrearDirectorio(Comando* comando);
void comandoCopiarArchivoDeFS(Comando* comando);
void comandoCopiarArchivoDeYFS(Comando* comando);
void comandoCopiarBloque(Comando* comando);
void comandoObtenerMD5(Comando* comando);
void comandoListarDirectorio(Comando* comando);
void comandoInformacionArchivo(Comando* comando);
void comandoFinalizar();
void comandoError();

//--------------------------------------- Funciones de Directorio -------------------------------------

Directorio* directorioCrear(int indice, String nombre, int padre);
void directorioControlSetearNombre(ControlDirectorio* control);
void directorioBuscarIdentificador(ControlDirectorio* control);
ControlDirectorio* directorioControlCrear(String rutaDirectorio);
bool directorioExisteIdentificador(int identificador);
void directorioPersistirEliminar(Directorio* directorio);
void directorioPersistirRenombrar(Directorio* directorio, String nuevoNombre);
void directorioPersistirMover(Directorio* directorio, Entero nuevoPadre);
void directorioIniciarEstructura();
Directorio* directorioBuscar(String path);
void directorioMostrarArchivos(int identificadorPadre);
bool directorioIndiceRespetaLimite(int indice);
bool directorioIndiceEstaOcupado(int indice);
bool directorioExisteIdentificador(int identificador);
bool directorioIndicesIguales(int unIndice, int otroIndice);
bool directorioSonIguales(Directorio* directorio, String nombreDirectorio, int idPadre);
Directorio* directorioCrear(int indice, String nombre, int padre);
int directorioBuscarIdentificadorLibre();
String directorioConfigurarEntradaArchivo(String indice, String nombre, String padre);
void directorioPersistir(Directorio* directorio);
void directorioBuscarIdentificador(ControlDirectorio* control);
int directorioIndicesACrear(String* nombresDirectorios, int indiceDirectorios);
bool directorioHaySuficientesIndices(ControlDirectorio* control);
void directorioControlSetearNombre(ControlDirectorio* control);
ControlDirectorio* directorioControlCrear(String rutaDirectorio);
void directorioControlarEntradas(ControlDirectorio* control, String path);
int directorioCrearDirectoriosRestantes(ControlDirectorio* control, String rutaDirectorio);
void directorioCrearEntradas(ControlDirectorio* control, String rutaDirectorio);
void directorioActualizar(ControlDirectorio* control, String rutaDirectorio);
bool directorioExiste(int idPadre, String nuevoNombre);
int directorioObtenerIdentificador(String path);
Directorio* directorioBuscarEnLista(int identificadorDirectorio);
bool directorioTieneAlgunArchivo(int identificador);
bool directorioTieneAlgunDirectorio(int identificador);
bool directorioTieneAlgo(int identificador);
void directorioEliminar(int identificador);
int directorioCrearConPersistencia(int identificador, String nombre, int identificadorPadre);
void directorioCrearMetadata(Entero identificador);
void directorioEliminarMetadata(Entero identificador);
bool directorioEsHijoDe(Directorio* hijo, Directorio* padre);

//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoBuscar(String path);
void archivoPersistirMover(Archivo* archivoAMover, int nuevoPadre);
void archivoDestruir(Archivo* archivo);
void archivoPersistirCrear(Archivo* metadata);
Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo);
void archivoDestruir(Archivo* archivo);
bool archivoExiste(int idPadre, String nombre);
void archivoIniciarEstructura();
void archivoPersistirRenombrar(Archivo* archivoARenombrar, String viejoNombre);
void archivoPersistirMover(Archivo* archivoAMover, int viejoPadre);
int archivoObtenerPosicion(Archivo* archivo);
void archivoPersistirEliminarBloque(Archivo* archivo, int numeroBloque, int numeroCopia);
void archivoPersistirControlCrear(Archivo* archivo);
void archivoPersistirControlEliminar(Archivo* archivo);
void archivoPersistirControlRenombrar(Archivo* archivo, String nuevoNombre);
void archivoPersistirControlMover(Archivo* archivo, Entero nuevoPadre);

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(int bloquesTotales, int bloquesLibres, Socket unSocket);
void nodoPersistirConectados();
void nodoLimpiarLista();
void nodoDestruir(Nodo* nodo);
void nodoRecuperarEstadoAnterior();
void nodoPersistirBitmap(Nodo* nodo);
void nodoFormatear(Nodo* nodo);
void nodoFormatearConectados();
void nodoVerificarBloquesLibres(Nodo* nodo, Lista nodosDisponibles);
bool nodoTieneBloquesLibres(Nodo* nodo);
bool nodoCantidadBloquesLibres(Nodo* unNodo, Nodo* otroNodo);
int nodoBuscarBloqueLibre(Nodo* nodo);
Nodo* nodoBuscar(String nombre);

//--------------------------------------- Funciones de Bloque -------------------------------------

Bloque* bloqueCrear(int bytes, int numero);
void bloqueDestruir(Bloque* bloque);
void bloqueCopiar(Bloque* bloque, Nodo* nodo, Entero numeroBloqueNodo);
void bloqueEnviarANodo(Socket unSocket, Entero numeroBloque, String buffer);

//--------------------------------------- Funciones de Copia Bloque -------------------------------------

CopiaBloque* copiaBloqueCrear(int numeroBloqueDelNodo, String nombreNodo);
void copiaBloqueEliminar(CopiaBloque* copia);

//--------------------------------------- Funciones de Metadata -------------------------------------

void metadataCrear();
void metadataEliminar();
void metadataIniciar();
void metadataRecuperar();

//--------------------------------------- Funciones de Ruta------------------------------------

String rutaObtenerUltimoNombre(String ruta);
bool rutaBarrasEstanSeparadas(String ruta);
String* rutaSeparar(String ruta);
bool rutaTieneAlMenosUnaBarra(String ruta);
bool rutaValida(String ruta);
bool rutaEsNumero(String ruta);

//--------------------------------------- Funciones Varias -------------------------------------
BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer);
void testCabecita();
void archivoBufferCopiarEn(String rutaArchivo);
