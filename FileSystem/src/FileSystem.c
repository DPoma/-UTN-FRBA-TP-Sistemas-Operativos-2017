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
	fileSystemAtenderProcesos();
	consolaAtenderComandos();
	fileSystemFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de File System -------------------------------------

void fileSystemIniciar(String flag) {
	pantallaLimpiar();
	semaforosCrear();
	semaforosIniciar();
	configuracionIniciar();
	nodoListaCrear();
	estadoControlActivar();
	estadoFileSystemInestable();
	if(stringIguales(flag, FLAG_C))
		fileSystemReiniciar();
	else
		fileSystemRecuperar();
}

void fileSystemAtenderProcesos() {
	socketYama = ERROR;
	listenerDataNode = socketCrearListener(configuracion->ipPropia,configuracion->puertoDataNode);
	listenerYama = socketCrearListener(configuracion->ipPropia,configuracion->puertoYama);
	listenerWorker = socketCrearListener(configuracion->ipPropia,configuracion->puertoWorker);
	hiloCrear(&hiloDataNode, (Puntero)dataNodeListener, NULL);
	hiloCrear(&hiloYama, (Puntero)yamaListener, NULL);
	hiloCrear(&hiloWorker, (Puntero)workerListener, NULL);
}

void fileSystemFinalizar() {
	sleep(2);
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
	archivoLogDestruir(archivoLog);
	configuracionRutasDestruir();
	bitmapDirectoriosDestruir();
	archivoListaDestruir();
	directorioListaDestruir();
	nodoListaDestruir();
	semaforosDestruir();
}

void fileSystemReiniciar() {
	metadataIniciar();
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] Debe conectar los nodos y luego formatear el File System"BLANCO);
}

void fileSystemRecuperar() {
	metadataRecuperar();
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] Conecte los nodos necesarios al File System"BLANCO);
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoYama, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	stringCopiar(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYama);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Esperando conexiones de Workers (Puerto: %s)", configuracion->puertoWorker);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta Metadata: %s", configuracion->rutaMetadata);
}

void configuracionIniciarCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "PUERTO_WORKER";
	campos[3] = "RUTA_METADATA";
	campos[4] = "IP_PROPIA";
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
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System iniciado");
}

void configuracionIniciarRutas() {
	rutaDirectorioArchivos = string_from_format("%s/archivos", configuracion->rutaMetadata);
	rutaDirectorioBitmaps = string_from_format("%s/bitmaps", configuracion->rutaMetadata);
	rutaArchivos = string_from_format("%s/Archivos.txt", configuracion->rutaMetadata);
	rutaDirectorios = string_from_format("%s/Directorios.txt", configuracion->rutaMetadata);
	rutaNodos = string_from_format("%s/Nodos.bin", configuracion->rutaMetadata);
	rutaBufferCrear();
}

void configuracionRutasDestruir() {
	memoriaLiberar(configuracion);
	memoriaLiberar(rutaNodos);
	memoriaLiberar(rutaDirectorioArchivos);
	memoriaLiberar(rutaDirectorioBitmaps);
	memoriaLiberar(rutaDirectorios);
	memoriaLiberar(rutaArchivos);
	rutaBufferDestruir();
}

//--------------------------------------- Funciones de Data Node -------------------------------------

void dataNodeListener() {
	hiloDetach(pthread_self());
	while(estadoControlIgualA(ACTIVADO))
		dataNodeCrearConexion();
}

void dataNodeCrearConexion() {
	Socket nuevoSocket = socketAceptar(listenerDataNode, ID_DATANODE);
	if(nuevoSocket != ERROR)
		dataNodeControlarConexion(nuevoSocket);
}

void dataNodeControlarConexion(Socket nuevoSocket) {
	Mensaje* mensaje = mensajeRecibir(nuevoSocket);
	Nodo* nodoTemporal = nodoCrear(mensaje->datos, nuevoSocket);
	if(estadoEjecucionIgualA(NORMAL))
		dataNodeReconectar(nodoTemporal);
	else
		dataNodeControlar(nodoTemporal);
	mensajeDestruir(mensaje);
}

void dataNodeReconectar(Nodo* nodoTemporal) {
	if(nodoInvalido(nodoTemporal))
		dataNodeRechazar(nodoTemporal);
	else
		dataNodeAceptarReconexion(nodoTemporal);
}

void dataNodeControlar(Nodo* nodoTemporal) {
	if(estadoFileSystemIgualA(ESTABLE))
		dataNodeReconectar(nodoTemporal);
	else
		dataNodeAdmitir(nodoTemporal);
}

void dataNodeAdmitir(Nodo* nodoTemporal) {
	if(nodoEstaConectado(nodoTemporal))
		dataNodeRechazar(nodoTemporal);
	else
		dataNodeAceptar(nodoTemporal);
}

void dataNodeAceptar(Nodo* nodoTemporal) {
	nodoAceptar(nodoTemporal);
	nodoListaAgregar(nodoTemporal);
}

void dataNodeRechazar(Nodo* nuevoNodo) {
	socketCerrar(nuevoNodo->socket);
	nodoDestruir(nuevoNodo);
}

void dataNodeAceptarReconexion(Nodo* nuevoNodo) {
	mutexBloquear(mutexTarea);
	Nodo* nodo = nodoActualizar(nuevoNodo);
	nodoLimpiarActividades();
	nodoAceptar(nodo);
	if(estadoFileSystemIgualA(INESTABLE) && archivoListaTodosDisponibles()) {
		mutexBloquear(mutexEstadoFileSystem);
		estadoFileSystem = ESTABLE;
		mutexDesbloquear(mutexEstadoFileSystem);
		imprimirMensaje(archivoLog, AMARILLO"[AVISO] El File System esta estable"BLANCO);
	}
	mutexDesbloquear(mutexTarea);
}


void dataNodeHilo(Nodo* nodo) {
	hiloDetach(pthread_self());
	int estado = ACTIVADO;
	while(estado)
		dataNodeAtender(nodo, &estado);
}

void dataNodeAtender(Nodo* nodo, int* estado) {
	Mensaje* mensaje = mensajeRecibir(nodo->socket);
	switch(mensaje->header.operacion) {
	case DESCONEXION: dataNodeFinalizar(nodo, estado); break;
	case LEER_BLOQUE: bloqueLeer(nodo, mensaje->datos, estado); break;
	case COPIAR_BLOQUE: bloqueCopiar(nodo, mensaje->datos, estado); break;
	case COPIAR_ARCHIVO: bloqueCopiarArchivo(nodo, mensaje->datos, estado); break;
	}
	mensajeDestruir(mensaje);
}

void dataNodeDestruir(Nodo* nodo, int* estado) {
	*estado = DESACTIVADO;
	socketCerrar(nodo->socket);
	imprimirMensaje1(archivoLog, AMARILLO"[AVISO] %s desconectado"BLANCO, nodo->nombre);
	int posicion = nodoListaPosicion(nodo);
	mutexBloquear(mutexListaNodos);
	listaEliminarDestruyendoElemento(listaNodos, posicion , (Puntero)nodoDestruir);
	mutexDesbloquear(mutexListaNodos);
}

void dataNodeDesactivar(Nodo* nodo, int* estado) {
	mutexBloquear(mutexTarea);
	nodoEstado(nodo, DESACTIVADO);
	if(nodoMensajeIgualA(nodo, DESACTIVADO)) {
		socketCerrar(nodo->socket);
		nodoSocket(nodo, ERROR);
		*estado = DESACTIVADO;
		imprimirMensaje1(archivoLog, AMARILLO"[AVISO] %s desconectado"BLANCO, nodo->nombre);
	}
	mutexDesbloquear(mutexTarea);

}

void dataNodeControlarFinalizacion(Nodo* nodo, int* estado) {
	if(estadoFileSystemIgualA(ESTABLE))
		dataNodeDesactivar(nodo, estado);
	else
		dataNodeDestruir(nodo, estado);
}

void dataNodeFinalizar(Nodo* nodo, int* estado) {
	if(estadoEjecucionIgualA(NORMAL))
		dataNodeDesactivar(nodo, estado);
	else
		dataNodeControlarFinalizacion(nodo, estado);
}

