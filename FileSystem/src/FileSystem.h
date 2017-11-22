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
#define ID_RENAME 3
#define ID_MV 4
#define ID_CAT 5
#define ID_MKDIR 6
#define ID_CPFROM 7
#define ID_CPTO 8
#define ID_CPBLOCK 9
#define ID_MD5 10
#define ID_LS 11
#define ID_INFO 12
#define ID_NODES 13
#define ID_BITMAPS 14
#define ID_HELP 15
#define ID_EXIT 16
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
#define NODES "nodes"
#define BITMAPS "bitmaps"
#define HELP "help"
#define EXIT "exit"
#define FLAG_C "--clean"
#define FLAG_B "-b"
#define FLAG_D "-d"
#define FLAG_T "-t"
#define OCUPADO '1'
#define FIN '\0'
#define ENTER '\n'
#define ESPACIO ' '
#define TAB '\t'
#define BARRA '/'
#define RAIZ "/"
#define ID_RAIZ 0
#define ARCHIVO_BINARIO "Binario"
#define ARCHIVO_TEXTO "Texto"
#define PREFIJO "yamafs:"
#define MAX_STRING 300
#define MAX_NOMBRE 255
#define MAX_DIR 100
#define MAX_IP 20
#define MAX_PUERTO 20
#define MAX_NODO 10
#define MAX_COPIAS 2
#define MAX_TIPOS 10
#define MAX_ARGS 5
#define MAX_CAMPOS 4
#define MAX_PREFIJO 7
#define NUEVO 0
#define NORMAL 1
#define INESTABLE 0
#define ESTABLE 1
#define EXITO 1
#define FRACASO -800

#define ACEPTAR_DATANODE 100
#define LEER_BLOQUE 101
#define ESCRIBIR_BLOQUE 102
#define COPIAR_BLOQUE 103
#define COPIAR_ARCHIVO 104
#define FINALIZAR_NODO 105

#define ACEPTAR_YAMA 200
#define ENVIAR_BLOQUES 201
#define ERROR_ARCHIVO 202

#define ALMACENADO_FINAL 306
#define ALMACENAR_BLOQUE 307
#define ALMACENAR_PATH 308

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	int identificador;
	String argumentos[MAX_ARGS];
} Comando;

typedef struct {
	char puertoYama[MAX_PUERTO];
	char puertoDataNode[MAX_PUERTO];
	char puertoWorker[MAX_PUERTO];
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
	char tipo[MAX_TIPOS];
	char nombre[MAX_NOMBRE];
	Lista listaBloques;
} Archivo;

typedef struct {
	int bytesUtilizados;
	int numeroBloque;
	Lista listaCopias;
} Bloque;

typedef struct {
	char nombreNodo[MAX_NODO];
	int  bloqueNodo;
} Copia;

typedef struct {
	char puerto[MAX_PUERTO];
	char ip[MAX_IP];
	char nombre[MAX_NODO];
	int estado;
	int mensajeEnviado;
	int tareasRealizadas;
	int bloquesLibres;
	int bloquesTotales;
	Bitmap bitmap;
	Socket socket;
} Nodo;

typedef struct __attribute__((packed)) {
	Entero numeroBloque;
	char bloque[BLOQUE];
} BloqueNodo;

typedef struct __attribute__((packed)) {
	char ip[MAX_IP];
	char puerto[MAX_PUERTO];
} Direccion;

typedef struct __attribute__((packed)) {
	Dir direccionCopia1;
	Entero numeroBloqueCopia1;
	Dir direccionCopia2;
	Entero numeroBloqueCopia2;
	Entero bytesUtilizados;
} BloqueYama;


typedef struct {
	Entero pathLocalSize;
	String pathLocal;
	Entero pathYamaSize;
	String pathYama;
} AlmacenadoFinal;

//--------------------------------------- Globales -------------------------------------

String campos[MAX_CAMPOS];
Configuracion* configuracion;
ArchivoLog archivoLog;
int estadoControl;
int estadoEjecucion;
int estadoFileSystem;
int directoriosDisponibles;
Hilo hiloConsola;
Hilo hiloDataNode;
Hilo hiloWorker;
Hilo hiloYama;
Lista listaDirectorios;
Lista listaArchivos;
Lista listaNodos;
Bitmap bitmapDirectorios;
Socket listenerYama;
Socket listenerDataNode;
Socket listenerWorker;
Socket socketYama;
String rutaDirectorioArchivos;
String rutaDirectorioBitmaps;
String rutaDirectorios;
String rutaArchivos;
String rutaNodos;
String rutaBuffer;
Nodo* nodoBuffer;
Bloque* bloqueBuffer;
Archivo* archivoBuffer;
Mutex* mutexTarea;
Mutex* mutexRuta;
Mutex* mutexNodo;
Mutex* mutexBloque;
Mutex* mutexArchivo;
Mutex* mutexListaNodos;
Mutex* mutexListaArchivos;
Mutex* mutexListaDirectorios;
Mutex* mutexBitmapDirectorios;
Mutex* mutexEstadoFileSystem;
Mutex* mutexEstadoEjecucion;
Mutex* mutexEstadoControl;
Mutex* mutexMensaje;
Mutex* mutexSocket;
Mutex* mutexEstado;
Semaforo* semaforoTarea;

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar();
void fileSystemCrearConsola();
void fileSystemAtenderProcesos();
void fileSystemFinalizar();
void fileSystemReiniciar();
void fileSystemRecuperar();

