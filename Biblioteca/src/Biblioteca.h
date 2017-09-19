/*
 * BibliotecaSockets.h
 *
 *  Created on: 12/8/2017
 *      Author: Dario Poma
 */

//--------------------------------------- Includes -------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include "commons/config.h"
#include "commons/log.h"
#include "commons/string.h"
#include "commons/collections/list.h"

//--------------------------------------- Constantes -------------------------------------

#define IP_LOCAL "127.0.0.1"
#define MAX 1024
#define LISTEN 10
#define ERROR -1
#define NULO 0
#define DESCONEXION 0
#define HANDSHAKE 1
#define ID_FILESYSTEM 1
#define ID_YAMA 2
#define ID_MASTER 3
#define ID_WORKER 4
#define ID_DATANODE 5

//--------------------------------------- Definiciones -------------------------------------

typedef int Socket;
typedef fd_set ListaSockets;
typedef void* Puntero;
typedef char* String;
typedef socklen_t Socklen;
typedef struct addrinfo* AddrInfo;
typedef struct sockaddr_in SockAddrIn;
typedef struct sockaddr* SockAddr;
typedef t_config* ArchivoConfig;
typedef t_log* ArchivoLog;
typedef t_log_level NivelLog;
typedef sem_t Semaforo;
typedef pthread_mutex_t Mutex;
typedef pthread_t Hilo;
typedef t_list* Lista;

//--------------------------------------- Estructuras -------------------------------------

typedef struct {
	int operacion;
	int tamanio;
} Header;

typedef struct {
	Header header;
	Puntero datos;
} Mensaje;

typedef struct {
	AddrInfo informacion;
	SockAddrIn address;
	Socklen tamanioAddress;
	String puerto;
	String ip;
} Conexion;


//--------------------------------------- Funciones para Socket -------------------------------------

void socketConfigurar(Conexion* conexion, String ip, String puerto);
int socketCrear(Conexion* conexion, String ip, String puerto);
void socketConectar(Conexion* conexion, Socket unSocket);
void socketBindear(Conexion* conexion, Socket unSocket);
void socketEscuchar(Socket unSocket, int ClientesEnEspera);
int socketAceptar(Socket unSocket, int idEsperada);
void socketRedireccionar(Socket unSocket);
void socketSelect(Socket cantidadSockets, ListaSockets* listaSockets);
int socketRecibir(Socket socketEmisor, Puntero buffer, int tamanioBuffer);
int socketEnviar(Socket socketReceptor, Puntero mensaje, int tamanioMensaje);
void socketCerrar(Socket unSocket);
bool socketSonIguales(Socket unSocket, Socket otroSocket);
bool socketSonDistintos(Socket unSocket, Socket otroSocket);
bool socketEsMayor(Socket unSocket, Socket otroSocket);
void socketError(int estado, String error);
Socket socketCrearListener(String puerto);
Socket socketCrearCliente(String ip, String puerto, int idProceso);

//--------------------------------------- Funciones para ListaSocket -------------------------------------

void listaSocketsAgregar(Socket unSocket, ListaSockets* listaSockets);
void listaSocketsEliminar(Socket unSocket, ListaSockets* listaSockets);
bool listaSocketsContiene(Socket unSocket, ListaSockets* listaSockets);
void listaSocketsLimpiar(ListaSockets* listaSockets);

//--------------------------------------- Funciones para Mensaje -------------------------------------

void* mensajeCrear(int operacion, Puntero dato, int tamanioDato);
void mensajeEnviar(Socket socketReceptor, int operacion, Puntero dato, int tamanioDato);
void mensajeSetearError(Mensaje* mensaje, int idError);
void mensajeVerificarEstado(Mensaje* mensaje, int bytes);
bool mensajeOperacionIgualA(Mensaje* mensaje, int operacion);
Mensaje* mensajeRecibir(Socket socketEmisor);
bool mensajeOperacionErronea(Mensaje* mensaje);
void mensajeDestruir(Mensaje* mensaje);
void mensajeAvisarDesconexion(Mensaje* mensaje);
bool mensajeConexionFinalizada(int bytes);
void mensajeRevisarConexion(Mensaje* mensaje, Socket socketReceptor, int bytes);
void mensajeObtenerDatos(Mensaje* mensaje, Socket socketReceptor);
bool mensajeDesconexion(Mensaje* mensaje) ;

//--------------------------------------- Funciones para Header -------------------------------------

Header headerCrear(int operacion, int tamanio);

//--------------------------------------- Funciones para ArchivoConfig -------------------------------------

ArchivoConfig archivoConfigCrear(String path, String* campos);
bool archivoConfigTieneCampo(ArchivoConfig archivoConfig, String campo);
bool archivoConfigFaltaCampo(ArchivoConfig archivoConfig, String campo);
String archivoConfigStringDe(ArchivoConfig archivoConfig, String campo);
int archivoConfigEnteroDe(ArchivoConfig archivoConfig, String campo);
long archivoConfigLongDe(ArchivoConfig archivoConfig, String campo);
double archivoConfigDoubleDe(ArchivoConfig archivoConfig, String campo);
String* archivoConfigArrayDe(ArchivoConfig archivoConfig, String campo);
int archivoConfigCantidadCampos(ArchivoConfig archivoConfig);
void archivoConfigDestruir(ArchivoConfig archivoConfig);
void archivoConfigSetearCampo(ArchivoConfig archivoConfig, String campo, String valor);
bool archivoConfigInvalido(ArchivoConfig archivoConfig, String* campos);
bool archivoConfigIncompleto(ArchivoConfig archivoConfig, String* campos);
bool archivoConfigInexistente(ArchivoConfig archivoConfig);

