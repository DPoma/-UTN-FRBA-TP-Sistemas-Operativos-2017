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
#define OCUPADO '1'
#define ARCHIVO_BINARIO "BINARIO"
#define ARCHIVO_TEXTO "TEXTO"
#define MAX_STRING 300
#define MAX_NOMBRE 255
#define MAX_DIR 100
#define MAX_COPIAS 2
#define NUEVO 0
#define NORMAL 1
#define INESTABLE 0
#define ESTABLE 1

#define ACEPTAR_NODO 100
#define LEER_BLOQUE 101
#define ESCRIBIR_BLOQUE 102
#define COPIAR_BLOQUE 103
#define COPIAR_BINARIO 104
#define COPIAR_TEXTO 105
#define FINALIZAR_NODO 106

#define ACEPTAR_YAMA 200


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
	Socket contadorSocket;
	Socket listenerYama;
	Socket listenerDataNode;
	Socket listenerWorker;
	Socket yama;
} Servidor;


typedef struct {
	char puertoYama[20];
	char puertoDataNode[20];
	char puertoWorker[20];
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
} Copia;

typedef struct {
	char puerto[20];
	char ip[20];
	char nombre[10];
	int estado;
	int bloquesLibres;
	int bloquesTotales;
	Bitmap* bitmap;
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

String campos[4];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoControl;
int estadoEjecucion;
int estadoFileSystem;
int directoriosDisponibles;
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
Nodo* nodoBuffer;
Bloque* bloqueBuffer;
Archivo* archivoBuffer;

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar();
void fileSystemCrearConsola();
void fileSystemAtenderProcesos();
void fileSystemFinalizar();
bool fileSystemEstable();

//--------------------------------------- Funciones de Configuracion -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionIniciarRutas();
void configuracionDestruirRutas();
void configuracionIniciarLog();
void configuracionIniciar();

//--------------------------------------- Funciones de Servidor -------------------------------------

void servidorInicializar(Servidor* servidor);
void servidorFinalizar(Servidor* servidor);
bool servidorCantidadSockets(Servidor* servidor);
void servidorIniciarListaSelect(Servidor* servidor);
void servidorControlarContadorSocket(Servidor* servidor, Socket unSocket);
void servidorEsperarSolicitud(Servidor* servidor);
void servidorFinalizarConexion(Servidor* servidor, Socket unSocket);
void servidorRecibirMensaje(Servidor* servidor, Socket unSocket);
void servidorIniciarContadorSocket(Servidor* servidor);
void servidorActivarListenerYama(Servidor* servidor);
void servidorActivarListenerDataNode(Servidor* servidor);
void servidorActivarListenerWorker(Servidor* servidor);
void servidorActivarListeners(Servidor* servidor);
void servidorAtenderSolicitudes(Servidor* servidor);
void servidorAceptarConexion(Servidor* servidor, Socket unSocket);
void servidorFinalizarProceso(Servidor* servidor, Socket unSocket);
void servidorLimpiarListas(Servidor* servidor);

void servidorRegistrarDataNode(Servidor* servidor, Socket nuevoSocket);
void servidorFinalizarDataNode(Servidor* servidor, Socket unSocket);
void servidorMensajeDataNode(Mensaje* mensaje, Socket unSocket);
void servidorReconectarDataNode(Servidor* servidor, Nodo* nodoTemporal);
bool servidorNodoEsNuevo(Nodo* nuevoNodo);
void servidorRevisarDataNode(Servidor* servidor, Nodo* nodoTemporal);
void servidorRechazarDataNode(Nodo* nuevoNodo);
void servidorAvisarDataNode(Nodo* nodo);
void servidorAceptarDataNode(Servidor* servidor, Nodo* nuevoNodo);
void servidorAceptarNuevoDataNode(Servidor* servidor, Nodo* nuevoNodo);
void servidorAceptarReconexionDataNode(Servidor* servidor, Nodo* nuevoNodo);
void servidorHabilitarNodo(Servidor* servidor, Nodo* nodoTemporal);

void servidorRegistrarYama(Servidor* servidor, Socket unSocket);
void servidorFinalizarYama();

void servidorRegistrarWorker(Servidor* servidor, Socket unSocket);
void servidorFinalizarWorker(Servidor* servidor, Socket unSocket);




//--------------------------------------- Funciones de Socket-------------------------------------

bool socketEsListenerYama(Servidor* servidor, Socket unSocket);
bool socketEsListener(Servidor* servidor, Socket unSocket);
bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket);
bool socketEsDataNode(Servidor* servidor, Socket unSocket);
bool socketEsYama(Servidor* servidor, Socket unSocket);
bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket);
bool socketEsWorker(Servidor* servidor, Socket unSocket);
bool socketEsListenerWorker(Servidor* servidor, Socket unSocket);

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
void comandoCopiarArchivoAYamaFS(Comando* comando);
int comandoCopiarArchivoDeYamaFS(Comando* comando);
void comandoCopiarBloque(Comando* comando);
void comandoObtenerMD5DeArchivo(Comando* comando);
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
void directorioPersistir();
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
bool directorioOrdenarPorIdentificador(Directorio* unDirectorio, Directorio* otroDirectorio);
void directorioRecuperarPersistencia();

