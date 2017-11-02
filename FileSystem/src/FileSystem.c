/*
 ============================================================================
 Name        : FileSystem.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso File System
 ============================================================================
 */

#include "FileSystem.h"

int main(int contadorArgumentos, String* argumentos) {
	fileSystemIniciar(argumentos[1]);
	fileSystemCrearConsola();
	fileSystemAtenderProcesos();
	fileSystemFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar(String flag) {
	pantallaLimpiar();
	semaforosCrear();
	semaforosIniciar();
	flagMensaje = DESACTIVADO;
	configuracionIniciar();
	estadoControlActivar();
	estadoFileSystemInestable();
	listaSockets = listaCrear(); //TODO GLOBAL
	nodoListaCrear();
	if(stringIguales(flag, FLAG_C)) {
		metadataIniciar();
		imprimirMensaje(archivoLog, AMARILLO"[AVISO] Para pasar a un estado estable primero debe formatear el File System"BLANCO);
	}
	else
		metadataRecuperar();
}

void fileSystemCrearConsola() {
	hiloCrear(&hiloConsola, (Puntero)consolaAtenderComandos, NULL);
}

void fileSystemAtenderProcesos() {
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorIniciar(servidor);
	while(estadoControlIgualA(ACTIVADO))
		servidorAtenderSolicitudes(servidor);
	servidorFinalizar(servidor);
}

void fileSystemFinalizar() {
	semaforoWait(semaforoFinal);
	archivoLogDestruir(archivoLog);
	configuracionDestruirRutas();
	bitmapDirectoriosDestruir();
	archivoListaDestruir();
	directorioListaDestruir();
	nodoListaDestruir();
	semaforosDestruir();
	sleep(2);
}
//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	stringCopiar(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYama);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Workers (Puerto: %s)", configuracion->puertoWorker);
	imprimirMensajeUno(archivoLog, "[INFORMACION] Ruta de metadata: %s", configuracion->rutaMetadata);
}

void configuracionIniciarCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "PUERTO_WORKER";
	campos[3] = "RUTA_METADATA";
}

void configuracionIniciar() {
	configuracionIniciarLog();
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionIniciarRutas();
	configuracionImprimir(configuracion);
}

void configuracionIniciarLog() {
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	archivoLog = archivoLogCrear(RUTA_LOG, "FileSystem");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System inicializado");
}

void configuracionIniciarRutas() {
	rutaDirectorioArchivos = string_from_format("%s/archivos", configuracion->rutaMetadata);
	rutaDirectorioBitmaps = string_from_format("%s/bitmaps", configuracion->rutaMetadata);
	rutaArchivos = string_from_format("%s/Archivos.txt", configuracion->rutaMetadata);
	rutaDirectorios = string_from_format("%s/Directorios.txt", configuracion->rutaMetadata);
	rutaNodos = string_from_format("%s/Nodos.bin", configuracion->rutaMetadata);
	mutexBloquear(mutexRuta);
	rutaBuffer = stringCrear(MAX_STRING);
	mutexDesbloquear(mutexRuta);

}

void configuracionDestruirRutas() {
	memoriaLiberar(configuracion);
	memoriaLiberar(rutaNodos);
	memoriaLiberar(rutaDirectorioArchivos);
	memoriaLiberar(rutaDirectorioBitmaps);
	memoriaLiberar(rutaDirectorios);
	memoriaLiberar(rutaArchivos);
	mutexBloquear(mutexRuta);
	memoriaLiberar(rutaBuffer);
	mutexDesbloquear(mutexRuta);
}

//--------------------------------------- Funciones de Servidor -------------------------------------

void servidorIniciar(Servidor* servidor) {
	servidorIniciarContadorSocket(servidor);
	servidorLimpiarListas(servidor);
	servidorActivarListeners(servidor);
}

void servidorFinalizar(Servidor* servidor) {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
	memoriaLiberar(servidor);
}

void servidorEsperarSolicitud(Servidor* servidor) {
	socketSelect(servidor->contadorSocket, &servidor->listaSelect);
}

bool servidorCantidadSockets(Servidor* servidor) {
	return servidor-> contadorSocket;
}

void servidorIniciarListaSelect(Servidor* servidor) {
	servidor->listaSelect = servidor->listaMaster;
}

void servidorIniciarContadorSocket(Servidor* servidor) {
	servidor->contadorSocket = 0;
}

void servidorControlarContadorSocket(Servidor* servidor, Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->contadorSocket))
		servidor->contadorSocket = unSocket;
}

void servidorAceptarConexion(Servidor* servidor, Socket unSocket) {
	listaSocketsAgregar(unSocket, &servidor->listaMaster);
	servidorControlarContadorSocket(servidor, unSocket);
}

void servidorActivarListeners(Servidor* servidor) {
	servidorActivarListenerDataNode(servidor);
	servidorActivarListenerYama(servidor);
	servidorActivarListenerWorker(servidor);
}

void servidorLimpiarListas(Servidor* servidor) {
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	listaSocketsLimpiar(&servidor->listaDataNodes);
	listaSocketsLimpiar(&servidor->listaWorkers);
}

void servidorActivarListenerYama(Servidor* servidor) {
	servidor->listenerYama = socketCrearListener(configuracion->puertoYama);
	listaSocketsAgregar(servidor->listenerYama, &servidor->listaMaster);
	servidorControlarContadorSocket(servidor, servidor->listenerYama);
}

void servidorActivarListenerDataNode(Servidor* servidor) {
	servidor->listenerDataNode = socketCrearListener(configuracion->puertoDataNode);
	listaSocketsAgregar(servidor->listenerDataNode, &servidor->listaMaster);
	servidorControlarContadorSocket(servidor, servidor->listenerDataNode);
}

void servidorActivarListenerWorker(Servidor* servidor) {
	servidor->listenerWorker = socketCrearListener(configuracion->puertoWorker);
	listaSocketsAgregar(servidor->listenerWorker, &servidor->listaMaster);
	servidorControlarContadorSocket(servidor, servidor->listenerWorker);
}

void servidorAtenderSolicitudes(Servidor* servidor) {
	servidorIniciarListaSelect(servidor);
	servidorEsperarSolicitud(servidor);
	Socket unSocket;
	for(unSocket = 0; unSocket <= servidor->contadorSocket ; unSocket++)
		if(servidorAtenderSocket(servidor, unSocket) == ID_EXIT)
			break;
}

int servidorAtenderSocket(Servidor* servidor, Socket unSocket) {
	if(socketRealizoSolicitud(servidor, unSocket))
		return servidorAtenderSolicitud(servidor, unSocket);
	return NULO;
}

int servidorAtenderSolicitud(Servidor* servidor, Socket unSocket) {
	if(socketEsListener(servidor, unSocket))
		return servidorSolicitudProceso(servidor, unSocket);
	else
		servidorSolicitudMensaje(servidor, unSocket);
	return NULO;
}

int servidorSolicitudProceso(Servidor* servidor, Socket unSocket) {
	if(socketEsListenerDataNode(servidor, unSocket))
		return servidorSolicitudDataNode(servidor, unSocket);
	else if(socketEsListenerYama(servidor, unSocket))
		servidorSolicitudYama(servidor, unSocket);
	else
		servidorSolicitudWorker(servidor, unSocket);
	return NULO;
}

void servidorSolicitudMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(socketEsDataNode(servidor, unSocket))
		servidorMensajeDataNode(servidor, mensaje, unSocket);
	else if(socketEsYama(servidor, unSocket))
		servidorMensajeYama(mensaje);
	else
		servidorMensajeWorker();
	mensajeDestruir(mensaje);
}

//--------------------------------------- Servidor Data Node -------------------------------------

int servidorSolicitudDataNode(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket = socketAceptar(unSocket, ID_DATANODE);
	if(estadoControlIgualA(ACTIVADO))
		servidorAtenderDataNode(servidor, nuevoSocket);
	else
		return ID_EXIT;
	return NULO;
}

void servidorAtenderDataNode(Servidor* servidor, Socket nuevoSocket) {
	Mensaje* mensaje = mensajeRecibir(nuevoSocket);
	Nodo* nodoTemporal = nodoCrear(mensaje->datos, nuevoSocket);
	if(estadoEjecucionIgualA(NORMAL))
		servidorReconectarDataNode(servidor, nodoTemporal);
	else
		servidorControlarDataNode(servidor, nodoTemporal);
	mensajeDestruir(mensaje);
}

void servidorReconectarDataNode(Servidor* servidor, Nodo* nodoTemporal) {
	if(nodoInvalido(nodoTemporal))
		servidorRechazarDataNode(nodoTemporal);
	else
		servidorAceptarReconexionDataNode(servidor, nodoTemporal);
}

void servidorControlarDataNode(Servidor* servidor, Nodo* nodoTemporal) {
	if(estadoFileSystemIgualA(ESTABLE))
		servidorReconectarDataNode(servidor, nodoTemporal);
	else
		servidorAdmitirDataNode(servidor, nodoTemporal);
}

void servidorAdmitirDataNode(Servidor* servidor, Nodo* nodoTemporal) {
	if(nodoEstaConectado(nodoTemporal))
		servidorRechazarDataNode(nodoTemporal);
	else
		servidorAceptarDataNode(servidor, nodoTemporal);
}

void servidorAceptarDataNode(Servidor* servidor, Nodo* nodoTemporal) {
	servidorRegistrarDataNode(servidor, nodoTemporal);
	nodoListaAgregar(nodoTemporal);
}

void servidorRechazarDataNode(Nodo* nuevoNodo) {
	socketCerrar(nuevoNodo->socket);
	nodoDestruir(nuevoNodo);
}

void servidorAceptarReconexionDataNode(Servidor* servidor, Nodo* nuevoNodo) {
	semaforoWait(semaforoTarea);
	Nodo* nodo = nodoActualizar(nuevoNodo);
	servidorRegistrarDataNode(servidor, nodo);
	if(estadoFileSystemIgualA(INESTABLE) && archivoListaTodosDisponibles()) {
		mutexBloquear(mutexEstadoFileSystem);
		estadoFileSystem = ESTABLE;
		mutexDesbloquear(mutexEstadoFileSystem);
		imprimirMensaje(archivoLog, "[ESTADO] El File System se encuentra estable");
	}
	semaforoSignal(semaforoTarea);
}

void servidorRegistrarDataNode(Servidor* servidor, Nodo* nodo) {
	servidorAceptarConexion(servidor, nodo->socket);
	listaSocketsAgregar(nodo->socket, &servidor->listaDataNodes);
	nodoAceptar(nodo);
}

void servidorMensajeDataNode(Servidor* servidor, Mensaje* mensaje, Socket unSocket) {
	switch(mensaje->header.operacion) {
	case LEER_BLOQUE: bloqueLeer(servidor, mensaje->datos); break;
	case COPIAR_BLOQUE: bloqueCopiar(mensaje->datos); break;
	case COPIAR_BINARIO: bloqueCopiarBinario(mensaje->datos); break;
	case COPIAR_TEXTO: bloqueCopiarTexto(mensaje->datos); break;
	case FINALIZAR_NODO:
		mutexBloquear(mutexRuta); puts("finalizar entre");servidorFinalizarDataNode(servidor, unSocket);
	mutexDesbloquear(mutexRuta); puts("finalizar sali");
	break;
	}
}

void servidorDesactivarDataNode(Servidor* servidor, Nodo* nodo) {
	if(flagMensaje == DESACTIVADO) {
		socketCerrar(nodo->socket);
		listaSocketsEliminar(nodo->socket, &servidor->listaDataNodes);
		listaSocketsEliminar(nodo->socket, &servidor->listaMaster);
		nodo->estado = DESACTIVADO;
		nodo->socket = ERROR;
		imprimirMensajeUno(archivoLog, AMARILLO"[CONEXION] %s desconectado"BLANCO, nodo->nombre);
	}
	else {
		Socket* unSocket = memoriaAlocar(sizeof(Socket));
		*unSocket = nodo->socket;
		socketAgregar(unSocket);
	}
}

void servidorControlarFinalizacionDataNode(Servidor* servidor, Nodo* nodo) {
	if(estadoFileSystemIgualA(ESTABLE))
		servidorDesactivarDataNode(servidor, nodo);
	else
		nodoListaEliminar(nodo);
}

void servidorFinalizarDataNode(Servidor* servidor, Socket unSocket) {
	Nodo* nodo = nodoBuscarPorSocket(unSocket);
	if(estadoEjecucionIgualA(NORMAL))
		servidorDesactivarDataNode(servidor, nodo);
	else
		servidorControlarFinalizacionDataNode(servidor, nodo);
}

//--------------------------------------- Servidor Yama -------------------------------------

void servidorSolicitudYama(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket = socketAceptar(unSocket, ID_YAMA);
	if(estadoFileSystemIgualA(ESTABLE)) {
		servidor->yama = nuevoSocket;
		socketYama = nuevoSocket;
		servidorAceptarConexion(servidor, nuevoSocket);
		mensajeEnviar(nuevoSocket, ACEPTAR_YAMA, &nuevoSocket, sizeof(Socket));
		imprimirMensaje(archivoLog, "[CONEXION] El proceso Yama se ha conectado");
	}
	else {
		socketCerrar(nuevoSocket);
		imprimirMensaje(archivoLog, AMARILLO"[AVISO] El proceso Yama no pudo conectarse, el File System no es estable"BLANCO);
	}
}


