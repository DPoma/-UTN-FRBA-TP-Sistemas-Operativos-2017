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

int main(int argc, String* argsv) {
	fileSystemIniciar(argsv[1]);
	fileSystemCrearConsola();
	fileSystemAtenderProcesos();
	fileSystemFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de File System -------------------------------------

//EL INICIO ESTA AL REVES PARA DEBUGEAR (SACAR ! A STRING IGUALES)

void fileSystemIniciar(String flag) {
	pantallaLimpiar();
	configuracionIniciarLog();
	configuracionIniciar();
	estadoSeguro = 0;
	fileSystemActivar();
	listaNodos = listaCrear();
	if(!stringIguales(flag, FLAG_C))
		metadataIniciar();
	else
		metadataRecuperar();
}

void fileSystemCrearConsola() {
	hiloCrear(&hiloConsola, (Puntero)consolaAtenderComandos, NULL);
}

void fileSystemAtenderProcesos() {
	Servidor* servidor = memoriaAlocar(sizeof(Servidor));
	servidorInicializar(servidor);
	while(fileSystemActivado())
		servidorAtenderPedidos(servidor);
	servidorFinalizar(servidor);
}

void fileSystemFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System finalizado");
	archivoLogDestruir(archivoLog);
	bitmapDestruir(bitmapDirectorios);
	configuracionDestruirRutas();
	listaDestruirConElementos(listaDirectorios, memoriaLiberar);
	listaDestruirConElementos(listaArchivos, (Puntero)archivoDestruir);
	listaDestruirConElementos(listaNodos, (Puntero)nodoDestruir);
	sleep(2);
}

bool fileSystemActivado() {
	return estadoFileSystem == ACTIVADO;
}

bool fileSystemDesactivado() {
	return estadoFileSystem == DESACTIVADO;
}

void fileSystemActivar() {
	estadoFileSystem = ACTIVADO;
}

void fileSystemDesactivar() {
	estadoFileSystem = DESACTIVADO;
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivo(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	stringCopiar(configuracion->puertoDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_DATANODE"));
	stringCopiar(configuracion->rutaMetadata, archivoConfigStringDe(archivoConfig, "RUTA_METADATA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYAMA);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
	imprimirMensajeUno(archivoLog, "[INFORMACION] Ruta de metadata: %s", configuracion->rutaMetadata);
}

void configuracionIniciarCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
	campos[2] = "RUTA_METADATA";
}

void configuracionIniciar() {
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionIniciarRutas();
	configuracionImprimir(configuracion);
}

void configuracionIniciarLog() {
	archivoLog = archivoLogCrear(RUTA_LOG, "FileSystem");
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System inicializado");
}

void configuracionIniciarRutas() {
	rutaDirectorioArchivos = string_from_format("%s/archivos", configuracion->rutaMetadata);
	rutaDirectorioBitmaps = string_from_format("%s/bitmaps", configuracion->rutaMetadata);
	rutaArchivos = string_from_format("%s/Archivos.txt", configuracion->rutaMetadata);
	rutaDirectorios = string_from_format("%s/Directorios.txt", configuracion->rutaMetadata);
	rutaNodos = string_from_format("%s/Nodos.bin", configuracion->rutaMetadata);
	rutaBuffer = string_from_format("%s/Buffer.txt", configuracion->rutaMetadata);
}

void configuracionDestruirRutas() {
	memoriaLiberar(configuracion);
	memoriaLiberar(rutaNodos);
	memoriaLiberar(rutaBuffer);
	memoriaLiberar(rutaDirectorioArchivos);
	memoriaLiberar(rutaDirectorioBitmaps);
	memoriaLiberar(rutaDirectorios);
	memoriaLiberar(rutaArchivos);
}

//--------------------------------------- Funciones de Servidor -------------------------------------

bool servidorObtenerMaximoSocket(Servidor* servidor) {
	return servidor->maximoSocket;
}

void servidorSetearListaSelect(Servidor* servidor) {
	servidor->listaSelect = servidor->listaMaster;
}

void servidorControlarMaximoSocket(Servidor* servidor, Socket unSocket) {
	if(socketEsMayor(unSocket, servidor->maximoSocket))
		servidor->maximoSocket = unSocket;
}

void servidorEsperarSolicitud(Servidor* servidor) {
	socketSelect(servidor->maximoSocket, &servidor->listaSelect);
}


void servidorFinalizarConexion(Servidor* servidor, Socket unSocket) {

	bool buscarNodoPorSocket(void* unNodo) {
		Nodo* nodo = (Nodo*)unNodo;;
		return nodo->socket == unSocket;
	}

	listaSocketsEliminar(unSocket, &servidor->listaMaster);
	if(socketEsDataNode(servidor, unSocket)) {
		listaSocketsEliminar(unSocket, &servidor->listaDataNodes);
		listaEliminarDestruyendoPorCondicion(listaNodos, (Puntero)buscarNodoPorSocket, (Puntero)nodoDestruir);
		imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha desconectado");
	}
	else
		imprimirMensaje(archivoLog, "[CONEXION] El proceso YAMA se ha desconectado");
	socketCerrar(unSocket);
}

void servidorRegistrarConexion(Servidor* servidor, Socket unSocket) {
	if(unSocket != ERROR) {
		listaSocketsAgregar(unSocket, &servidor->listaMaster);
		servidorControlarMaximoSocket(servidor, unSocket);
	}
}

Socket servidorAceptarYAMA(Servidor* servidor, Socket unSocket) {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(unSocket, ID_YAMA);
	if(nuevoSocket != ERROR) {
		servidor->procesoYAMA = nuevoSocket;
		imprimirMensaje(archivoLog, "[CONEXION] El proceso YAMA se ha conectado");
	}
	socketYama = nuevoSocket;
	return nuevoSocket;
}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeDesconexion(mensaje))
		servidorFinalizarConexion(servidor, unSocket);
	else {
		if(mensaje->header.operacion == 14) {;
			printf("%s", (String)mensaje->datos);
		}
	}
	mensajeDestruir(mensaje);
}

void servidorActivarListenerYAMA(Servidor* servidor) {
	servidor->listenerYAMA = socketCrearListener(configuracion->puertoYAMA);
	listaSocketsAgregar(servidor->listenerYAMA, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerYAMA);
}

void servidorActivarListenerDataNode(Servidor* servidor) {
	servidor->listenerDataNode = socketCrearListener(configuracion->puertoDataNode);
	listaSocketsAgregar(servidor->listenerDataNode, &servidor->listaMaster);
	servidorControlarMaximoSocket(servidor, servidor->listenerDataNode);
}

void servidorActivarListeners(Servidor* servidor) {
	servidorActivarListenerDataNode(servidor);
	servidorActivarListenerYAMA(servidor);
}

void servidorInicializar(Servidor* servidor) {
	servidor->maximoSocket = 0;
	listaSocketsLimpiar(&servidor->listaMaster);
	listaSocketsLimpiar(&servidor->listaSelect);
	listaSocketsLimpiar(&servidor->listaDataNodes);
	servidorActivarListeners(servidor);
}

void servidorFinalizar(Servidor* servidor) {
	memoriaLiberar(servidor);
}

void servidorRegistrarDataNode(Servidor* servidor, Socket nuevoSocket) {
	if(nuevoSocket != ERROR) {
		//if(estadoSeguro == 0) { TODO POnerlo lindo
			listaSocketsAgregar(nuevoSocket, &servidor->listaDataNodes);
			Mensaje* mensaje = mensajeRecibir(nuevoSocket);
			Nodo* nodo = nodoCrear(20480, 20480, nuevoSocket);
			memcpy(nodo->nombre, mensaje->datos, 10);
			memcpy(nodo->ip, mensaje->datos+10, 20);
			memcpy(nodo->puerto, mensaje->datos+30, 20);
			printf("IP = %s\n", nodo->ip);
			printf("puerto = %s\n", nodo->puerto);
			printf("nombre = %s\n", nodo->nombre);
			mensajeDestruir(mensaje);
			listaAgregarElemento(listaNodos, nodo);
			imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha conectado");
		//}
		//else {
			//Nodo* nodo = nodoRecuperar();
			//listaAgregarElemento(listaNodos, nodo);
			//imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha reconectado");
		//}
	}
}

void servidorAtenderPedidos(Servidor* servidor) {
	servidorSetearListaSelect(servidor);
	servidorEsperarSolicitud(servidor);
	Socket unSocket;
	Socket maximoSocket = servidor->maximoSocket;
	for(unSocket = 0; unSocket <= maximoSocket; unSocket++)
		if (socketRealizoSolicitud(servidor, unSocket)) {
			if(socketEsListener(servidor, unSocket)) {
				Socket nuevoSocket;
				if(socketEsListenerDataNode(servidor, unSocket)) {
					nuevoSocket = socketAceptar(unSocket, ID_DATANODE);
					if(fileSystemDesactivado())
						break;
					servidorRegistrarDataNode(servidor, nuevoSocket);
				}
				else
					nuevoSocket = servidorAceptarYAMA(servidor, unSocket);
				servidorRegistrarConexion(servidor, nuevoSocket);
			}
			else
				servidorRecibirMensaje(servidor, unSocket);
		}
}

//--------------------------------------- Funciones de Socket -------------------------------------

bool socketEsDataNode(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaDataNodes);
}