//--------------------------------------- Funciones de Yama -------------------------------------

void yamaListener() {
	hiloDetach(pthread_self());
	while(estadoControlIgualA(ACTIVADO)) {
		socketYama = socketAceptar(listenerYama, ID_YAMA);
	if(socketYama != ERROR)
		yamaControlar();
	}
}

void yamaControlar() {
	if(estadoFileSystemIgualA(ESTABLE))
		yamaAceptar();
	else
		yamaRechazar();
}

void yamaAceptar() {
	mensajeEnviar(socketYama, ACEPTAR_YAMA, NULL, 0);
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] YAMA conectado"BLANCO);
	int estado = ACTIVADO;
	while(estado)
		yamaAtender(&estado);
}

void yamaDesconectar() {
	if(socketYama != ERROR)
		mensajeEnviar(socketYama, DESCONEXION, NULL, NULO);
}

void yamaRechazar() {
	socketCerrar(socketYama);
	imprimirMensaje(archivoLog, ROJO"[ERROR] YAMA fue rechazado, el File System no esta estable"BLANCO);
}

void yamaAtender(int* estado) {
	Mensaje* mensaje = mensajeRecibir(socketYama);
	switch(mensaje->header.operacion) {
		case DESCONEXION: yamaFinalizar(estado); break;
		case ENVIAR_BLOQUES: yamaEnviarBloques(mensaje->datos); break;
	}
	mensajeDestruir(mensaje);
}

void yamaFinalizar(int* estado) {
	socketCerrar(socketYama);
	*estado = DESACTIVADO;
	imprimirMensaje(archivoLog, AMARILLO"[CONEXION] YAMA desconectado"BLANCO);
}

void yamaEnviarBloques(Puntero datos) {
	int idMaster = *(Entero*)datos;
	String rutaDecente = stringTomarDesdePosicion(datos+sizeof(Entero), MAX_PREFIJO);
	Archivo* archivo = archivoBuscarPorRuta(rutaDecente);
	memoriaLiberar(rutaDecente);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo solicitado por YAMA no existe"BLANCO);
		mensajeEnviar(socketYama, ERROR_ARCHIVO, &idMaster, sizeof(Entero));
		return;
	}
	int cantidad = listaCantidadElementos(archivo->listaBloques);
	BloqueYama* bloques = yamaConvertirArchivo(archivo, idMaster);
	mensajeEnviar(socketYama, ENVIAR_BLOQUES, bloques, sizeof(Entero)+sizeof(BloqueYama)*cantidad);
	memoriaLiberar(bloques);
}

BloqueYama* yamaConvertirArchivo(Archivo* archivo, Entero idMaster) {
	int indice;
	int cantidad = listaCantidadElementos(archivo->listaBloques);
	Puntero bloques = memoriaAlocar(sizeof(Entero)+sizeof(BloqueYama)*cantidad);
	listaOrdenar(archivo->listaBloques, (Puntero)bloqueOrdenarPorNumero);
	memcpy(bloques, &idMaster, sizeof(Entero));
	for(indice = 0; indice < cantidad; indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		BloqueYama bloqueYama = yamaConvertirBloque(bloque);
		memcpy(bloques+sizeof(Entero)+sizeof(BloqueYama)*indice, &bloqueYama, sizeof(BloqueYama));
	}
	return bloques;
}

BloqueYama yamaConvertirBloque(Bloque* bloque) {
	BloqueYama bloqueYama;
	Copia* copia1 = listaObtenerElemento(bloque->listaCopias, 0);
	Copia* copia2 = listaObtenerElemento(bloque->listaCopias, 1);
	Direccion direccion1 = nodoObtenerDireccion(copia1->nombreNodo);
	Direccion direccion2 = nodoObtenerDireccion(copia2->nombreNodo);
	memcpy(&bloqueYama.direccionCopia1, &direccion1, sizeof(Dir));
	memcpy(&bloqueYama.direccionCopia2, &direccion2, sizeof(Dir));
	bloqueYama.numeroBloqueCopia1 = copia1->bloqueNodo;
	bloqueYama.numeroBloqueCopia2 = copia2->bloqueNodo;
	bloqueYama.bytesUtilizados = bloque->bytesUtilizados;
	return bloqueYama;
}

//--------------------------------------- Funciones de Worker -------------------------------------


void workerAvisarAlmacenado(int resultado, Socket unSocket) {
	if(resultado != ERROR)
		mensajeEnviar(unSocket, EXITO, NULL, 0);
	else
		mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

int workerAlmacenarBloque(Archivo* archivo, Mensaje* mensaje, Entero* numeroBloque, int* resultado) {
	*resultado = bloqueGuardar(archivo, mensaje->datos+sizeof(Entero), *(Entero*)mensaje->datos, *numeroBloque);
	if(*resultado == ERROR)
		return DESACTIVADO;
	*numeroBloque = *numeroBloque + 1;
	return ACTIVADO;
}

int workerAlmacenarArchivo(Archivo* archivo, Socket socketWorker) {
	int estado = ACTIVADO;
	int resultado = OK;
	Entero numeroBloque = 0;
	while(estado) {
		Mensaje* mensaje = mensajeRecibir(socketWorker);
		switch(mensaje->header.operacion) {
			case DESCONEXION: estado = DESACTIVADO; socketCerrar(socketWorker); resultado = ERROR; break;
			case ALMACENAR_BLOQUE: estado = workerAlmacenarBloque(archivo, mensaje, &numeroBloque, &resultado); break;
			case ALMACENADO_FINAL: estado = DESACTIVADO;
		}
		mensajeDestruir(mensaje);
	}
	return resultado;
}

int workerAlmacenadoFinal(String pathYama, Socket socketWorker) {
	String rutaDecente = stringTomarDesdePosicion(pathYama, MAX_PREFIJO);
	if(!rutaValida(rutaDecente)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return ERROR;
	}
	String rutaDirectorio = rutaObtenerDirectorio(rutaDecente);
	Directorio* directorio = directorioBuscar(rutaDirectorio);
	if(directorio == NULL) {
		memoriaLiberar(rutaDirectorio);
		memoriaLiberar(rutaDecente);
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio ingresado no existe"BLANCO);
		return ERROR;
	}
	String nombreArchivo = rutaObtenerUltimoNombre(rutaDecente);
	if(archivoExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Un archivo con ese nombre ya existe en el directorio destino"BLANCO);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(rutaDirectorio);
		memoriaLiberar(rutaDecente);
		return ERROR;
	}
	if(directorioExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Un directorio con ese nombre ya existe en el directorio destino"BLANCO);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(rutaDirectorio);
		memoriaLiberar(rutaDecente);
		return ERROR;
	}
	Archivo* archivo = archivoCrear(nombreArchivo, directorio->identificador, ARCHIVO_TEXTO);
	memoriaLiberar(nombreArchivo);
	memoriaLiberar(rutaDirectorio);
	memoriaLiberar(rutaDecente);
	int estado = workerAlmacenarArchivo(archivo, socketWorker);
	archivoControlar(archivo, estado);
	return estado;
}

void workerListener() {
	hiloDetach(pthread_self());
	while(estadoControlIgualA(ACTIVADO)) {
		Socket unSocket = socketAceptar(listenerWorker, ID_WORKER);
		if(unSocket != ERROR) {
			Socket* socketWorker = memoriaAlocar(sizeof(Socket));
			*socketWorker = unSocket;
			hiloCrear(&hiloWorker, (Puntero)workerAtender, socketWorker);
		}
	}
}

void workerAtender(Socket* socketWorker) {
	Mensaje* mensaje = mensajeRecibir(*socketWorker);
	String pathYama = string_from_format("%s", mensaje->datos);
	mensajeDestruir(mensaje);
	int resultado = workerAlmacenadoFinal(pathYama,*socketWorker);
	workerAvisarAlmacenado(resultado, *socketWorker);
	memoriaLiberar(pathYama);
	memoriaLiberar(socketWorker);
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
			return false;
	return true;
}

bool consolaArgumentoEsNumero(String ruta) {
	int indice;
	for(indice=0; indice<stringLongitud(ruta); indice++)
		if(ruta[indice] < 48 || ruta[indice] > 57)
			return false;
	return true;
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
	if(stringIguales(comando, BITMAPS))
		return ID_BITMAPS;
	if(stringIguales(comando, MKNDIR))
		return ID_MKNDIR;
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
			stringIguales(comando, CPTO) ||
			stringIguales(comando, MKNDIR);
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
			stringIguales(comando, NODES) ||
			stringIguales(comando, BITMAPS) ||
			stringIguales(comando, MKNDIR);
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
	else
		comando->identificador = ERROR;
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
		case ID_CPTO: comandoCopiarArchivoDeYamaFS(comando, ACTIVADO); break;
		case ID_CPBLOCK: comandoCopiarBloque(comando); break;
		case ID_MD5: comandoObtenerMD5DeArchivo(comando); break;
		case ID_LS: comandoListarDirectorio(comando); break;
		case ID_INFO: comandoInformacionArchivo(comando); break;
		case ID_NODES: comandoInformacionNodos(); break;
		case ID_BITMAPS: comandoInformacionBitmaps(); break;
		case ID_MKNDIR: comandoCrearVariosDirectorios(comando); break;
		case ID_HELP: comandoAyuda(); break;
		case ID_EXIT: comandoFinalizar();break;
		default: comandoError(); break;
	}
}