void servidorMensajeYama(Mensaje* mensaje) {
	switch(mensaje->header.operacion) {
		case ENVIAR_BLOQUES: archivoEnviarBloques(mensaje->datos);
	}
}

void servidorFinalizarYama() {
	imprimirMensaje(archivoLog, "[CONEXION] El proceso YAMA se ha desconectado");
}

//--------------------------------------- Servidor Worker -------------------------------------

void servidorSolicitudWorker(Servidor* servidor, Socket unSocket) {
	//TODO
}

void servidorMensajeWorker() {
	//TODO
}

void servidorFinalizarWorker(Servidor* servidor, Socket unSocket) {
	//TODO
}

//--------------------------------------- Funciones de Socket -------------------------------------

bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaSelect);
}

bool socketEsDataNode(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaDataNodes);
}

bool socketEsWorker(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaWorkers);
}

bool socketEsYama(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->yama, unSocket);
}

bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerDataNode, unSocket);
}

bool socketEsListenerYama(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerYama, unSocket);
}

bool socketEsListenerWorker(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerWorker, unSocket);
}

bool socketEsListener(Servidor* servidor, Socket unSocket) {
	return socketEsListenerDataNode(servidor, unSocket) ||
			socketEsListenerYama(servidor, unSocket) ||
			socketEsListenerWorker(servidor, unSocket);
}

void socketAgregar(Socket* unSocket) {
	mutexBloquear(mutexListaSockets);
	listaAgregarElemento(listaSockets, unSocket);
	mutexDesbloquear(mutexListaSockets);
}

int socketCantidad() {
	mutexBloquear(mutexListaSockets);
	int cantidad = listaCantidadElementos(listaSockets);
	mutexDesbloquear(mutexListaSockets);
	return cantidad;
}

void socketDestruir() {
	mutexBloquear(mutexListaSockets);
	listaDestruirConElementos(listaSockets, memoriaLiberar);
	mutexDesbloquear(mutexListaSockets);
}

Socket* socketObtener(int indice) {
	mutexBloquear(mutexListaSockets);
	Socket* unSocket = listaObtenerElemento(listaSockets, indice);
	mutexDesbloquear(mutexListaSockets);
	return unSocket;
}

void socketEliminar(int posicion) {
	mutexBloquear(mutexListaSockets);
	listaEliminarDestruyendoElemento(listaSockets, posicion, memoriaLiberar);
	mutexDesbloquear(mutexListaSockets);
}


//--------------------------------------- Funciones de Consola -------------------------------------

bool consolaEntradaRespetaLimiteEspacios(String entrada) {
	int indice;
	int contador = 0;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO))
			contador++;
	return contador <=  4;
}

bool consolaEntradaSinEspacioEnExtremos(String entrada) {
	if(stringDistintos(entrada, VACIO))
		return caracterDistintos(entrada[0], ESPACIO) &&
				caracterDistintos(entrada[stringLongitud(entrada)-1],ESPACIO);
	else
		return false;
}

bool consolaEntradaEspaciosSeparados(String entrada) {
 	int indice;
	for(indice = 0; caracterDistintos(entrada[indice],FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO) && (caracterIguales(entrada[indice-1],ESPACIO) ||
			caracterIguales(entrada[indice+1],ESPACIO)))
			return false;
	return true;
}

bool consolaEntradaEspaciosBienUtilizados(String entrada) {
	return consolaEntradaSinEspacioEnExtremos(entrada) &&
			consolaEntradaEspaciosSeparados(entrada);

}

bool consolaEntradaSinTabs(String entrada) {
	int indice;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],TAB))
			return 0;
	return 1;
}

bool consolaEntradaTieneEspaciosNecesarios(String entrada) {
	return consolaEntradaRespetaLimiteEspacios(entrada) &&
			consolaEntradaSinTabs(entrada);
}

bool consolaEntradaLlena(String entrada) {
	return stringDistintos(entrada, VACIO);
}

bool consolaEntradaDecente(String entrada) {
	return consolaEntradaTieneEspaciosNecesarios(entrada) &&
			consolaEntradaEspaciosBienUtilizados(entrada) &&
			consolaEntradaLlena(entrada);
}

int consolaIdentificarComando(String comando) {
	if(stringNulo(comando))
		return ERROR;
	if(stringIguales(comando, FORMAT))
		return ID_FORMAT;
	if(stringIguales(comando, INFO))
		return ID_INFO;
	if(stringIguales(comando, RM))
		return ID_RM;
	if(stringIguales(comando, RENAME))
		return ID_RENAME;
	if(stringIguales(comando, MV))
		return ID_MV;
	if(stringIguales(comando, CAT))
		return ID_CAT;
	if(stringIguales(comando, MKDIR))
		return ID_MKDIR;
	if(stringIguales(comando, CPFROM))
		return ID_CPFROM;
	if(stringIguales(comando, CPTO))
		return ID_CPTO;
	if(stringIguales(comando, CPBLOCK))
		return ID_CPBLOCK;
	if(stringIguales(comando, MD5))
		return ID_MD5;
	if(stringIguales(comando, LS))
		return ID_LS;
	if(stringIguales(comando, INFO))
		return ID_INFO;
	if(stringIguales(comando, RENAME))
		return ID_RENAME;
	if(stringIguales(comando, NODES))
		return ID_NODES;
	if(stringIguales(comando, HELP))
		return ID_HELP;
	if(stringIguales(comando, EXIT))
		return ID_EXIT;
	else
		return ERROR;
}

bool consolaComandoTipoUno(String comando) {
	return stringIguales(comando, RM) ||
			stringIguales(comando, CAT) ||
			stringIguales(comando, MKDIR) ||
			stringIguales(comando, MD5) ||
			stringIguales(comando, LS) ||
			stringIguales(comando, INFO);
}

bool consolaComandoTipoDos(String comando) {
	return stringIguales(comando, RENAME) ||
			stringIguales(comando, MV) ||
			stringIguales(comando, CPTO);
}

bool consolaComandoTipoTres(String comando) {
	return stringIguales(comando, CPFROM) ||
			stringIguales(comando, CPBLOCK);
}

bool consolaComandoExiste(String comando) {
	return consolaComandoTipoUno(comando) ||
			consolaComandoTipoDos(comando) ||
			consolaComandoTipoTres(comando) ||
			stringIguales(comando, FORMAT) ||
			stringIguales(comando, EXIT) ||
			stringIguales(comando, HELP) ||
			stringIguales(comando, NODES);
}

bool consolaValidarComandoSinTipo(String* subcadenas) {
	return stringNulo(subcadenas[1]);
}

bool consolaValidarComandoTipoUno(String* subcadenas) {
	return stringValido(subcadenas[1]) &&
			stringNulo(subcadenas[2]);
}

bool consolaValidarComandoTipoDos(String* subcadenas) {
	return stringValido(subcadenas[1]) &&
			stringValido(subcadenas[2]) &&
		   stringNulo(subcadenas[3]);
}

bool consolaValidarComandoTipoTres(String* subcadenas) {
	return stringValido(subcadenas[1]) &&
		   stringValido(subcadenas[2]) &&
		   stringValido(subcadenas[3]) &&
		   stringNulo(subcadenas[4]);
}

bool consolaComandoEliminarFlagB(String* subcadenas) {
	return stringIguales(subcadenas[0], RM) &&
			stringIguales(subcadenas[1], FLAG_B);
}

bool consolaComandoEliminarFlagD(String* subcadenas) {
	return stringIguales(subcadenas[0], RM) &&
			stringIguales(subcadenas[1], FLAG_D);
}

bool consolaValidarComandoFlagB(String* subcadenas) {
	return stringValido(subcadenas[1]) &&
			   stringValido(subcadenas[2]) &&
			   stringValido(subcadenas[3]) &&
			   stringValido(subcadenas[4]);
}

bool consolaValidarComandoFlagD(String* subcadenas) {
	return stringValido(subcadenas[1]) &&
			   stringValido(subcadenas[2]) &&
			   stringNulo(subcadenas[3]) &&
			   stringNulo(subcadenas[4]);
}

bool consolaComandoControlarArgumentos(String* subcadenas) {
	if(consolaComandoEliminarFlagB(subcadenas))
		return consolaValidarComandoFlagB(subcadenas);
	if(consolaComandoEliminarFlagD(subcadenas))
		return consolaValidarComandoFlagD(subcadenas);
	if(consolaComandoTipoUno(subcadenas[0]))
		return consolaValidarComandoTipoUno(subcadenas);
	if(consolaComandoTipoDos(subcadenas[0]) )
		return consolaValidarComandoTipoDos(subcadenas);
	if(consolaComandoTipoTres(subcadenas[0]))
		return consolaValidarComandoTipoTres(subcadenas);
	else
		return consolaValidarComandoSinTipo(subcadenas);
}

bool consolaValidarComando(String* buffer) {
	if(consolaComandoExiste(buffer[0]))
		return consolaComandoControlarArgumentos(buffer);
	else
		return false;
}

bool consolaflagInvalido(String flag) {
	return stringDistintos(flag, FLAG_T) &&
			stringDistintos(flag, FLAG_B);
}

String consolaLeerEntrada() {
	int indice;
	int caracterLeido;
	char* cadena = memoriaAlocar(MAX_STRING);
	for(indice = 0; caracterDistintos((caracterLeido= caracterObtener()),ENTER); indice++)
		cadena[indice] = caracterLeido;
	cadena[indice] = FIN;
	return cadena;
}

void consolaIniciarArgumentos(String* argumentos) {
	argumentos[0] = NULL;
	argumentos[1] = NULL;
	argumentos[2] = NULL;
	argumentos[3] = NULL;
	argumentos[4] = NULL;
}

void consolaLiberarArgumentos(String* argumentos) {
	memoriaLiberar(argumentos[0]);
	memoriaLiberar(argumentos[1]);
	memoriaLiberar(argumentos[2]);
	memoriaLiberar(argumentos[3]);
	memoriaLiberar(argumentos[4]);
}

void consolaObtenerArgumentos(String* buffer, String entrada) {
	int indice;
	int indiceBuffer = 0;
	int ultimoEspacio = -1;
	for(indice = 0; caracterDistintos(entrada[indice], FIN); indice++)
		if(caracterIguales(entrada[indice],ESPACIO)) {
			buffer[indiceBuffer] = stringTomarCantidad(entrada, ultimoEspacio+1, indice-ultimoEspacio-1);
			ultimoEspacio = indice;
			indiceBuffer++;
		}
	buffer[indiceBuffer] = stringTomarCantidad(entrada, ultimoEspacio+1, indice-ultimoEspacio-1);
}

void consolaSetearArgumentos(String* argumentos, String* buffer) {
	argumentos[0]=buffer[1];
	argumentos[1]=buffer[2];
	argumentos[2]=buffer[3];
}

void consolaConfigurarComando(Comando* comando, String entrada) {
	consolaIniciarArgumentos(comando->argumentos);
	if(consolaEntradaDecente(entrada)) {
		consolaObtenerArgumentos(comando->argumentos, entrada);
		if(consolaValidarComando(comando->argumentos))
			comando->identificador = consolaIdentificarComando(comando->argumentos[0]);
		else
			comando->identificador = ERROR;
	}
	else {
		comando->identificador = ERROR;
	}
}

void consolaEjecutarComando(Comando* comando) {
	switch(comando->identificador) {
		case ID_FORMAT: comandoFormatearFileSystem(); break;
		case ID_RM: comandoEliminar(comando); break;
		case ID_RENAME: comandoRenombrar(comando); break;
		case ID_MV: comandoMover(comando); break;
		case ID_CAT: comandoMostrarArchivo(comando); break;
		case ID_MKDIR: comandoCrearDirectorio(comando); break;
		case ID_CPFROM: comandoCopiarArchivoAYamaFS(comando); break;
		case ID_CPTO: comandoCopiarArchivoDeYamaFS(comando); break;
		case ID_CPBLOCK: comandoCopiarBloque(comando); break;
		case ID_MD5: comandoObtenerMD5DeArchivo(comando); break;
		case ID_LS: comandoListarDirectorio(comando); break;
		case ID_INFO: comandoInformacionArchivo(comando); break;
		case ID_NODES: comandoInformacionNodos(); break;
		case ID_HELP: comandoAyuda(); break;
		case ID_EXIT: comandoFinalizar();break;
		default: comandoError(); break;
	}
}

void consolaDestruirComando(Comando* comando, String entrada) {
	consolaLiberarArgumentos(comando->argumentos);
	memoriaLiberar(entrada);
}

void consolaLaburar() {
	char* entrada = consolaLeerEntrada();
	if(estadoControlIgualA(ACTIVADO)) {
		Comando comando;
		consolaConfigurarComando(&comando, entrada);
		consolaEjecutarComando(&comando);
		consolaDestruirComando(&comando, entrada);
	}
}