//--------------------------------------- Funciones de Configuracion -------------------------------------

void configuracionImprimir(Configuracion* configuracion);
Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig);
void configuracionIniciarRutas();
void configuracionRutasDestruir();
void configuracionIniciarLog();
void configuracionIniciar();

//--------------------------------------- Funciones de Servidor -------------------------------------
void dataNodeListener();
void dataNodeCrearConexion();
void dataNodeControlarConexion(Socket nuevoSocket);
void dataNodeReconectar(Nodo* nodoTemporal);
void dataNodeControlar(Nodo* nodoTemporal);
void dataNodeReconectar(Nodo* nodoTemporal);
void dataNodeAdmitir(Nodo* nodoTemporal);
void dataNodeAceptar(Nodo* nodoTemporal);
void dataNodeRechazar(Nodo* nuevoNodo);
void dataNodeAceptarReconexion(Nodo* nuevoNodo);
void dataNodeAtender(Nodo* nodo, int* estado);
void dataNodeFinalizar(Nodo* nodo, int* estado);
void dataNodeDesactivar(Nodo* nodo, int* estado);
void dataNodeDestruir(Nodo* nodo, int* estado);
void dataNodeControlarFinalizacion(Nodo* nodo, int* estado);
void dataNodeHilo(Nodo* nodo);

void yamaListener();
void yamaAceptar();
void yamaRechazar();
void yamaAtender();
void yamaDesconectar();
void yamaFinalizar();
void yamaControlar();
BloqueYama* yamaConvertirArchivo(Archivo* archivo, Entero idMaster);
void yamaEnviarBloques(Puntero path);

//--------------------------------------- Funciones de Worker -------------------------------------

void workerListener();
int workerAlmacenarArchivo(Archivo* archivo, Socket socketWorker);
int workerAlmacenarBloque(Archivo* archivo, Mensaje* mensaje, Entero* numeroBloque, int* resultado);
void workerAvisarAlmacenado(int resultado, Socket unSocket);
Comando workerConfigurarComando(AlmacenadoFinal* almacenado);
AlmacenadoFinal* workerRecibirDatos(Mensaje* mensaje);
void workerDestruirDatos(AlmacenadoFinal* almacenado);
void workerAtender(Socket* socketWorker);

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
bool consolaArgumentoEsNumero(String ruta);

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem();
void comandoEliminar(Comando* comando);
void comandoEliminarCopia(Comando* comando);
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
void comandoInformacionNodos();
void comandoInformacionBitmaps();
void comandoAyuda();
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
void directorioListaAgregar(Directorio* directorio);
int directorioListaCantidad();
Directorio* directorioListaObtener(int posicion);
void directorioListaCrear();
void directorioListaDestruir();
void directorioIniciarControl();

//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoBuscarPorRuta(String path);
void archivoDestruir(Archivo* archivo);
void archivoPersistir(Archivo* metadata);
Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo);
void archivoDestruir(Archivo* archivo);
bool archivoExiste(int idPadre, String nombre);
int archivoListaPosicion(Archivo* archivo);
void archivoPersistirEliminarBloque(Archivo* archivo, int numeroBloque, int numeroCopia);
void archivoPersistirControl();
bool archivoOrdenarPorNombre(Archivo* unArchivo, Archivo* otroArchivo);
void archivoRecuperarPersistencia();
void archivoRecuperarPersistenciaDetallada(String nombre, int padre);
bool archivoBinario(Archivo* archivo);
int archivoLeer(Comando* comando);
int archivoAlmacenar(Comando* comando);
int archivoCantidadBloques(String ruta);
bool archivoDisponible(Archivo* archivo);
Archivo* archivoListaObtener(int indice);
void archivoListaCrear();
void archivoListaAgregar(Archivo* archivo);
int archivoListaCantidad();
bool archivoListaTodosDisponibles();
void archivoListaDestruir();
void archivoGuardar(Archivo* archivo);
void archivoControlar(Archivo* archivo, int estado);
int almacenadoFinalEnviarArchivo(Archivo* archivo, File file);
int archivoAlmacenarBinario(Archivo* archivo, File file);

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(Puntero datos, Socket nuevoSocket);
void nodoPersistir();
void nodoLimpiarLista();
void nodoDestruir(Nodo* nodo);
void nodoRecuperarPersistencia();
void nodoPersistirBitmap(Nodo* nodo);
void nodoFormatear(Nodo* nodo);
void nodoFormatearConectados();
void nodoVerificarBloquesLibres(Nodo* nodo);
bool nodoTieneBloquesLibres(Nodo* nodo);
bool nodoOrdenarPorUtilizacion(Nodo* unNodo, Nodo* otroNodo);
int nodoBuscarBloqueLibre(Nodo* nodo);
Nodo* nodoBuscarPorNombre(String nombre);
void nodoRecuperarPersistenciaBitmap(Nodo* nodo);
Nodo* nodoBuscarPorSocket(Socket unSocket);
int nodoListaPosicion(Nodo* nodo);
int nodoBloquesLibresTotales();
bool nodoOrdenarPorActividad(Nodo* unNodo, Nodo* otroNodo);
void nodoLimpiarActividades();
bool nodoConectado(Nodo* nodo);
Nodo* nodoListaObtener(int posicion);
int nodoListaCantidad();
void nodoListaAgregar(Nodo* nodo);
Nodo* nodoActualizar(Nodo* nodoTemporal);
bool nodoInvalido(Nodo* nodoTemporal);
bool nodoEstaConectado(Nodo* nuevoNodo);
void nodoListaEliminar(Nodo* nodo);
void nodoListaDestruir();
void nodoListaOrdenarPorActividad();
bool nodoListaAlgunoDisponible();
void nodoAceptar(Nodo* nodo);
void nodoListaCrear();
bool nodoDisponible(Nodo* nodo);
void nodoDesconectar(Nodo* nodo);
void nodoDesconectarATodos();
bool nodoDesconectado(Nodo* nodo);
void nodoActivarDesconexion(Nodo* nodo, int* estado);
void nodoSocket(Nodo* nodo, int estado);
void nodoMensaje(Nodo* nodo, int estado);
void nodoEstado(Nodo* nodo, int estado);
bool nodoMensajeIgualA(Nodo* nodo, int estado);
Direccion nodoObtenerDireccion(String nombreNodo);