void consolaDestruirComando(Comando* comando, String entrada) {
	consolaLiberarArgumentos(comando->argumentos);
	memoriaLiberar(entrada);
}

String consolaLeerEntrada() {
	int indice;
	int caracterLeido;
	char* cadena = memoriaAlocar(MAX_STRING);
	for(indice = 0; caracterDistintos((caracterLeido= caracterObtener()),ENTER) && indice < MAX_STRING; indice++)
		cadena[indice] = caracterLeido;
	if(indice == MAX_STRING) {
		memoriaLiberar(cadena);
		imprimirMensaje(archivoLog, ROJO"[ERROR] Entrada demasiado larga"BLANCO);
		return NULL;
	}
	else
		cadena[indice] = FIN;
	return cadena;
}

bool consolaSinFormatear() {
	return estadoEjecucionIgualA(NUEVO) &&
			estadoFileSystemIgualA(INESTABLE);
}

void consolaEjecutar() {
	char* entrada = consolaLeerEntrada();
	if(entrada == NULL)
		return;
	if(consolaSinFormatear() && stringIguales(entrada, FORMAT)) {
		comandoFormatearFileSystem();
		memoriaLiberar(entrada);
		return;
	}
	if(consolaSinFormatear()) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Primero debe formatear el File System para utilizarlo"BLANCO);
		memoriaLiberar(entrada);
		return;
	}
	if(estadoControlIgualA(ACTIVADO)) {
		Comando comando;
		consolaConfigurarComando(&comando, entrada);
		consolaEjecutarComando(&comando);
		consolaDestruirComando(&comando, entrada);
	}
}

void consolaAtenderComandos() {
	while(estadoControlIgualA(ACTIVADO))
		consolaEjecutar();
}

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem() {
	archivoListaDestruir();
	directorioListaDestruir();
	bitmapDirectoriosDestruir();
	nodoFormatearConectados();
	metadataIniciar();
	estadoFileSystemEstable();
	imprimirMensaje(archivoLog, "[FORMATEO] El File System fue formateado");
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] El File System esta estable"BLANCO);
}

int comandoCrearDirectorio(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return ERROR;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El directorio raiz no puede ser creado"BLANCO);
		return ERROR;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo != NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] Un archivo ya tiene el mismo nombre"BLANCO);
		return ERROR;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	if(directorio != NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] Un directorio ya tiene el mismo nombre"BLANCO);
		return ERROR;
	}
	ControlDirectorio* control = directorioControlCrear(comando->argumentos[1]);
	int resultado = OK;
	while(stringValido(control->nombreDirectorio)) {
		directorioBuscarIdentificador(control);
		if (directorioExisteIdentificador(control->identificadorDirectorio))
			directorioControlarEntradas(control, comando->argumentos[1]);
		int estado = directorioCrearDirectoriosRestantes(control, comando->argumentos[1]);
		if(estado == ERROR) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] Se alcanzo el limite de directorios permitidos (100)"BLANCO);
			resultado = ERROR;
			break;
		}
		if(estado == -2) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El nombre del directorio es demasiado largo"BLANCO);
			resultado = ERROR;
			break;
		}
	directorioControlSetearNombre(control);
	}
	int indice;
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
	return resultado;
}

void comandoRenombrar(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(rutaValida(comando->argumentos[2])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El nombre no es valido"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El directorio raiz no puede ser renombrado"BLANCO);
		return;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL && directorio == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo o directorio no existe"BLANCO);
		return;
	}
	if(directorio != NULL) {
		int existeArchivo = archivoExiste(directorio->identificadorPadre, comando->argumentos[2]);
		if(directorioExiste(directorio->identificadorPadre, comando->argumentos[2]) || existeArchivo) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El nuevo nombre para el directorio ya existe"BLANCO);
			return;
		}
		stringCopiar(directorio->nombre, comando->argumentos[2]);
		directorioPersistir();
		imprimirMensaje2(archivoLog, "[DIRECTORIO] El directorio %s fue renombrado por %s", comando->argumentos[1], directorio->nombre);
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
		imprimirMensaje2(archivoLog, "[ARCHIVO] El archivo %s fue renombrado por %s", comando->argumentos[1], archivo->nombre);

	}
}

void comandoMover(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(!rutaValida(comando->argumentos[2])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio raiz no puede ser movido"BLANCO);
		return;
	}
	Directorio* directorioNuevoPadre;
	if(stringIguales(comando->argumentos[2], RAIZ))
		directorioNuevoPadre = directorioBuscarEnLista(0);
	else
		directorioNuevoPadre = directorioBuscar(comando->argumentos[2]);
	if(directorioNuevoPadre == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio destino no existe"BLANCO);
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	if(archivo == NULL && directorio == NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El archivo o directorio no existe"BLANCO);
		return;
	}
	if(directorio != NULL) {
		int existeArchivo = archivoExiste(directorioNuevoPadre->identificador, directorio->nombre);
		if(directorioExiste(directorioNuevoPadre->identificador, directorio->nombre) || existeArchivo)  {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio destino  ya tiene un archivo o directorio con el mismo nombre"BLANCO);
			return;
		}
		if(directorioEsHijoDe(directorioNuevoPadre, directorio)) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio no puede moverse a uno de sus subdirectorios"BLANCO);
			return;
		}
		if(directorioNuevoPadre->identificador == directorio->identificador) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio no puede moverse a si mismo"BLANCO);
			return;
		}

		directorio->identificadorPadre = directorioNuevoPadre->identificador;
		directorioPersistir();
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensaje2(archivoLog, "[DIRECTORIO] El directorio %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
	else {
		int existeDirectorio = directorioExiste(directorioNuevoPadre->identificador, archivo->nombre);
		if(archivoExiste(directorioNuevoPadre->identificador, archivo->nombre) || existeDirectorio) {
			imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio destino  ya tiene un archivo o directorio con el mismo nombre"BLANCO);
			return;
		}
		String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
		fileLimpiar(rutaArchivo);
		memoriaLiberar(rutaArchivo);
		archivo->identificadorPadre = directorioNuevoPadre->identificador;
		archivoPersistir(archivo);
		archivoPersistirControl();
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensaje2(archivoLog, "[DIRECTORIO] El archivo %s fue movido a %s", nombre, comando->argumentos[2]);
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
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[2], RAIZ)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(!consolaArgumentoEsNumero(comando->argumentos[3])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El numero de bloque no es valido"BLANCO);
		return;
	}
	if(!consolaArgumentoEsNumero(comando->argumentos[4])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El numero de copia del bloque no es valido"BLANCO);
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[2]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no existe"BLANCO);
		return;
	}
	int numeroBloque = atoi(comando->argumentos[3]);
	int numeroCopia = atoi(comando->argumentos[4]);
	Bloque* bloque = listaObtenerElemento(archivo->listaBloques, numeroBloque);

	if(bloque == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El numero de bloque no existe"BLANCO);
		return;
	}
	Copia* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
	if(copia == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El numero de copia no existe"BLANCO);
		return;
	}
	if(listaCantidadElementos(bloque->listaCopias) == 1) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] No se puede eliminar la copia ya que es la ultima"BLANCO);
		return;
	}
	listaEliminarDestruyendoElemento(bloque->listaCopias, numeroCopia, (Puntero)copiaDestruir);
	archivoPersistir(archivo);
	nodoPersistir();
	imprimirMensaje3(archivoLog, "[BLOQUE] La copia N°%i del bloque N°%i del archivo %s ha sido eliminada",(int*)numeroCopia, (int*)numeroBloque, comando->argumentos[2]);
}