//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoBuscar(String path);
void archivoDestruir(Archivo* archivo);
void archivoPersistir(Archivo* metadata);
Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo);
void archivoDestruir(Archivo* archivo);
bool archivoExiste(int idPadre, String nombre);
int archivoObtenerPosicion(Archivo* archivo);
void archivoPersistirEliminarBloque(Archivo* archivo, int numeroBloque, int numeroCopia);
void archivoPersistirControl();
bool archivoOrdenarPorNombre(Archivo* unArchivo, Archivo* otroArchivo);
void archivoRecuperarPersistencia();
void archivoRecuperarPersistenciaEspecifica(String nombre, int padre);
bool archivoEsBinario(Archivo* archivo);
int archivoLeer(Comando* comando);
int archivoAlmacenar(Comando* comando);
int archivoCantidadBloques(String ruta);
bool archivoDisponible(Archivo* archivo);

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(Puntero datos, Socket nuevoSocket);
void nodoPersistir();
void nodoLimpiarLista();
void nodoDestruir(Nodo* nodo);
void nodoRecuperarPersistencia();
void nodoPersistirBitmap(Nodo* nodo);
void nodoFormatear(Nodo* nodo);
void nodoFormatearConectados();
void nodoVerificarBloquesLibres(Nodo* nodo, Lista nodosDisponibles);
bool nodoTieneBloquesLibres(Nodo* nodo);
bool nodoCantidadBloquesLibres(Nodo* unNodo, Nodo* otroNodo);
int nodoBuscarBloqueLibre(Nodo* nodo);
Nodo* nodoBuscar(String nombre);
void nodoRecuperarPersistenciaBitmap(Nodo* nodo);
void nodoDesactivar(Nodo* nodo);
Nodo* nodoBuscarPorSocket(Socket unSocket);
int nodoPosicionEnLista(Nodo* nodo);
int nodoBloquesLibres();
void nodoDesactivar(Nodo* nodo);

//--------------------------------------- Funciones de Bloque -------------------------------------

Bloque* bloqueCrear(int bytes, int numero);
void bloqueDestruir(Bloque* bloque);
void bloqueCopiarEnNodo(Bloque* bloque, Nodo* nodo, Entero numeroBloqueNodo);
void bloqueEnviarANodo(Socket unSocket, Entero numeroBloque, String buffer);
int bloqueEnviarCopiasANodos(Bloque* bloque, String buffer);
bool bloqueOrdenarPorNumero(Bloque* unBloque, Bloque* otroBloque);
void bloqueCopiar(Puntero datos);
void bloqueLeer(Puntero datos);
void bloqueCopiarTexto(Puntero datos);
void bloqueCopiarBinario(Puntero datos);
bool bloqueDisponible(Bloque* bloque);
BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer, int tamanioUtilizado);

//--------------------------------------- Funciones de Copia Bloque -------------------------------------

Copia* copiaBloqueCrear(int numeroBloqueDelNodo, String nombreNodo);
void copiaBloqueEliminar(Copia* copia);
bool copiaDisponible(Copia* copia);

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
