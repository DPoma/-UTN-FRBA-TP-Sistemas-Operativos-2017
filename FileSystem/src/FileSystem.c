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

void iniciar() {
	listaArchivos = listaCrear();
	listaDirectorios = listaCrear();
	listaNodos = listaCrear();
	bitmapDirectorios = bitmapCrear(13);
	directoriosDisponibles = 100;
	fileLimpiar(RUTA_NODOS);
	fileLimpiar(RUTA_DIRECTORIOS);
	testCabecita();
}

void fileSystemIniciar(String flag) {
	pantallaLimpiar();
	logIniciar();
	configuracionIniciar();
	fileSystemActivar();
	if(!stringIguales(flag, FLAG_CLEAN))
		iniciar();
	else
		nodoRecuperarEstadoAnterior();
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
	imprimirMensaje(archivoLog, "[EJECUCION] Finalizando proceso File System...");
	archivoLogDestruir(archivoLog);
	bitmapDestruir(bitmapDirectorios);
	memoriaLiberar(configuracion);
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
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexion de YAMA (Puerto: %s)", configuracion->puertoYAMA);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Data Nodes (Puerto: %s)", configuracion->puertoDataNode);
}

void configuracionSetearCampos() {
	campos[0] = "PUERTO_YAMA";
	campos[1] = "PUERTO_DATANODE";
}

void configuracionIniciar() {
	configuracionSetearCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (Puntero)configuracionLeerArchivo, campos);
	configuracionImprimir(configuracion);
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
			stringIguales(comando, CPFROM) ||
			stringIguales(comando, CPTO);
}

bool consolaComandoTipoTres(String comando) {
	return stringIguales(comando, CPBLOCK);
}