//--------------------------------------- Funciones de Bloque -------------------------------------

Bloque* bloqueCrear(int bytes, int numero);
void bloqueDestruir(Bloque* bloque);
int copiaGuardarEnNodo(Bloque* bloque, Nodo* nodo);
int copiaEnviarANodo(Bloque* bloque, Nodo* nodo, String buffer);
int bloqueEnviarCopias(Bloque* bloque, String buffer);
int bloqueGuardar(Archivo* archivo, String buffer, size_t bytes, Entero numeroBloque);
bool bloqueOrdenarPorNumero(Bloque* unBloque, Bloque* otroBloque);
void bloqueCopiar(Nodo* nodo, Puntero datos, int* estado);
void bloqueLeer(Nodo* nodo, Puntero datos, int* estado);
void bloqueCopiarArchivo(Nodo* nodo, Puntero datos, int* estado);
bool bloqueDisponible(Bloque* bloque);
BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer, int tamanioUtilizado);
BloqueYama yamaConvertirBloque(Bloque* bloque);
bool bloqueEstaEnNodo(Bloque* bloque, Nodo* nodo);

//--------------------------------------- Funciones de Copia -------------------------------------

Copia* copiaCrear(int numeroBloqueDelNodo, String nombreNodo);
void copiaEliminar(Copia* copia);
void copiaDestruir(Copia* copia);
bool copiaDisponible(Copia* copia);
bool copiaOrdenarPorActividadDelNodo(Copia* unaCopia, Copia* otraCopia);

//--------------------------------------- Funciones de Metadata -------------------------------------

void metadataCrear();
void metadataEliminar();
void metadataIniciar();
void metadataRecuperar();

//--------------------------------------- Funciones de Ruta ------------------------------------

String rutaObtenerUltimoNombre(String ruta);
bool rutaBarrasEstanSeparadas(String ruta);
String* rutaSeparar(String ruta);
bool rutaTieneAlMenosUnaBarra(String ruta);
bool rutaValida(String ruta);
void rutaBufferCrear();
void rutaBufferDestruir();
void rutaYamaDecente(Comando* comando, int indice);
bool rutaTienePrefijoYama(String ruta);
String rutaObtenerDirectorio(String pathArchivo);
bool rutaValidaAlmacenar(String ruta);
bool rutaParaArchivo(String ruta);

//--------------------------------------- Funciones de Estado ------------------------------------

bool estadoEjecucionIgualA(int estado);
bool estadoFileSystemIgualA(int estado);
bool estadoControlIgualA(int estado);
void estadoControlActivar();
void estadoFileSystemEstable();
void estadoFileSystemInestable();
void estadoEjecucionNormal();
void estadoControlDesactivar();
void estadoMensaje(int estado);
bool estadoMensajeIgualA(int estado);

//--------------------------------------- Funciones de Bitmaps de directorios ------------------------------------

void bitmapDirectoriosDestruir();
void bitmapDirectoriosCrear();
bool bitmapDirectoriosBitOcupado(int indice);
void bitmapDirectoriosOcuparBit(int posicion);

//--------------------------------------- Funciones de Semaforo ------------------------------------

void semaforosCrear();
void semaforosIniciar();
void semaforosDestruir();