void consolaAtenderComandos() {
	hiloDetach(pthread_self());
	while(estadoControlIgualA(ACTIVADO))
		consolaLaburar();
	semaforoSignal(semaforoFinal);
}

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem() {
	archivoListaDestruir();
	directorioListaDestruir();
	bitmapDirectoriosDestruir();
	metadataIniciar();
	nodoFormatearConectados();
	estadoFileSystemEstable();
	//TODO Ponerlo lindo
	/*
	int cantidadNodos = listaCantidadElementos(listaNodos);
	ConexionNodo nodos[cantidadNodos];
	int indice;
	for(indice=0; indice<cantidadNodos; indice++) {
		Nodo* nodo = listaObtenerElemento(listaNodos, indice);
		stringCopiar(nodos[indice].ip, nodo->ip);
		stringCopiar(nodos[indice].puerto, nodo->puerto);
	}
//	mensajeEnviar(socketYama, 201, nodos, cantidadNodos*sizeof(ConexionNodo));
	*/
	imprimirMensaje(archivoLog, "[ESTADO] El File System ha sido formateado");
}

void comandoCrearDirectorio(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio raiz no puede ser creado");
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo != NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo o directorio ya existe");
		return;
	}
	ControlDirectorio* control = directorioControlCrear(comando->argumentos[1]);
	while(stringValido(control->nombreDirectorio)) {
		directorioBuscarIdentificador(control);
		if (directorioExisteIdentificador(control->identificadorDirectorio))
			directorioControlarEntradas(control, comando->argumentos[1]);
		else if(directorioHaySuficientesIndices(control)) {
			int estado = directorioCrearDirectoriosRestantes(control, comando->argumentos[1]);
			if(estado == ERROR) {
				imprimirMensaje(archivoLog,"[ERROR] El nombre del directorio es demasiado largo");
				break;
			}
		}
		else {
			imprimirMensaje(archivoLog,"[ERROR] Se alcanzo el limite de directorios permitidos (100)");
			break;
		}
		directorioControlSetearNombre(control);
	}
	int indice;
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
}

void comandoRenombrar(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio raiz no puede ser renombrado");
		return;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL && directorio == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo o directorio no existe");
		return;
	}
	if(directorio != NULL) {
		int existeArchivo = archivoExiste(directorio->identificadorPadre, comando->argumentos[2]);
		if(directorioExiste(directorio->identificadorPadre, comando->argumentos[2]) || existeArchivo) {
			imprimirMensaje(archivoLog, "[ERROR] El nuevo nombre para el directorio ya existe");
			return;
		}
		stringCopiar(directorio->nombre, comando->argumentos[2]);
		directorioPersistir();
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El directorio %s fue renombrado por %s", comando->argumentos[1], directorio->nombre);
	}
	else {
		int existeDirectorio = directorioExiste(archivo->identificadorPadre, comando->argumentos[2]);
		if(archivoExiste(archivo->identificadorPadre, comando->argumentos[2]) || existeDirectorio) {
			imprimirMensaje(archivoLog, "[ARCHIVO] El nuevo nombre para el archivo ya existe");
			return;
		}
		String antiguaRuta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
		String nuevaRuta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, comando->argumentos[2]);
		rename(antiguaRuta, nuevaRuta);
		memoriaLiberar(antiguaRuta);
		memoriaLiberar(nuevaRuta);
		stringCopiar(archivo->nombre, comando->argumentos[2]);
		archivoPersistir(archivo);
		archivoPersistirControl();
		imprimirMensajeDos(archivoLog, "[ARCHIVO] El archivo %s fue renombrado por %s", comando->argumentos[1], archivo->nombre);

	}
}

void comandoMover(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(!rutaValida(comando->argumentos[2])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio raiz no puede ser movido");
		return;
	}
	Directorio* directorioNuevoPadre;
	if(stringIguales(comando->argumentos[2], "/"))
		directorioNuevoPadre = directorioBuscarEnLista(0);
	else
		directorioNuevoPadre = directorioBuscar(comando->argumentos[2]);
	if(directorioNuevoPadre == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio destino no existe");
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	if(archivo == NULL && directorio == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo o directorio no existe");
		return;
	}
	if(directorio != NULL) {
		int existeArchivo = archivoExiste(directorioNuevoPadre->identificador, directorio->nombre);
		if(directorioExiste(directorioNuevoPadre->identificador, directorio->nombre) || existeArchivo)  {
			imprimirMensaje(archivoLog, "[ERROR] La nueva ubicacion del archivo o directorio ya existe");
			return;
		}
		if(directorioEsHijoDe(directorioNuevoPadre, directorio)) {
			imprimirMensaje(archivoLog, "[ERROR] El directorio no puede moverse a uno de sus subdirectorios");
			return;
		}
		if(directorioNuevoPadre->identificador == directorio->identificador) {
			imprimirMensaje(archivoLog, "[ERROR] El directorio no puede moverse a si mismo");
			return;
		}

		directorio->identificadorPadre = directorioNuevoPadre->identificador;
		directorioPersistir();
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El directorio %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
	else {
		int existeDirectorio = directorioExiste(directorioNuevoPadre->identificador, archivo->nombre);
		if(archivoExiste(directorioNuevoPadre->identificador, archivo->nombre) || existeDirectorio) {
			imprimirMensaje(archivoLog, "[ERROR] La nueva ubicacion del archivo o directorio ya existe");
			return;
		}
		String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
		fileLimpiar(rutaArchivo);
		memoriaLiberar(rutaArchivo);
		archivo->identificadorPadre = directorioNuevoPadre->identificador;
		archivoPersistir(archivo);
		archivoPersistirControl();
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El archivo %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
}

void comandoEliminar(Comando* comando) {
	if(stringIguales(comando->argumentos[1], FLAG_D))
		comandoEliminarDirectorio(comando);
	else if(stringIguales(comando->argumentos[1], FLAG_B))
		comandoEliminarCopia(comando);
	else
		comandoEliminarArchivo(comando);
}

void comandoEliminarCopia(Comando* comando) {
	if(!rutaValida(comando->argumentos[2])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[2], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(!rutaEsNumero(comando->argumentos[3])) {
		imprimirMensaje(archivoLog,"[ERROR] El numero de bloque no es valido");
		return;
	}
	if(!rutaEsNumero(comando->argumentos[4])) {
		imprimirMensaje(archivoLog,"[ERROR] El numero de copia del bloque no es valido");
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[2]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo no existe");
		return;
	}
	int numeroBloque = atoi(comando->argumentos[3]);
	int numeroCopia = atoi(comando->argumentos[4]);
	Bloque* bloque = listaObtenerElemento(archivo->listaBloques, numeroBloque);

	if(bloque == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El numero de bloque no existe");
		return;
	}
	Copia* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
	if(copia == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El numero de copia no existe");
		return;
	}
	if(listaCantidadElementos(bloque->listaCopias) == 1) {
		imprimirMensaje(archivoLog, "[ERROR] No se puede eliminar la copia ya que es la ultima");
		return;
	}
	listaEliminarDestruyendoElemento(bloque->listaCopias, numeroCopia, (Puntero)copiaDestruir);
	archivoPersistir(archivo);
	nodoPersistir();
	imprimirMensajeTres(archivoLog, "[BLOQUE] La copia N°%i del bloque N°%i del archivo %s ha sido eliminada",(int*)numeroCopia, (int*)numeroBloque, comando->argumentos[2]);
}


void comandoEliminarDirectorio(Comando* comando) {
	String ruta = comando->argumentos[2];
	if(!rutaValida(ruta)) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(ruta, "/")) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio raiz no puede ser eliminado");
		return;
	}
	Directorio* directorio = directorioBuscar(ruta);
	if(directorio == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio no existe");
		return;
	}
	if(directorioTieneAlgo(directorio->identificador))
		imprimirMensajeUno(archivoLog,"[ERROR] El directorio %s no esta vacio",ruta);
	else {
		directorioEliminar(directorio->identificador);
		imprimirMensajeUno(archivoLog,"[DIRECTORIO] El directorio %s ha sido eliminado",ruta);
	}
}

void comandoEliminarArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo no existe");
		return;
	}
	int posicion = archivoListaPosicion(archivo);
	String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
	fileLimpiar(rutaArchivo);
	memoriaLiberar(rutaArchivo);
	mutexBloquear(mutexListaArchivos);
	listaEliminarDestruyendoElemento(listaArchivos, posicion, (Puntero)archivoDestruir);
	mutexDesbloquear(mutexListaArchivos);
	archivoPersistirControl();
	nodoPersistir();
	imprimirMensajeUno(archivoLog, "[ARCHIVO] El archivo %s ha sido eliminado", comando->argumentos[1]);
}

void comandoListarDirectorio(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		directorioMostrarArchivos(0);
		return;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	if(directorio != NULL)
		directorioMostrarArchivos(directorio->identificador);
	else
		imprimirMensaje(archivoLog, "[ERROR] El directorio no existe");
}

void comandoInformacionArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo no existe");
		return;
	}
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Nombre: %s", archivo->nombre);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Tipo: %s", archivo->tipo);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Ubicacion: %s", comando->argumentos[1]);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] ID Padre: %i", (int*)archivo->identificadorPadre);
	int indice;
	int tamanio = 0;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		tamanio+= bloque->bytesUtilizados;
	}
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Tamanio: %i bytes", (int*)tamanio);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Bloques: %i", (int*)listaCantidadElementos(archivo->listaBloques));
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		imprimirMensajeDos(archivoLog, "[ARCHIVO] Bloque %i: %i bytes", (int*)indice, (int*)bloque->bytesUtilizados);
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			Copia* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			imprimirMensajeTres(archivoLog, "[ARCHIVO] Copia %i en: Nodo: %s | Bloque: %i", (int*)indiceCopia, copiaBloque->nombreNodo, (int*)copiaBloque->bloqueNodo);
		}
	}
}

void comandoCopiarBloque(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	mutexBloquear(mutexArchivo);
	archivoBuffer = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivoBuffer == NULL) {
		mutexDesbloquear(mutexArchivo);
		imprimirMensaje(archivoLog, "[ERROR] El archivo no existe");
		return;
	}
	mutexDesbloquear(mutexArchivo);
	if(!rutaEsNumero(comando->argumentos[2])) {
		imprimirMensaje(archivoLog,"[ERROR] El numero de bloque no es valido");
		return;
	}
	mutexBloquear(mutexNodo);
	nodoBuffer = nodoBuscarPorNombre(comando->argumentos[3]);
	if(nodoBuffer == NULL) {
		mutexDesbloquear(mutexNodo);
		imprimirMensaje(archivoLog,"[ERROR] El nodo no existe");
		return;
	}
	mutexDesbloquear(mutexNodo);
	Entero numeroBloque = atoi(comando->argumentos[2]);
	mutexBloquear(mutexArchivo);
	mutexBloquear(mutexBloque);
	bloqueBuffer = listaObtenerElemento(archivoBuffer->listaBloques, numeroBloque);
	mutexDesbloquear(mutexArchivo);
	if(bloqueBuffer == NULL) {
		mutexDesbloquear(mutexBloque);
		imprimirMensaje(archivoLog, "[ERROR] El numero de bloque no existe");
		return;
	}
	mutexDesbloquear(mutexBloque);
	mutexBloquear(mutexNodo);
	if(nodoBuscarBloqueLibre(nodoBuffer) == ERROR) {
		mutexDesbloquear(mutexNodo);
		imprimirMensaje(archivoLog, "[ERROR] No hay bloques libres en el nodo");
		return;
	}
	mutexDesbloquear(mutexNodo);
	mutexBloquear(mutexBloque);
	if(listaEstaVacia(bloqueBuffer->listaCopias)) {
		mutexDesbloquear(mutexBloque);
		imprimirMensaje(archivoLog, "[ERROR] El bloque no tiene copias en ningun nodo (NUNCA deberia pasar)");
		return;
	}
	mutexDesbloquear(mutexBloque);
	if(!nodoConectado(nodoBuffer)) {
		mutexDesbloquear(mutexNodo);
		imprimirMensaje(archivoLog, "[ERROR] El nodo que va a guardar la nueva copia no esta conectado");
		return;
	}
	int indice;
	int bloqueSinEnviar = true;
	mutexBloquear(mutexBloque);
	int cantidadCopias = listaCantidadElementos(bloqueBuffer->listaCopias);
	mutexDesbloquear(mutexBloque);
	for(indice = 0; indice < cantidadCopias && bloqueSinEnviar; indice++) {
		mutexBloquear(mutexBloque);
		Copia* copia = listaObtenerElemento(bloqueBuffer->listaCopias, indice);
		mutexDesbloquear(mutexBloque);
		semaforoWait(semaforoTarea);
		Nodo* nodoConBloque = nodoBuscarPorNombre(copia->nombreNodo);
		if(nodoConectado(nodoConBloque)) {
			mensajeEnviar(nodoConBloque->socket, COPIAR_BLOQUE, &copia->bloqueNodo, sizeof(Entero));
			bloqueSinEnviar = false;
		}
		semaforoSignal(semaforoTarea);
	}
	if(bloqueSinEnviar) {
		imprimirMensaje(archivoLog, "[ERROR] No hay nodos conectados para obtener la copia");

		return;
	}
}

void comandoCopiarArchivoAYamaFS(Comando* comando) {
	archivoAlmacenar(comando);
}