int consolaComandoCantidadArgumentos(String comando) {
	if(consolaComandoTipoUno(comando))
		return 1;
	if(consolaComandoTipoDos(comando))
		return 2;
	if(consolaComandoTipoTres(comando))
		return 3;
	else
		return 0;
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

bool consolaComandoEsRemoverFlag(String buffer) {
	return stringIguales(buffer, FLAG_B) ||
			stringIguales(buffer, FLAG_D);
}

bool consolaComandoRemoverFlagB(String* subcadenas) {
	return stringIguales(subcadenas[0], RM) &&
			stringIguales(subcadenas[1], FLAG_B);
}

bool consolaComandoRemoverFlagD(String* subcadenas) {
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
	if(consolaComandoRemoverFlagB(subcadenas))
		return consolaValidarComandoFlagB(subcadenas);
	if(consolaComandoRemoverFlagD(subcadenas))
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

void consolaNormalizarRemoverFlagB(String* buffer) {
	buffer[0] = stringDuplicar(RMB);
	buffer[1] = buffer[2];
	buffer[2] = buffer[3];
	buffer[3] = buffer[4];
	buffer[4] = NULL;
}

void consolaNormalizarRemoverFlagD(String* buffer) {
	buffer[0] = stringDuplicar(RMD);
	buffer[1] = buffer[2];
	buffer[2] = buffer[3];
	buffer[3] = buffer[4];
	buffer[4] = NULL;
}

void consolaNormalizarComando(String* buffer) {
	if(stringIguales(buffer[1], FLAG_B))
		consolaNormalizarRemoverFlagB(buffer);
	else
		consolaNormalizarRemoverFlagD(buffer);
}

bool consolaValidarComando(String* buffer) {
	if(consolaComandoExiste(buffer[0]))
		return consolaComandoControlarArgumentos(buffer);
	else
		return false;
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
		case ID_RM: comandoRemover(comando); break;
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

bool socketEsDataNode(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaDataNodes);
}

bool socketEsWorker(Servidor* servidor, Socket unSocket) {
	return listaSocketsContiene(unSocket, &servidor->listaWorkers);
}

bool socketEsYAMA(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->procesoYAMA, unSocket);
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
	return nuevoSocket;
}

bool socketEsListenerDataNode(Servidor* servidor, Socket unSocket) {
	return socketSonIguales(servidor->listenerDataNode, unSocket);
}


void servidorAceptarConexion(Servidor* servidor, Socket unSocket) {

}

void servidorRecibirMensaje(Servidor* servidor, Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensajeDesconexion(mensaje))
		servidorFinalizarConexion(servidor, unSocket);
	else {
		puts("MENSAJE");
	}


	mensajeDestruir(mensaje);
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

void servidorControlarSocket(Servidor* servidor, Socket unSocket) {

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
		listaSocketsAgregar(nuevoSocket, &servidor->listaDataNodes);
		Mensaje* mensaje = mensajeRecibir(nuevoSocket);
		Nodo* nodo = nodoCrear(mensaje->datos, 190, 0, nuevoSocket);
		mensajeDestruir(mensaje);
		listaAgregarElemento(listaNodos, nodo);
		imprimirMensaje(archivoLog, "[CONEXION] Un proceso Data Node se ha conectado");
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

//--------------------------------------- Funciones de Comando -------------------------------------

void comandoFormatearFileSystem() {
	listaDestruirConElementos(listaDirectorios, memoriaLiberar);
	listaDestruirConElementos(listaArchivos, (Puntero)archivoDestruir);
	bitmapDestruir(bitmapDirectorios);
	bitmapDirectorios = bitmapCrear(13);
	listaDirectorios = listaCrear();
	listaArchivos = listaCrear();
	directoriosDisponibles = 100;
	fileLimpiar(RUTA_DIRECTORIOS);
	archivoIniciarEstructura();
	nodoIniciarEstructura();
	imprimirMensaje(archivoLog, "[ESTADO] El File System ha sido formateado");
}

void comandoRemover(Comando* comando) {
	if(stringIguales(comando->argumentos[1], FLAG_D))
		comandoRemoverDirectorio(comando);
	else if(stringIguales(comando->argumentos[1], FLAG_B))
		comandoRemoverBloque(comando);
	else
		comandoRemoverArchivo(comando);
}

void comandoRemoverBloque(Comando* comando) {
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

	if(listaCantidadElementos(bloque->listaCopias) == 1) {
		imprimirMensaje(archivoLog, "[ERROR] No se puede eliminar el bloque ya que es el ultimo");
		return;
	}

	CopiaBloque* copia = listaObtenerElemento(bloque->listaCopias, numeroCopia);

	if(copia == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El numero de copia no existe");
		return;
	}

	copiaBloqueEliminar(copia);
	listaEliminarDestruyendoElemento(bloque->listaCopias, numeroCopia, memoriaLiberar);
	imprimirMensajeTres(archivoLog, "[BLOQUE] La copia N°%i del bloque N°%i del archivo %s ha sido eliminada",(int*)numeroCopia, (int*)numeroBloque, comando->argumentos[2]);
}



void comandoRemoverDirectorio(Comando* comando) {
	String ruta = comando->argumentos[2];

	if (stringIguales(ruta , "/")) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio raiz no puede ser eliminado");
		return;
	}

	int identificador = directorioObtenerIdentificador(ruta);

	if(!directorioExisteIdentificador(identificador)) {
		imprimirMensaje(archivoLog,"[ERROR] El directorio no existe");
		return;
	}

	if(directorioTieneAlgo(identificador))
		imprimirMensajeUno(archivoLog,"El directorio %s no puede ser eliminado ya que posee archivos o directorios",ruta);
	else {
		directorioEliminar(identificador);
		directorioPersistirRemover(identificador);
		imprimirMensajeUno(archivoLog,"El directorio %s ha sido eliminado",ruta);
	}
}

void comandoRemoverArchivo(Comando* comando) {
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
	listaEliminarDestruyendoElemento(listaArchivos, posicion, (Puntero)archivoDestruir);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] El archivo %s fue eliminado", comando->argumentos[1]);
}



void comandoRenombrar(Comando* comando) {
	int identificador = directorioObtenerIdentificador(comando->argumentos[1]);
	Directorio* directorio = directorioBuscarEnLista(identificador);
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);

	if(directorio != NULL) {

		if(directorioExisteElNuevoNombre(directorio->identificadorPadre, comando->argumentos[2])) {
			imprimirMensaje(archivoLog, "[ERROR] El nuevo nombre para el directorio ya existe");
			return;
		}

		String viejoNombre= rutaObtenerUltimoNombre(comando->argumentos[1]);
		stringCopiar(directorio->nombre, comando->argumentos[2]);
		directorioPersistirRenombrar(identificador, directorio->nombre);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El directorio %s fue renombrado por %s", viejoNombre, directorio->nombre);
		memoriaLiberar(viejoNombre);
	}

	else if(archivo != NULL) {

		if(archivoExiste(archivo->identificadorPadre, comando->argumentos[2])) {
			imprimirMensaje(archivoLog, "[ARCHIVO] El nuevo nombre para el archivo ya existe");
			return;
		}

		String viejoNombre= rutaObtenerUltimoNombre(comando->argumentos[1]);
		stringCopiar(archivo->nombre, comando->argumentos[2]);
		archivoPersistirRenombrar(archivo, viejoNombre);
		imprimirMensajeDos(archivoLog, "[ARCHIVO] El archivo %s fue renombrado por %s", viejoNombre, archivo->nombre);
		memoriaLiberar(viejoNombre);
	}
	else
		imprimirMensaje(archivoLog, "[ERROR] El archivo o directorio no existe");
}