void comandoEliminarDirectorio(Comando* comando) {
	String ruta = comando->argumentos[2];
	if(!rutaValida(ruta)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(ruta, RAIZ)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio raiz no puede ser eliminado"BLANCO);
		return;
	}
	Directorio* directorio = directorioBuscar(ruta);
	if(directorio == NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El directorio no existe"BLANCO);
		return;
	}
	if(directorioTieneAlgo(directorio->identificador))
		imprimirMensaje1(archivoLog,ROJO"[ERROR] El directorio %s no esta vacio"BLANCO,ruta);
	else {
		directorioEliminar(directorio->identificador);
		imprimirMensaje1(archivoLog,"[DIRECTORIO] El directorio %s ha sido eliminado",ruta);
	}
}

void comandoEliminarArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio raiz no puede ser eliminado"BLANCO);
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no existe"BLANCO);
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
	imprimirMensaje1(archivoLog, "[ARCHIVO] El archivo %s ha sido eliminado", comando->argumentos[1]);
}

void comandoListarDirectorio(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		directorioMostrarArchivos(0);
		return;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	if(directorio != NULL)
		directorioMostrarArchivos(directorio->identificador);
	else
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio no existe"BLANCO);
}

void comandoInformacionArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no existe"BLANCO);
		return;
	}
	printf("[ARCHIVO] Nombre: %s\n", archivo->nombre);
	printf("[ARCHIVO] Tipo: %s\n", archivo->tipo);
	printf("[ARCHIVO] Ubicacion: %s\n", comando->argumentos[1]);
	printf("[ARCHIVO] ID Padre: %i\n", archivo->identificadorPadre);
	listaOrdenar(archivo->listaBloques, (Puntero)bloqueOrdenarPorNumero);
	int indice;
	int tamanio = 0;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		tamanio+= bloque->bytesUtilizados;
	}
	printf("[ARCHIVO] Tamanio: %i bytes\n", tamanio);
	printf("[ARCHIVO] Bloques: %i\n", listaCantidadElementos(archivo->listaBloques));
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		printf("[ARCHIVO] Bloque %i: %i bytes\n", indice, bloque->bytesUtilizados);
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			Copia* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			printf("[ARCHIVO] Copia %i en: Nodo: %s | Bloque: %i\n", indiceCopia, copiaBloque->nombreNodo, copiaBloque->bloqueNodo);
		}
	}
}

void comandoCopiarBloque(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(stringIguales(comando->argumentos[1], RAIZ)) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	if(!consolaArgumentoEsNumero(comando->argumentos[2])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El numero de bloque no es valido"BLANCO);
		return;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no existe"BLANCO);
		return;
	}
	Nodo* nodoReceptor = nodoBuscarPorNombre(comando->argumentos[3]);
	if(nodoReceptor == NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El nodo no existe"BLANCO);
		return;
	}
	if(nodoBuscarBloqueLibre(nodoReceptor) == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El Nodo no tiene bloques libres"BLANCO);
		return;
	}
	Entero numeroBloque = atoi(comando->argumentos[2]);
	Bloque* bloque = listaObtenerElemento(archivo->listaBloques, numeroBloque);
	if(bloque == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El numero de bloque no existe"BLANCO);
		return;
	}
	if(bloqueEstaEnNodo(bloque, nodoReceptor)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El Nodo ya posee una copia del bloque"BLANCO);
		return;
	}
	mutexBloquear(mutexTarea);
	if(nodoDesconectado(nodoReceptor)) {
		mutexDesbloquear(mutexTarea);
		imprimirMensaje(archivoLog, ROJO"[ERROR] El nodo que va a guardar la nueva copia no esta conectado"BLANCO);
		return;
	}
	int indice;
	int bloqueSinEnviar = true;
	int cantidadCopias = listaCantidadElementos(bloque->listaCopias);
	for(indice = 0; indice < cantidadCopias && bloqueSinEnviar; indice++) {
		Copia* copia = listaObtenerElemento(bloque->listaCopias, indice);
		Nodo* nodoConBloque = nodoBuscarPorNombre(copia->nombreNodo);
		if(nodoConectado(nodoConBloque)) {
			nodoConBloque->mensajeEnviado = ACTIVADO;
			nodoReceptor->mensajeEnviado = ACTIVADO;
			mutexBloquear(mutexNodo);
			nodoBuffer = nodoReceptor;
			mutexDesbloquear(mutexNodo);
			mutexBloquear(mutexArchivo);
			archivoBuffer = archivo;
			mutexDesbloquear(mutexArchivo);
			mutexBloquear(mutexBloque);
			bloqueBuffer = bloque;
			mutexDesbloquear(mutexBloque);
			mensajeEnviar(nodoConBloque->socket, COPIAR_BLOQUE, &copia->bloqueNodo, sizeof(Entero));
			bloqueSinEnviar = false;
		}
	}
	mutexDesbloquear(mutexTarea);
	if(bloqueSinEnviar) {
		imprimirMensaje(archivoLog, "[ERROR] No hay nodos conectados para obtener la copia");
		return;
	}
}

void comandoCopiarArchivoAYamaFS(Comando* comando) {
	archivoAlmacenar(comando);
}

int comandoCopiarArchivoDeYamaFS(Comando* comando, int rutaYama) {
	if(rutaYama == ACTIVADO) {
		if((!rutaTieneAlMenosUnaBarra(comando->argumentos[1]) || !rutaBarrasEstanSeparadas(comando->argumentos[1])) || !rutaTienePrefijoYama(comando->argumentos[1])) {
			imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
			return ERROR;
		}
		rutaYamaDecente(comando, 1);
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no existe"BLANCO);
		return ERROR;
	}
	if(!archivoDisponible(archivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo no esta disponible"BLANCO);
		return ERROR;
	}
	File file = fileAbrir(comando->argumentos[2], ESCRITURA);
	if(file == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] La ruta es invalida"BLANCO);
		return ERROR;
	}
	fileCerrar(file);
	mutexBloquear(mutexRuta);
	stringCopiar(rutaBuffer, comando->argumentos[2]);
	mutexDesbloquear(mutexRuta);
	listaOrdenar(archivo->listaBloques, (Puntero)bloqueOrdenarPorNumero);
	int indice;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques) ;indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		int copiaSinEnviar = true;
		int indiceCopias;
		listaOrdenar(bloque->listaCopias, (Puntero)copiaOrdenarPorActividadDelNodo);
		semaforoWait(semaforoTarea);
		mutexBloquear(mutexTarea);
		for(indiceCopias=0; indiceCopias<listaCantidadElementos(bloque->listaCopias) && copiaSinEnviar; indiceCopias++) {
			Copia* copia = listaObtenerElemento(bloque->listaCopias, indiceCopias);
			Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
			if(nodoConectado(nodo)) {
				mutexBloquear(mutexBloque);
				bloqueBuffer = bloque;
				mutexDesbloquear(mutexBloque);
				mensajeEnviar(nodo->socket, COPIAR_ARCHIVO, &copia->bloqueNodo, sizeof(Entero));
				nodoMensaje(nodo,ACTIVADO);
				nodo->tareasRealizadas++;
				copiaSinEnviar = false;
			}
		}
		mutexDesbloquear(mutexTarea);
		if(copiaSinEnviar) {
			imprimirMensaje1(archivoLog, ROJO"[ERROR] No se pudo leer el bloque N°%d, se aborta la operacion"BLANCO, (int*)indice);
			mutexBloquear(mutexRuta);
			fileLimpiar(rutaBuffer);
			mutexDesbloquear(mutexRuta);
			semaforoSignal(semaforoTarea);
			nodoLimpiarActividades();
			return ERROR;
		}
	}
	nodoLimpiarActividades();
	if(rutaYama == ACTIVADO)
		imprimirMensaje2(archivoLog, "[ARCHIVO] El archivo %s se copio en %s", archivo->nombre, comando->argumentos[2]);
	return OK;
}