bool socketEsWorker(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaWorkers);
}

bool socketEsYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->procesoYAMA, unSocket);
}

bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerDataNode, unSocket);
}


bool socketEsListenerYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerYAMA, unSocket);
}

bool socketEsListener(Servidor* servidor, Socket unSocket) {
	return socketEsListenerDataNode(servidor, unSocket) || socketEsListenerYAMA(servidor, unSocket);
}

bool socketRealizoSolicitud(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaSelect);
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
			stringIguales(comando, EXIT);
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
	char* cadena = memoriaAlocar(MAX);
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
		case ID_CPFROM: comandoCopiarArchivoDeFS(comando); break;
		case ID_CPTO: comandoCopiarArchivoDeYFS(comando); break;
		case ID_CPBLOCK: comandoCopiarBloque(comando); break;
		case ID_MD5: comandoObtenerMD5(comando); break;
		case ID_LS: comandoListarDirectorio(comando); break;
		case ID_INFO: comandoInformacionArchivo(comando); break;
		case ID_EXIT: comandoFinalizar();break;
		default: comandoError(); break;
	}
}

void consolaDestruirComando(Comando* comando, String entrada) {
	consolaLiberarArgumentos(comando->argumentos);
	memoriaLiberar(entrada);
}

void consolaAtenderComandos() {
	hiloDetach(pthread_self());
	while(fileSystemActivado()) {
		char* entrada = consolaLeerEntrada();
		if(fileSystemActivado()) {
			Comando comando;
			consolaConfigurarComando(&comando, entrada);
			consolaEjecutarComando(&comando);
			consolaDestruirComando(&comando, entrada);
		}
	}
}

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem() {
	listaDestruirConElementos(listaDirectorios, memoriaLiberar);
	listaDestruirConElementos(listaArchivos, (Puntero)archivoDestruir);
	bitmapDestruir(bitmapDirectorios);
	metadataIniciar();
	nodoFormatearConectados();
	nodoPersistirConectados();
	estadoSeguro = 1;
	//TODO Ponerlo lindo
	int cantidadNodos = listaCantidadElementos(listaNodos);
	ConexionNodo nodos[cantidadNodos];
	int indice;
	for(indice=0; indice<cantidadNodos; indice++) {
		Nodo* nodo = listaObtenerElemento(listaNodos, indice);
		stringCopiar(nodos[indice].ip, nodo->ip);
		stringCopiar(nodos[indice].puerto, nodo->puerto);
	}
	mensajeEnviar(socketYama, 201, nodos, cantidadNodos*sizeof(ConexionNodo));
	imprimirMensaje(archivoLog, "[ESTADO] El File System ha sido formateado");
}

void comandoEliminar(Comando* comando) {
	if(stringIguales(comando->argumentos[1], FLAG_D))
		comandoEliminarDirectorio(comando);
	else if(stringIguales(comando->argumentos[1], FLAG_B))
		comandoEliminarBloque(comando);
	else
		comandoEliminarArchivo(comando);
}