void comandoMover(Comando* comando) {
	Directorio* directorio = directorioBuscar(comando->argumentos[1]);
	Directorio* directorioNuevoPadre = directorioBuscar(comando->argumentos[2]);
	Archivo* archivo = archivoBuscar(comando->argumentos[1]);

	if(directorio != NULL && directorioNuevoPadre != NULL) {

		if(directorioExisteElNuevoNombre(directorioNuevoPadre->identificador, directorio->nombre)) {
			imprimirMensaje(archivoLog, "[ERROR] La ruta destino ya existe");
			return;
		}

		directorio->identificadorPadre = directorioNuevoPadre->identificador;
		directorioPersistirMover(directorio->identificador, directorioNuevoPadre->identificador);
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El directorio %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
	else if(archivo != NULL && directorioNuevoPadre != NULL) {

		if(archivoExiste(directorioNuevoPadre->identificador, archivo->nombre)) {
			imprimirMensaje(archivoLog, "[ERROR] La ruta destino ya existe");
			return;
		}

		archivoPersistirMover(archivo, directorioNuevoPadre->identificador);
		archivo->identificadorPadre = directorioNuevoPadre->identificador;
		String nombre = rutaObtenerUltimoNombre(comando->argumentos[1]);
		imprimirMensajeDos(archivoLog, "[DIRECTORIO] El archivo %s fue movido a %s", nombre, comando->argumentos[2]);
		memoriaLiberar(nombre);
	}
	else
		imprimirMensaje(archivoLog, "[ERROR] El archivo o directorio no existe");
}



void comandoMostrarArchivo(Comando* comando) {

}

void comandoCrearDirectorio(Comando* comando) {

	if(stringIguales(comando->argumentos[1], "/")) {
		imprimirMensaje(archivoLog, "[ERROR] El directorio raiz no puede ser creado");
		return;
	}

	ControlDirectorio* control = directorioControlCrear(comando->argumentos[1]);

	while(stringValido(control->nombreDirectorio)) {
		directorioBuscarIdentificador(control);
		directorioActualizar(control, comando->argumentos[1]);
		directorioControlSetearNombre(control);
	}

	int indice;
	for(indice=0; stringValido(control->nombresDirectorios[indice]); indice++)
		memoriaLiberar(control->nombresDirectorios[indice]);
	memoriaLiberar(control->nombresDirectorios);
	memoriaLiberar(control);
}

void comandoCopiarArchivoDeFS(Comando* comando) {
	Archivo* archivo = archivoBuscar(comando->argumentos[2]);
	int n;
	if(archivo != NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo ya existe en el File System");
		return;
	}

	File file = fileAbrir(comando->argumentos[1], "r");
	if(file == NULL) {
		imprimirMensaje(archivoLog, "[ERROR] El archivo a copiar no existe");
		return;
	}
	String buffer = malloc(BLOQUE);
	Nodo* nodo = listaObtenerElemento(listaNodos, 0);
	for(n = 0; fread(buffer, sizeof(char),BLOQUE, file) == BLOQUE; n++) {
		mensajeEnviar(nodo->socket, ESCRIBIR, buffer, BLOQUE);
	}
	imprimirMensaje(archivoLog, "[ARCHIVO] El archivo se copio en el File System con exito");
}

void comandoCopiarArchivoDeYFS(Comando* comando) {

}

void comandoCopiarBloque(Comando* comando) {

}

void comandoObtenerMD5(Comando* comando) {
	//if (archivoEstaEnLista(nombreArchivo)) {
		int pidHijo;
		int longitudMensaje;
		int descriptores[2];
		pipe(descriptores);
		String md5DeArchivo = memoriaAlocar(MAX_STRING);
		String nombreArchivo = rutaObtenerUltimoNombre(comando->argumentos[1]);
		pidHijo = fork();
		if(pidHijo == -1)
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
	int identificador = directorioObtenerIdentificador(comando->argumentos[1]);
	Directorio* directorio = directorioBuscarEnLista(identificador);
	if(directorio != NULL)
		directorioMostrarArchivos(directorio);
	else
		imprimirMensaje(archivoLog, "[ERROR] El directorio no existe");
}

void comandoInformacionArchivo(Comando* comando) {
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
		tamanio+= bloque->bytes;
	}
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Tamanio: %i bytes", (int*)tamanio);
	imprimirMensajeUno(archivoLog, "[ARCHIVO] Bloques utilizados: %i", (int*)listaCantidadElementos(archivo->listaBloques));
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		imprimirMensajeDos(archivoLog, "[ARCHIVO] Bloque %i: %i bytes", (int*)indice, (int*)bloque->bytes);
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

void directorioMostrarArchivos(Directorio* directorioPadre) {
	int indice;

	for(indice=0; indice<listaCantidadElementos(listaDirectorios); indice++) {
		Directorio* directorio = listaObtenerElemento(listaDirectorios, indice);
		if(directorio->identificadorPadre == directorioPadre->identificador)
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (d)", directorio->nombre);
	}

	for(indice=0; indice<listaCantidadElementos(listaArchivos); indice++) {
		Archivo* archivo = listaObtenerElemento(listaArchivos, indice);
		if(archivo->identificadorPadre == directorioPadre->identificador)
			imprimirMensajeUno(archivoLog, "[DIRECTORIO] %s (a)", archivo->nombre);
	}
}

void directorioPersistir(Directorio* directorio){
	File archivoDirectorio = fileAbrir(RUTA_DIRECTORIOS,"a+");
	String indice = stringConvertirEntero(directorio->identificador);
	String padre = stringConvertirEntero(directorio->identificadorPadre);
	String entrada = directorioConfigurarEntradaArchivo(indice, directorio->nombre, padre);
	archivoPersistirEntrada(archivoDirectorio, entrada);
	fileCerrar(archivoDirectorio);
	memoriaLiberar(entrada);
	memoriaLiberar(indice);
	memoriaLiberar(padre);
}

void directorioPersistirRemover(int identificador) {
	File archivoDirectorio = fileAbrir(RUTA_DIRECTORIOS,"r");
	File archivoAuxiliar = fileAbrir(RUTA_AUXILIAR, "w");
	char buffer[200];
	char copia[200];
	int identificadorArchivo;
	memset(buffer, '\0', 200);
	memset(copia, '\0', 200);
	while (fgets(buffer, sizeof(buffer), archivoDirectorio) != NULL) {
		if (strcmp(buffer, "\n") != 0) {
			strcpy(copia, buffer);
			identificadorArchivo = atoi(strtok(buffer, ";"));
			if (identificadorArchivo != identificador)
				fprintf(archivoAuxiliar, "%s", copia);
			memset(buffer, '\0', 200);
			memset(copia, '\0', 200);
		}
	}

	fileCerrar(archivoDirectorio);
	fileCerrar(archivoAuxiliar);
	archivoDirectorio = fileAbrir(RUTA_DIRECTORIOS,"w");
	archivoAuxiliar = fileAbrir(RUTA_AUXILIAR, "r");
	memset(buffer, '\0', 200);
	while (fgets(buffer, sizeof(buffer), archivoAuxiliar) != NULL) {
		fprintf(archivoDirectorio, "%s", buffer);
		memset(buffer, '\0', 200);
	}
	fileCerrar(archivoDirectorio);
	fileCerrar(archivoAuxiliar);

}


void directorioPersistirRenombrar(int idPadre, char*nuevoNombre) {
	File archivoDirectorio = fopen(RUTA_DIRECTORIOS, "r");
	FILE* archivoAuxiliar =  fopen(RUTA_AUXILIAR, "w");
	char buffer[200];
	char copia_buffer[200];
	char nueva_copia[200];
	memset(nueva_copia, '\0', 200);
	memset(buffer, '\0', 200);
	memset(copia_buffer, '\0', 200);
	char *saveptr;
	char id [10];
	char padre [10];
	while (fgets(buffer, sizeof(buffer), archivoDirectorio) != NULL) {
		if (strcmp(buffer, "\n") != 0) {
			strcpy(copia_buffer, buffer);
			strcpy(id,strtok_r(buffer, ";", &saveptr));
			strcpy(padre,strtok_r(NULL, ";", &saveptr));
			strcpy(padre,strtok_r(NULL, ";", &saveptr));
			if (atoi(id) == idPadre) {
				strcat(nueva_copia, id);
				strcat(nueva_copia, ";");
				strcat(nueva_copia, nuevoNombre);
				strcat(nueva_copia, ";");
				strcat(nueva_copia, padre);
				fprintf(archivoAuxiliar, "%s", nueva_copia);
			} else
				fprintf(archivoAuxiliar, "%s", copia_buffer);
			memset(buffer, '\0', 200);
			memset(copia_buffer, '\0', 200);
		}
	}
	fclose(archivoDirectorio);
	fclose(archivoAuxiliar);
	archivoDirectorio = fopen(RUTA_DIRECTORIOS, "w");
	archivoAuxiliar = fopen(RUTA_AUXILIAR, "r");
	memset(buffer, '\0', 200);
	while (fgets(buffer, sizeof(buffer), archivoAuxiliar) != NULL) {
		fprintf(archivoDirectorio, "%s", buffer);
		memset(buffer, '\0', 200);
	}
	fclose(archivoDirectorio);
	fclose(archivoAuxiliar);
}

void directorioPersistirMover(int idPadre, int nuevoPadre) {
	FILE* dir;
	FILE* aux;
	dir = fopen(RUTA_DIRECTORIOS, "r");
	aux = fopen(RUTA_AUXILIAR, "w");
	char buffer[200];
	char copia_buffer[200];
	char nueva_copia[200];
	memset(nueva_copia, '\0', 200);
	memset(buffer, '\0', 200);
	memset(copia_buffer, '\0', 200);
	char *saveptr;
	char id [10];
	char nombre[MAX_STRING];
	char padre [10];
	char* padreNuevo = string_itoa(nuevoPadre);
	while (fgets(buffer, sizeof(buffer), dir) != NULL) {
		if (strcmp(buffer, "\n") != 0) {
			strcpy(copia_buffer, buffer);
			strcpy(id,strtok_r(buffer, ";", &saveptr));
			strcpy(nombre,strtok_r(NULL, ";", &saveptr));
			strcpy(padre,strtok_r(NULL, ";", &saveptr));
			if (atoi(id) == idPadre) {
				strcat(nueva_copia, id);
				strcat(nueva_copia, ";");
				strcat(nueva_copia, nombre);
				strcat(nueva_copia, ";");
				strcat(nueva_copia, padreNuevo);
				strcat(nueva_copia, "\n");
				fprintf(aux, "%s", nueva_copia);
			} else
				fprintf(aux, "%s", copia_buffer);
			memset(buffer, '\0', 200);
			memset(copia_buffer, '\0', 200);
		}
	}
	fclose(dir);
	fclose(aux);
	dir = fopen(RUTA_DIRECTORIOS, "w");
	aux = fopen(RUTA_AUXILIAR, "r");
	memset(buffer, '\0', 200);
	while (fgets(buffer, sizeof(buffer), aux) != NULL) {
		fprintf(dir, "%s", buffer);
		memset(buffer, '\0', 200);
	}
	fclose(dir);
	fclose(aux);
	free(padreNuevo);
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
	for(indice = 1; directorioIndiceEstaOcupado(indice); indice++);
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
		imprimirMensaje(archivoLog, "[ERROR] El directorio ya existe");
	else
		control->identificadorPadre = control->identificadorDirectorio;
	control->indiceNombresDirectorios++;
}

void directorioCrearDirectoriosRestantes(ControlDirectorio* control, String rutaDirectorio) {
	while(stringValido(control->nombresDirectorios[control->indiceNombresDirectorios])) {
		int indice = directorioBuscarIdentificadorLibre();
		Directorio* directorio = directorioCrear(indice, control->nombresDirectorios[control->indiceNombresDirectorios], control->identificadorPadre);
		bitmapOcuparBit(bitmapDirectorios, indice);
		listaAgregarElemento(listaDirectorios, directorio);
		control->identificadorPadre = indice;
		control->indiceNombresDirectorios++;
		directoriosDisponibles--;
		directorioPersistir(directorio);
	}
	imprimirMensajeUno(archivoLog, "[DIRECTORIO] El directorio %s fue creado", rutaDirectorio);
}

void directorioCrearEntradas(ControlDirectorio* control, String rutaDirectorio) {
	if(directorioHaySuficientesIndices(control))
		directorioCrearDirectoriosRestantes(control, rutaDirectorio);
	else
		imprimirMensaje(archivoLog,"[ERROR] No se pudo crear el directorio por superar el limite permitido (100)");
}

void directorioActualizar(ControlDirectorio* control, String rutaDirectorio) {
	if (directorioExisteIdentificador(control->identificadorDirectorio))
		directorioControlarEntradas(control, rutaDirectorio);
	else
		directorioCrearEntradas(control, rutaDirectorio);
}



bool directorioExisteElNuevoNombre(int idPadre, String nuevoNombre) {

	bool existeNuevoNombre(Directorio* directorio) {
		return idPadre == directorio->identificadorPadre &&
				stringIguales(nuevoNombre, directorio->nombre);
	}

	return listaCumpleAlguno(listaDirectorios, (Puntero)existeNuevoNombre);
}

int directorioObtenerIdentificador(String path) {
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


//--------------------------------------- Funciones de Archivo -------------------------------------

Archivo* archivoCrear(String nombreArchivo, int idPadre, String tipo) {
	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	stringCopiar(archivo->nombre, nombreArchivo);
	archivo->identificadorPadre = idPadre;
	stringCopiar(archivo->tipo, tipo);
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

	//Por si un directorio de la ruta no existe, me aseguro que el
	//ultimo nombre sea el que salio del while ya que deberian ser iguales siempre
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


void archivoIniciarEstructura() {
	/*
	int indice;
	for(indice=0; indice<listaCantidadElementos(listaArchivos); indice++) {
		Archivo* archivo = listaObtenerElemento(listaArchivos, indice);
		String idPadre = stringConvertirEntero(archivo->identificadorPadre);
		String ruta = memoriaAlocar(MAX_STRING);
		stringCopiar(ruta, RUTA_ARCHIVOS);
		stringConcatenar(&ruta, idPadre);
		stringConcatenar(&ruta, "/");
		stringConcatenar(&ruta, archivo->nombre);
		fileLimpiar(ruta);
		memoriaLiberar(idPadre);
		memoriaLiberar(ruta);
	}
	for(indice=0; indice<listaCantidadElementos(listaDirectorios); indice++) {
		Directorio* directorio = listaObtenerElemento(listaDirectorios, indice);
		String idPadre = stringConvertirEntero(directorio->identificador);
		String ruta = memoriaAlocar(MAX_STRING);
		stringCopiar(ruta, RUTA_ARCHIVOS);
		stringConcatenar(&ruta, idPadre);
		rmdir(ruta);
		memoriaLiberar(idPadre);
		memoriaLiberar(ruta);
	}
	listaDestruirConElementos(listaArchivos, (Puntero)archivoDestruir);
	*/
}

void archivoPersistir(Archivo* archivo) {
	String rutaDirectorio = string_from_format("%s/%i", RUTA_ARCHIVOS, archivo->identificadorPadre);
	mkdir(rutaDirectorio, 0777);
	memoriaLiberar(rutaDirectorio);
	String ruta = string_from_format("%s/%i/%s", RUTA_ARCHIVOS, archivo->identificadorPadre, archivo->nombre);
	File file = fileAbrir(ruta, "a+");
	memoriaLiberar(ruta);
	fprintf(file, "NOMBRE=%s\n", archivo->nombre);
	fprintf(file, "ID_PADRE=%i\n", archivo->identificadorPadre);
	fprintf(file, "TIPO=%s\n", archivo->tipo);
	int indice;
	for(indice = 0; indice < listaCantidadElementos(archivo->listaBloques); indice++) {
		Bloque* bloque = listaObtenerElemento(archivo->listaBloques, indice);
		fprintf(file, "BLOQUE%i_BYTES=%i\n",indice, bloque->bytes);
		int indiceCopia;
		for(indiceCopia = 0; indiceCopia < listaCantidadElementos(bloque->listaCopias); indiceCopia++) {
			CopiaBloque* copiaBloque = listaObtenerElemento(bloque->listaCopias, indiceCopia);
			fprintf(file, "BLOQUE%i_COPIA%i=[%s,%i]\n", indice, indiceCopia, copiaBloque->nombreNodo, copiaBloque->bloqueNodo);
		}
	}
	fileCerrar(file);
}


void archivoPersistirRenombrar(Archivo* archivoARenombrar, String viejoNombre) {
	String rutaArchivo = string_from_format("%s/%i/%s", RUTA_ARCHIVOS, archivoARenombrar->identificadorPadre, viejoNombre);
	File archivo = fileAbrir(rutaArchivo, LECTURA);
	File archivoAuxiliar =  fileAbrir(RUTA_AUXILIAR, ESCRITURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, archivo) != NULL) {
		if (stringDistintos(buffer, "\n")) {
			if(stringContiene(buffer, "NOMBRE=")) {
				String nombre = string_from_format("NOMBRE=%s\n", archivoARenombrar->nombre);
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
	auxiliarCopiarEn(rutaArchivo);
}


void archivoPersistirMover(Archivo* archivoAMover, int nuevoPadre) {
	String rutaArchivo = string_from_format("%s/%i/%s", RUTA_ARCHIVOS, archivoAMover->identificadorPadre, archivoAMover->nombre);
	String rutaArchivoMovido = string_from_format("%s/%i/%s", RUTA_ARCHIVOS, nuevoPadre, archivoAMover->nombre);
	File archivo = fileAbrir(rutaArchivo, LECTURA);
	fileLimpiar(rutaArchivo);
	memoriaLiberar(rutaArchivo);
	File archivoAuxiliar =  fileAbrir(RUTA_AUXILIAR, ESCRITURA);
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
	auxiliarCopiarEn(rutaArchivoMovido);
}

//--------------------------------------- Funciones de Nodo -------------------------------------

Nodo* nodoCrear(String nombre, int bloquesTotales, int bloquesLibres, Socket unSocket) {
	Nodo* nodo = memoriaAlocar(sizeof(Nodo));
	stringCopiar(nodo->nombre, nombre);
	nodo->bloquesTotales = bloquesTotales;
	nodo->bloquesLibres = bloquesLibres;
	nodo->socket = unSocket;
	nodo->bitmap = bitmapCrear((nodo->bloquesTotales+7)/8);
	return nodo;
}

void nodoDestruir(Nodo* nodo) {
	bitmapDestruir(nodo->bitmap);
	memoriaLiberar(nodo);
}

void nodoRecuperarEstadoAnterior() {
	listaNodos = listaCrear();
	listaArchivos = listaCrear();
	imprimirMensaje(archivoLog, "[ESTADO] Recuperando estado anterior");
	ArchivoConfig archivoNodo = config_create(RUTA_NODOS);
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
		Nodo* nodo = nodoCrear(nombreNodo, bloquesTotales, bloquesLibres, 0);
		memoriaLiberar(campo);
		listaAgregarElemento(listaNodos, nodo);
	}
	archivoConfigDestruir(archivoNodo);
}


void nodoPersistir() {
	File archivo = fileAbrir(RUTA_NODOS, "a+");
	int indice;
	int contadorBloquesTotales = 0;
	int contadorBloquesLibres = 0;
	for(indice = 0; indice < listaCantidadElementos(listaNodos); indice++) {
		Nodo* unNodo = listaObtenerElemento(listaNodos, indice);
		fprintf(archivo, "NOMBRE_NODO%i=%s\n", indice, unNodo->nombre);
		fprintf(archivo, "%s_BLOQUES_TOTALES=%i\n", unNodo->nombre, unNodo->bloquesTotales);
		fprintf(archivo, "%s_BLOQUES_LIBRES=%i\n",unNodo->nombre, unNodo->bloquesLibres);
		contadorBloquesTotales+=unNodo->bloquesTotales;
		contadorBloquesLibres+=unNodo->bloquesLibres;
	}
	fprintf(archivo, "NODOS_CONECTADOS=%i\n", indice);
	fprintf(archivo, "BLOQUES_LIBRES=%i\n", contadorBloquesTotales);
	fprintf(archivo, "BLOQUES_TOTALES=%i\n", contadorBloquesLibres);
	fileCerrar(archivo);
}


void nodoPersistirBitmap(Nodo* nodo) {
	String ruta = string_from_format("%s/%s", RUTA_BITMAPS, nodo->nombre);
	File archivo = fileAbrir(ruta, ESCRITURA);
	memoriaLiberar(ruta);
	int indice;
	for(indice = 0; indice < nodo->bloquesTotales; indice++)
		fprintf(archivo, "%i", bitmapBitOcupado(nodo->bitmap, indice));
	fprintf(archivo, "\n");
	fileCerrar(archivo);
}

void nodoIniciarEstructura() {
	fileLimpiar(RUTA_NODOS);
	nodoPersistir();
	int indice;
	for(indice = 0; indice < listaCantidadElementos(listaNodos); indice++)
		nodoPersistirBitmap(listaObtenerElemento(listaNodos, indice));
}

//--------------------------------------- Funciones de Bloque-------------------------------------

Bloque* bloqueCrear(int bytes) {
	Bloque* bloque = memoriaAlocar(sizeof(Bloque));
	bloque->bytes = bytes;
	bloque->listaCopias = listaCrear();
	return bloque;
}

void bloqueDestruir(Bloque* bloque) {
	listaDestruirConElementos(bloque->listaCopias, memoriaLiberar);
	memoriaLiberar(bloque);
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

//--------------------------------------- Funciones varias------------------------------------

void auxiliarCopiarEn(String rutaArchivo) {
	File archivo = fileAbrir(rutaArchivo, ESCRITURA);
	File archivoAuxiliar = fileAbrir(RUTA_AUXILIAR, LECTURA);
	String buffer = stringCrear(MAX_STRING);
	while (fgets(buffer, MAX_STRING, archivoAuxiliar) != NULL) {
		fprintf(archivo, "%s", buffer);
		stringLimpiar(buffer, MAX_STRING);
	}
	fileCerrar(archivo);
	fileCerrar(archivoAuxiliar);
	memoriaLiberar(buffer);
	memoriaLiberar(rutaArchivo);
}

void logIniciar() {
	archivoLog = archivoLogCrear(RUTA_LOG, "FileSystem");
	imprimirMensajeProceso("# PROCESO FILE SYSTEM");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso File System inicializado");
}


String rutaObtenerUltimoNombre(String ruta) {
	int indice;
	int ultimaBarra;
	for(indice=0; ruta[indice] != FIN; indice++)
		if(caracterIguales(ruta[indice], '/'))
			ultimaBarra = indice;
	String directorio = stringTomarDesdePosicion(ruta+1, ultimaBarra);
	return directorio;
}

void testear(String mensaje, void* algo) {
	printf(mensaje, algo);
	puts("");
}

void testCabecita() {
	Nodo* nodo1 = nodoCrear("NODIN1", 100, 95, 99);
	bitmapOcuparBit(nodo1->bitmap, 99);
	bitmapOcuparBit(nodo1->bitmap, 2);

	Nodo* nodo2 = nodoCrear("NODIN2", 100, 95, 99);
	bitmapOcuparBit(nodo2->bitmap, 97);
	nodoPersistirBitmap(nodo1);
	nodoPersistirBitmap(nodo2);

	listaAgregarElemento(listaNodos, nodo1);
	listaAgregarElemento(listaNodos, nodo2);

	Archivo* archivo = memoriaAlocar(sizeof(Archivo));
	archivo->identificadorPadre = 1;
	archivo->listaBloques = listaCrear();
	stringCopiar(archivo->nombre, "test");
	stringCopiar(archivo->tipo, "TEXTO");

	Bloque* bloque0 = memoriaAlocar(sizeof(Bloque));
	bloque0->bytes = 1014;
	bloque0->listaCopias = listaCrear();

	Bloque* bloque1 = memoriaAlocar(sizeof(Bloque));
	bloque1->bytes = 101;
	bloque1->listaCopias = listaCrear();

	CopiaBloque* copia0Bloque0 = memoriaAlocar(sizeof(CopiaBloque));
	copia0Bloque0->bloqueNodo = 99;
	stringCopiar(copia0Bloque0->nombreNodo, "NODIN1");

	CopiaBloque* copia1Bloque0 = memoriaAlocar(sizeof(CopiaBloque));;
	copia1Bloque0->bloqueNodo = 97;
	stringCopiar(copia1Bloque0->nombreNodo, "NODIN2");

	CopiaBloque* copia0Bloque1 = memoriaAlocar(sizeof(CopiaBloque));;
	copia0Bloque1->bloqueNodo = 2;

	stringCopiar(copia0Bloque1->nombreNodo, "NODIN1");
	listaAgregarElemento(bloque0->listaCopias, copia0Bloque0);
	listaAgregarElemento(bloque0->listaCopias, copia1Bloque0);
	listaAgregarElemento(bloque1->listaCopias, copia0Bloque1);
	listaAgregarElemento(archivo->listaBloques, bloque0);
	listaAgregarElemento(archivo->listaBloques, bloque1);
	listaAgregarElemento(listaArchivos, archivo);
	archivoPersistir(archivo);
}