void comandoMostrarArchivo(Comando* comando) {
	archivoLeer(comando);
}

void comandoObtenerMD5DeArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return;
	}
	String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[1]);
	String MD5Archivo = memoriaAlocar(MAX_STRING);
	String ruta = string_from_format("%s/%s", configuracion->rutaMetadata, nombreArchivo);
	comando->argumentos[2] = memoriaAlocar(MAX_STRING);
	stringCopiar(comando->argumentos[2], ruta);
	if(comandoCopiarArchivoDeYamaFS(comando, DESACTIVADO) == ERROR) {
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(MD5Archivo);
		memoriaLiberar(ruta);
		return;
	}
	sleep(1);
	int pidHijo;
	int longitudMensaje;
	int descriptores[2];
	pipe(descriptores);
	pidHijo = fork();
	if(pidHijo == ERROR)
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el fork (Estas en problemas amigo)"BLANCO);
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
	int indice;
	for(indice = 0; MD5Archivo[indice] != ESPACIO; indice++);
	String md5 = stringTomarDesdeInicio(MD5Archivo, indice);
	printf("[ARCHIVO] El MD5 del archivo es %s\n", md5);
	log_info(archivoLog,"[ARCHIVO] El MD5 del archivo es %s", md5);
	fileLimpiar(ruta);
	memoriaLiberar(ruta);
	memoriaLiberar(MD5Archivo);
	memoriaLiberar(md5);
	memoriaLiberar(nombreArchivo);
}

void comandoInformacionNodos() {
	int indice;
	int bloquesTotales = 0;
	int bloquesLibres = 0;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		puts("------------------------------------------");
		printf("Nombre: %s\n", nodo->nombre);
		printf("IP: %s | Puerto: %s\n", nodo->ip, nodo->puerto);
		printf("Bloques totales %i\n", nodo->bloquesTotales);
		printf("Bloques libres %i\n", nodo->bloquesLibres);
		bloquesTotales += nodo->bloquesTotales;
		bloquesLibres += nodo->bloquesLibres;
		if(nodoConectado(nodo))
			puts("Estado: Conectado");
		else
			puts("Estado: Desconectado");

	}
	if(indice == 0) {
		puts("No hay nodos conectados");
		return;
	}
	puts("------------------------------------------");
	printf("Bloques totales %i\n", bloquesTotales);
	printf("Bloques libres %i\n", bloquesLibres);
}

void comandoInformacionBitmaps() {
	int indice;
	for(indice = 0; indice < nodoListaCantidad(); indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		printf("Bitmap %s:\n", nodo->nombre);
		int bit;
		for(bit=0; bit < nodo->bloquesTotales; bit++)
			printf("%d", bitmapBitOcupado(nodo->bitmap, bit));
		puts("");
		puts("------------------------------------------");
	}
}

void comandoCrearVariosDirectorios(Comando* comando) {
	int indice;
	int cantidadDirectorios = atoi(comando->argumentos[2]);
	int resultado;
	String base = comando->argumentos[1];
	for(indice = 0; indice < cantidadDirectorios; indice++) {
		comando->argumentos[1] = string_from_format("%s%i", base, indice);
		resultado = comandoCrearDirectorio(comando);
		memoriaLiberar(comando->argumentos[1]);
		if(resultado == ERROR)
			break;
	}
	comando->argumentos[1] = base;
}

void comandoAyuda() {
	puts("-------------------------------");
	puts("1)  format | Formatea YamaFS");
	puts("2)  mkdir <Path-yama-dir> | Crea un directorio");
	puts("3)  ls <Path-yama-dir> | Lista un directorio");
	puts("4)  rename <Path-yama-dir> <Nuevo-nombre> | Renombra un archivo o directorio");
	puts("5)  mv <Path-yama-dir> <Path-yama-dir> | Mueve un archivo o directorio");
	puts("6)  rm <Path-yama-arch> | Elimina un archivo");
	puts("7)  rm -b <Path-yama-arch> <N°bloque> <N°copia> | Elimina un bloque de un archivo");
	puts("8)  rm -d <Path-yama-dir> | Elimina un directorio");
	puts("9)  cpto <Path-yama-arch> <Path-local-arch> | Copia un archivo de YamaFS al FS local");
	puts("10) cpfrom -t  <Path-local-arch> <Path-yama-arch> | Copia un archivo de texto a YamaFS");
	puts("11) cpfrom -b <Path-local-arch> <Path-yama-arch> | Copia un archivo binario a YamaFS");
	puts("12) cpblock <Path-yama-arch> <N°bloque> <Nombre-Nodo>");
	puts("13) md5 <Path-yama-arch> | Retorna el MD5 del archivo");
	puts("14) cat <Path-yama-arch> | Muestra el contenido de un archivo");
	puts("15) info <Path-yama-arch> | Muestra la informacion de un archivo");
	puts("16) nodes | Muestra la informacion de los nodos");
	puts("17) bitmaps | Muestro los bitmaps de los nodos");
	puts("18) exit | Finaliza YamaFS");
}

void comandoFinalizar() {
	estadoControlDesactivar();
	shutdown(listenerDataNode, SHUT_RDWR);
	shutdown(listenerYama, SHUT_RDWR);
	shutdown(listenerWorker, SHUT_RDWR);
	socketCerrar(listenerYama);
	socketCerrar(listenerWorker);
	yamaDesconectar();
	nodoDesconectarATodos();
}

void comandoError() {
	imprimirMensaje(archivoLog, ROJO"[ERROR] Comando invalido"BLANCO);
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
			imprimirMensaje1(archivoLog, "[DIRECTORIO] (d) %s ", directorio->nombre);
			flag = ACTIVADO;
		}
	}
	for(indice=0; indice < archivoListaCantidad(); indice++) {
		Archivo* archivo = archivoListaObtener(indice);
		if(archivo->identificadorPadre == identificadorPadre) {
			imprimirMensaje1(archivoLog, "[DIRECTORIO] (a) %s", archivo->nombre);
			flag = ACTIVADO;
		}
	}
	if(flag == DESACTIVADO)
		imprimirMensaje(archivoLog, "[DIRECTORIO] El directorio esta vacio");
}

int directorioBuscarIdentificadorLibre() {
	int indice;
	for(indice = 0; directorioIndiceEstaOcupado(indice); indice++);
	if(indice < MAX_DIR)
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
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo o directorio ya existe"BLANCO);
	else
		control->identificadorPadre = control->identificadorDirectorio;
	control->indiceNombresDirectorios++;
}

void directorioIniciarControl() {
	bitmapDirectoriosCrear();
	directoriosDisponibles = MAX_DIR;
	directorioCrearConPersistencia(0, "root", -1);
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
	return OK;
}