int comandoCopiarArchivoDeYamaFS(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return ERROR;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo no existe");
		return ERROR;
	}
	if(!archivoDisponible(archivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no esta disponible"BLANCO);
		return ERROR;
	}
	File file = fileAbrir(comando->argumentos[2], ESCRITURA);
	if(file == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] La ruta es invalida");
		return ERROR;
	}
	fileCerrar(file);
	mutexBloquear(mutexRuta);
	stringCopiar(rutaBuffer, comando->argumentos[2]);
	mutexDesbloquear(mutexRuta);
	int indice;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques) ;indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		int copiaSinEnviar = true;
		int indiceCopias;
		listaOrdenar(bloque->listaCopias, (Puntero)copiaOrdenarPorActividadDelNodo);
		for(indiceCopias=0; indiceCopias<listaCantidadElementos(bloque->listaCopias) && copiaSinEnviar; indiceCopias++) {
			semaforoWait(semaforoTarea);
			Copia* copia = listaObtenerElemento(bloque->listaCopias, indiceCopias);
			Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
			if(nodoConectado(nodo)) {
				if(archivoBinario(archivo))
					mensajeEnviar(nodo->socket, COPIAR_BINARIO, &copia->bloqueNodo, sizeof(Entero));
				else
					mensajeEnviar(nodo->socket, COPIAR_TEXTO, &copia->bloqueNodo, sizeof(Entero));
				nodo->actividadesRealizadas++;
				copiaSinEnviar = false;
			}
		}
		if(copiaSinEnviar) {
			imprimirMensaje(archivoLog, "[ERROR] Una copia no pudo ser enviada, se aborta la operacion");
			nodoLimpiarActividades();
			return ERROR;
		}
	}
	nodoLimpiarActividades();
	imprimirMensajeDos(archivoLog, "[ARCHIVO] El archivo %s se copio en %s", archivo->nombre, comando->argumentos[2]);
	return NULO;
}

void comandoMostrarArchivo(Comando* comando) {
	archivoLeer(comando);
}

void comandoObtenerMD5DeArchivo(Comando* comando) {
	String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[1]);
	String MD5Archivo = memoriaAlocar(MAX_STRING);
	String ruta = string_from_format("%s/%s", configuracion->rutaMetadata, nombreArchivo);
	comando->argumentos[2] = memoriaAlocar(MAX_STRING);
	stringCopiar(comando->argumentos[2], ruta);
	if(comandoCopiarArchivoDeYamaFS(comando) == ERROR) {
		fileLimpiar(ruta);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(MD5Archivo);
		memoriaLiberar(ruta);
		return;
	}
	int pidHijo;
	int longitudMensaje;
	int descriptores[2];
	pipe(descriptores);
	pidHijo = fork();
	if(pidHijo == ERROR)
		imprimirMensaje(archivoLog, "[ERROR] Fallo el fork (Estas en problemas amigo)");
	else if(pidHijo == 0) {
		close(descriptores[0]);
		close(1);
		dup2(descriptores[1], 1);
		execlp("/usr/bin/md5sum", "md5sum", ruta, NULL);
		close(descriptores[1]);
	} if(pidHijo > 0) {
		close(descriptores[1]);
		wait(NULL);
		int bytesLeidos;
		while((bytesLeidos = read(descriptores[0], MD5Archivo, MAX_STRING)) > 0)
			if(bytesLeidos > 0)
				longitudMensaje = bytesLeidos;
		close(descriptores[0]);
	}
	memcpy(MD5Archivo+longitudMensaje, "\0", 1);
	printf("[ARCHIVO] El MD5 del archivo es %s", MD5Archivo);
	log_info(archivoLog,"[ARCHIVO] El MD5 del archivo es %s", MD5Archivo);
	fileLimpiar(ruta);
	memoriaLiberar(ruta);
	memoriaLiberar(MD5Archivo);
	memoriaLiberar(nombreArchivo);
}

void comandoFinalizar() {
	 mutexBloquear(mutexEstadoControl);
	 estadoControl = DESACTIVADO;
	 mutexDesbloquear(mutexEstadoControl);
	 socketCrearCliente(IP_LOCAL, configuracion->puertoDataNode, ID_DATANODE);
}

void comandoInformacionNodos() {
	int indice;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		puts("------------------------------------------");
		printf("Nombre: %s\n", nodo->nombre);
		printf("IP: %s | Puerto: %s\n", nodo->ip, nodo->puerto);
		printf("Bloques totales %i\n", nodo->bloquesTotales);
		printf("Bloques libres %i\n", nodo->bloquesLibres);
		if(nodoConectado(nodo))
			puts("Estado: Conectado");
		else
			puts("Estado: Desconectado");
	}
}

void comandoAyuda() {
	puts("-------------------------------");
	puts("1)  format | Formatea YamaFS");
	puts("2)  mkdir <Path-dir> | Crea un directorio");
	puts("3)  ls <Path-dir> | Lista un directorio");
	puts("4)  rename <Path-dir> <Nuevo-nombre> | Renombra un archivo o directorio");
	puts("5)  mv <Path-dir> <Path-nuevo-dir> | Renombra un archivo o directorio");
	puts("6)  rm <Path-arch> | Elimina un archivo");
	puts("7)  rm -b <Path-arch> <N°bloque> <N°copia> | Elimina un bloque de un archivo");
	puts("8)  rm -d <Path-dir> | Elimina un directorio");
	puts("9)  cpto <Path-Yama> <Path-FS> | Copia un archivo de YamaFS al FS local");
	puts("10) cpfrom -t  <Path-arch-local> <Path-dir> | Copia un archivo de texto a YamaFS");
	puts("11) cpfrom -b <Path-arch-local> <Path-dir> | Copia un archivo binario a YamaFS");
	puts("12) md5 <Path-arch> | Retorna el MD5 del archivo");
	puts("13) cat <Path-arch> | Muestra el contenido de un archivo");
	puts("14) info <Path-Arch> | Muestra la informacion de un archivo");
	puts("15) nodes | Muestra la informacion de los nodos");
	puts("16) exit | Finaliza YamaFS");
}

void comandoError() {
	imprimirMensaje(archivoLog, "[ERROR] Comando invalido");
}

//--------------------------------------- Funciones de Directorio -------------------------------------

Directorio* directorioCrear(int indice, String nombre, int padre) {
	Directorio* directorio = memoriaAlocar(sizeof(Directorio));
	directorio->identificador = indice;
	directorio->identificadorPadre = padre;
	stringCopiar(directorio->nombre, nombre);
	return directorio;
}

Directorio* directorioBuscar(String path) {
	int identificador = directorioObtenerIdentificador(path);
	Directorio* directorio = directorioBuscarEnLista(identificador);
	return directorio;
}

void directorioMostrarArchivos(int identificadorPadre) {
	int indice;
	int flag = DESACTIVADO;
	for(indice=0; indice<directorioListaCantidad(); indice++) {
		Directorio* directorio = directorioListaObtener(indice);
		if(directorio->identificadorPadre == identificadorPadre) {
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (d)", directorio->nombre);
			flag = ACTIVADO;
		}
	}
	for(indice=0; indice < archivoListaCantidad(); indice++) {
		Archivo* archivo = archivoListaObtener(indice);
		if(archivo->identificadorPadre == identificadorPadre) {
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (a)", archivo->nombre);
			flag = ACTIVADO;
		}
	}
	if(flag == DESACTIVADO)
		imprimirMensaje(archivoLog, "[DIRECTORIO] El directorio esta vacio");
}

int directorioBuscarIdentificadorLibre() {
	int indice;
	for(indice = 0; directorioIndiceEstaOcupado(indice); indice++);
	if(indice < 100)
		return indice;
	else
		return ERROR;
}

String directorioConfigurarEntradaArchivo(String indice, String nombre, String padre) {
	String buffer = stringCrear(MAX_STRING);
	stringConcatenar(buffer,indice);
	stringConcatenar(buffer, ";");
	stringConcatenar(buffer, nombre);
	stringConcatenar(buffer,";");
	stringConcatenar(buffer,padre);
	stringConcatenar(buffer,"\n");
	return buffer;
}

void directorioBuscarIdentificador(ControlDirectorio* control) {
	Directorio* directorio;
	int indice;
	for(indice = 0; indice < directorioListaCantidad(); indice++) {
		directorio = directorioListaObtener(indice);
		if(directorioSonIguales(directorio, control->nombreDirectorio, control->identificadorPadre)) {
			control->identificadorDirectorio = directorio->identificador;
			return;
		}
	}
	control->identificadorDirectorio = ERROR;
}

int directorioIndicesACrear(String* nombresDirectorios, int indiceDirectorios) {
	int directoriosACrear = 0;
	int indiceDirectoriosNuevos;
	for(indiceDirectoriosNuevos = indiceDirectorios; stringValido(nombresDirectorios[indiceDirectoriosNuevos]); indiceDirectoriosNuevos++)
		directoriosACrear++;
	return directoriosACrear;
}

ControlDirectorio* directorioControlCrear(String rutaDirectorio) {
	ControlDirectorio* controlDirectorio = memoriaAlocar(sizeof(ControlDirectorio));
	controlDirectorio->nombresDirectorios = rutaSeparar(rutaDirectorio);
	controlDirectorio->indiceNombresDirectorios = 0;
	controlDirectorio->identificadorDirectorio = 0;
	controlDirectorio->identificadorPadre = 0;
	directorioControlSetearNombre(controlDirectorio);
	return controlDirectorio;
}

void directorioControlarEntradas(ControlDirectorio* control, String path) {
	if(control->nombresDirectorios[control->indiceNombresDirectorios + 1] == NULL)
		imprimirMensaje(archivoLog, "[ERROR] El archivo o directorio ya existe");
	else
		control->identificadorPadre = control->identificadorDirectorio;
	control->indiceNombresDirectorios++;
}

void directorioCrearMetadata(Entero identificador) {
	String directorio = string_from_format("%s/%i", rutaDirectorioArchivos, identificador);
	mkdir(directorio, 0777);
	memoriaLiberar(directorio);
}

void directorioEliminarMetadata(Entero identificador) {
	String directorio = string_from_format("%s/%i", rutaDirectorioArchivos, identificador);
	fileLimpiar(directorio);
	memoriaLiberar(directorio);
}

int directorioCrearConPersistencia(int identificador, String nombre, int identificadorPadre) {
	if(stringLongitud(nombre) >= MAX_NOMBRE)
		return ERROR;
	Directorio* directorio = directorioCrear(identificador, nombre, identificadorPadre);
	bitmapDirectoriosOcuparBit(identificador);
	directorioListaAgregar(directorio);
	directoriosDisponibles--;
	directorioCrearMetadata(directorio->identificador);
	directorioPersistir();
	return 0;
}

int directorioCrearDirectoriosRestantes(ControlDirectorio* control, String rutaDirectorio) {
	while(stringValido(control->nombresDirectorios[control->indiceNombresDirectorios])) {
		int indice = directorioBuscarIdentificadorLibre();
		int estado = directorioCrearConPersistencia(indice, control->nombresDirectorios[control->indiceNombresDirectorios], control->identificadorPadre);
		if(estado == ERROR)
			return ERROR;
		control->identificadorPadre = indice;
		control->indiceNombresDirectorios++;
	}
	imprimirMensajeUno(archivoLog, "[DIRECTORIO] El directorio %s fue creado", rutaDirectorio);
	return 0;
}

bool directorioExiste(int idPadre, String nuevoNombre) {

	bool existeNuevoNombre(Directorio* directorio) {
		return idPadre == directorio->identificadorPadre &&
				stringIguales(nuevoNombre, directorio->nombre);
	}
	mutexBloquear(mutexListaDirectorios);
	int resultado = listaCumpleAlguno(listaDirectorios, (Puntero)existeNuevoNombre);
	mutexDesbloquear(mutexListaDirectorios);
	return resultado;
}

int directorioObtenerIdentificador(String path) {
	if(stringIguales(path, "/"))
		return 0;
	int id = ERROR;
	int indice;
	ControlDirectorio* control = directorioControlCrear(path);
	while(stringValido(control->nombreDirectorio)) {
		directorioBuscarIdentificador(control);
		if(control->identificadorDirectorio != ERROR) {
			if(control->nombresDirectorios[control->indiceNombresDirectorios + 1] == NULL) {
				id = control->identificadorDirectorio;
				break;
			}
			else
				control->identificadorPadre = control->identificadorDirectorio;
			control->indiceNombresDirectorios++;
			directorioControlSetearNombre(control);
		}
		else
			break;
	}
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
	return id;
}

Directorio* directorioBuscarEnLista(int identificadorDirectorio) {

	bool buscarPorId(Directorio* directorio) {
		return directorio->identificador == identificadorDirectorio;
	}
	mutexBloquear(mutexListaDirectorios);
	Directorio* directorio = listaBuscar(listaDirectorios, (Puntero)buscarPorId);
	mutexDesbloquear(mutexListaDirectorios);
	return directorio;
}

bool directorioTieneAlgunArchivo(int identificador) {

	bool archivoEsHijo(Archivo* archivo) {
		return archivo->identificadorPadre == identificador;
	}

	mutexBloquear(mutexListaArchivos);
	int resultado = listaCumpleAlguno(listaArchivos, (Puntero)archivoEsHijo);
	mutexDesbloquear(mutexListaArchivos);
	return resultado;
}

