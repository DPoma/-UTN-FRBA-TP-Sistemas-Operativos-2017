/*
 ============================================================================
 Name        : BibliotecaGeneral.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Biblioteca general para dummies
 ============================================================================
 */

#include "Biblioteca.h"

//--------------------------------------- Funciones para Socket -------------------------------------

void socketConfigurar(Conexion* conexion, String ip, String puerto) {
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// No importa si uso IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;		// Asigna el address del localhost: 127.0.0.1
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP
	getaddrinfo(ip, puerto, &hints, &conexion->informacion);// Carga en serverInfo los datos de la conexion
}

int socketCrear(Conexion* conexion, String ip, String puerto) {
	socketConfigurar(conexion, ip, puerto);
	Socket unSocket = socket(conexion->informacion->ai_family, conexion->informacion->ai_socktype, conexion->informacion->ai_protocol);
	socketError(unSocket, "socket");
	return unSocket;
}

void socketConectar(Conexion* conexion, Socket unSocket) {
	int estado = connect(unSocket, conexion->informacion->ai_addr, conexion->informacion->ai_addrlen);
	socketError(estado, "connect");
	freeaddrinfo(conexion->informacion);
}

void socketBindear(Conexion* conexion, Socket unSocket) {
	int estado = bind(unSocket, conexion->informacion->ai_addr, conexion->informacion->ai_addrlen);
	socketError(estado, "bind");
	freeaddrinfo(conexion->informacion);
}

void socketEscuchar(Socket unSocket, int clientesEsperando) {
	int estado = listen(unSocket, clientesEsperando);
	socketError(estado, "listen");
}

int socketAceptar(Conexion* conexion, Socket unSocket) {
	conexion->tamanioAddress = sizeof(SockAddrIn);
	int estado = accept(unSocket, (struct sockaddr*)&conexion->address, &conexion->tamanioAddress);
	if(estado == -1)
		perror("accept");
	return estado;
}