int directorioCrearDirectoriosRestantes(ControlDirectorio* control, String rutaDirectorio) {
	while(stringValido(control->nombresDirectorios[control->indiceNombresDirectorios])) {
		int indice = directorioBuscarIdentificadorLibre();
		if(indice == ERROR)
			return ERROR;
		int estado = directorioCrearConPersistencia(indice, control->nombresDirectorios[control->indiceNombresDirectorios], control->identificadorPadre);
		if(estado == ERROR)
			return -2;
		control->identificadorPadre = indice;
		control->indiceNombresDirectorios++;
	}
	imprimirMensaje1(archivoLog, "[DIRECTORIO] El directorio %s fue creado", rutaDirectorio);
	return OK;
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
	if(stringIguales(path, RAIZ))
		return ID_RAIZ;
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
	return indice < MAX_DIR;
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
		imprimirMensaje1(archivoLog, ROJO"[ERROR] No se encontro el archivo %s"BLANCO, rutaDirectorios);
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

void archivoGuardar(Archivo* archivo) {
	archivoListaAgregar(archivo);
	archivoPersistir(archivo);
	archivoPersistirControl();
	nodoPersistir();
	imprimirMensaje1(archivoLog, "[ARCHIVO] El archivo %s fue copiado en File System", archivo->nombre);
}

void archivoControlar(Archivo* archivo, int estado) {
	if(estado != ERROR)
		archivoGuardar(archivo);
	else
		archivoDestruir(archivo);
}

int archivoAlmacenarBinario(Archivo* archivo, File file) {
	String buffer = stringCrear(BLOQUE);
	size_t bytes;
	int estado = OK;
	int numeroBloque;
	for(numeroBloque = 0; (bytes = fread(buffer, sizeof(char), BLOQUE, file)) == BLOQUE; numeroBloque++) {
		estado = bloqueGuardar(archivo, buffer, bytes, numeroBloque);
		if(estado == ERROR)
			break;
	}
	if(estado != ERROR && bytes != NULO)
		estado = bloqueGuardar(archivo, buffer, bytes, numeroBloque);
	memoriaLiberar(buffer);
	return estado;
}

int archivoAlmacenarTexto(Archivo* archivo, File file) {
	String buffer = stringCrear(BLOQUE+1);
	String datos = stringCrear(BLOQUE);
	int bytesDisponibles = BLOQUE;
	int indiceDatos = 0;
	int estado = OK;
	int numeroBloque = 0;
	ssize_t tamanioBuffer;
	size_t limite = BLOQUE;
	while((tamanioBuffer = getline(&buffer, &limite,  file)) != ERROR) {
		if(tamanioBuffer > BLOQUE) {
			estado = ERROR;
			imprimirMensaje(archivoLog, ROJO"[ERROR] El registro leido es demasiado largo"BLANCO);
			break;
		}
		if(tamanioBuffer <= bytesDisponibles) {
			memcpy(datos+indiceDatos, buffer, tamanioBuffer);
			bytesDisponibles -= tamanioBuffer;
			indiceDatos += tamanioBuffer;
		}
		else {
			estado = bloqueGuardar(archivo, datos, BLOQUE-bytesDisponibles, numeroBloque);
			if(estado == ERROR)
				break;
			memoriaLiberar(datos);
			datos = stringCrear(BLOQUE);
			bytesDisponibles = BLOQUE;
			indiceDatos = 0;
			numeroBloque++;
			memcpy(datos+indiceDatos, buffer, tamanioBuffer);
			bytesDisponibles -= tamanioBuffer;
			indiceDatos += tamanioBuffer;
		}
	}
	if(estado != ERROR && bytesDisponibles < BLOQUE)
		estado = bloqueGuardar(archivo, datos, BLOQUE-bytesDisponibles, numeroBloque);
	memoriaLiberar(datos);
	memoriaLiberar(buffer);
	return estado;
}

int archivoAlmacenar(Comando* comando) {
	if(consolaflagInvalido(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Flag invalido"BLANCO);
		return ERROR;
	}
	if(rutaInvalidaAlmacenar(comando->argumentos[3])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return ERROR;
	}
	rutaYamaDecente(comando, 3);
	String rutaDirectorio = rutaObtenerDirectorio(comando->argumentos[3]);
	Directorio* directorio = directorioBuscar(rutaDirectorio);
	if(directorio == NULL) {
		memoriaLiberar(rutaDirectorio);
		imprimirMensaje(archivoLog, ROJO"[ERROR] El directorio ingresado no existe"BLANCO);
		return ERROR;
	}
	String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[3]);
	if(archivoExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Un archivo con ese nombre ya existe en el directorio destino"BLANCO);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(rutaDirectorio);
		return ERROR;
	}
	if(directorioExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Un directorio con ese nombre ya existe en el directorio destino"BLANCO);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(rutaDirectorio);
		return ERROR;
	}
	File file = fileAbrir(comando->argumentos[2], LECTURA);
	if(file == NULL) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] El archivo a copiar no existe"BLANCO);
		memoriaLiberar(nombreArchivo);
		memoriaLiberar(rutaDirectorio);
		return ERROR;
	}
	Archivo* archivo = archivoCrear(nombreArchivo, directorio->identificador, comando->argumentos[1]);
	memoriaLiberar(nombreArchivo);
	memoriaLiberar(rutaDirectorio);
	int estado = NULO;
	if(stringIguales(comando->argumentos[1], FLAG_B))
		estado = archivoAlmacenarBinario(archivo, file);
	else
		estado = archivoAlmacenarTexto(archivo, file);
	fileCerrar(file);
	archivoControlar(archivo, estado);
	return estado;
}

int archivoLeer(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] La ruta ingresada no es valida"BLANCO);
		return ERROR;
	}
	Archivo* archivo = archivoBuscarPorRuta(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] El archivo no existe"BLANCO);
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
		semaforoWait(semaforoTarea);
		mutexBloquear(mutexTarea);
		for(numeroCopia=0; numeroCopia <listaCantidadElementos(bloque->listaCopias) && bloqueSinImprimir; numeroCopia++) {
			Copia* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
			Nodo* nodo = nodoBuscarPorNombre(copia->nombreNodo);
			if(nodoConectado(nodo)) {
				mensajeEnviar(nodo->socket, LEER_BLOQUE ,&copia->bloqueNodo, sizeof(Entero));
				nodoMensaje(nodo, ACTIVADO);
				nodo->tareasRealizadas++;
				bloqueSinImprimir = false;
			}
		}
		mutexDesbloquear(mutexTarea);
		if(bloqueSinImprimir) {
			imprimirMensaje1(archivoLog,ROJO"[ERROR] No se pudo leer el bloque N°%d"BLANCO, (int*)numeroBloque);
			nodoLimpiarActividades();
			semaforoSignal(semaforoTarea);
			return ERROR;
		}
	}
	nodoLimpiarActividades();
	return OK;
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
		imprimirMensaje1(archivoLog, ROJO"[ERROR] No se encontro el archivo %s"BLANCO, rutaArchivos);
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
		imprimirMensaje1(archivoLog, ROJO"[ERROR] No se encontro el archivo %s"BLANCO, ruta);
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
	nodoSocket(nodo, nuevoSocket);
	nodoEstado(nodo, ACTIVADO);
	nodoMensaje(nodo, DESACTIVADO);
	nodo->tareasRealizadas = 0;
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