void comandoEliminarBloque(Comando* comando) {
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
	Archivo* archivo = archivoBuscar(comando->argumentos[2]);
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
	CopiaBloque* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
	if(copia == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El numero de copia no existe");
		return;
	}
	if(listaCantidadElementos(bloque->listaCopias) == 1) {
		imprimirMensaje(archivoLog, "[ERROR] No se puede eliminar el bloque ya que es el ultimo");
		return;
	}
	archivoPersistirEliminarBloque(archivo, numeroBloque, numeroCopia);
	copiaBloqueEliminar(copia);
	listaEliminarDestruyendoElemento(bloque->listaCopias, numeroCopia, memoriaLiberar);
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
		directorioPersistirEliminar(directorio);
		directorioEliminarMetadata(directorio->identificador);
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
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo no existe");
		return;
	}
	int indice;
	for(indice=0; indice<listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		int indiceCopia;
		for(indiceCopia=0; indiceCopia<listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			CopiaBloque* copia = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			copiaBloqueEliminar(copia);
		}
	}
	int posicion = archivoObtenerPosicion(archivo);
	archivoPersistirControlEliminar(archivo);
	String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
	fileLimpiar(rutaArchivo);
	memoriaLiberar(rutaArchivo);
	listaEliminarDestruyendoElemento(listaArchivos, posicion, (Puntero)archivoDestruir);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] El archivo %s ha sido eliminado", comando->argumentos[1]);
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
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
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
		directorioPersistirRenombrar(directorio, comando->argumentos[2]);
		stringCopiar(directorio->nombre, comando->argumentos[2]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El directorio %s fue renombrado por %s", comando->argumentos[1], directorio->nombre);
	}
	else {
		int existeDirectorio = directorioExiste(archivo->identificadorPadre, comando->argumentos[2]);
		if(archivoExiste(archivo->identificadorPadre, comando->argumentos[2]) || existeDirectorio) {
			imprimirMensaje(archivoLog, "[ARCHIVO] El nuevo nombre para el archivo ya existe");
			return;
		}
		archivoPersistirRenombrar(archivo, comando->argumentos[2]);
		String antiguaRuta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
		String nuevaRuta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, comando->argumentos[2]);
		rename(antiguaRuta, nuevaRuta);
		stringCopiar(archivo->nombre, comando->argumentos[2]);
		imprimirMensajeDos(archivoLog, "[ARCHIVO] El archivo %s fue renombrado por %s", comando->argumentos[1], archivo->nombre);
		memoriaLiberar(antiguaRuta);
		memoriaLiberar(nuevaRuta);
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
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
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
		directorioPersistirMover(directorio, directorioNuevoPadre->identificador);
		directorio->identificadorPadre = directorioNuevoPadre->identificador;
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
		archivoPersistirMover(archivo, directorioNuevoPadre->identificador);
		archivo->identificadorPadre = directorioNuevoPadre->identificador;
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El archivo %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
}

void comandoMostrarArchivo(Comando* comando) {
	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog,"[ERROR] El archivo no existe");
		return;
	}
	int numeroBloque;
	for(numeroBloque=0; numeroBloque < listaCantidadElementos(archivo->listaBloques); numeroBloque++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, numeroBloque);
		int numeroCopia;
		int bloqueNoFueImpreso = true;
		for(numeroCopia=0; numeroCopia<listaCantidadElementos(bloque->listaCopias) && bloqueNoFueImpreso; numeroCopia++) {
			CopiaBloque* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);
			Nodo* nodo = nodoBuscar(copia->nombreNodo);
			if(nodo != NULL) {
				mensajeEnviar(nodo->socket, 101 ,&copia->bloqueNodo, sizeof(Entero));
				bloqueNoFueImpreso = false;
			}
		}
		if(bloqueNoFueImpreso) {
			imprimirMensaje(archivoLog,"[ERROR] No hay nodos disponibles para obtener el bloque");
			return;
		}
	}
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
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
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


int bloqueEnviarCopiasANodos(Bloque* bloque, String buffer, int tamanioUtilizado) {
	Lista nodosDisponibles = listaFiltrar(listaNodos, (Puntero)nodoTieneBloquesLibres);
	int copiasEnviadas;
	for(copiasEnviadas = 0; listaTieneElementos(nodosDisponibles) && copiasEnviadas < MAX_COPIAS; copiasEnviadas++) {
		listaOrdenar(nodosDisponibles, (Puntero)nodoCantidadBloquesLibres);
		Nodo* nodo = listaPrimerElemento(nodosDisponibles);
		Entero numeroBloqueNodo = nodoBuscarBloqueLibre(nodo);
		bloqueCopiar(bloque, nodo, numeroBloqueNodo);
		bloqueEnviarANodo(nodo->socket, numeroBloqueNodo, buffer, tamanioUtilizado);
		nodoVerificarBloquesLibres(nodo, nodosDisponibles);
	}
	listaDestruir(nodosDisponibles);
	return copiasEnviadas;
}