bool directorioTieneAlgunDirectorio(int identificador) {

	bool directorioEsHijo(Directorio* directorio) {
		return directorio->identificadorPadre == identificador;
	}
	mutexBloquear(mutexListaDirectorios);
	int resultado = listaCumpleAlguno(listaDirectorios, (Puntero)directorioEsHijo);
	mutexDesbloquear(mutexListaDirectorios);
	return resultado;
}

bool directorioTieneAlgo(int identificador) {
	return directorioTieneAlgunDirectorio(identificador) ||
			directorioTieneAlgunArchivo(identificador);
}

void directorioEliminar(int identificador) {

	bool tieneElMismoId(Directorio* directorio) {
		return directorio->identificador == identificador;
	}
	mutexBloquear(mutexListaDirectorios);
	listaEliminarDestruyendoPorCondicion(listaDirectorios, (Puntero)tieneElMismoId, (Puntero)memoriaLiberar);
	mutexDesbloquear(mutexListaDirectorios);
	mutexBloquear(mutexBitmapDirectorios);
	bitmapLiberarBit(bitmapDirectorios, identificador);
	mutexDesbloquear(mutexBitmapDirectorios);
	directoriosDisponibles++;
	directorioEliminarMetadata(identificador);
	directorioPersistir();
}

bool directorioIndiceEstaOcupado(int indice) {
	return directorioIndiceRespetaLimite(indice) &&
			bitmapDirectoriosBitOcupado(indice);
}

bool directorioIndiceRespetaLimite(int indice) {
	return indice < 100;
}

bool directorioExisteIdentificador(int identificador) {
	return identificador != ERROR;
}

bool directorioIndicesIguales(int unIndice, int otroIndice) {
	return unIndice == otroIndice;
}

bool directorioSonIguales(Directorio* directorio, String nombreDirectorio, int idPadre) {
	return stringIguales(directorio->nombre, nombreDirectorio) && directorioIndicesIguales(directorio->identificadorPadre, idPadre);
}

bool directorioHaySuficientesIndices(ControlDirectorio* control) {
	return directorioIndicesACrear(control->nombresDirectorios, control->indiceNombresDirectorios) <= directoriosDisponibles;
}

void directorioControlSetearNombre(ControlDirectorio* control) {
	control->nombreDirectorio = control->nombresDirectorios[control->indiceNombresDirectorios];
}

bool directorioEsHijoDe(Directorio* hijo, Directorio* padre) {
	return hijo->identificadorPadre == padre->identificador;
}

bool directorioOrdenarPorIdentificador(Directorio* unDirectorio, Directorio* otroDirectorio) {
	return unDirectorio->identificador < otroDirectorio->identificador;
}

void directorioListaAgregar(Directorio* directorio) {
	mutexBloquear(mutexListaDirectorios);
	listaAgregarElemento(listaDirectorios, directorio);
	mutexDesbloquear(mutexListaDirectorios);
}

int directorioListaCantidad() {
	mutexBloquear(mutexListaDirectorios);
	int cantidad = listaCantidadElementos(listaDirectorios);
	mutexDesbloquear(mutexListaDirectorios);
	return cantidad;
}

Directorio* directorioListaObtener(int posicion) {
	mutexBloquear(mutexListaDirectorios);
	Directorio* directorio = listaObtenerElemento(listaDirectorios, posicion);
	mutexDesbloquear(mutexListaDirectorios);
	return directorio;
}

void directorioListaCrear() {
	mutexBloquear(mutexListaDirectorios);
	listaDirectorios = listaCrear();
	mutexDesbloquear(mutexListaDirectorios);
}

void directorioListaDestruir() {
	mutexBloquear(mutexListaDirectorios);
	listaDestruirConElementos(listaDirectorios, memoriaLiberar);
	mutexDesbloquear(mutexListaDirectorios);
}

void directorioPersistir(){
	File archivoDirectorio = fileAbrir(rutaDirectorios, ESCRITURA);
	mutexBloquear(mutexListaDirectorios);
	listaOrdenar(listaDirectorios, (Puntero)directorioOrdenarPorIdentificador);
	mutexDesbloquear(mutexListaDirectorios);
	fprintf(archivoDirectorio, "DIRECTORIOS=%i\n", directorioListaCantidad());
	int indice;
	for(indice = 0; indice < directorioListaCantidad(); indice++) {
		Directorio* directorio = directorioListaObtener(indice);
		String lineaIdentificador = string_from_format("IDENTIFICADOR%i=%i\n", indice, directorio->identificador);
		String lineaNombre = string_from_format("NOMBRE%i=%s\n", indice, directorio->nombre);
		String lineaPadre = string_from_format("PADRE%i=%i\n", indice, directorio->identificadorPadre);
		fprintf(archivoDirectorio, "%s", lineaIdentificador);
		fprintf(archivoDirectorio, "%s", lineaNombre);
		fprintf(archivoDirectorio, "%s", lineaPadre);
		memoriaLiberar(lineaIdentificador);
		memoriaLiberar(lineaNombre);
		memoriaLiberar(lineaPadre);
	}
	fileCerrar(archivoDirectorio);
}

void directorioRecuperarPersistencia() {
	File file = fileAbrir(rutaArchivos, LECTURA);
	bitmapDirectoriosCrear();
	if(file == NULL) {
		imprimirMensajeUno(archivoLog, "[ERROR] No se encontro el archivo %s", rutaDirectorios);
		return;
	}
	fileCerrar(file);
	directorioListaCrear();
	ArchivoConfig config = config_create(rutaDirectorios);
	int cantidadDirectorios = archivoConfigEnteroDe(config, "DIRECTORIOS");
	int indice;
	for(indice = 0; indice < cantidadDirectorios; indice++) {
		String lineaIdentificador = string_from_format("IDENTIFICADOR%i", indice);
		String lineaNombre = string_from_format("NOMBRE%i", indice);
		String lineaPadre = string_from_format("PADRE%i", indice);
		Directorio* directorio = memoriaAlocar(sizeof(Directorio));
		directorio->identificador = archivoConfigEnteroDe(config, lineaIdentificador);
		directorio->identificadorPadre = archivoConfigEnteroDe(config, lineaPadre);
		stringCopiar(directorio->nombre, archivoConfigStringDe(config, lineaNombre));
		directorioListaAgregar(directorio);
		memoriaLiberar(lineaIdentificador);
		memoriaLiberar(lineaNombre);
		memoriaLiberar(lineaPadre);
	}
	archivoConfigDestruir(config);
	for(indice = 0; indice < cantidadDirectorios; indice++)
		bitmapDirectoriosOcuparBit(indice);
	directoriosDisponibles = MAX_DIR - cantidadDirectorios;
}

//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo) {
	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	stringCopiar(archivo->nombre, nombreArchivo);
	archivo->identificadorPadre = idPadre;
	if(stringIguales(tipo, FLAG_B))
		stringCopiar(archivo->tipo, ARCHIVO_BINARIO);
	else
		stringCopiar(archivo->tipo, ARCHIVO_TEXTO);
	archivo->listaBloques = listaCrear();
	return archivo;
}

void archivoDestruir(Archivo* archivo) {
	listaDestruirConElementos(archivo->listaBloques, (Puntero)bloqueDestruir);
	memoriaLiberar(archivo);
}

Archivo* archivoBuscarPorRuta(String ruta) {
	Archivo* archivo = NULL;
	String ultimo = rutaObtenerUltimoNombre(ruta);
	int indice;
	ControlDirectorio* control = directorioControlCrear(ruta);
	while(stringValido(control->nombreDirectorio)) {
		directorioBuscarIdentificador(control);
		if(control->identificadorDirectorio != ERROR) {
			control->identificadorPadre = control->identificadorDirectorio;
			control->indiceNombresDirectorios++;
			directorioControlSetearNombre(control);
		}
		else
			break;
	}

	bool buscarArchivo(Archivo* archivo) {
		return control->identificadorPadre == archivo->identificadorPadre &&
				stringIguales(control->nombreDirectorio, archivo->nombre);
	}

	if(stringIguales(control->nombreDirectorio, ultimo)) {
		mutexBloquear(mutexListaArchivos);
		archivo = listaBuscar(listaArchivos, (Puntero)buscarArchivo);
		mutexDesbloquear(mutexListaArchivos);
	}
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
	memoriaLiberar(ultimo);
	return archivo;
}

bool archivoExiste(int idPadre, String nombre) {

	bool existeNombre(Archivo* archivo) {
		return idPadre == archivo->identificadorPadre &&
				stringIguales(nombre, archivo->nombre);
	}

	mutexBloquear(mutexListaArchivos);
	int resultado = listaCumpleAlguno(listaArchivos, (Puntero)existeNombre);
	mutexDesbloquear(mutexListaArchivos);
	return resultado;
}

int archivoCantidadBloques(String ruta) {
	struct stat informacionArchivo;
	stat(ruta, &informacionArchivo);
	int tamanioArchivo = informacionArchivo.st_size;
	int cantidadBloques = (int)ceil((double)(tamanioArchivo)/(double)BLOQUE);
	return cantidadBloques;
}

bool archivoOrdenarPorNombre(Archivo* unArchivo, Archivo* otroArchivo) {
	return unArchivo->nombre[0] < otroArchivo->nombre[0];
}

bool archivoBinario(Archivo* archivo) {
	return stringIguales(archivo->tipo, ARCHIVO_BINARIO);
}

bool archivoDisponible(Archivo* archivo) {
	return listaCumplenTodos(archivo->listaBloques, (Puntero)bloqueDisponible);
}

int archivoAlmacenar(Comando* comando) {
	if(consolaflagInvalido(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, "[ERROR] Flag invalido");
		return ERROR;
	}
	if(!rutaValida(comando->argumentos[3])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return ERROR;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[3]);
	if(directorio == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El directorio ingresado no existe");
		return ERROR;
	}
	String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[2]);
	if(archivoExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, "[ERROR] Un archivo con ese nombre ya existe en el directorio destino");
		memoriaLiberar(nombreArchivo);
		return ERROR;
	}
	if(directorioExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, "[ERROR] Un directorio con ese nombre ya existe en el directorio destino");
		memoriaLiberar(nombreArchivo);
		return ERROR;
	}
	File file = fileAbrir(comando->argumentos[2], LECTURA);
	if(file == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo a copiar no existe");
		memoriaLiberar(nombreArchivo);
		return ERROR;
	}
	int cantidadBloques = nodoBloquesLibresTotales();
	int bloquesArchivo = archivoCantidadBloques(comando->argumentos[2]) * MAX_COPIAS;
	if(cantidadBloques < bloquesArchivo) {
		imprimirMensaje(archivoLog, "[ERROR] No hay suficientes bloques libres");
		fileCerrar(file);
		memoriaLiberar(nombreArchivo);
		return ERROR;
	}
	Archivo* archivo = archivoCrear(nombreArchivo, directorio->identificador, comando->argumentos[1]);
	memoriaLiberar(nombreArchivo);
	int copiasEnviadas = 0;
	if(stringIguales(comando->argumentos[1], FLAG_B)) {
		String buffer = stringCrear(BLOQUE);
		int numeroBloqueArchivo;
		for(numeroBloqueArchivo = 0; fread(buffer, sizeof(char), BLOQUE, file) == BLOQUE; numeroBloqueArchivo++) {
			Bloque* bloque = bloqueCrear(BLOQUE, numeroBloqueArchivo);
			listaAgregarElemento(archivo->listaBloques, bloque);
			copiasEnviadas = bloqueEnviarCopiasANodos(bloque, buffer);
			if(copiasEnviadas == ERROR)
				break;
		}
		memoriaLiberar(buffer);
	}
	else {
		String buffer = stringCrear(MAX_STRING);
		String datos = stringCrear(BLOQUE);
		int bytesDisponibles = BLOQUE-1;
		int numeroBloqueArchivo = 0;
		while(fgets(buffer, MAX_STRING, file) != NULL) {
			int tamanioBuffer = stringLongitud(buffer);
			if(tamanioBuffer <= bytesDisponibles) {
				stringConcatenar(datos, buffer);
				bytesDisponibles-= tamanioBuffer;
			}
			else {
				int bytesUtilizados = stringLongitud(datos)+1;
				bytesDisponibles = BLOQUE-1;
				Bloque* bloque = bloqueCrear(bytesUtilizados, numeroBloqueArchivo);
				listaAgregarElemento(archivo->listaBloques, bloque);
				copiasEnviadas = bloqueEnviarCopiasANodos(bloque, datos);
				if(copiasEnviadas == ERROR)
					break;
				numeroBloqueArchivo++;
				memoriaLiberar(datos);
				datos = stringCrear(BLOQUE);
				stringConcatenar(datos, buffer);
				bytesDisponibles-= tamanioBuffer;
			}
		}
		if(copiasEnviadas != ERROR && stringLongitud(datos) > 0 ) {
			int bytesUtilizados = stringLongitud(datos)+1;
			Bloque* bloque = bloqueCrear(bytesUtilizados, numeroBloqueArchivo);
			listaAgregarElemento(archivo->listaBloques, bloque);
			copiasEnviadas = bloqueEnviarCopiasANodos(bloque, datos);
		}
		memoriaLiberar(datos);
		memoriaLiberar(buffer);
	}
	fileCerrar(file);
	nodoLimpiarActividades();
	if(copiasEnviadas == ERROR) {
		archivoDestruir(archivo);
		return ERROR;
	}
	else {
		archivoListaAgregar(archivo);
		archivoPersistir(archivo);
		archivoPersistirControl();
		nodoPersistir();
		imprimirMensajeUno(archivoLog, "[ARCHIVO] El archivo %s fue copiado en File System", comando->argumentos[2]);
		return 0;
	}
}