void nodoListaOrdenarPorUtilizacion() {
	mutexBloquear(mutexListaNodos);
	listaOrdenar(listaNodos, (Puntero)nodoOrdenarPorUtilizacion);
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
		mutexBloquear(mutexSocket);
		int resultado = nodo->socket == unSocket;
		mutexDesbloquear(mutexSocket);
		return resultado;
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

bool nodoOrdenarPorUtilizacion(Nodo* unNodo, Nodo* otroNodo) {
	double porcentajeUnNodo = (double)(unNodo->bloquesTotales - unNodo->bloquesLibres) / (double)unNodo->bloquesTotales;
	double porcentajeOtroNodo = (double)(otroNodo->bloquesTotales - otroNodo->bloquesLibres) / (double)otroNodo->bloquesTotales;
	return porcentajeUnNodo < porcentajeOtroNodo;
}

bool nodoOrdenarPorActividad(Nodo* unNodo, Nodo* otroNodo) {
	return unNodo->tareasRealizadas < otroNodo->tareasRealizadas;
}

bool nodoTieneBloquesLibres(Nodo* nodo) {
	return nodo->bloquesLibres > 0;
}

bool nodoConectado(Nodo* nodo) {
	mutexBloquear(mutexEstado);
	int resultado = nodo->estado == ACTIVADO;
	mutexDesbloquear(mutexEstado);
	return resultado;
}

bool nodoDesconectado(Nodo* nodo) {
	mutexBloquear(mutexEstado);
	int resultado = nodo->estado == DESACTIVADO;
	mutexDesbloquear(mutexEstado);
	return resultado;
}

void nodoVerificarBloquesLibres(Nodo* nodo) {
	if(nodo->bloquesLibres == NULO) {
		imprimirMensaje1(archivoLog, AMARILLO"[AVISO] El %s tiene todos sus bloques ocupados"BLANCO, nodo->nombre);
	}
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

bool nodoEstaConectado(Nodo* nuevoNodo) {
	Nodo* nodo = nodoBuscarPorNombre(nuevoNodo->nombre);
	if(nodo == NULL)
		return false;
	if(nodoConectado(nodo)) {
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
		nodo->tareasRealizadas = 0;
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

Direccion nodoObtenerDireccion(String nombreNodo) {
	Nodo* nodo = nodoBuscarPorNombre(nombreNodo);
	Direccion direccion;
	stringLimpiar(direccion.ip, 20);
	stringLimpiar(direccion.puerto, 20);
	stringCopiar(direccion.ip, nodo->ip);
	stringCopiar(direccion.puerto, nodo->puerto);
	return direccion;
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
	mutexBloquear(mutexListaNodos);
	listaIterar(listaNodos, (Puntero)nodoFormatear);
	mutexDesbloquear(mutexListaNodos);
}

void nodoDesconectar(Nodo* nodo) {
	if(nodoConectado(nodo)) {
		mutexBloquear(mutexSocket);
		mensajeEnviar(nodo->socket, DESCONEXION, NULL, NULO);
		mutexDesbloquear(mutexSocket);
	}
}

void nodoDesconectarATodos() {
	mutexBloquear(mutexListaNodos);
	listaIterar(listaNodos, (Puntero)nodoDesconectar);
	mutexDesbloquear(mutexListaNodos);
}

void nodoEstado(Nodo* nodo, int estado) {
	mutexBloquear(mutexEstado);
	nodo->estado = estado;
	mutexDesbloquear(mutexEstado);
}

void nodoMensaje(Nodo* nodo, int estado) {
	mutexBloquear(mutexMensaje);
	nodo->mensajeEnviado = estado;
	mutexDesbloquear(mutexMensaje);
}

bool nodoMensajeIgualA(Nodo* nodo, int estado) {
	mutexBloquear(mutexMensaje);
	int resultado = nodo->mensajeEnviado == estado;
	mutexDesbloquear(mutexMensaje);
	return resultado;
}

void nodoSocket(Nodo* nodo, int estado) {
	mutexBloquear(mutexSocket);
	nodo->socket = estado;
	mutexDesbloquear(mutexSocket);
}

Nodo* nodoActualizar(Nodo* nodoTemporal) {
	Nodo* nodo = nodoBuscarPorNombre(nodoTemporal->nombre);
	nodoEstado(nodo, ACTIVADO);
	nodoSocket(nodo, nodoTemporal->socket);
	stringCopiar(nodo->ip, nodoTemporal->ip);
	stringCopiar(nodo->puerto, nodoTemporal->puerto);
	nodoDestruir(nodoTemporal);
	return nodo;
}

void nodoAceptar(Nodo* nodo) {
	mensajeEnviar(nodo->socket, ACEPTAR_DATANODE, NULL, NULO);
	Hilo dataNode;
	hiloCrear(&dataNode, (Puntero)dataNodeHilo, nodo);
	imprimirMensaje1(archivoLog, AMARILLO"[AVISO] %s conectado"BLANCO, nodo->nombre);
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
	fwrite(nodo->bitmap->bitarray, sizeof(char), nodo->bitmap->size, archivo);
	fileCerrar(archivo);
}

void nodoRecuperarPersistencia() {
	File file = fileAbrir(rutaArchivos, LECTURA);
	if(file == NULL) {
		imprimirMensaje1(archivoLog, ROJO"[ERROR] No se encontro el archivo %s"BLANCO, rutaNodos);
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
		nodoEstado(nodo, DESACTIVADO);
		nodoMensaje(nodo, DESACTIVADO);
		nodoSocket(nodo, ERROR);
		stringCopiar(nodo->ip, "-");
		stringCopiar(nodo->puerto, "-");
		nodo->tareasRealizadas = 0;
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
		imprimirMensaje1(archivoLog, ROJO"[ERROR] No se encontro el archivo %s"BLANCO, rutaNodos);
		memoriaLiberar(ruta);
		return;
	}
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
	fread(nodo->bitmap->bitarray, sizeof(char), nodo->bitmap->size, file);
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

int bloqueEnviarCopias(Bloque* bloque, String buffer) {
	int copiasEnviadas = 0;
	int indice = 0;
	mutexBloquear(mutexTarea);
	nodoListaOrdenarPorUtilizacion();
	for(indice = 0; indice < nodoListaCantidad() && copiasEnviadas < MAX_COPIAS; indice++) {
		Nodo* nodo = nodoListaObtener(indice);
		if(nodoDisponible(nodo)) {
			copiaEnviarANodo(bloque, nodo, buffer);
			copiasEnviadas++;
		}
	}
	mutexDesbloquear(mutexTarea);
	if(copiasEnviadas < MAX_COPIAS) {
		imprimirMensaje1(archivoLog, ROJO"[ERROR] El bloque N°%i no pudo copiarse porque no hay nodos disponibles"BLANCO, (int*)bloque->numeroBloque);
		return ERROR;
	}
	return OK;
}

void bloqueCopiar(Nodo* nodo, Puntero datos, int* estado) {
	mutexBloquear(mutexNodo);
	mutexBloquear(mutexBloque);
	int numeroBloqueNodo = copiaEnviarANodo(bloqueBuffer, nodoBuffer, datos);
	mutexDesbloquear(mutexBloque);
	nodoActivarDesconexion(nodoBuffer, estado);
	mutexDesbloquear(mutexNodo);
	nodoActivarDesconexion(nodo, estado);
	mutexBloquear(mutexArchivo);
	archivoPersistir(archivoBuffer);
	mutexDesbloquear(mutexArchivo);
	nodoPersistir();
	mutexBloquear(mutexNodo);
	imprimirMensaje2(archivoLog, "[BLOQUE] El bloque fue copiado en el bloque N°%i de %s", (int*)numeroBloqueNodo, nodoBuffer->nombre);
	mutexDesbloquear(mutexNodo);
}

void nodoActivarDesconexion(Nodo* nodo, int* estado) {
	nodoMensaje(nodo, DESACTIVADO);
	if(nodoDesconectado(nodo))
		dataNodeDesactivar(nodo, estado);
}

void bloqueLeer(Nodo* nodo, Puntero datos, int* estado) {
	printf("%.1048576s", (String)datos);
	nodoActivarDesconexion(nodo, estado);
	semaforoSignal(semaforoTarea);
}

void bloqueCopiarArchivo(Nodo* nodo, Puntero datos, int* estado) {
	mutexBloquear(mutexRuta);
	File file = fileAbrir(rutaBuffer, "a+");
	mutexDesbloquear(mutexRuta);
	mutexBloquear(mutexBloque);
	fwrite(datos, sizeof(char), bloqueBuffer->bytesUtilizados, file);
	mutexDesbloquear(mutexBloque);
	fileCerrar(file);
	nodoActivarDesconexion(nodo, estado);
	semaforoSignal(semaforoTarea);
}

bool bloqueOrdenarPorNumero(Bloque* unBloque, Bloque* otroBloque) {
	return unBloque->numeroBloque < otroBloque->numeroBloque;
}

bool bloqueDisponible(Bloque* bloque) {
	return listaCumpleAlguno(bloque->listaCopias, (Puntero)copiaDisponible);
}

int bloqueGuardar(Archivo* archivo, String buffer, size_t bytes, Entero numeroBloque) {
	Bloque* bloque = bloqueCrear(bytes, numeroBloque);
	listaAgregarElemento(archivo->listaBloques, bloque);
	int estado = bloqueEnviarCopias(bloque, buffer);
	return estado;
}

BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer, int tamanioUtilizado) {
	BloqueNodo* bloqueNodo = memoriaAlocar(sizeof(BloqueNodo));
	bloqueNodo->numeroBloque = numeroBloque;
	memcpy(bloqueNodo->bloque, buffer, tamanioUtilizado);
	return bloqueNodo;
}

bool bloqueEstaEnNodo(Bloque* bloque, Nodo* nodo) {

	bool copiaEstaEnNodo(Copia* copia) {
		return stringIguales(copia->nombreNodo, nodo->nombre);
	}

	return listaCumpleAlguno(bloque->listaCopias, (Puntero)copiaEstaEnNodo);
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
	return nodoConectado(nodo);
}

int copiaGuardarEnNodo(Bloque* bloque, Nodo* nodo) {
	Entero numeroBloqueNodo = nodoBuscarBloqueLibre(nodo);
	Copia* copia = copiaCrear(numeroBloqueNodo, nodo->nombre);
	bitmapOcuparBit(nodo->bitmap, numeroBloqueNodo);
	nodo->bloquesLibres--;
	nodoVerificarBloquesLibres(nodo);
	listaAgregarElemento(bloque->listaCopias, copia);
	return numeroBloqueNodo;
}

int copiaEnviarANodo(Bloque* bloque, Nodo* nodo, String buffer) {
	int numeroBloqueNodo = copiaGuardarEnNodo(bloque, nodo);
	BloqueNodo* bloqueNodo = bloqueNodoCrear(numeroBloqueNodo, buffer, BLOQUE);
	mutexBloquear(mutexSocket);
	mensajeEnviar(nodo->socket, ESCRIBIR_BLOQUE, bloqueNodo, sizeof(Entero)+BLOQUE);
	mutexDesbloquear(mutexSocket);
	memoriaLiberar(bloqueNodo);
	return numeroBloqueNodo;
}

bool copiaOrdenarPorActividadDelNodo(Copia* unaCopia, Copia* otraCopia) {
	Nodo* unNodo = nodoBuscarPorNombre(unaCopia->nombreNodo);
	Nodo* otroNodo = nodoBuscarPorNombre(otraCopia->nombreNodo);
	return unNodo->tareasRealizadas < otroNodo->tareasRealizadas;
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
	directorioIniciarControl();
	archivoPersistirControl();
	nodoPersistir();
}

void metadataRecuperar() {
	archivoRecuperarPersistencia();
	directorioRecuperarPersistencia();
	nodoRecuperarPersistencia();
	estadoEjecucionNormal();
}

//--------------------------------------- Funciones de Ruta------------------------------------

String rutaObtenerUltimoNombre(String ruta) {
	int indice;
	int ultimaBarra;
	for(indice=0; ruta[indice] != FIN; indice++)
		if(caracterIguales(ruta[indice], BARRA))
			ultimaBarra = indice;
	String directorio = stringTomarDesdePosicion(ruta+1, ultimaBarra);
	return directorio;
}

bool rutaTieneAlMenosUnaBarra(String ruta) {
	return stringContiene(ruta, RAIZ);
}

bool rutaBarrasEstanSeparadas(String ruta) {
	int indice;
	for(indice=0; indice<stringLongitud(ruta); indice++) {
		if(caracterIguales(ruta[indice], BARRA)) {
			if(indice==0) {
				if(caracterIguales(ruta[indice+1], BARRA))
					return false;
			}
			else {
				if(indice==stringLongitud(ruta)-1 && ruta[indice-1] != ':')
					return false;
				else
					if(caracterIguales(ruta[indice-1], BARRA) ||
						caracterIguales(ruta[indice+1], BARRA))
						return false;
			}

		}
	}
	return true;
}

bool rutaTienePrefijoYama(String ruta) {
	String prefijo = stringTomarDesdeInicio(ruta, MAX_PREFIJO);
	int resultado = stringIguales(prefijo, PREFIJO);
	memoriaLiberar(prefijo);
	return resultado;
}

bool rutaValida(String ruta) {
	return 	caracterIguales(ruta[0], BARRA) &&
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

String rutaObtenerDirectorio(String pathArchivo) {
	int indice;
	int ultimaBarra;
	for(indice=0; pathArchivo[indice] != FIN; indice++)
		if(pathArchivo[indice] == BARRA)
			ultimaBarra = indice;
	String pathDirectorio;
	if(ultimaBarra != NULO)
		pathDirectorio = stringTomarDesdeInicio(pathArchivo, ultimaBarra);
	else
		pathDirectorio = string_from_format("/");
	return pathDirectorio;
}


void rutaBufferCrear() {
	mutexBloquear(mutexRuta);
	rutaBuffer = stringCrear(MAX_STRING);
	mutexDesbloquear(mutexRuta);
}

void rutaBufferDestruir() {
	mutexBloquear(mutexRuta);
	memoriaLiberar(rutaBuffer);
	mutexDesbloquear(mutexRuta);
}

void rutaYamaDecente(Comando* comando, int indice) {
	String rutaDecente = stringTomarDesdePosicion(comando->argumentos[indice], MAX_PREFIJO);
	memoriaLiberar(comando->argumentos[indice]);
	comando->argumentos[indice] = rutaDecente;
}

bool rutaInvalidaAlmacenar(String ruta) {
	return !rutaTieneAlMenosUnaBarra(ruta) ||
			!rutaBarrasEstanSeparadas(ruta) ||
			!rutaTienePrefijoYama(ruta) ||
			!rutaParaArchivo(ruta);
}

bool rutaParaArchivo(String ruta) {
	int indice;
	for(indice=0; ruta[indice] != FIN; indice++);
	if(indice == 0)
		return false;
	else
	return ruta[indice-1] != BARRA;
}

//--------------------------------------- Funciones de Estado ------------------------------------

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

void estadoControlDesactivar() {
	 mutexBloquear(mutexEstadoControl);
	 estadoControl = DESACTIVADO;
	 mutexDesbloquear(mutexEstadoControl);
}

bool estadoControlIgualA(int estado) {
	mutexBloquear(mutexEstadoControl);
	int resultado = estadoControl == estado;
	mutexDesbloquear(mutexEstadoControl);
	return resultado;
}

void estadoEjecucionNuevo() {
	mutexBloquear(mutexEstadoEjecucion);
	estadoEjecucion = NUEVO;
	mutexDesbloquear(mutexEstadoEjecucion);
}

void estadoEjecucionNormal() {
	mutexBloquear(mutexEstadoEjecucion);
	estadoEjecucion = NORMAL;
	mutexDesbloquear(mutexEstadoEjecucion);
}

bool estadoEjecucionIgualA(int estado) {
	mutexBloquear(mutexEstadoEjecucion);
	int resultado = estadoEjecucion == estado;
	mutexDesbloquear(mutexEstadoEjecucion);
	return resultado;
}

//--------------------------------------- Funciones de Bitmap de Directorios------------------------------------

void bitmapDirectoriosCrear() {
	mutexBloquear(mutexBitmapDirectorios);
	bitmapDirectorios = bitmapCrear(MAX_DIR);
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
	mutexTarea = memoriaAlocar(sizeof(Mutex));
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
	mutexMensaje = memoriaAlocar(sizeof(Mutex));
	mutexSocket = memoriaAlocar(sizeof(Mutex));
	mutexEstado = memoriaAlocar(sizeof(Mutex));
}

void semaforosIniciar() {
	semaforoIniciar(semaforoTarea, 1);
	mutexIniciar(mutexTarea);
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
	mutexIniciar(mutexMensaje);
	mutexIniciar(mutexSocket);
	mutexIniciar(mutexEstado);
}

void semaforosDestruir() {
	semaforoDestruir(semaforoTarea);
	memoriaLiberar(semaforoTarea);
	memoriaLiberar(mutexTarea);
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
	memoriaLiberar(mutexMensaje);
	memoriaLiberar(mutexSocket);
	memoriaLiberar(mutexEstado);
}