void comandoCopiarArchivoDeFS(Comando* comando) {
	if(consolaflagInvalido(comando->argumentos[1])) {
		imprimirMensaje(archivoLog, "[ERROR] Flag invalido");
		return;
	}
	if(!rutaValida(comando->argumentos[3])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	Directorio* directorio = directorioBuscar(comando->argumentos[3]);
	if(directorio == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El directorio ingresado no existe");
		return;
	}
	String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[2]);
	if(archivoExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, "[ERROR] Un archivo con ese nombre ya existe en el directorio destino");
		memoriaLiberar(nombreArchivo);
		return;
	}
	if(directorioExiste(directorio->identificador, nombreArchivo)) {
		imprimirMensaje(archivoLog, "[ERROR] Un directorio con ese nombre ya existe en el directorio destino");
		memoriaLiberar(nombreArchivo);
		return;
	}
	File file = fileAbrir(comando->argumentos[2], LECTURA);
	if(file == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo a copiar no existe");
		memoriaLiberar(nombreArchivo);
		return;
	}
	Archivo* archivo = archivoCrear(nombreArchivo, directorio->identificador, comando->argumentos[1]);
	memoriaLiberar(nombreArchivo);
	int estado = ACTIVADO;//TODO vaidar si la linea es larga
	if(stringIguales(comando->argumentos[1], FLAG_B)) {
		String buffer = stringCrear(BLOQUE);
		int numeroBloqueArchivo;
		for(numeroBloqueArchivo = 0; fread(buffer, sizeof(char),BLOQUE, file) == BLOQUE; numeroBloqueArchivo++) {
			Bloque* bloque = bloqueCrear(BLOQUE, numeroBloqueArchivo);
			listaAgregarElemento(archivo->listaBloques, bloque);
			int copiasEnviadas = bloqueEnviarCopiasANodos(bloque, buffer, sizeof(BloqueNodo));
			if(copiasEnviadas < MAX_COPIAS) {
				estado = ERROR;
				break;
			}
		}
	}
	else {
		String buffer = stringCrear(MAX_STRING);
		String datos = stringCrear(BLOQUE);
		String lineaFaltante = stringCrear(BLOQUE);
		int bytesDisponibles = BLOQUE-1;
		int numeroBloqueArchivo = 0;
		while(fgets(buffer, MAX_STRING, file) != NULL) {
			int tamanioBuffer = stringLongitud(buffer);
			if(tamanioBuffer > BLOQUE-1) {
				estado = -2;
				break;
			}
			//int tamanioLineaFaltante = stringLongitud(lineaFaltante);
			//if(tamanioLineaFaltante > 0) {
			//stringConcatenar(&datos, lineaFaltante);
			//	bytesDisponibles-= stringLongitud(lineaFaltante);
			//stringLimpiar(lineaFaltante, BLOQUE);
			//}
			if(tamanioBuffer <= bytesDisponibles) {
				stringConcatenar(&datos, buffer);
				bytesDisponibles-= tamanioBuffer;
			}
			else {
				//(stringCopiar(lineaFaltante, buffer);
				//TODO ver con varios nodos
				int bytesUtilizados = stringLongitud(datos)+1;
				bytesDisponibles = BLOQUE-1;
				Bloque* bloque = bloqueCrear(bytesUtilizados, numeroBloqueArchivo);
				listaAgregarElemento(archivo->listaBloques, bloque);
				int copiasEnviadas = bloqueEnviarCopiasANodos(bloque, datos, sizeof(Entero)+bytesUtilizados);
				if(copiasEnviadas < MAX_COPIAS) {
					estado = ERROR;
					break;
				}
				numeroBloqueArchivo++;
				memoriaLiberar(datos);
				datos = stringCrear(BLOQUE); //TODO linea faltante en if
				stringConcatenar(&datos, buffer);
				bytesDisponibles-= tamanioBuffer;
			}
		}
		if(estado == ACTIVADO && stringLongitud(datos) > 0  ) {
			int bytesUtilizados = stringLongitud(datos)+1;
			Bloque* bloque = bloqueCrear(bytesUtilizados, numeroBloqueArchivo);
			listaAgregarElemento(archivo->listaBloques, bloque);
			int copiasEnviadas = bloqueEnviarCopiasANodos(bloque, datos, sizeof(Entero)+bytesUtilizados);
			if(copiasEnviadas < MAX_COPIAS)
				estado = ERROR;
		}
		memoriaLiberar(datos);
		memoriaLiberar(buffer);
		memoriaLiberar(lineaFaltante);
	}
	fileCerrar(file);
	if(estado == -2) {
		archivoDestruir(archivo);
		imprimirMensaje(archivoLog, ROJO"[ERROR] La linea leida es demasiado larga, se aborta la operacion"BLANCO);
	}else if(estado == ERROR) {
		archivoDestruir(archivo);
		imprimirMensaje(archivoLog, ROJO"[ERROR] No hay nodos o bloques disponibles, se aborta la operacion"BLANCO);
	} else {
		archivoPersistirCrear(archivo);
		listaAgregarElemento(listaArchivos, archivo);
		imprimirMensajeUno(archivoLog, "[ARCHIVO] El archivo %s fue copiado en File System", comando->argumentos[2]);
	}
}


void comandoCopiarArchivoDeYFS(Comando* comando) {

}

void comandoCopiarBloque(Comando* comando) {

}

void comandoObtenerMD5(Comando* comando) {

	if(!rutaValida(comando->argumentos[1])) {
		imprimirMensaje(archivoLog,"[ERROR] La ruta ingresada no es valida");
		return;
	}
	//if (archivoEstaEnLista(nombreArchivo)) {
		int pidHijo;
		int longitudMensaje;
		int descriptores[2];
		pipe(descriptores);
		String md5DeArchivo = memoriaAlocar(MAX_STRING);
		String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[1]);
		pidHijo = fork();
		if(pidHijo == ERROR)
			imprimirMensaje(archivoLog, "[ERROR] Fallo el fork (Estas en problemas amigo)");
		else if(pidHijo == 0) {
			close(descriptores[0]);
			close(1);
			dup2(descriptores[1], 1);
			execlp("/usr/bin/md5sum", "md5sum", comando->argumentos[1], NULL);
			close(descriptores[1]);
		} if(pidHijo > 0) {
			close(descriptores[1]);
			wait(NULL);
			int bytesLeidos;
			while((bytesLeidos = read(descriptores[0], md5DeArchivo, MAX_STRING)) > 0)
				if(bytesLeidos > 0)
					longitudMensaje = bytesLeidos;
			close(descriptores[0]);
		}
		memcpy(md5DeArchivo+longitudMensaje, "\0", 1);
		printf("[ARCHIVO] El MD5 es %s", md5DeArchivo);
		log_info(archivoLog,"[ARCHIVO] El MD5 es %s", md5DeArchivo);
		memoriaLiberar(md5DeArchivo);
		memoriaLiberar(nombreArchivo);
	//} else
		//imprimirMensaje("[ERROR] El archivo no se encuentra en el File System YAMA");
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
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);
	if(archivo == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo no existe");
		return;
	}
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Nombre: %s", archivo->nombre);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Tipo: %s", archivo->tipo);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Ubicacion: %s", comando->argumentos[1]);
	int indice;
	int tamanio = 0;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		tamanio+= bloque->bytesUtilizados;
	}
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Tamanio: %i bytes", (int*)tamanio);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Bloques utilizados: %i", (int*)listaCantidadElementos(archivo->listaBloques));
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		imprimirMensajeDos(archivoLog, "[ARCHIVO] Bloque %i: %i bytes", (int*)indice, (int*)bloque->bytesUtilizados);
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			CopiaBloque* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			imprimirMensajeCuatro(archivoLog, "[ARCHIVO] Bloque %i copia %i en: Nodo: %s | Bloque: %i", (int*)indice,
					(int*)indiceCopia, copiaBloque->nombreNodo, (int*)copiaBloque->bloqueNodo);
		}
	}
}

void comandoFinalizar() {
	 fileSystemDesactivar();
	 socketCrearCliente(IP_LOCAL, configuracion->puertoDataNode, ID_DATANODE);
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
	int flag = 0;
	for(indice=0; indice<listaCantidadElementos(listaDirectorios); indice++) {
		Directorio* directorio = listaObtenerElemento(listaDirectorios, indice);
		if(directorio->identificadorPadre == identificadorPadre) {
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (d)", directorio->nombre);
			flag = 1;
		}
	}
	for(indice=0; indice<listaCantidadElementos(listaArchivos); indice++) {
		Archivo* archivo = listaObtenerElemento(listaArchivos, indice);
		if(archivo->identificadorPadre == identificadorPadre) {
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (a)", archivo->nombre);
			flag = 1;
		}
	}

	if(flag == 0)
		imprimirMensaje(archivoLog, "[DIRECTORIO] El directorio esta vacio");
}