//--------------------------------------- Funciones para ArchivoLog -------------------------------------

ArchivoLog archivoLogCrear(String nombreArchivo, String nombrePrograma);
void archivoLogDestruir(ArchivoLog archivoLog);
void archivoLogInformarMensaje(ArchivoLog archivoLog, String mensaje);
void archivoLogInformarPeligro(ArchivoLog archivoLog, String mensajePeligro);
void archivoLogInformarError(ArchivoLog archivoLog, String mensajeError);
void archivoLogInformarTrace(ArchivoLog archivoLog, String mensajeTrace);
void archivoLogInformarDebug(ArchivoLog archivoLog, String mensajeDebug);
String archivoLogNivelLogAString(NivelLog nivelLog);
NivelLog archivoLogStingANivelLog(String stringNivelLog);
void archivoLogValidar(String rutaArchivo);

//--------------------------------------- Funciones para Semaforo -------------------------------------


void semaforoCrear(Semaforo* semaforo, unsigned int valor);
void semaforoWait(Semaforo* semaforo);
void semaforoSignal(Semaforo* semaforo);
void semaforoValor(Semaforo* semaforo, int* buffer);
void semaforoDestruir(Semaforo* semaforo);

//--------------------------------------- Funciones para Mutex -------------------------------------

void mutexCrear(Mutex* mutex);
void mutexBloquear(Mutex* mutex);
void mutexDesbloquear(Mutex* mutex);

//--------------------------------------- Funciones para Hilo -------------------------------------

void hiloCrear(Hilo* hilo, void*(*funcionHilo)(void*), void* parametroHilo);
void hiloSalir();
void hiloCancelar(Hilo hilo);
void hiloEsperar(Hilo hilo);
void hiloDetach(Hilo hilo);
Hilo hiloId();

//--------------------------------------- Funciones para Lista -------------------------------------

Lista listaCrear();
void listaDestruir(Lista lista);
void listaDestruirConElementos(Lista lista, void(*funcion)(void*));
int listaAgregarElemento(Lista lista, void* elemento);
void listaAgregarEnPosicion(Lista lista, void* elemento, int posicion);
void listaAgregarOtraLista(Lista lista, Lista otraLista);
void* listaObtenerElemento(Lista lista, int posicion);
Lista listaTomar(Lista lista, int cantidadElementos);
Lista listaSacar(Lista lista, int cantidadElementos);
Lista listaFiltrar(Lista lista, bool(*funcion)(void*));
Lista listaMapear(Lista lista, void*(*funcion)(void*));
void* listaReemplazarElemento(Lista lista, void* elemento, int posicion);
void listaReemplazarDestruyendoElemento(Lista lista, void* elemento, int posicion, void(*funcion)(void*));
void listaEliminarElemento(Lista lista, int posicion);
void listaEliminarDestruyendoElemento(Lista lista, int posicion, void(*funcion)(void*));
void listaEliminarPorCondicion(Lista lista, bool(*funcion)(void*));
void listaEliminarDestruyendoPorCondicion(Lista lista, bool(*funcion)(void*), void(*funcionDestruir)(void*));
void listaLimpiar(Lista lista);
void listaLimpiarDestruyendoTodo(Lista lista, void(*funcion)(void*));
void listaIterar(Lista lista, void(*funcion)(void*));
void* listaEncontrar(Lista lista, bool(*funcion)(void*));
int listaCantidadElementos(Lista lista);
bool listaEstaVacia(Lista lista);
void listaOrdenar(Lista lista, bool(*funcion)(void*, void*));
int listaCuantosCumplen(Lista lista, bool(*funcion)(void*));
bool listCumpleAlguno(Lista lista, bool(*funcion)(void*));
bool listaCumplenTodos(Lista lista, bool(*funcion)(void*));

//--------------------------------------- Funciones para String -------------------------------------

bool stringContiene(String unString, String otroString);
String stringConvertirEntero(int entero);
String stringRepetirCaracter(char caracter, int repeticiones);
void  stringAgregarString(String* unString, String otroString);
String stringDuplicar(String string);
void stringPonerEnMayuscula(String string);
void stringPonerEnMinuscula(String string);
void stringPonerEnCapital(String string);
void stringRemoverVacios(String* string);
void stringRemoverVaciosIzquierda(String* string);
void stringRemoverVaciosDerecha(String* string);
int stringLongitud(String string);
bool stringEstaVacio(String string);
bool stringEmpiezaCon(String string, String stringComienzo);
bool stringTerminaCon(String string, String stringTerminacion);
String stringDarVuelta(String string);
String stringTomarCantidad(String string, int desde, int cantidad);
String stringTomarDesdePosicion(String string, int posicion);
String stringTomarDesdeInicio(String string, int cantidad);

//--------------------------------------- Funciones de HandShake-------------------------------------

int handShakeRecepcionExitosa(Socket unSocket, int idEsperada);
int handShakeEnvioExitoso(Socket unSocket, int idProceso);
void handShakeError(Socket unSocket);
int handShakeRecepcionFallida(Socket unSocket, int idEsperada);
int handShakeEnvioFallido(Socket unSocket, int idProceso);
bool handShakeRealizado(Mensaje* mensaje);
bool handShakeAceptado(Mensaje* mensaje);
bool handShakeIdsIguales(int idEnviada, int idEsperada);

//--------------------------------------- Funciones varias -------------------------------------

void funcionSenial(int senial);
void imprimirMensajeProceso(String mensaje);