int archivoLeer(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return ERROR;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo no existe");
		return ERROR;
	}
	if(!archivoDisponible(archivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no esta disponible"BLANCO);
		return ERROR;
	}
	listaOrdenar(archivo->listaBloques, (Puntero)bloqueOrdenarPorNumero);
	int numeroBloque;
	for(numeroBloque=0; numeroBloque < listaCantidadElementos(archivo->listaBloques); numeroBloque++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, numeroBloque);
		int numeroCopia;
		int bloqueSinImprimir = true;
		listaOrdenar(bloque->listaCopias, (Puntero)copiaOrdenarPorActividadDelNodo);
		for(numeroCopia=0; numeroCopia <listaCantidadElementos(bloque->listaCopias) && bloqueSinImprimir; numeroCopia++) {
			Copia* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
			Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
			mutexBloquear(mutexRuta);
			puts("envio entre");
			if(nodoConectado(nodo)) {
				mensajeEnviar(nodo->socket, LEER_BLOQUE ,&copia->bloqueNodo, sizeof(Entero));
				flagMensaje = ACTIVADO;
				nodo->actividadesRealizadas++;
				bloqueSinImprimir = false;
			}
			mutexDesbloquear(mutexRuta);
			puts("envio sali");
		}
		if(bloqueSinImprimir) {
			imprimirMensaje(archivoLog,"[ERROR] No hay nodos disponibles para obtener un bloque");
			nodoLimpiarActividades();
			return ERROR;
		}
	}
	nodoLimpiarActividades();
	return 0;
}


void archivoListaCrear() {
	mutexBloquear(mutexListaArchivos);
	listaArchivos = listaCrear();
	mutexDesbloquear(mutexListaArchivos);
}

void archivoListaDestruir() {
	mutexBloquear(mutexListaArchivos);
	listaDestruirConElementos(listaArchivos, (Puntero)archivoDestruir);
	mutexDesbloquear(mutexListaArchivos);
}

void archivoListaAgregar(Archivo* archivo) {
	mutexBloquear(mutexListaArchivos);
	listaAgregarElemento(listaArchivos, archivo);
	mutexDesbloquear(mutexListaArchivos);
}

int archivoListaCantidad() {
	mutexBloquear(mutexListaArchivos);
	int cantidad = listaCantidadElementos(listaArchivos);
	mutexDesbloquear(mutexListaArchivos);
	return cantidad;
}

int archivoListaPosicion(Archivo* archivo) {
	int indice;
	for(indice=0; archivo != archivoListaObtener(indice); indice++);
	return indice;
}

Archivo* archivoListaObtener(int indice) {
	mutexBloquear(mutexListaArchivos);
	Archivo* archivo = listaObtenerElemento(listaArchivos, indice);
	mutexDesbloquear(mutexListaArchivos);
	return archivo;
}

bool archivoListaTodosDisponibles() {
	mutexBloquear(mutexListaArchivos);
	int resultado = listaCumplenTodos(listaArchivos, (Puntero)archivoDisponible);
	mutexDesbloquear(mutexListaArchivos);
	return resultado;
}

void archivoPersistir(Archivo* archivo) {
	String ruta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
	File file = fileAbrir(ruta, ESCRITURA);
	memoriaLiberar(ruta);
	fprintf(file, "NOMBRE=%s\n", archivo->nombre);
	fprintf(file, "ID_PADRE=%i\n", archivo->identificadorPadre);
	fprintf(file, "TIPO=%s\n", archivo->tipo);
	fprintf(file, "BLOQUES=%i\n", listaCantidadElementos(archivo->listaBloques));
	int indice;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		fprintf(file, "BLOQUE%i_BYTES=%i\n",indice, bloque->bytesUtilizados);
		fprintf(file, "BLOQUE%i_COPIAS=%i\n",indice, listaCantidadElementos(bloque->listaCopias));
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			Copia* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			fprintf(file, "BLOQUE%i_COPIA%i=[%s,%i]\n", indice, indiceCopia, copiaBloque->nombreNodo, copiaBloque->bloqueNodo);
		}
	}
	fileCerrar(file);
}

void archivoPersistirControl(){
	File file = fileAbrir(rutaArchivos, ESCRITURA);
	mutexBloquear(mutexListaArchivos);
	listaOrdenar(listaArchivos, (Puntero)archivoOrdenarPorNombre);
	mutexDesbloquear(mutexListaArchivos);
	fprintf(file, "ARCHIVOS=%i\n", archivoListaCantidad());
	int indice;
	for(indice = 0; indice < archivoListaCantidad(); indice++) {
		Archivo* archivo = archivoListaObtener(indice);
		String lineaNombre = string_from_format("NOMBRE%i=%s\n", indice, archivo->nombre);
		String lineaPadre = string_from_format("PADRE%i=%i\n", indice, archivo->identificadorPadre);
		fprintf(file, "%s", lineaNombre);
		fprintf(file, "%s", lineaPadre);
		memoriaLiberar(lineaNombre);
		memoriaLiberar(lineaPadre);
	}
	fileCerrar(file);
}

void archivoRecuperarPersistencia() {
	File file = fileAbrir(rutaArchivos, LECTURA);
	if(file == NULL) {
		imprimirMensajeUno(archivoLog, "[ERROR] No se encontro el archivo %s", rutaArchivos);
		return;
	}
	fileCerrar(file);
	archivoListaCrear();
	ArchivoConfig config = config_create(rutaArchivos);
	int cantidadArchivos = archivoConfigEnteroDe(config, "ARCHIVOS");
	int indice;
	for(indice = 0; indice < cantidadArchivos; indice++) {
		String lineaNombre = string_from_format("NOMBRE%i", indice);
		String lineaPadre = string_from_format("PADRE%i", indice);
		String nombre = stringCrear(MAX_NOMBRE);
		stringCopiar(nombre, archivoConfigStringDe(config, lineaNombre));
		int padre = archivoConfigEnteroDe(config, lineaPadre);
		memoriaLiberar(lineaNombre);
		memoriaLiberar(lineaPadre);
		archivoRecuperarPersistenciaDetallada(nombre, padre);
	}
	archivoConfigDestruir(config);
}

void archivoRecuperarPersistenciaDetallada(String nombre, int padre) {
	String ruta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, padre, nombre);
	File file = fileAbrir(ruta, LECTURA);
	if(file == NULL) {
		imprimirMensajeUno(archivoLog, "[ERROR] No se encontro el archivo %s", ruta);
		memoriaLiberar(ruta);
		memoriaLiberar(nombre);
		return;
	}
	fileCerrar(file);
	ArchivoConfig config = config_create(ruta);
	memoriaLiberar(ruta);
	memoriaLiberar(nombre);
	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	stringCopiar(archivo->nombre, archivoConfigStringDe(config, "NOMBRE"));
	archivo->identificadorPadre = archivoConfigEnteroDe(config, "ID_PADRE");
	stringCopiar(archivo->tipo, archivoConfigStringDe(config, "TIPO"));
	int indiceBloques;
	archivo->listaBloques = listaCrear();
	int cantidadBloques = archivoConfigEnteroDe(config, "BLOQUES");
	for(indiceBloques = 0; indiceBloques < cantidadBloques; indiceBloques++) {
		Bloque* bloque = memoriaAlocar(sizeof(Bloque));
		String lineaBloque = string_from_format("BLOQUE%i_BYTES", indiceBloques);
		bloque->numeroBloque = indiceBloques;
		bloque->bytesUtilizados = archivoConfigEnteroDe(config, lineaBloque);
		memoriaLiberar(lineaBloque);
		listaAgregarElemento(archivo->listaBloques, bloque);
		bloque->listaCopias = listaCrear();
		String lineaCantidadCopias = string_from_format("BLOQUE%i_COPIAS", indiceBloques);
		int cantidadCopias = archivoConfigEnteroDe(config, lineaCantidadCopias);
		memoriaLiberar(lineaCantidadCopias);
		int indiceCopias;
		for(indiceCopias=0; indiceCopias < cantidadCopias; indiceCopias++) {
			Copia* copia = memoriaAlocar(sizeof(Copia));
			String lineaCopia = string_from_format("BLOQUE%i_COPIA%i", indiceBloques, indiceCopias);
			String* datosCopia = archivoConfigArrayDe(config, lineaCopia);
			memoriaLiberar(lineaCopia);
			stringCopiar(copia->nombreNodo, datosCopia[0]);
			copia->bloqueNodo = atoi(datosCopia[1]);
			memoriaLiberar(datosCopia[0]);
			memoriaLiberar(datosCopia[1]);
			memoriaLiberar(datosCopia);
			listaAgregarElemento(bloque->listaCopias, copia);
		}
	}
	archivoListaAgregar(archivo);
	archivoConfigDestruir(config);
}

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(Puntero datos, Socket nuevoSocket) {
	Nodo* nodo = memoriaAlocar(sizeof(Nodo));
	nodo->socket = nuevoSocket;
	nodo->estado = ACTIVADO;
	nodo->actividadesRealizadas = 0;
	memcpy(&nodo->bloquesTotales, datos, sizeof(Entero));
	nodo->bloquesLibres = nodo->bloquesTotales;
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
	String* tokens = rutaSeparar(datos+sizeof(Entero));
	stringCopiar(nodo->nombre, tokens[0]);
	stringCopiar(nodo->ip, tokens[1]);
	stringCopiar(nodo->puerto, tokens[2]);
	memoriaLiberar(tokens[0]);
	memoriaLiberar(tokens[1]);
	memoriaLiberar(tokens[2]);
	memoriaLiberar(tokens);
	return nodo;
}

void nodoDestruir(Nodo* nodo) {
	bitmapDestruir(nodo->bitmap);
	memoriaLiberar(nodo);
}

void nodoListaCrear() {
	mutexBloquear(mutexListaNodos);
	listaNodos = listaCrear();
	mutexDesbloquear(mutexListaNodos);
}

void nodoListaDestruir() {
	mutexBloquear(mutexListaNodos);
	listaDestruirConElementos(listaNodos, (Puntero)nodoDestruir);
	mutexDesbloquear(mutexListaNodos);
}

void nodoListaAgregar(Nodo* nodo) {
	mutexBloquear(mutexListaNodos);
	listaAgregarElemento(listaNodos, nodo);
	mutexDesbloquear(mutexListaNodos);
}

void nodoListaEliminar(Nodo* nodo) {
	int posicion = nodoListaPosicion(nodo);
	mutexBloquear(mutexListaNodos);
	listaEliminarDestruyendoElemento(listaNodos, posicion , (Puntero)nodoDestruir);
	mutexDesbloquear(mutexListaNodos);
}

int nodoListaCantidad() {
	mutexBloquear(mutexListaNodos);
	int cantidadNodos = listaCantidadElementos(listaNodos);
	mutexDesbloquear(mutexListaNodos);
	return cantidadNodos;
}

Nodo* nodoListaObtener(int posicion) {
	mutexBloquear(mutexListaNodos);
	Nodo* nodo = listaObtenerElemento(listaNodos, posicion);
	mutexDesbloquear(mutexListaNodos);
	return nodo;
}

int nodoListaPosicion(Nodo* nodo) {
	int posicion;
	for(posicion = 0; nodo != nodoListaObtener(posicion); posicion++);
	return posicion;
}

void nodoListaOrdenarPorActividad() {
	mutexBloquear(mutexListaNodos);
	listaOrdenar(listaNodos, (Puntero)nodoOrdenarPorActividad);
	mutexDesbloquear(mutexListaNodos);
}

bool nodoListaAlgunoDisponible() {
	mutexBloquear(mutexListaNodos);
	int resultado = listaCumpleAlguno(listaNodos, (Puntero)nodoDisponible);
	mutexDesbloquear(mutexListaNodos);
	return resultado;
}

Nodo* nodoBuscarPorNombre(String nombre) {

	bool buscarPorNombre(Nodo* nodo) {
		return stringIguales(nodo->nombre, nombre);
	}

	mutexBloquear(mutexListaNodos);
	Nodo* nodo = listaBuscar(listaNodos, (Puntero)buscarPorNombre);
	mutexDesbloquear(mutexListaNodos);
	return nodo;
}

Nodo* nodoBuscarPorSocket(Socket unSocket) {

	bool buscarPorSocket(Nodo* nodo) {
		return nodo->socket == unSocket;
	}

	mutexBloquear(mutexListaNodos);
	Nodo* nodo = listaBuscar(listaNodos, (Puntero)buscarPorSocket);
	mutexDesbloquear(mutexListaNodos);
	return nodo;
}

int nodoBuscarBloqueLibre(Nodo* nodo) {
	int indice;
	for(indice = 0; bitmapBitOcupado(nodo->bitmap, indice); indice++);
		if(indice < nodo->bloquesTotales)
			return indice;
		else
			return ERROR;
}

bool nodoListaOrdenarBloquesLibres(Nodo* unNodo, Nodo* otroNodo) {
	return unNodo->bloquesLibres > otroNodo->bloquesLibres;
}