void directorioPersistir(Directorio* directorio){
	File archivoDirectorio = fileAbrir(rutaDirectorios,"a+");
	String indice = stringConvertirEntero(directorio->identificador);
	String padre = stringConvertirEntero(directorio->identificadorPadre);
	String entrada = directorioConfigurarEntradaArchivo(indice, directorio->nombre, padre);
	fprintf(archivoDirectorio, "%s", entrada);
	fileCerrar(archivoDirectorio);
	memoriaLiberar(entrada);
	memoriaLiberar(indice);
	memoriaLiberar(padre);
}

void directorioPersistirEliminar(Directorio* directorio) {
	File archivoDirectorio = fileAbrir(rutaDirectorios, LECTURA);
	File archivoBuffer = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s;%i", directorio->identificador, directorio->nombre, directorio->identificadorPadre);
	while (fgets(buffer, MAX_STRING, archivoDirectorio) != NULL) {
		if (stringDistintos(buffer, "\n"))
			if (!stringContiene(buffer, linea))
				fprintf(archivoBuffer, "%s", buffer);
		stringLimpiar(buffer, MAX_STRING);
	}
	fileCerrar(archivoDirectorio);
	fileCerrar(archivoBuffer);
	memoriaLiberar(linea);
	memoriaLiberar(buffer);
	archivoBufferCopiarEn(rutaDirectorios);
}


void directorioPersistirRenombrar(Directorio* directorio, String nuevoNombre) {
	File archivoDirectorio = fileAbrir(rutaDirectorios, LECTURA);
	File archivoBuffer = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s;%i", directorio->identificador, directorio->nombre, directorio->identificadorPadre);
	while(fgets(buffer, MAX_STRING, archivoDirectorio) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if (stringContiene(buffer, linea))
				fprintf(archivoBuffer, "%i;%s;%i\n", directorio->identificador, nuevoNombre, directorio->identificadorPadre);
			else
				fprintf(archivoBuffer, "%s", buffer);
		}
		stringLimpiar(buffer, MAX_STRING);
	}
	fileCerrar(archivoDirectorio);
	fileCerrar(archivoBuffer);
	memoriaLiberar(linea);
	memoriaLiberar(buffer);
	archivoBufferCopiarEn(rutaDirectorios);
}

void directorioPersistirMover(Directorio* directorio, Entero nuevoPadre) {
	File archivoDirectorio = fileAbrir(rutaDirectorios, LECTURA);
	File archivoBuffer = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s;%i", directorio->identificador, directorio->nombre, directorio->identificadorPadre);
	while (fgets(buffer, MAX_STRING, archivoDirectorio) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if (stringContiene(buffer, linea))
				fprintf(archivoBuffer, "%i;%s;%i\n", directorio->identificador, directorio->nombre, nuevoPadre);
			else
				fprintf(archivoBuffer, "%s", buffer);
		}
		stringLimpiar(buffer, MAX_STRING);
	}
	fileCerrar(archivoDirectorio);
	fileCerrar(archivoBuffer);
	memoriaLiberar(linea);
	memoriaLiberar(buffer);
	archivoBufferCopiarEn(rutaDirectorios);
}

bool directorioIndiceRespetaLimite(int indice) {
	return indice < 100;
}

bool directorioIndiceEstaOcupado(int indice) {
	return directorioIndiceRespetaLimite(indice) &&
			bitmapBitOcupado(bitmapDirectorios, indice);
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
	stringConcatenar(&buffer,indice);
	stringConcatenar(&buffer, ";");
	stringConcatenar(&buffer, nombre);
	stringConcatenar(&buffer,";");
	stringConcatenar(&buffer,padre);
	stringConcatenar(&buffer,"\n");
	return buffer;
}

void directorioBuscarIdentificador(ControlDirectorio* control) {
	Directorio* directorio;
	int indice;
	for(indice = 0; indice < listaCantidadElementos(listaDirectorios); indice++) {
		directorio = listaObtenerElemento(listaDirectorios, indice);
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

bool directorioHaySuficientesIndices(ControlDirectorio* control) {
	return directorioIndicesACrear(control->nombresDirectorios, control->indiceNombresDirectorios) <= directoriosDisponibles;
}

void directorioControlSetearNombre(ControlDirectorio* control) {
	control->nombreDirectorio = control->nombresDirectorios[control->indiceNombresDirectorios];
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
	bitmapOcuparBit(bitmapDirectorios, identificador);
	listaAgregarElemento(listaDirectorios, directorio);
	directoriosDisponibles--;
	directorioCrearMetadata(directorio->identificador);
	directorioPersistir(directorio);
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

	return listaCumpleAlguno(listaDirectorios, (Puntero)existeNuevoNombre);
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

	Directorio* directorio = listaBuscar(listaDirectorios, (Puntero)buscarPorId);
	return directorio;
}

bool directorioTieneAlgunArchivo(int identificador) {

	bool archivoEsHijo(Archivo* archivo) {
		return archivo->identificadorPadre == identificador;
	}

	return listaCumpleAlguno(listaArchivos, (Puntero)archivoEsHijo);
}


bool directorioTieneAlgunDirectorio(int identificador) {

	bool directorioEsHijo(Directorio* directorio) {
		return directorio->identificadorPadre == identificador;
	}

	return listaCumpleAlguno(listaDirectorios, (Puntero)directorioEsHijo);
}

bool directorioTieneAlgo(int identificador) {
	return directorioTieneAlgunDirectorio(identificador) ||
			directorioTieneAlgunArchivo(identificador);
}


void directorioEliminar(int identificador) {

	bool tieneElMismoId(Directorio* directorio) {
		return directorio->identificador == identificador;
	}

	listaEliminarDestruyendoPorCondicion(listaDirectorios, (Puntero)tieneElMismoId, (Puntero)memoriaLiberar);
	bitmapLiberarBit(bitmapDirectorios, identificador);
	directoriosDisponibles++;
}

bool directorioEsHijoDe(Directorio* hijo, Directorio* padre) {
	return hijo->identificadorPadre == padre->identificador;
}

//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo) {
	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	stringCopiar(archivo->nombre, nombreArchivo);
	archivo->identificadorPadre = idPadre;
	if(stringIguales(tipo, FLAG_B))
		stringCopiar(archivo->tipo, "BINARIO");
	else
		stringCopiar(archivo->tipo, "TEXTO");
	archivo->listaBloques = listaCrear();
	return archivo;
}


void archivoDestruir(Archivo* archivo) {
	listaDestruirConElementos(archivo->listaBloques, (Puntero)bloqueDestruir);
	memoriaLiberar(archivo);
}

Archivo* archivoBuscar(String path) {
	Archivo* archivo = NULL;
	String ultimo = rutaObtenerUltimoNombre(path);
	int indice;
	ControlDirectorio* control = directorioControlCrear(path);
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

	bool buscar(Archivo* archivo) {
		return control->identificadorPadre == archivo->identificadorPadre &&
				stringIguales(control->nombreDirectorio, archivo->nombre);
	}

	if(stringIguales(control->nombreDirectorio, ultimo))
		archivo = listaBuscar(listaArchivos, (Puntero)buscar);
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
	memoriaLiberar(ultimo);
	return archivo;
}

bool archivoExiste(int idPadre, String nombre) {

	bool existeNuevoNombre(Archivo* archivo) {
		return idPadre == archivo->identificadorPadre &&
				stringIguales(nombre, archivo->nombre);
	}

	return listaCumpleAlguno(listaArchivos, (Puntero)existeNuevoNombre);
}

int archivoObtenerPosicion(Archivo* archivo) {
	int indice;
	for(indice=0; archivo != listaObtenerElemento(listaArchivos, indice); indice++);
	return indice;
}

void archivoPersistirCrear(Archivo* archivo) {
	String ruta = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
	File file = fileAbrir(ruta, "a+");
	memoriaLiberar(ruta);
	fprintf(file, "NOMBRE=%s\n", archivo->nombre);
	fprintf(file, "ID_PADRE=%i\n", archivo->identificadorPadre);
	fprintf(file, "TIPO=%s\n", archivo->tipo);
	int indice;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		fprintf(file, "BLOQUE%i_BYTES=%i\n",indice, bloque->bytesUtilizados);
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			CopiaBloque* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			fprintf(file, "BLOQUE%i_COPIA%i=[%s,%i]\n", indice, indiceCopia, copiaBloque->nombreNodo, copiaBloque->bloqueNodo);
		}
	}
	fileCerrar(file);
	archivoPersistirControlCrear(archivo);
}