void socketRedireccionar(Socket unSocket) {
	int yes = 1;
	int estado = setsockopt(unSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	socketError(estado, "setsockopt");
}

void socketSelect(int cantidadSockets, ListaSockets* listaSockets) {
	int estado = select(cantidadSockets + 1, listaSockets, NULL, NULL, NULL);
	socketError(estado, "select");
}

int socketRecibir(Socket socketEmisor, Dato buffer, int tamanioBuffer) {
	int estado = recv(socketEmisor, buffer, tamanioBuffer, MSG_WAITALL);
	socketError(estado, "recv");
	return estado;
}

int socketEnviar(Socket socketReceptor, Dato mensaje, int tamanioMensaje) {
	int estado = send(socketReceptor, mensaje, tamanioMensaje, 0);
	socketError(estado, "send");
	return estado;
}

void socketCerrar(Socket unSocket) {
	close(unSocket);
}

bool socketSonIguales(Socket unSocket, Socket otroSocket) {
	return unSocket == otroSocket;
}

bool socketSonDistintos(Socket unSocket, Socket otroSocket) {
	return unSocket != otroSocket;
}

bool socketEsMayor(Socket unSocket, Socket otroSocket) {
	return unSocket > otroSocket;
}

void socketError(int estado, String error) {
	if(estado == -1) {
		perror(error);
		exit(1);
	}
}

Socket socketCrearListener(String ip, String puerto) {
	Conexion conexion;
	Socket listener = socketCrear(&conexion, ip, puerto);
	socketRedireccionar(listener);
	socketBindear(&conexion, listener);
	socketEscuchar(listener, 10);
	return listener;
}

Socket socketCrearCliente(String ip, String puerto) {
	Conexion conexion;
	Socket cliente = socketCrear(&conexion, ip, puerto);
	socketConectar(&conexion, cliente);
	return cliente;
}

//--------------------------------------- Funciones para ListaSockets-------------------------------------

void listaSocketsAgregar(Socket unSocket, ListaSockets* listaSockets) {
	FD_SET(unSocket, listaSockets);
}

void listaSocketsEliminar(Socket unSocket, ListaSockets* listaSockets) {
	FD_CLR(unSocket, listaSockets);
}

bool listaSocketsContiene(Socket unSocket, ListaSockets* listaSockets) {
	return FD_ISSET(unSocket, listaSockets);
}

void listaSocketsLimpiar(ListaSockets* listaSockets) {
	FD_ZERO(listaSockets);
}

//--------------------------------------- Funciones para Mensaje -------------------------------------

void* mensajeCrear(int operacion, void* dato, int tamanioDato) {
	Header header = headerCrear(operacion, tamanioDato);
	void* buffer = malloc(sizeof(Header)+tamanioDato);
	memcpy(buffer, &header, sizeof(Header));
	memcpy(buffer+sizeof(Header), dato, tamanioDato);
	return buffer;
}

void mensajeEnviar(int socketReceptor, int operacion, void* dato, int tamanioDato) {
	void* buffer = mensajeCrear(operacion, dato, tamanioDato);
	socketEnviar(socketReceptor, buffer, sizeof(Header)+tamanioDato);
}

void mensajeSetearError(Mensaje* mensaje, int operacion) {
	mensaje->header.operacion = operacion;
	mensaje->header.tamanio = -1;
	mensaje->dato = NULL;
}

//Si los bytes da 0 es porque se cerro la conexion manualmente y da -1 si se cerro por algun error
void mensajeVerificarEstado(Mensaje* mensaje, int bytes) {
	if(bytes == 0) {
		mensajeSetearError(mensaje, -1);
	}
}

//Bytes mayor a 0 quiere decir que llegaron datos
Mensaje* mensajeRecibir(int socketReceptor) {
	Mensaje* mensaje = malloc(sizeof(Mensaje));
	int bytes = socketRecibir(socketReceptor, &mensaje->header, sizeof(Header));
	if(bytes > 0) {
		int tamanioDato = mensaje->header.tamanio;
		mensaje->dato = malloc(tamanioDato);
		bytes = socketRecibir(socketReceptor, mensaje->dato, tamanioDato);
		mensajeVerificarEstado(mensaje, bytes);
	} else {
		mensajeVerificarEstado(mensaje, bytes);
	}
	return mensaje;
}

void mensajeDestruir(Mensaje* mensaje) {
	if(mensaje->dato != NULL)
		free(mensaje->dato);
	free(mensaje);
}

bool mensajeOperacionIgualA(Mensaje* mensaje, int operacion) {
	return mensaje->header.operacion == operacion;
}

bool mensajeOperacionErronea(Mensaje* mensaje) {
	return mensaje->header.operacion < 0;
}

//--------------------------------------- Funciones para Header -------------------------------------

Header headerCrear(int operacion, int tamanio) {
	Header header;
	header.operacion = operacion;
	header.tamanio = tamanio;
	return header;
}

//--------------------------------------- Funciones para Configuracion -------------------------------------

void* configuracionCrear(String rutaArchivo, void*(*configProcesoCrear)(ArchivoConfig archivoConfiguracion), String* campos) {
	ArchivoConfig archivo = archivoConfigCrear(rutaArchivo, campos);
	return configProcesoCrear(archivo);
}

//--------------------------------------- Funciones para ArchivoConfiguracion -------------------------------------

ArchivoConfig archivoConfigCrear(String path, String* campos) {
	ArchivoConfig archivoConfig = config_create(path);
	//ArchivoConfig archivoConfig = malloc(sizeof(ArchivoConfig));
	//archivoConfig= config_create(path);
	if(archivoConfigInvalido(archivoConfig, campos)) {
		puts("Archivo de configuracion invalido");
		exit(EXIT_FAILURE);
	}
	return archivoConfig;
}

bool archivoConfigTieneCampo(ArchivoConfig archivoConfig, String campo) {
	return config_has_property(archivoConfig, campo);
}

String archivoConfigStringDe(ArchivoConfig archivoConfig, String campo) {
	return config_get_string_value(archivoConfig, campo);
}

int archivoConfigEnteroDe(ArchivoConfig archivoConfig, String campo) {
	return config_get_int_value(archivoConfig, campo);
}

long archivoConfigLongDe(ArchivoConfig archivoConfig, String campo) {
	return config_get_long_value(archivoConfig, campo);
}

double archivoConfigDoubleDe(ArchivoConfig archivoConfig, String campo) {
	return config_get_double_value(archivoConfig, campo);
}

String* archivoConfigArrayDe(ArchivoConfig archivoConfig, String campo) {
	return config_get_array_value(archivoConfig, campo);
}

int archivoConfigCantidadCampos(ArchivoConfig archivoConfig) {
	return config_keys_amount(archivoConfig);
}

void archivoConfigDestruir(ArchivoConfig archivoConfig) {
	config_destroy(archivoConfig);
}

void archivoConfigSetearCampo(ArchivoConfig archivoConfig, String campo, String valor) {
	config_set_value(archivoConfig, campo, valor);
}

bool archivoConfigInvalido(ArchivoConfig archivoConfig, String* campos) {
	return archivoConfigIncompleto(archivoConfig, campos) || archivoConfigInexistente(archivoConfig) ;
}

bool archivoConfigInexistente(ArchivoConfig archivoConfig) {
	return archivoConfig == NULL;
}

bool archivoConfigIncompleto(ArchivoConfig archivoConfig, String* campos) {
	int indice;
	for(indice = 0; indice<archivoConfigCantidadCampos(archivoConfig); indice++) {
		if(!archivoConfigTieneCampo(archivoConfig, campos[indice]))
			return 1;
	}
	return 0;
}

//--------------------------------------- Funciones para ArchivoLog -------------------------------------

ArchivoLog archivoLogCrear(String rutaArchivo, String nombrePrograma) {
	archivoLogValidar(rutaArchivo);
	ArchivoLog archivoLog = log_create(rutaArchivo, nombrePrograma, 1, 1);
	return archivoLog;
}

void archivoLogDestruir(ArchivoLog archivoLog) {
	log_destroy(archivoLog);
}

void archivoLogInformarMensaje(ArchivoLog archivoLog, String mensaje) {
	log_info(archivoLog, mensaje);
}

void archivoLogInformarAdvertencia(ArchivoLog archivoLog, String mensajePeligro, ...) {
	log_warning(archivoLog, mensajePeligro);
}

void archivoLogInformarError(ArchivoLog archivoLog, String mensajeError){
	log_error(archivoLog, mensajeError);

}

void archivoLogInformarTrace(ArchivoLog archivoLog, String mensajeTrace) {
	log_trace(archivoLog, mensajeTrace);
}

void archivoLogInformarDebug(ArchivoLog archivoLog, String mensajeDebug) {
	log_debug(archivoLog, mensajeDebug);
}

String archivoLogNivelLogAString(NivelLog nivelLog) {
	return log_level_as_string(nivelLog);
}

NivelLog archivoLogStingANivelLog(String stringNivelLog) {
	return log_level_from_string(stringNivelLog);
}

void archivoLogValidar(String rutaArchivo) {
	FILE* archivoLog = fopen(rutaArchivo, "r");
	if(archivoLog != NULL) {
		remove(rutaArchivo);
		fclose(archivoLog);
	}
}

//--------------------------------------- Funciones para Semaforo -------------------------------------

void semaforoCrear(Semaforo* semaforo, unsigned int valor) {
	sem_init(semaforo, 0, valor);
}

void semaforoWait(Semaforo* semaforo) {
	sem_wait(semaforo);
}

void semaforoSignal(Semaforo* semaforo) {
	sem_post(semaforo);
}

void semaforoDestruir(Semaforo* semaforo) {
	sem_destroy(semaforo);
}

void semaforoValor(Semaforo* semaforo, int* buffer) {
	sem_getvalue(semaforo, buffer);
}

//--------------------------------------- Funciones Mutex -------------------------------------

void mutexCrear(Mutex* mutex) {
	pthread_mutex_init(mutex, NULL);
}

void mutexBloquear(Mutex* mutex) {
	pthread_mutex_lock(mutex);
}

void mutexDesbloquear(Mutex* mutex) {
	pthread_mutex_unlock(mutex);
}

//--------------------------------------- Funciones para Hilo -------------------------------------

void hiloCrear(Hilo* hilo, void*(*funcionHilo)(void*), void* parametroHilo) {
	pthread_create(hilo, NULL, funcionHilo, parametroHilo);
}

void hiloEsperar(Hilo hilo) {
	pthread_join(hilo, NULL);
}

void hiloSalir() {
	pthread_exit(NULL);
}

void hiloCancelar(Hilo hilo) {
	pthread_cancel(hilo);
}

void hiloDetach(Hilo hilo) {
	pthread_detach(hilo);
}

Hilo hiloId() {
	 return pthread_self();
}

//--------------------------------------- Funciones para Lista -------------------------------------

Lista listaCrear() {
	return list_create();
}

void listaDestruir(Lista lista) {
	list_destroy(lista);
}

void listaDestruirConElementos(Lista lista, void(*funcion)(void*)) {
	list_destroy_and_destroy_elements(lista, funcion);
}

int listaAgregarElemento(Lista lista, void* elemento) {
	return list_add(lista, elemento);
}

void listaAgregarEnPosicion(Lista lista, void* elemento, int posicion) {
	list_add_in_index(lista, posicion, elemento);
}
void listaAgregarOtraLista(Lista lista, Lista otraLista) {
	list_add_all(lista, otraLista);
}

void* listaObtenerElemento(Lista lista, int posicion) {
	return list_get(lista, posicion);
}

Lista listaTomar(Lista lista, int cantidadElementos) {
	return list_take(lista, cantidadElementos);
}

Lista listaSacar(Lista lista, int cantidadElementos) {
	return list_take_and_remove(lista, cantidadElementos);
}

Lista listaFiltrar(Lista lista, bool(*funcion)(void*)) {
	return list_filter(lista, funcion);
}

Lista listaMapear(Lista lista, void*(*funcion)(void*)) {
	return list_map(lista, funcion);
}
void* listaReemplazarElemento(Lista lista, void* elemento, int posicion) {
	return list_replace(lista, posicion, elemento);
}
void listaReemplazarDestruyendoElemento(Lista lista, void* elemento, int posicion, void(*funcion)(void*)) {
	list_replace_and_destroy_element(lista, posicion, elemento, funcion);
}
void listaEliminarElemento(Lista lista, int posicion) {
	list_remove(lista, posicion);
}
void listaEliminarDestruyendoElemento(Lista lista, int posicion, void(*funcion)(void*)) {
	list_remove_and_destroy_element(lista, posicion, funcion);
}
void listaEliminarPorCondicion(Lista lista, bool(*funcion)(void*)) {
	list_remove_by_condition(lista, funcion);
}
void listaEliminarDestruyendoPorCondicion(Lista lista, bool(*funcion)(void*), void(*funcionDestruir)(void*)) {
	list_remove_and_destroy_by_condition(lista, funcion, funcionDestruir);
}

void listaLimpiar(Lista lista) {
	list_clean(lista);
}

void listaLimpiarDestruyendoTodo(Lista lista, void(*funcion)(void*)) {
	list_clean_and_destroy_elements(lista, funcion);
}
void listaIterar(Lista lista, void(*funcion)(void*)) {
	list_iterate(lista, funcion);
}
void* listaEncontrar(Lista lista, bool(*funcion)(void*)) {
	return list_find(lista, funcion);
}

int listaCantidadElementos(Lista lista) {
	return list_size(lista);
}
bool listaEstaVacia(Lista lista) {
	return list_is_empty(lista);
}
void listaOrdenar(Lista lista, bool(*funcion)(void*, void*)) {
	list_sort(lista, funcion);
}
int listaCuantosCumplen(Lista lista, bool(*funcion)(void*)) {
	return list_count_satisfying(lista, funcion);
}
bool listCumpleAlguno(Lista lista, bool(*funcion)(void*)) {
	return list_any_satisfy(lista, funcion);
}
bool listaCumplenTodos(Lista lista, bool(*funcion)(void*)) {
	return list_all_satisfy(lista, funcion);
}

//--------------------------------------- Funciones para String -------------------------------------

bool stringContiene(String unString, String otroString) {
	return string_contains(unString, otroString);
}

String stringConvertirEntero(int entero) {
	return string_itoa(entero);
}

String stringRepetirCaracter(char caracter, int repeticiones) {
	return string_repeat(caracter, repeticiones);
}

void stringAgregarString(String* unString, String otroString) {
	string_append(unString, otroString);
}

String stringDuplicar(String string) {
	return string_duplicate(string);
}

void stringPonerEnMayuscula(String string) {
	string_to_upper(string);
}

void stringPonerEnMinuscula(String string) {
	string_to_lower(string);
}

void stringPonerEnCapital(String string) {
	string_capitalized(string);
}

void stringRemoverVacios(String* string) {
	string_trim(string);
}

void stringRemoverVaciosIzquierda(String* string) {
	string_trim_left(string);
}

void stringRemoverVaciosDerecha(String* string) {
	string_trim_right(string);
}

int stringLongitud(String string) {
	return string_length(string);
}

bool stringEstaVacio(String string) {
	return string_is_empty(string);
}

bool stringEmpiezaCon(String string, String stringComienzo) {
	return string_starts_with(string, stringComienzo);
}

bool stringTerminaCon(String string, String stringTerminacion) {
	return string_ends_with(string, stringTerminacion);
}

String stringDarVuelta(String string) {
	return string_reverse(string);
}

String stringTomarCantidad(String string, int desde, int cantidad) {
	return string_substring(string, desde, cantidad);
}

String stringTomarDesdePosicion(String string, int posicion) {
	return string_substring_from(string, posicion);
}

String stringTomarDesdeInicio(String string, int cantidad) {
	return string_substring_until(string, cantidad);
}

//--------------------------------------- Funciones para señales -------------------------------------

void senialAsignarFuncion(int unaSenial, void(*funcion)(int)) {
	signal(unaSenial, funcion);
}

void imprimirMensajeProceso(char* mensaje) {
	puts("----------------------------------------------------------------");
	puts(mensaje);
	puts("----------------------------------------------------------------");
}