bool nodoOrdenarPorActividad(Nodo* unNodo, Nodo* otroNodo) {
	return unNodo->actividadesRealizadas < otroNodo->actividadesRealizadas;
}

bool nodoTieneBloquesLibres(Nodo* nodo) {
	return nodo->bloquesLibres > 0;
}

bool nodoConectado(Nodo* nodo) {
	return nodo->estado == ACTIVADO;
}

bool nodoDesconectado(Nodo* nodo) {
	return nodo->estado == DESACTIVADO;
}

void nodoVerificarBloquesLibres(Nodo* nodo) {
	if(nodo->bloquesLibres == 0)
		imprimirMensajeUno(archivoLog, AMARILLO"[ADVERTENCIA] El %s tiene todos sus bloques ocupados"BLANCO, nodo->nombre);
}

bool nodoDisponible(Nodo* nodo) {
	return nodoConectado(nodo) &&
			nodoTieneBloquesLibres(nodo);
}

bool nodoNuevo(Nodo* nuevoNodo) {
	Nodo* nodo = nodoBuscarPorNombre(nuevoNodo->nombre);
	if(nodo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] No se permite conexiones de nuevos Nodos"BLANCO);
		return true;
	}
	return false;
}

bool nodoEstaConectado(Nodo* nuevoNodo) { //TODO ver
	Nodo* nodo = nodoBuscarPorNombre(nuevoNodo->nombre);
	if(nodo == NULL)
		return false;
	if(nodo->estado == ACTIVADO) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Un nodo con ese nombre ya esta conectado"BLANCO);
		return true;
	}
	return false;
}

bool nodoInvalido(Nodo* nodoTemporal) {
	return nodoNuevo(nodoTemporal) ||
			nodoEstaConectado(nodoTemporal);
}

void nodoLimpiarActividades() {
	int indice;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		nodo->actividadesRealizadas = 0;
	}
}

int nodoBloquesLibresTotales() {
	int bloquesLibres = 0;
	int indice;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		bloquesLibres+= nodo->bloquesLibres;
	}
	return bloquesLibres;
}

void nodoFormatear(Nodo* nodo) {
	nodo->bloquesLibres = nodo->bloquesTotales;
	bitmapDestruir(nodo->bitmap);
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
}

void nodoFormatearConectados() {
	mutexBloquear(mutexListaNodos);
	listaEliminarDestruyendoPorCondicion(listaNodos, (Puntero)nodoDesconectado, (Puntero)nodoDestruir);
	mutexDesbloquear(mutexListaNodos);
	int indice;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* unNodo = nodoListaObtener(indice);
		nodoFormatear(unNodo);
	}
}

Nodo* nodoActualizar(Nodo* nodoTemporal) {
	Nodo* nodo = nodoBuscarPorNombre(nodoTemporal->nombre);
	nodo->estado = ACTIVADO;
	nodo->socket = nodoTemporal->socket;
	stringCopiar(nodo->ip, nodoTemporal->ip);
	stringCopiar(nodo->puerto, nodoTemporal->puerto);
	nodoDestruir(nodoTemporal);
	return nodo;
}

void nodoAceptar(Nodo* nodo) {
	mensajeEnviar(nodo->socket, ACEPTAR_NODO, &nodo->socket, sizeof(Socket)); //TODO debe ser nulo el msj, solo me interesa la operacion
	imprimirMensajeUno(archivoLog, AMARILLO"[CONEXION] %s conectado"BLANCO, nodo->nombre);
}

void nodoPersistir() {
	File archivo = fileAbrir(rutaNodos, ESCRITURA);
	fprintf(archivo, "NODOS=%i\n", nodoListaCantidad());
	int indice;
	int contadorBloquesTotales = 0;
	int contadorBloquesLibres = 0;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* unNodo = nodoListaObtener(indice);
		fprintf(archivo, "NOMBRE%i=%s\n",indice, unNodo->nombre);
		fprintf(archivo, "BLOQUES_TOTALES%i=%i\n", indice, unNodo->bloquesTotales);
		fprintf(archivo, "BLOQUES_LIBRES%i=%i\n",indice, unNodo->bloquesLibres);
		contadorBloquesTotales+=unNodo->bloquesTotales;
		contadorBloquesLibres+=unNodo->bloquesLibres;
		nodoPersistirBitmap(unNodo);
	}
	fprintf(archivo, "BLOQUES_TOTALES=%i\n", contadorBloquesTotales);
	fprintf(archivo, "BLOQUES_LIBRES=%i\n", contadorBloquesLibres);
	fileCerrar(archivo);
}

void nodoPersistirBitmap(Nodo* nodo) {
	String ruta = string_from_format("%s/%s", rutaDirectorioBitmaps, nodo->nombre);
	File archivo = fileAbrir(ruta, ESCRITURA);
	memoriaLiberar(ruta);
	int indice;
	for(indice = 0; indice < nodo->bloquesTotales; indice++)
		fprintf(archivo, "%i", bitmapBitOcupado(nodo->bitmap, indice));
	fprintf(archivo, "\n");
	fileCerrar(archivo);
}

void nodoRecuperarPersistencia() {
	File file = fileAbrir(rutaArchivos, LECTURA);
	if(file == NULL) {
		imprimirMensajeUno(archivoLog, "[ERROR] No se encontro el archivo %s", rutaNodos);
		return;
	}
	fileCerrar(file);
	ArchivoConfig config = config_create(rutaNodos);
	int cantidadNodos = archivoConfigEnteroDe(config, "NODOS");
	int indice;
	for(indice = 0; indice < cantidadNodos; indice++) {
		String lineaNombre = string_from_format("NOMBRE%i", indice);
		String lineaTotales = string_from_format("BLOQUES_TOTALES%i", indice);
		String lineaLibres = string_from_format("BLOQUES_LIBRES%i", indice);
		Nodo* nodo = memoriaAlocar(sizeof(Nodo));
		stringCopiar(nodo->nombre, archivoConfigStringDe(config, lineaNombre));
		nodo->bloquesTotales = archivoConfigEnteroDe(config, lineaTotales);
		nodo->bloquesLibres = archivoConfigEnteroDe(config, lineaLibres);
		nodo->estado = DESACTIVADO;
		nodo->socket = ERROR;
		nodo->actividadesRealizadas = 0;
		memoriaLiberar(lineaNombre);
		memoriaLiberar(lineaTotales);
		memoriaLiberar(lineaLibres);
		nodoListaAgregar(nodo);
		nodoRecuperarPersistenciaBitmap(nodo);
	}
	archivoConfigDestruir(config);
}

void nodoRecuperarPersistenciaBitmap(Nodo* nodo) {
	String ruta = string_from_format("%s/%s", rutaDirectorioBitmaps, nodo->nombre);
	File file = fileAbrir(ruta, LECTURA);
	if(file == NULL) {
		imprimirMensajeUno(archivoLog, "[ERROR] No se encontro el archivo %s", rutaNodos);
		memoriaLiberar(ruta);
		return;
	}
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
	int indice;
	for(indice = 0; indice < nodo->bloquesTotales; indice++) {
		int bit = fgetc(file);
		if(bit == OCUPADO)
			bitmapOcuparBit(nodo->bitmap, indice);
	}
	memoriaLiberar(ruta);
	fileCerrar(file);
}

//--------------------------------------- Funciones de Bloque-------------------------------------

Bloque* bloqueCrear(int bytes, int numero) {
	Bloque* bloque = memoriaAlocar(sizeof(Bloque));
	bloque->bytesUtilizados = bytes;
	bloque->listaCopias = listaCrear();
	bloque->numeroBloque = numero;
	return bloque;
}

void bloqueDestruir(Bloque* bloque) {
	listaDestruirConElementos(bloque->listaCopias, (Puntero)copiaDestruir);
	memoriaLiberar(bloque);
}

int bloqueEnviarANodo(Bloque* bloque, Nodo* nodo, String buffer) {
	Entero numeroBloqueNodo = nodoBuscarBloqueLibre(nodo);
	bloqueCopiarEnNodo(bloque, nodo, numeroBloqueNodo);
	BloqueNodo* bloqueNodo = bloqueNodoCrear(numeroBloqueNodo, buffer, BLOQUE);
	mensajeEnviar(nodo->socket, ESCRIBIR_BLOQUE, bloqueNodo, sizeof(Entero)+BLOQUE);
	memoriaLiberar(bloqueNodo);
	return numeroBloqueNodo;
}

void bloqueCopiarEnNodo(Bloque* bloque, Nodo* nodo, Entero numeroBloqueNodo) {
	Copia* copia = copiaCrear(numeroBloqueNodo, nodo->nombre);
	bitmapOcuparBit(nodo->bitmap, numeroBloqueNodo);
	nodo->bloquesLibres--;
	nodoVerificarBloquesLibres(nodo);
	listaAgregarElemento(bloque->listaCopias, copia);
}

int bloqueEnviarCopiasANodos(Bloque* bloque, String buffer) {
	int copiasEnviadas = 0;
	int indice = 0;

	bool bloqueForzarCopiado() {
		return indice == (nodoListaCantidad()-1) && nodoListaAlgunoDisponible() && copiasEnviadas < MAX_COPIAS;
	}
	semaforoWait(semaforoTarea);
	nodoListaOrdenarPorActividad();
	for(indice = 0; indice < nodoListaCantidad() && copiasEnviadas < MAX_COPIAS; indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		if(nodoDisponible(nodo)) {
			bloqueEnviarANodo(bloque, nodo, buffer);
			copiasEnviadas++;
			nodo->actividadesRealizadas++;
		}
		if(bloqueForzarCopiado()) {
			nodoListaOrdenarPorActividad();
			indice = -1;
		}
	}
	semaforoSignal(semaforoTarea);
	if(copiasEnviadas == 0) {
		imprimirMensajeUno(archivoLog, "[ERROR] El bloque N°%i no pudo copiarse en ningun Nodo", (int*)bloque->numeroBloque);
		return ERROR;
	}
	if(copiasEnviadas < MAX_COPIAS) {
		imprimirMensajeDos(archivoLog, "[AVISO] El bloque N°%i no pudo copiarse %i veces ", (int*)bloque->numeroBloque, (int*)MAX_COPIAS);
		return NULO;
	}
	return NULO;
}

void bloqueCopiar(Puntero datos) {
	mutexBloquear(mutexNodo);
	if(nodoConectado(nodoBuffer)) {
		mutexBloquear(mutexBloque);
		int numeroBloqueNodo = bloqueEnviarANodo(bloqueBuffer, nodoBuffer, datos);
		mutexDesbloquear(mutexBloque);
		mutexDesbloquear(mutexNodo);
		semaforoSignal(semaforoTarea);
		mutexBloquear(mutexArchivo);
		archivoPersistir(archivoBuffer);
		mutexDesbloquear(mutexArchivo);
		nodoPersistir();
		mutexBloquear(mutexNodo);
		imprimirMensajeDos(archivoLog, "[BLOQUE] El bloque fue copiado en el bloque N°%i de %s", (int*)numeroBloqueNodo, nodoBuffer->nombre);
		mutexDesbloquear(mutexNodo);
	}
	else {
		mutexDesbloquear(mutexNodo);
		semaforoSignal(semaforoTarea);
		imprimirMensaje(archivoLog, ROJO"[BLOQUE] No pudo copiarse el bloque, el nodo receptor no esta conectado (FALLO EL SEMAFORO, NUNCA DEBERIA PASAR)"BLANCO);
	}
}

void servidorDestruirSockets(Servidor* servidor) {
	int indice;
	for(indice=0; indice < socketCantidad(); indice++) {
		Socket* unSocket = socketObtener(indice);
		servidorFinalizarDataNode(servidor, *unSocket);
		socketEliminar(indice);
	}
}

void bloqueLeer(Servidor* servidor, Puntero datos) {
	mutexBloquear(mutexRuta);
	puts("bloque leer entre");
	//printf("%s", (String)datos);
	flagMensaje = DESACTIVADO;
	servidorDestruirSockets(servidor);
	mutexDesbloquear(mutexRuta);
	puts("bloque leer sali");
}

void bloqueCopiarBinario(Puntero datos) {
	mutexBloquear(mutexRuta);
	File file = fileAbrir(rutaBuffer, "a+");
	mutexDesbloquear(mutexRuta);
	fwrite(datos, sizeof(char), BLOQUE, file);
	fileCerrar(file);
	semaforoSignal(semaforoTarea);
}

void bloqueCopiarTexto(Puntero datos) {
	mutexBloquear(mutexRuta);
	File file = fileAbrir(rutaBuffer, "a+");
	mutexDesbloquear(mutexRuta);
	fprintf(file, "%s", (String)datos);
	fileCerrar(file);
	semaforoSignal(semaforoTarea);
}

bool bloqueOrdenarPorNumero(Bloque* unBloque, Bloque* otroBloque) {
	return unBloque->numeroBloque < otroBloque->numeroBloque;
}

bool bloqueDisponible(Bloque* bloque) {
	return listaCumpleAlguno(bloque->listaCopias, (Puntero)copiaDisponible);
}

BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer, int tamanioUtilizado) {
	BloqueNodo* bloqueNodo = memoriaAlocar(sizeof(BloqueNodo));
	bloqueNodo->numeroBloque = numeroBloque;
	memcpy(bloqueNodo->bloque, buffer, tamanioUtilizado);
	return bloqueNodo;
}