void archivoPersistirRenombrar(Archivo* archivoRenombrar, String nuevoNombre) {
	String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivoRenombrar->identificadorPadre, archivoRenombrar->nombre);
	File archivo = fileAbrir(rutaArchivo, LECTURA);
	File archivoAuxiliar =  fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, archivo) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(stringContiene(buffer, "NOMBRE=")) {
				String nombre = string_from_format("NOMBRE=%s\n", nuevoNombre);
				fprintf(archivoAuxiliar, "%s", nombre);
				memoriaLiberar(nombre);
				}
			else
				fprintf(archivoAuxiliar, "%s", buffer);
			stringLimpiar(buffer, 200);
		}
	}
	fileCerrar(archivo);
	fileCerrar(archivoAuxiliar);
	memoriaLiberar(buffer);
	archivoBufferCopiarEn(rutaArchivo);
	memoriaLiberar(rutaArchivo);
	archivoPersistirControlRenombrar(archivoRenombrar, nuevoNombre);
}

void archivoPersistirMover(Archivo* archivoAMover, Entero nuevoPadre) {
	String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivoAMover->identificadorPadre, archivoAMover->nombre);
	String rutaArchivoMovido = string_from_format("%s/%i/%s", rutaDirectorioArchivos, nuevoPadre, archivoAMover->nombre);
	File archivo = fileAbrir(rutaArchivo, LECTURA);
	fileLimpiar(rutaArchivo);
	memoriaLiberar(rutaArchivo);
	File archivoAuxiliar =  fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, archivo) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(stringContiene(buffer, "ID_PADRE=")) {
				String padre = string_from_format("ID_PADRE=%i\n", nuevoPadre);
				fprintf(archivoAuxiliar, "%s", padre);
				memoriaLiberar(padre);
				}
			else
				fprintf(archivoAuxiliar, "%s", buffer);
			stringLimpiar(buffer, MAX_STRING);
		}
	}
	fileCerrar(archivo);
	fileCerrar(archivoAuxiliar);
	memoriaLiberar(buffer);
	archivoBufferCopiarEn(rutaArchivoMovido);
	memoriaLiberar(rutaArchivoMovido);
	archivoPersistirControlMover(archivoAMover, nuevoPadre);
}


void archivoPersistirControlCrear(Archivo* archivo){
	File file = fileAbrir(rutaArchivos,"a+");
	String entrada = string_from_format("%i;%s\n", archivo->identificadorPadre, archivo->nombre);
	fprintf(file, "%s", entrada);
	fileCerrar(file);
	memoriaLiberar(entrada);
}

void archivoPersistirControlEliminar(Archivo* archivo){
	File control = fileAbrir(rutaArchivos, LECTURA);
	File auxiliar = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s", archivo->identificadorPadre, archivo->nombre);
	while (fgets(buffer, MAX_STRING, control) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(!stringContiene(buffer, linea))
				fprintf(auxiliar, "%s", buffer);
			stringLimpiar(buffer, MAX_STRING);
		}
	}
	fileCerrar(auxiliar);
	fileCerrar(control);
	memoriaLiberar(buffer);
	memoriaLiberar(linea);
	archivoBufferCopiarEn(rutaArchivos);
}


void archivoPersistirControlRenombrar(Archivo* archivo, String nuevoNombre){
	File control = fileAbrir(rutaArchivos, LECTURA);
	File auxiliar = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s", archivo->identificadorPadre, archivo->nombre);
	while (fgets(buffer, MAX_STRING, control) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(stringContiene(buffer, linea))
				fprintf(auxiliar, "%i;%s\n", archivo->identificadorPadre, nuevoNombre);
			else
				fprintf(auxiliar, "%s", buffer);
			stringLimpiar(buffer, MAX_STRING);
		}
	}
	fileCerrar(auxiliar);
	fileCerrar(control);
	memoriaLiberar(buffer);
	memoriaLiberar(linea);
	archivoBufferCopiarEn(rutaArchivos);
}

void archivoPersistirControlMover(Archivo* archivo, Entero nuevoPadre){
	File control = fileAbrir(rutaArchivos, LECTURA);
	File auxiliar = fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	String linea = string_from_format("%i;%s", archivo->identificadorPadre, archivo->nombre);
	while (fgets(buffer, MAX_STRING, control) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(stringContiene(buffer, linea))
				fprintf(auxiliar, "%i;%s\n", nuevoPadre, archivo->nombre);
			else
				fprintf(auxiliar, "%s", buffer);
			stringLimpiar(buffer, MAX_STRING);
		}
	}
	fileCerrar(auxiliar);
	fileCerrar(control);
	memoriaLiberar(buffer);
	memoriaLiberar(linea);
	archivoBufferCopiarEn(rutaArchivos);
}


void archivoPersistirEliminarBloque(Archivo* archivo, int numeroBloque, int numeroCopia) {
	String rutaArchivo = string_from_format("%s/%i/%s", rutaDirectorioArchivos, archivo->identificadorPadre, archivo->nombre);
	String linea = string_from_format("BLOQUE%i_COPIA%i=", numeroBloque, numeroCopia);
	File file = fileAbrir(rutaArchivo, LECTURA);
	File archivoAuxiliar =  fileAbrir(rutaBuffer, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, file) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(!stringContiene(buffer, linea))
				fprintf(archivoAuxiliar, "%s", buffer);
			stringLimpiar(buffer, MAX_STRING);
		}
	}
	fileCerrar(file);
	fileCerrar(archivoAuxiliar);
	memoriaLiberar(buffer);
	memoriaLiberar(linea);
	archivoBufferCopiarEn(rutaArchivo);
	memoriaLiberar(rutaArchivo);
}

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(int bloquesTotales, int bloquesLibres, Socket unSocket) {
	Nodo* nodo = memoriaAlocar(sizeof(Nodo));
	nodo->bloquesTotales = bloquesTotales;
	nodo->bloquesLibres = bloquesLibres;
	nodo->socket = unSocket;
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
	return nodo;
}

void nodoFormatear(Nodo* nodo) {
	nodo->bloquesLibres = nodo->bloquesTotales;
	bitmapDestruir(nodo->bitmap);
	nodo->bitmap = bitmapCrear(nodo->bloquesTotales);
}

void nodoFormatearConectados() {
	int indice;
	for(indice = 0; indice < listaCantidadElementos(listaNodos); indice++) {
		Nodo* unNodo = listaObtenerElemento(listaNodos, indice);
		nodoFormatear(unNodo);
	}
}

void nodoDestruir(Nodo* nodo) {
	bitmapDestruir(nodo->bitmap);
	memoriaLiberar(nodo);
}

void nodoRecuperarEstadoAnterior() {
	listaNodos = listaCrear();
	listaArchivos = listaCrear();
	imprimirMensaje(archivoLog, "[ESTADO] Recuperando estado anterior");
	ArchivoConfig archivoNodo = config_create(rutaNodos);
	int indice;
	int nodosConectados = archivoConfigEnteroDe(archivoNodo, "NODOS_CONECTADOS");
	for(indice = 0; indice < nodosConectados; indice++) {
		String campo = memoriaAlocar(30);
		stringCopiar(campo, "NOMBRE_NODO");
		String numeroNodo = stringConvertirEntero(indice);
		stringConcatenar(&campo,numeroNodo);
		memoriaLiberar(numeroNodo);
		String nombreNodo = archivoConfigStringDe(archivoNodo, campo);
		memoriaLiberar(campo);
		campo = memoriaAlocar(30);
		stringCopiar(campo, nombreNodo);
		stringConcatenar(&campo, "_BLOQUES_TOTALES");
		int bloquesTotales = archivoConfigEnteroDe(archivoNodo, campo);
		memoriaLiberar(campo);
		campo = memoriaAlocar(30);
		stringCopiar(campo, nombreNodo);
		stringConcatenar(&campo, "_BLOQUES_LIBRES");
		int bloquesLibres = archivoConfigEnteroDe(archivoNodo, campo);
		Nodo* nodo = nodoCrear(bloquesTotales, bloquesLibres, 0);
		stringCopiar(nodo->nombre,nombreNodo);
		memoriaLiberar(campo);
		listaAgregarElemento(listaNodos, nodo);
	}
	archivoConfigDestruir(archivoNodo);
}