//--------------------------------------- Funciones de Copia -------------------------------------

Copia* copiaCrear(int numeroBloqueDelNodo, String nombreNodo) {
	Copia* copiaBloque = memoriaAlocar(sizeof(Copia));
	copiaBloque->bloqueNodo = numeroBloqueDelNodo;
	stringCopiar(copiaBloque->nombreNodo, nombreNodo);
	return copiaBloque;
}

void copiaEliminar(Copia* copia) {
	Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
	bitmapLiberarBit(nodo->bitmap, copia->bloqueNodo);
	nodo->bloquesLibres++;
}

void copiaDestruir(Copia* copia) {
	copiaEliminar(copia);
	memoriaLiberar(copia);
}

bool copiaDisponible(Copia* copia) {
	Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
	return nodo->estado == ACTIVADO;
}

bool copiaOrdenarPorActividadDelNodo(Copia* unaCopia, Copia* otraCopia) {
	Nodo* unNodo = nodoBuscarPorNombre(unaCopia->nombreNodo);
	Nodo* otroNodo = nodoBuscarPorNombre(otraCopia->nombreNodo);
	return unNodo->actividadesRealizadas < otroNodo->actividadesRealizadas;
}

//--------------------------------------- Funciones de Metadata------------------------------------

void metadataCrear() {
	mkdir(configuracion->rutaMetadata, 0777);
	mkdir(rutaDirectorioArchivos, 0777);
	mkdir(rutaDirectorioBitmaps, 0777);
}

void metadataEliminar() {
	String comando = string_from_format("rm -r -f %s", configuracion->rutaMetadata);
	system(comando);
	memoriaLiberar(comando);
}

void metadataIniciar() {
	metadataEliminar();
	metadataCrear();
	archivoListaCrear();
	directorioListaCrear();
	bitmapDirectoriosCrear();
	directoriosDisponibles = MAX_DIR;
	directorioCrearConPersistencia(0, "root", -1);
	archivoPersistirControl();
	nodoPersistir();
	mutexBloquear(mutexEstadoEjecucion);
	estadoEjecucion = NUEVO;
	mutexDesbloquear(mutexEstadoEjecucion);
}

void metadataRecuperar() {
	mutexBloquear(mutexEstadoEjecucion);
	estadoEjecucion = NORMAL;
	mutexDesbloquear(mutexEstadoEjecucion);
	archivoRecuperarPersistencia();
	directorioRecuperarPersistencia();
	nodoRecuperarPersistencia();
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] Para pasar a un estado estable conecte los nodos necesarios"BLANCO);
}

//--------------------------------------- Funciones de Ruta------------------------------------

String rutaObtenerUltimoNombre(String ruta) {
	int indice;
	int ultimaBarra;
	for(indice=0; ruta[indice] != FIN; indice++)
		if(caracterIguales(ruta[indice], '/'))
			ultimaBarra = indice;
	String directorio = stringTomarDesdePosicion(ruta+1, ultimaBarra);
	return directorio;
}

bool rutaTieneAlMenosUnaBarra(String ruta) {
	return stringContiene(ruta, "/");
}

bool rutaBarrasEstanSeparadas(String ruta) {
	int indice;
	for(indice=0; indice<stringLongitud(ruta); indice++) {
		if(caracterIguales(ruta[indice], '/')) {
			if(indice==0) {
				if(caracterIguales(ruta[indice+1], '/'))
					return false;
			}
			else {
				if(indice==stringLongitud(ruta)-1)
					return false;
				else
					if(caracterIguales(ruta[indice-1], '/') ||
						caracterIguales(ruta[indice+1], '/'))
						return false;
			}

		}
	}
	return true;
}

bool rutaValida(String ruta) {
	return caracterIguales(ruta[0], '/') &&
			rutaTieneAlMenosUnaBarra(ruta) &&
			rutaBarrasEstanSeparadas(ruta);
}

String* rutaSeparar(String ruta) {
	int indice;
	int contadorDirectorios = 0;
	for(indice = 0; ruta[indice] != FIN; indice++)
		if(caracterIguales(ruta[indice], BARRA))
			contadorDirectorios++;
	String* directorios = memoriaAlocar((contadorDirectorios+1)*sizeof(String));
	int PosicionUltimaBarra = 0;
	contadorDirectorios = 0;
	for(indice = 0; caracterDistintos(ruta[indice], FIN); indice++)
		if(caracterIguales(ruta[indice],BARRA) && indice != 0) {
			directorios[contadorDirectorios] = stringTomarCantidad(ruta, PosicionUltimaBarra+1, indice-PosicionUltimaBarra-1);
			PosicionUltimaBarra = indice;
			contadorDirectorios++;
		}
	directorios[contadorDirectorios] = stringTomarCantidad(ruta, PosicionUltimaBarra+1, indice-PosicionUltimaBarra-1);
	directorios[contadorDirectorios+1] = NULL;
	return directorios;
}

bool rutaEsNumero(String ruta) {
	int indice;
	for(indice=0; indice<stringLongitud(ruta); indice++)
		if(ruta[indice] < 48 || ruta[indice] > 57)
			return false;
	return true;
}

//--------------------------------------- Funciones de Estado ------------------------------------

bool estadoEjecucionIgualA(int estado) {
	mutexBloquear(mutexEstadoEjecucion);
	int resultado = estadoEjecucion == estado;
	mutexDesbloquear(mutexEstadoEjecucion);
	return resultado;
}

void estadoFileSystemEstable() {
	mutexBloquear(mutexEstadoFileSystem);
	estadoFileSystem = ESTABLE;
	mutexDesbloquear(mutexEstadoFileSystem);
}

void estadoFileSystemInestable() {
	mutexBloquear(mutexEstadoFileSystem);
	estadoFileSystem = INESTABLE;
	mutexDesbloquear(mutexEstadoFileSystem);
}

bool estadoFileSystemIgualA(int estado) {
	mutexBloquear(mutexEstadoFileSystem);
	int resultado = estadoFileSystem == estado;
	mutexDesbloquear(mutexEstadoFileSystem);
	return resultado;
}

void estadoControlActivar() {
	mutexBloquear(mutexEstadoControl);
	estadoControl = ACTIVADO;
	mutexDesbloquear(mutexEstadoControl);
}

bool estadoControlIgualA(int estado) {
	mutexBloquear(mutexEstadoControl);
	int resultado = estadoControl == estado;
	mutexDesbloquear(mutexEstadoControl);
	return resultado;
}

//--------------------------------------- Funciones de Bitmap de Directorios------------------------------------

void bitmapDirectoriosCrear() {
	mutexBloquear(mutexBitmapDirectorios);
	bitmapDirectorios = bitmapCrear(13);
	mutexDesbloquear(mutexBitmapDirectorios);
}

void bitmapDirectoriosDestruir() {
	mutexBloquear(mutexBitmapDirectorios);
	bitmapDestruir(bitmapDirectorios);
	mutexDesbloquear(mutexBitmapDirectorios);
}

bool bitmapDirectoriosBitOcupado(int indice) {
	mutexBloquear(mutexBitmapDirectorios);
	int resultado = bitmapBitOcupado(bitmapDirectorios, indice);
	mutexDesbloquear(mutexBitmapDirectorios);
	return resultado;
}

void bitmapDirectoriosOcuparBit(int posicion) {
	mutexBloquear(mutexBitmapDirectorios);
	bitmapOcuparBit(bitmapDirectorios, posicion);
	mutexDesbloquear(mutexBitmapDirectorios);
}

//--------------------------------------- Funciones de Semaforo ------------------------------------

void semaforosCrear() {
	semaforoTarea = memoriaAlocar(sizeof(Semaforo));
	semaforoFinal = memoriaAlocar(sizeof(Semaforo));
	mutexListaArchivos = memoriaAlocar(sizeof(Mutex));
	mutexListaNodos = memoriaAlocar(sizeof(Mutex));
	mutexListaDirectorios = memoriaAlocar(sizeof(Mutex));
	mutexBitmapDirectorios = memoriaAlocar(sizeof(Mutex));
	mutexArchivo = memoriaAlocar(sizeof(Mutex));
	mutexNodo = memoriaAlocar(sizeof(Mutex));
	mutexBloque = memoriaAlocar(sizeof(Mutex));
	mutexRuta = memoriaAlocar(sizeof(Mutex));
	mutexEstadoFileSystem = memoriaAlocar(sizeof(Mutex));
	mutexEstadoEjecucion = memoriaAlocar(sizeof(Mutex));
	mutexEstadoControl = memoriaAlocar(sizeof(Mutex));
	mutexListaSockets = memoriaAlocar(sizeof(Mutex));
}

void semaforosIniciar() {
	semaforoIniciar(semaforoTarea, 1);
	semaforoIniciar(semaforoFinal, 0);
	mutexIniciar(mutexListaSockets);
	mutexIniciar(mutexListaArchivos);
	mutexIniciar(mutexListaNodos);
	mutexIniciar(mutexListaDirectorios);
	mutexIniciar(mutexArchivo);
	mutexIniciar(mutexNodo);
	mutexIniciar(mutexBloque);
	mutexIniciar(mutexRuta);
	mutexIniciar(mutexBitmapDirectorios);
	mutexIniciar(mutexEstadoEjecucion);
	mutexIniciar(mutexEstadoFileSystem);
	mutexIniciar(mutexEstadoControl);
}

void semaforosDestruir() {
	semaforoDestruir(semaforoTarea);
	semaforoDestruir(semaforoFinal);
	memoriaLiberar(semaforoFinal);
	memoriaLiberar(semaforoTarea);
	memoriaLiberar(mutexListaSockets);
	memoriaLiberar(mutexListaArchivos);
	memoriaLiberar(mutexListaNodos);
	memoriaLiberar(mutexListaDirectorios);
	memoriaLiberar(mutexArchivo);
	memoriaLiberar(mutexNodo);
	memoriaLiberar(mutexBloque);
	memoriaLiberar(mutexRuta);
	memoriaLiberar(mutexBitmapDirectorios);
	memoriaLiberar(mutexEstadoEjecucion);
	memoriaLiberar(mutexEstadoFileSystem);
	memoriaLiberar(mutexEstadoControl);
}



//TODO para yama

Dir nodoObtenerDireccion(String nombreNodo) {
	Nodo* nodo = nodoBuscarPorNombre(nombreNodo);
	Dir direccion;
	stringLimpiar(direccion.ip, 20);
	stringLimpiar(direccion.port, 20);
	stringCopiar(direccion.ip, nodo->ip);
	stringCopiar(direccion.port, nodo->puerto);
	return direccion;
}

BloqueYama bloqueConvertirParaYama(Bloque* bloque) {
	BloqueYama bloqueYama;
	Copia* copia1 = listaObtenerElemento(bloque->listaCopias, 0);
	Copia* copia2 = listaObtenerElemento(bloque->listaCopias, 1);
	Dir direccion1 = nodoObtenerDireccion(copia1->nombreNodo);
	Dir direccion2 = nodoObtenerDireccion(copia2->nombreNodo);
	memcpy(&bloqueYama.direccionCopia1, &direccion1, sizeof(Dir));
	memcpy(&bloqueYama.direccionCopia2, &direccion2, sizeof(Dir));
	bloqueYama.numeroBloqueCopia1 = copia1->bloqueNodo;
	bloqueYama.numeroBloqueCopia2 = copia2->bloqueNodo;
	bloqueYama.bytesUtilizados = bloque->bytesUtilizados;
	return bloqueYama;
}

BloqueYama* archivoConvertirYama(Archivo* archivo) {
	int indice;
	int cantidad = listaCantidadElementos(archivo->listaBloques);
	BloqueYama* bloques = memoriaAlocar(sizeof(BloqueYama)*cantidad);
	listaOrdenar(archivo->listaBloques, (Puntero)bloqueOrdenarPorNumero);
	for(indice = 0; indice < cantidad; indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		bloques[indice] = bloqueConvertirParaYama(bloque);
	}
	return bloques;
}

void archivoEnviarBloques(String path) {
	Archivo* archivo = archivoBuscarPorRuta(path);
	int cantidad = listaCantidadElementos(archivo->listaBloques);
	BloqueYama* bloques = archivoConvertirYama(archivo);
	//TODO test
	int indice;
	for(indice = 0; indice < cantidad; indice++) {
		printf("bytes: %i\n",bloques[indice].bytesUtilizados);
		printf("ip copia1 %s\n",bloques[indice].direccionCopia1.ip);
		printf("dir copia1: %s\n", bloques[indice].direccionCopia1.port);
		printf("ip copia2 %s\n",bloques[indice].direccionCopia2.ip);
		printf("dir copia2: %s\n", bloques[indice].direccionCopia2.port);
	}
	mensajeEnviar(socketYama, ENVIAR_BLOQUES, bloques, sizeof(BloqueYama)*cantidad);
}




//TODO el data bin cambia o solo cambia la primera vez (VER)
//TODO cpblock, cpto, md5, cpfrom, cat (funcionan si no se matan los nodos)
//TODO ver si hay que borrar todo con clean ya que luego tengo que tirar el format
//TODO ver cpfrom cantidad de bloques nose si importa el numero de bloques
//TODO si hay espacios para una copia de cada bloque se admite o no el archivo
//TODO Esta prohibido dos copias en un mismo nodo