void nodoPersistirConectados() {
	File archivo = fileAbrir(rutaNodos, "a+");
	int indice;
	int contadorBloquesTotales = 0;
	int contadorBloquesLibres = 0;
	for(indice = 0; indice < listaCantidadElementos(listaNodos); indice++) {
		Nodo* unNodo = listaObtenerElemento(listaNodos, indice);
		fprintf(archivo, "NOMBRE_NODO=%s\n", unNodo->nombre);
		fprintf(archivo, "%s_BLOQUES_TOTALES=%i\n", unNodo->nombre, unNodo->bloquesTotales);
		fprintf(archivo, "%s_BLOQUES_LIBRES=%i\n",unNodo->nombre, unNodo->bloquesLibres);
		contadorBloquesTotales+=unNodo->bloquesTotales;
		contadorBloquesLibres+=unNodo->bloquesLibres;
		nodoPersistirBitmap(listaObtenerElemento(listaNodos, indice));
	}
	fprintf(archivo, "NODOS_CONECTADOS=%i\n", indice);
	fprintf(archivo, "BLOQUES_LIBRES=%i\n", contadorBloquesTotales);
	fprintf(archivo, "BLOQUES_TOTALES=%i\n", contadorBloquesLibres);
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

int nodoBuscarBloqueLibre(Nodo* nodo) {
	int indice;
		for(indice = 0; bitmapBitOcupado(nodo->bitmap, indice); indice++);
		if(indice < nodo->bloquesTotales)
			return indice;
		else
			return ERROR;
}

bool nodoCantidadBloquesLibres(Nodo* unNodo, Nodo* otroNodo) {
	return unNodo->bloquesLibres > otroNodo->bloquesLibres;
}

bool nodoTieneBloquesLibres(Nodo* nodo) {
	return nodo->bloquesLibres > 0;
}

void nodoVerificarBloquesLibres(Nodo* nodo, Lista nodosDisponibles) {
	if(nodo->bloquesLibres == 0) {
		imprimirMensajeUno(archivoLog, AMARILLO"[ADVERTENCIA] No hay bloques libres en %s"BLANCO, nodo->nombre);
		nodosDisponibles = listaFiltrar(listaNodos, (Puntero)nodoTieneBloquesLibres);
	}
}

Nodo* nodoBuscar(String nombre) {

	bool buscarPorNombre(Nodo* nodo) {
		return stringIguales(nodo->nombre, nombre);
	}

	return listaBuscar(listaNodos, (Puntero)buscarPorNombre);
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
	listaDestruirConElementos(bloque->listaCopias, memoriaLiberar);
	memoriaLiberar(bloque);
}

void bloqueEnviarANodo(Socket unSocket, Entero numeroBloque, String buffer, int tamanioUtilizado) {
	BloqueNodo* bloqueDataBin = bloqueNodoCrear(numeroBloque, buffer);
	mensajeEnviar(unSocket, ESCRIBIR, bloqueDataBin, tamanioUtilizado);
	memoriaLiberar(bloqueDataBin);
}

void bloqueCopiar(Bloque* bloque, Nodo* nodo, Entero numeroBloqueNodo) {
	//TODO Actualizar archivo del nodooooooooooo
	CopiaBloque* copia = copiaBloqueCrear(numeroBloqueNodo, nodo->nombre);
	bitmapOcuparBit(nodo->bitmap, numeroBloqueNodo);
	nodo->bloquesLibres--;
	listaAgregarElemento(bloque->listaCopias, copia);
}


//--------------------------------------- Funciones de Copia Bloque-------------------------------------

CopiaBloque* copiaBloqueCrear(int numeroBloqueDelNodo, String nombreNodo) {
	CopiaBloque* copiaBloque = memoriaAlocar(sizeof(CopiaBloque));
	copiaBloque->bloqueNodo = numeroBloqueDelNodo;
	stringCopiar(copiaBloque->nombreNodo, nombreNodo);
	return copiaBloque;
}

void copiaBloqueEliminar(CopiaBloque* copia) {

	bool buscarNodo(Nodo* nodo) {
		return stringIguales(nodo->nombre,copia->nombreNodo);
	}

	Nodo* nodo = listaBuscar(listaNodos, (Puntero)buscarNodo);
	bitmapLiberarBit(nodo->bitmap, copia->bloqueNodo);
	nodoPersistirBitmap(nodo);
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
	listaArchivos = listaCrear();
	listaDirectorios = listaCrear();
	bitmapDirectorios = bitmapCrear(13);
	directoriosDisponibles = MAX_DIR;
	directorioCrearConPersistencia(0, "root", -1);
	//testCabecita();
}

void metadataRecuperar() {

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
	for(indice=0; indice<stringLongitud(ruta); indice++) {
		if(ruta[indice] < 48 || ruta[indice] > 57)
			return false;
	}
	return true;
}

//--------------------------------------- Funciones varias ------------------------------------

void archivoBufferCopiarEn(String rutaArchivo) {
	File archivo = fileAbrir(rutaArchivo, ESCRITURA);
	File archivoAuxiliar = fileAbrir(rutaBuffer, LECTURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, archivoAuxiliar) != NULL) {
		fprintf(archivo, "%s", buffer);
		stringLimpiar(buffer, MAX_STRING);
	}
	fileCerrar(archivo);
	fileCerrar(archivoAuxiliar);
	memoriaLiberar(buffer);
}

BloqueNodo* bloqueNodoCrear(Entero numeroBloque, String buffer) {
	BloqueNodo* bloqueNodo = memoriaAlocar(sizeof(BloqueNodo));
	bloqueNodo->numeroBloque = numeroBloque;
	stringCopiar(bloqueNodo->bloque, buffer);
	return bloqueNodo;
}

void testCabecita() {
	Nodo* nodo1 = nodoCrear(10, 1, 99);
	stringCopiar(nodo1->nombre, "NODO1");
	bitmapOcuparBit(nodo1->bitmap, 4);
	bitmapOcuparBit(nodo1->bitmap, 9);
	Nodo* nodo2 = nodoCrear(5, 3, 99);
	stringCopiar(nodo1->nombre, "NODO2");
	bitmapOcuparBit(nodo2->bitmap, 0);
	nodoPersistirBitmap(nodo1);
	nodoPersistirBitmap(nodo2);
	listaAgregarElemento(listaNodos, nodo1);
	listaAgregarElemento(listaNodos, nodo2);
	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	archivo->identificadorPadre = 0;
	archivo->listaBloques = listaCrear();
	stringCopiar(archivo->nombre, "test");
	stringCopiar(archivo->tipo, "TEXTO");
	Bloque* bloque0 = memoriaAlocar(sizeof(Bloque));
	bloque0->bytesUtilizados = 1014;
	bloque0->listaCopias = listaCrear();
	Bloque* bloque1 = memoriaAlocar(sizeof(Bloque));
	bloque1->bytesUtilizados = 101;
	bloque1->listaCopias = listaCrear();
	CopiaBloque* copia0Bloque0 = memoriaAlocar(sizeof(CopiaBloque));
	copia0Bloque0->bloqueNodo = 4;
	stringCopiar(copia0Bloque0->nombreNodo, "NODIN1");
	CopiaBloque* copia1Bloque0 = memoriaAlocar(sizeof(CopiaBloque));;
	copia1Bloque0->bloqueNodo = 0;
	stringCopiar(copia1Bloque0->nombreNodo, "NODIN2");
	CopiaBloque* copia0Bloque1 = memoriaAlocar(sizeof(CopiaBloque));;
	copia0Bloque1->bloqueNodo = 9;
	stringCopiar(copia0Bloque1->nombreNodo, "NODIN1");
	listaAgregarElemento(bloque0->listaCopias, copia0Bloque0);
	listaAgregarElemento(bloque0->listaCopias, copia1Bloque0);
	listaAgregarElemento(bloque1->listaCopias, copia0Bloque1);
	listaAgregarElemento(archivo->listaBloques, bloque0);
	listaAgregarElemento(archivo->listaBloques, bloque1);
	listaAgregarElemento(listaArchivos, archivo);
	archivoPersistirCrear(archivo);
}

/*
 * diferenciar nombre de archivo y directorio
mkdir con nombres existentes entre archivos y dire
el rename si hay un archivo o dir igual nombre modifica el directorio
El mover si hay un archivo y dir con el mismo nombre dice que no se puede mover un dirctorio a si mismo

Hacer cpfrom

valgrind --log-file=/home/utnso/Escritorio/log --leak-check=full --show-leak-kinds=all ./FileSystem

cpfrom -b /home/utnso/Escritorio/test2 /home

Si es con clean una vez que formateo dejo de escuchar el socket de datanodes
Si NO es con clean una vez que recupero el estado anterior dejo de escuchar datanodes
*/
