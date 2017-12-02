/*
 ============================================================================
 Name        : Worker.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso Worker
 ============================================================================
 */

#include "Worker.h"

int main(void) {
	workerIniciar();
	workerAtenderProcesos();
	workerFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de Worker -------------------------------------

void workerIniciar() {
	configuracionIniciar();
	mkdir(configuracion->rutaTemporales, 0777);
	pidPadre = getpid();
	zombieDetector();
	estadoWorker = ACTIVADO;
}

void workerAtenderProcesos() {
	listenerMaster = socketCrearListener(configuracion->ipPropia, configuracion->puertoMaster);
	while(estadoWorker)
		workerAceptarMaster();
}

void workerAceptarMaster() {
	Socket nuevoSocket = socketAceptar(listenerMaster, ID_MASTER);
	if(nuevoSocket != ERROR)
		masterAtenderOperacion(nuevoSocket);
}

void masterAtenderOperacion(Socket unSocket) {
	pid_t pid = fork();
	if(pid == 0)
		masterRealizarOperacion(unSocket);
	else if(pid > 0)
		socketCerrar(unSocket);
	else
		imprimirError(archivoLog, "[ERROR] Error en el fork(), estas jodido");
}

void masterRealizarOperacion(Socket unSocket) {
	socketCerrar(listenerMaster);
	socketBuffer = unSocket;
	Mensaje* mensaje = mensajeRecibir(unSocket);
	switch(mensaje->header.operacion) {
		case DESCONEXION: masterDesconectar(unSocket); break;
		case TRANSFORMACION: transformacion(mensaje, unSocket); break;
		case REDUCCION_LOCAL: reduccionLocal(mensaje, unSocket); break;
		case REDUCCION_GLOBAL: reduccionGlobal(mensaje, unSocket); break;
		case ALMACENADO_FINAL: almacenadoFinal(mensaje, unSocket); break;
		case CONEXION_WORKER: reduccionGlobalEnviarLinea(mensaje, unSocket); break;
	}
	mensajeDestruir(mensaje);
	workerFinalizar();
	exit(EXIT_SUCCESS);
}

void masterDesconectar(Socket unSocket) {
	socketCerrar(unSocket);
	imprimirError(archivoLog, "[ERROR] Un Master se desconecto");
}

void workerDesconectar(Socket unSocket, String nombre) {
	socketCerrar(unSocket);
	if(stringDistintos(nombre, configuracion->nombreNodo))
		imprimirError1(archivoLog, "[ERROR] %s desconectado", nombre);
}

void workerFinalizar() {
	if(pidPadre == getpid())
		imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	memoriaLiberar(configuracion);
	archivoLogDestruir(archivoLog);
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSystem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystemDataNode, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM_DATANODE"));
	stringCopiar(configuracion->puertoFileSystemWorker, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM_WORKER"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	stringCopiar(configuracion->tamanioDataBin, archivoConfigStringDe(archivoConfig, "TAMANIO_DATABIN"));
	stringCopiar(configuracion->rutaTemporales, archivoConfigStringDe(archivoConfig, "RUTA_TEMPORALES"));
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	stringCopiar(configuracion->rutaLogWorker, archivoConfigStringDe(archivoConfig, "RUTA_LOG_WORKER"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionIniciar() {
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionIniciarLog();
	configuracionImprimir(configuracion);
	dataBinConfigurar();
	senialAsignarFuncion(SIGINT, configuracionSenial);
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(configuracion->rutaLogWorker, "Worker");
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Nombre: %s", configuracion->nombreNodo);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Esperando conexiones de Master (Puerto: %s)", configuracion->puertoMaster);
}

void configuracionIniciarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM_DATANODE";
	campos[2] = "PUERTO_FILESYSTEM_WORKER";
	campos[3] = "PUERTO_MASTER";
	campos[4] = "NOMBRE_NODO";
	campos[5] = "RUTA_DATABIN";
	campos[6] = "IP_PROPIA";
	campos[7] = "TAMANIO_DATABIN";
	campos[8] = "RUTA_LOG_DATANODE";
	campos[9] = "RUTA_LOG_WORKER";
	campos[10] = "RUTA_TEMPORALES";
}

void configuracionCalcularBloques() {
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirError(archivoLog,"[ERROR] Fallo el open()");
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirError(archivoLog, "[ERROR] Fallo el fstat()");
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	dataBinTamanio = estadoArchivo.st_size;
	dataBinBloques = (Entero)ceil((double)dataBinTamanio/(double)BLOQUE);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Cantidad de bloques: %d", (int*)dataBinBloques);
}

void configuracionSenial(int senial) {
	if(getpid() == pidPadre) {
		puts("");
		estadoWorker = DESACTIVADO;
		shutdown(listenerMaster, SHUT_RDWR);
	}
	else {
		if(salidaTemporal != NULL)
			remove(salidaTemporal);
		exit(EXIT_SUCCESS);
	}

}

//--------------------------------------- Funciones de Transformacion -------------------------------------

void transformacion(Mensaje* mensaje, Socket unSocket) {
	Transformacion* transformacion = memoriaAlocar(sizeof(Transformacion));
	transformacionObtenerScript(transformacion, mensaje);
	salidaTemporal = transformacionCrearScript(transformacion);
	int estado = ACTIVADO;
	while(estado) {
		Mensaje* otroMensaje = mensajeRecibir(unSocket);
		switch(otroMensaje->header.operacion) {
			case DESCONEXION: masterDesconectar(unSocket); estado = DESACTIVADO; break;
			case TRANSFORMACION: transformacionProcesarBloque(transformacion, mensaje, otroMensaje, unSocket, salidaTemporal); break;
			case EXITO: transformacionFinalizar(transformacion, unSocket, &estado); break;
		}
		mensajeDestruir(otroMensaje);
	}
	fileLimpiar(salidaTemporal);
	memoriaLiberar(salidaTemporal);
	salidaTemporal = NULL;
	transformacionDestruir(transformacion);
}

int transformacionEjecutar(Transformacion* transformacion, String pathScript) {
	salidaTemporal = transformacionCrearBloque(transformacion);
	String pathDestino = string_from_format("%s/%s", configuracion->rutaTemporales, transformacion->nombreResultado);
	String comando = string_from_format("chmod 0777 %s", pathScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado == NULO) {
		comando = string_from_format("cat %s | %s | sort > %s", salidaTemporal, pathScript, pathDestino);
		resultado = system(comando);
		memoriaLiberar(comando);
	}
	fileLimpiar(salidaTemporal);
	memoriaLiberar(pathDestino);
	memoriaLiberar(salidaTemporal);
	salidaTemporal = NULL;
	return resultado;
}

void transformacionFinalizar(Transformacion* transformacion, Socket unSocket, int* estado) {
	imprimirAviso1(archivoLog, "[AVISO] Master #%d: Transformaciones realizadas con exito", (int*)transformacion->idMaster);
	socketCerrar(unSocket);
	*estado = DESACTIVADO;
}

void transformacionFinalizarBloque(int resultado, Socket unSocket, Transformacion* transformacion) {
	if(resultado == NULO)
		transformacionExito(transformacion, unSocket);
	else
		transformacionFracaso(transformacion, unSocket);
}

void transformacionExito(Transformacion* transformacion, Socket unSocket) {
	int resultado = mensajeEnviar(unSocket, EXITO, &transformacion->numeroBloque, sizeof(Entero));
	if(resultado != ERROR)
		imprimirMensaje2(archivoLog,"[TRANSFORMACION] Master #%d: Operacion exitosa en bloque N°%d", (int*)transformacion->idMaster, (int*)transformacion->numeroBloque);
	else
		imprimirMensaje2(archivoLog,"[TRANSFORMACION] Master #%d: Operacion fallida en bloque N°%d",(int*)transformacion->idMaster, (int*)transformacion->numeroBloque);
}

void transformacionFracaso(Transformacion* transformacion, Socket unSocket) {
	imprimirMensaje2(archivoLog,"[TRANSFORMACION] Master #%d: Operacion fallida en bloque N°%d", (int*)transformacion->idMaster, (int*)transformacion->numeroBloque);
	mensajeEnviar(unSocket, FRACASO, &transformacion->numeroBloque, sizeof(Entero));
}

void transformacionDestruir(Transformacion* transformacion) {
	memoriaLiberar(transformacion->script);
	memoriaLiberar(transformacion);
}

void transformacionObtenerScript(Transformacion* transformacion, Mensaje* mensaje) {
	transformacion->scriptSize = mensaje->header.tamanio;
	transformacion->script = memoriaAlocar(transformacion->scriptSize);
	memcpy(transformacion->script, mensaje->datos, transformacion->scriptSize);
}

void transformacionObtenerBloque(Transformacion* transformacion, Puntero datos) {
	memcpy(&transformacion->numeroBloque, datos, sizeof(Entero));
	memcpy(&transformacion->bytesUtilizados, datos+sizeof(Entero), sizeof(Entero));
	memcpy(transformacion->nombreResultado, datos+sizeof(Entero)*2, TEMPSIZE);
	memcpy(&transformacion->idMaster, datos+sizeof(Entero)*2+TEMPSIZE, sizeof(Entero));
}

void transformacionProcesarBloque(Transformacion* transformacion, Mensaje* mensaje, Mensaje* otroMensaje, Socket unSocket, String pathScript) {
	transformacionObtenerBloque(transformacion, otroMensaje->datos);
	imprimirMensaje1(archivoLog, "[CONEXION] Master #%d conectado exitosamente", (int*)transformacion->idMaster);
	pid_t pid = fork();
	if(pid == 0) {
		int resultado = transformacionEjecutar(transformacion, pathScript);
		transformacionFinalizarBloque(resultado, unSocket, transformacion);
		transformacionDestruir(transformacion);
		memoriaLiberar(pathScript);
		mensajeDestruir(mensaje);
		mensajeDestruir(otroMensaje);
		workerFinalizar();
		exit(EXIT_SUCCESS);
	}
}

String transformacionCrearScript(Transformacion* transformacion) {
	pid_t pid = getpid();
	String path = string_from_format("%s/scriptTransf%d", configuracion->rutaTemporales, (int)pid);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(transformacion->script, sizeof(char), transformacion->scriptSize, file);
	fileCerrar(file);
	path = string_from_format("%s/./scriptTransf%d", configuracion->rutaTemporales, (int)pid);
	return path;
}

String transformacionCrearBloque(Transformacion* transformacion) {
	String path = string_from_format("%s/%sBloque", configuracion->rutaTemporales, transformacion->nombreResultado);
	File file = fileAbrir(path, ESCRITURA);
	Puntero puntero = getBloque(transformacion->numeroBloque);
	fwrite(puntero, sizeof(char), transformacion->bytesUtilizados, file);
	fileCerrar(file);
	return path;
}

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket) {
	ReduccionLocal* reduccion = reduccionLocalRecibirDatos(mensaje->datos);
	imprimirMensaje1(archivoLog, "[CONEXION] Master #%d conectado exitosamente", (int*)reduccion->idMaster);
	String temporales = reduccionLocalObtenerTemporales(reduccion);
	int resultado = reduccionLocalEjecutar(reduccion, temporales);
	reduccionLocalFinalizar(resultado, unSocket, reduccion->idMaster);
	reduccionLocalDestruir(reduccion);
}

ReduccionLocal* reduccionLocalRecibirDatos(Puntero datos) {
	ReduccionLocal* reduccion = memoriaAlocar(sizeof(ReduccionLocal));
	memcpy(&reduccion->scriptSize, datos, sizeof(Entero));
	reduccion->script = memoriaAlocar(reduccion->scriptSize);
	memcpy(reduccion->script, datos+sizeof(Entero), reduccion->scriptSize);
	memcpy(&reduccion->cantidadTemporales,datos+INTSIZE+reduccion->scriptSize,INTSIZE);
	reduccion->nombresTemporales = memoriaAlocar(reduccion->cantidadTemporales*TEMPSIZE);
	memcpy(reduccion->nombresTemporales, datos+INTSIZE*2+reduccion->scriptSize, reduccion->cantidadTemporales*TEMPSIZE);
	memcpy(reduccion->nombreResultado, datos+INTSIZE*2+reduccion->scriptSize+reduccion->cantidadTemporales*TEMPSIZE, TEMPSIZE);
	memcpy(&reduccion->idMaster, datos+INTSIZE*2+reduccion->scriptSize+reduccion->cantidadTemporales*TEMPSIZE+TEMPSIZE, sizeof(Entero));
	return reduccion;
}

int reduccionLocalEjecutar(ReduccionLocal* reduccion, String temporales) {
	String archivoReduccion = string_from_format("%s/%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	salidaTemporal = reduccionLocalCrearScript(reduccion);
	String comando = string_from_format("chmod 0777 %s", salidaTemporal);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado == NULO) {
		comando = string_from_format("sort -m %s | cat | %s > %s", temporales, salidaTemporal, archivoReduccion);
		resultado = system(comando);
	}
	fileLimpiar(salidaTemporal);
	memoriaLiberar(comando);
	memoriaLiberar(archivoReduccion);
	memoriaLiberar(salidaTemporal);
	salidaTemporal = NULL;
	memoriaLiberar(temporales);
	return resultado;
}

void reduccionLocalFinalizar(int resultado, Socket unSocket, int idMaster) {
	if(resultado == NULO)
		reduccionLocalExito(unSocket, idMaster);
	else
		reduccionLocalFracaso(unSocket, idMaster);
}

void reduccionLocalDestruir(ReduccionLocal* reduccion) {
	memoriaLiberar(reduccion->nombresTemporales);
	memoriaLiberar(reduccion->script);
	memoriaLiberar(reduccion);
}

void reduccionLocalExito(Socket unSocket, int idMaster) {
	int resultado = mensajeEnviar(unSocket, EXITO, NULL, NULO);
	if(resultado != ERROR)
		imprimirAviso1(archivoLog,"[AVISO] Master #%d: Reduccion local realizada con exito", (int*)idMaster);
	else
		imprimirError1(archivoLog,"[ERROR] Master #%d:Reduccion local fallida", (int*)idMaster);
}

void reduccionLocalFracaso(Socket unSocket, int idMaster) {
	imprimirError1(archivoLog, "[ERROR] Master #%d: Reduccion local fallida", (int*)idMaster);
	mensajeEnviar(unSocket, FRACASO, NULL, NULO);
}

String reduccionLocalCrearScript(ReduccionLocal* reduccion) {
	String path = string_from_format("%s/scriptReducL%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(reduccion->script, sizeof(char), reduccion->scriptSize-1, file);
	fileCerrar(file);
	path = string_from_format("%s/./scriptReducL%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	return path;
}

String reduccionLocalObtenerTemporales(ReduccionLocal* reduccion) {
	String temporales = stringCrear((TEMPSIZE+stringLongitud(configuracion->rutaTemporales)+1)*reduccion->cantidadTemporales + reduccion->cantidadTemporales);
	int indice;
	for(indice=0; indice < reduccion->cantidadTemporales; indice++) {
		String buffer = reduccion->nombresTemporales+TEMPSIZE*indice;
		stringConcatenar(temporales, configuracion->rutaTemporales);
		stringConcatenar(temporales, "/");
		stringConcatenar(temporales, buffer);
		stringConcatenar(temporales, " ");
	}
	return temporales;
}

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobal(Mensaje* mensaje, Socket unSocket) {
	ReduccionGlobal* reduccion = reduccionGlobalRecibirDatos(mensaje->datos);
	imprimirMensaje1(archivoLog, "[CONEXION] Master #%d conectado exitosamente", (int*)reduccion->idMaster);
	imprimirAviso2(archivoLog, "[AVISO] Master #%d: %s es el encargado de la reduccion global", (int*)reduccion->idMaster, configuracion->nombreNodo);
	int resultado = reduccionGlobalAparearTemporales(reduccion);
	if(resultado != ERROR)
		resultado = reduccionGlobalEjecutar(reduccion);
	reduccionGlobalFinalizar(resultado, unSocket, reduccion->idMaster);
	reduccionGlobalDestruir(reduccion);
}

ReduccionGlobal* reduccionGlobalRecibirDatos(Puntero datos) {
	ReduccionGlobal* reduccion = memoriaAlocar(sizeof(ReduccionGlobal));
	memcpy(&reduccion->scriptSize, (Entero*)datos, sizeof(Entero));
	reduccion->script = memoriaAlocar(reduccion->scriptSize);
	memcpy(reduccion->script, datos+sizeof(Entero), reduccion->scriptSize);
	memcpy(&reduccion->cantidadNodos, datos+sizeof(Entero)+reduccion->scriptSize, sizeof(Entero));
	reduccion->nodos = memoriaAlocar(reduccion->cantidadNodos*sizeof(Nodo));
	int indice;
	for(indice = 0; indice < reduccion->cantidadNodos; indice++)
		reduccion->nodos[indice] = *(Nodo*)(datos+sizeof(Entero)*2+reduccion->scriptSize+sizeof(Nodo)*indice);
	memcpy(reduccion->nombreResultado, datos+sizeof(Entero)*2+reduccion->scriptSize+sizeof(Nodo)*reduccion->cantidadNodos, TEMPSIZE);
	memcpy(&reduccion->idMaster, datos+sizeof(Entero)*2+reduccion->scriptSize+sizeof(Nodo)*reduccion->cantidadNodos+TEMPSIZE, sizeof(Entero));
	return reduccion;
}

int reduccionGlobalAparearTemporales(ReduccionGlobal* reduccion) {
	imprimirMensaje1(archivoLog,"[REDUCCION GLOBAL] Master #%d: Realizando apareo de archivos", (int*)reduccion->idMaster);
	Lista listaApareados = listaCrear();
	int resultado = reduccionGlobalAlgoritmoApareo(reduccion, listaApareados);
	listaDestruirConElementos(listaApareados, (Puntero)memoriaLiberar);
	return resultado;
}

int reduccionGlobalEjecutar(ReduccionGlobal* reduccion) {
	imprimirMensaje1(archivoLog,"[REDUCCION GLOBAL] Master #%d: Apareo de archivos finalizado", (int*)reduccion->idMaster);
	salidaTemporal = reduccionGlobalCrearScript(reduccion);
	String archivoSalida = string_from_format("%s/%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	String comando = string_from_format("chmod 0777 %s", salidaTemporal);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado == NULO) {
		comando = string_from_format("sort %s | cat | %s > %s", reduccion->pathApareo, salidaTemporal, archivoSalida);
		resultado = system(comando);
	}
	fileLimpiar(salidaTemporal);
	fileLimpiar(reduccion->pathApareo);
	memoriaLiberar(comando);
	memoriaLiberar(archivoSalida);
	memoriaLiberar(salidaTemporal);
	salidaTemporal = NULL;
	return resultado;
}

void reduccionGlobalFinalizar(int resultado, Socket unSocket, int idMaster) {
	if(resultado == NULO)
		reduccionGlobalExito(unSocket, idMaster);
	else
		reduccionGlobalFracaso(unSocket, idMaster);
}

void reduccionGlobalDestruir(ReduccionGlobal* reduccion) {
	memoriaLiberar(reduccion->nodos);
	memoriaLiberar(reduccion->script);
	memoriaLiberar(reduccion->pathApareo);
	memoriaLiberar(reduccion);
}

void reduccionGlobalExito(Socket unSocket, int idMaster) {
	int resultado = mensajeEnviar(unSocket, EXITO, NULL, NULO);
	if(resultado != ERROR)
		imprimirAviso1(archivoLog,"[AVISO] Master #%d: Reduccion global realizada con exito", (int*)idMaster);
	else
		imprimirError1(archivoLog,"[ERROR] Master #%d: Reduccion global fallida", (int*)idMaster);
}

void reduccionGlobalFracaso(Socket unSocket, int idMaster) {
	mensajeEnviar(unSocket, FRACASO, NULL, NULO);
	imprimirError1(archivoLog,"[ERROR] Master #%d: Reduccion global fallida", (int*)idMaster);
}

int reduccionGlobalRealizarConexiones(ReduccionGlobal* reduccion, Lista listaApareados) {
	int resultado = OK;
	int indice;
	for(indice=0; indice < reduccion->cantidadNodos; indice++) {
		Apareo* apareo = memoriaAlocar(sizeof(Apareo));
		stringCopiar(apareo->nombre, reduccion->nodos[indice].nodo.nombre);
		apareo->socketWorker = socketCrearCliente(reduccion->nodos[indice].nodo.ip, reduccion->nodos[indice].nodo.port, ID_MASTER);
		if(apareo->socketWorker == ERROR) {
			resultado = ERROR;
			break;
		}
		String buffer = stringCrear(TEMPSIZE+10);
		memcpy(buffer,reduccion->nodos[indice].temporal, TEMPSIZE);
		memcpy(buffer+TEMPSIZE, configuracion->nombreNodo, 10);
		resultado = mensajeEnviar(apareo->socketWorker, CONEXION_WORKER, buffer, TEMPSIZE+10);
		memoriaLiberar(buffer);
		if(resultado == ERROR)
			break;
		listaAgregarElemento(listaApareados, apareo);
	}
	return resultado;
}

int reduccionGlobalObtenerLineas(Lista listaApareados) {
	int indice;
	int resultado = OK;
	for(indice=0; indice < listaCantidadElementos(listaApareados); indice++) {
		Apareo* apareo = listaObtenerElemento(listaApareados, indice);
		apareo->linea = reduccionGlobalEncargadoPedirLinea(apareo);
		if(stringIguales(apareo->linea, "ERROR")) {
			resultado = ERROR;
			break;
		}
	}
	return resultado;
}

void reduccionGlobalCompararLineas(Lista listaApareados, Apareo* apareo) {
	int indice;
	for(indice=0; indice < listaCantidadElementos(listaApareados); indice++) {
		Apareo* otroApareo = listaObtenerElemento(listaApareados, indice);
		apareo = reduccionGlobalLineaMasCorta(apareo, otroApareo);
	}
}

int reduccionGlobalEscribirLinea(Apareo* apareo, Lista listaApareados, File archivoResultado) {
	int resultado = OK;
	fwrite(apareo->linea, sizeof(char), stringLongitud(apareo->linea), archivoResultado);
	memoriaLiberar(apareo->linea);
	apareo->linea = reduccionGlobalEncargadoPedirLinea(apareo);
	if(stringIguales(apareo->linea, "ERROR"))
		resultado = ERROR;
	return resultado;
}

int reduccionGlobalAlgoritmoApareo(ReduccionGlobal* reduccion, Lista listaApareados) {
	int resultado = reduccionGlobalRealizarConexiones(reduccion, listaApareados);
	if(resultado != ERROR) {
		reduccion->pathApareo = string_from_format("%s/%sAp", configuracion->rutaTemporales, reduccion->nombreResultado);
		File archivoResultado = fileAbrir(reduccion->pathApareo, ESCRITURA);
		resultado = reduccionGlobalObtenerLineas(listaApareados);
		if(resultado != ERROR) {
			Apareo* apareo = listaPrimerElemento(listaApareados);
			while(!listaEstaVacia(listaApareados)) {
				if(apareo->linea == NULL) {
					reduccionGlobalDestruirLineaNula(listaApareados);
					if(listaEstaVacia(listaApareados))
						break;
					apareo = listaPrimerElemento(listaApareados);
				}
				reduccionGlobalCompararLineas(listaApareados, apareo);
				resultado = reduccionGlobalEscribirLinea(apareo, listaApareados, archivoResultado);
				if(resultado == ERROR)
					break;
			}
		}
		fileCerrar(archivoResultado);
	}
	return resultado;
}

Apareo* reduccionGlobalLineaMasCorta(Apareo* unApareo, Apareo* otroApareo) {
	int resultado = strcmp(unApareo->linea, otroApareo->linea);
	static int contador=0;
	contador++;
	if(contador==1000){
		printf("[APAREO] Procesando...\n");
		contador=0;
	}
	if(resultado == NULO)
		return unApareo;
	if(resultado < NULO)
		return unApareo;
	else
		return otroApareo;
}

int reduccionGlobalEnviarRespuesta(Socket socketWorker, Puntero buffer) {
	int resultado;
	if(buffer != NULL)
		resultado = mensajeEnviar(socketWorker, ENVIAR_LINEA, buffer, stringLongitud(buffer)+1);
	else
		resultado = mensajeEnviar(socketWorker, ENVIAR_LINEA, NULL, NULO);
	return resultado;
}

int reduccionGlobalEsperarPedido(Socket socketWorker, Puntero buffer, String nombre) {
	int resultado = OK;
	Mensaje* mensaje = mensajeRecibir(socketWorker);
	if(mensaje->header.operacion == PEDIR_LINEA)
		resultado = reduccionGlobalEnviarRespuesta(socketWorker, buffer);
	else {
		workerDesconectar(socketWorker, nombre);
		resultado = ERROR;
	}
	mensajeDestruir(mensaje);
	return resultado;
}

void reduccionGlobalEnviarLinea(Mensaje* mensaje, Socket socketWorker) {
	if(stringDistintos(configuracion->nombreNodo, (String)mensaje->datos+TEMPSIZE))
		imprimirAviso1(archivoLog, "[AVISO] %s conectado para reduccion global", (String)mensaje->datos+TEMPSIZE);
	int resultado = OK;
	String pathReduccionLocal= string_from_format("%s/%s", configuracion->rutaTemporales, (String)mensaje->datos);
	File archivoReduccionLocal = fileAbrir(pathReduccionLocal, LECTURA);
	memoriaLiberar(pathReduccionLocal);
	String buffer = stringCrear(BLOQUE);
	while(fgets(buffer, BLOQUE, archivoReduccionLocal)) {
		resultado = reduccionGlobalEsperarPedido(socketWorker, buffer, (String)mensaje->datos+TEMPSIZE);
		if(resultado == ERROR)
			break;
		memoriaLiberar(buffer);
		buffer = stringCrear(BLOQUE);
	}
	memoriaLiberar(buffer);
	if(resultado != ERROR)
		reduccionGlobalEsperarPedido(socketWorker, NULL, (String)mensaje->datos+TEMPSIZE);
	fileCerrar(archivoReduccionLocal);
}

String reduccionGlobalCopiarLinea(Mensaje* mensaje) {
	if(mensaje->datos == NULL)
		return NULL;
	String linea = stringCrear(mensaje->header.tamanio);
	memcpy(linea, mensaje->datos, mensaje->header.tamanio);
	return linea;
}

String reduccionGlobalEncargadoPedirLinea(Apareo* apareo) {
	String linea = NULL;
	int resultado = mensajeEnviar(apareo->socketWorker, PEDIR_LINEA, NULL, NULO);
	if(resultado == ERROR)
			return "ERROR";
	Mensaje* mensaje = mensajeRecibir(apareo->socketWorker);
	if(mensaje->header.operacion == ENVIAR_LINEA)
		linea = reduccionGlobalCopiarLinea(mensaje);
	else {
			workerDesconectar(apareo->socketWorker, apareo->nombre);
			linea = "ERROR";
	}
	mensajeDestruir(mensaje);
	return linea;
}

void reduccionGlobalDestruirLineaNula(Lista listaApareados) {
	bool buscarLineaVacia(Apareo* apareo) {return apareo->linea == NULL;}
	listaEliminarDestruyendoPorCondicion(listaApareados, (Puntero)buscarLineaVacia, memoriaLiberar);
}

String reduccionGlobalCrearScript(ReduccionGlobal* reduccion) {
	String path = string_from_format("%s/scriptReducG%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(reduccion->script, sizeof(char), reduccion->scriptSize-1, file);
	fileCerrar(file);
	path = string_from_format("%s/./scriptReducG%s", configuracion->rutaTemporales, reduccion->nombreResultado);
	return path;
}

//--------------------------------------- Funciones de Almacenado Final -------------------------------------

void almacenadoFinal(Mensaje* mensaje, Socket socketMaster) {
	String pathArchivo = string_from_format("%s/%s", configuracion->rutaTemporales, mensaje->datos);
	int resultado = almacenadoFinalEjecutar(pathArchivo, mensaje->datos+TEMPSIZE);
	int idMaster;
	memcpy(&idMaster, mensaje->datos+mensaje->header.tamanio-sizeof(Entero), sizeof(Entero));
	imprimirMensaje1(archivoLog, "[CONEXION] Master #%d conectado exitosamente", (int*)idMaster);
	memoriaLiberar(pathArchivo);
	almacenadoFinalFinalizar(resultado, socketMaster, idMaster);
}

int almacenadoFinalEjecutar(String pathArchivo, String pathYama) {
	Socket socketFileSystem =  almacenadoFinalConectarAFileSystem();
	if(socketFileSystem == ERROR)
		return ERROR;
	int resultado = almacenadoFinalEnviarArchivo(pathArchivo, pathYama, socketFileSystem);
	if(resultado != ERROR)
		imprimirMensaje1(archivoLog,"[ALMACENADO FINAL] Guardando archivo en %s", pathYama);
	return resultado;
}

int almacenadoFinalEnviarArchivo(String pathArchivo, String pathYama, Socket socketFileSystem) {
	mensajeEnviar(socketFileSystem, ALMACENAR_PATH, pathYama, stringLongitud(pathYama)+1);
	Mensaje* unMensaje = mensajeRecibir(socketFileSystem);
	if(unMensaje->header.operacion <= DESCONEXION) {
		mensajeDestruir(unMensaje);
		return ERROR;
	}
	mensajeDestruir(unMensaje);
	File file = fileAbrir(pathArchivo, LECTURA);
	String buffer = stringCrear(BLOQUE+1);
	BloqueWorker* bloqueWorker = memoriaAlocar(sizeof(BloqueWorker));
	memset(bloqueWorker->datos, '\0', BLOQUE);
	int bytesDisponibles = BLOQUE;
	int indiceDatos = 0;
	int estado = OK;
	while(fgets(buffer, BLOQUE, file) != NULL) {
		int tamanioBuffer = stringLongitud(buffer);
		if(tamanioBuffer <= bytesDisponibles) {
			memcpy(bloqueWorker->datos+indiceDatos, buffer, tamanioBuffer);
			bytesDisponibles -= tamanioBuffer;
			indiceDatos += tamanioBuffer;
		}
		else {
			bloqueWorker->bytesUtilizados = BLOQUE-bytesDisponibles;
			mensajeEnviar(socketFileSystem, ALMACENAR_BLOQUE, bloqueWorker, sizeof(BloqueWorker));
			Mensaje* mensaje = mensajeRecibir(socketFileSystem);
			if(mensaje->header.operacion <= DESCONEXION) {
				estado = ERROR;
				mensajeDestruir(mensaje);
				break;
			}
			mensajeDestruir(mensaje);
			memoriaLiberar(bloqueWorker);
			bloqueWorker = memoriaAlocar(sizeof(BloqueWorker));
			memset(bloqueWorker->datos, '\0', BLOQUE);
			bytesDisponibles = BLOQUE;
			indiceDatos = 0;
			memcpy(bloqueWorker->datos+indiceDatos, buffer, tamanioBuffer);
			bytesDisponibles -= tamanioBuffer;
			indiceDatos += tamanioBuffer;
		}
	}
	if(estado != ERROR && !stringEstaVacio(buffer)) {
		bloqueWorker->bytesUtilizados = BLOQUE-bytesDisponibles;
		mensajeEnviar(socketFileSystem, ALMACENAR_BLOQUE, bloqueWorker, sizeof(BloqueWorker));
		Mensaje* mensaje = mensajeRecibir(socketFileSystem);
		if(mensaje->header.operacion <= DESCONEXION)
			estado = ERROR;
		mensajeDestruir(mensaje);
	}
	if(estado != ERROR)
		mensajeEnviar(socketFileSystem, ALMACENADO_FINAL, NULL, NULO);
	memoriaLiberar(bloqueWorker);
	memoriaLiberar(buffer);
	fileCerrar(file);
	return estado;
}

Socket almacenadoFinalConectarAFileSystem() {
	imprimirMensaje2(archivoLog,"[ALMACENADO FINAL] Conectando a File System (IP:%s | Puerto:%s)", configuracion->ipFileSystem, configuracion->puertoFileSystemWorker);
	Socket socketFileSystem =socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystemWorker, ID_WORKER);
	if(socketFileSystem != ERROR)
		imprimirMensaje(archivoLog,"[ALMACENADO FINAL] Conexion existosa con File System");
	else
		imprimirError(archivoLog, "[ERROR] No se pudo realizar la conexion con File System");
	return socketFileSystem;
}


void almacenadoFinalFinalizar(int resultado, Socket unSocket, int idMaster) {
	if(resultado != ERROR)
		almacenadoFinalExito(unSocket, idMaster);
	else
		almacenadoFinalFracaso(unSocket, idMaster);
}

void almacenadoFinalExito(Socket unSocket, int idMaster) {
	int resultado = mensajeEnviar(unSocket, EXITO, NULL, NULO);
	if(resultado != ERROR)
		imprimirAviso1(archivoLog,"[AVISO] Master #%d: Almacenado final realizado con exito", (int*)idMaster);
	else
		imprimirError1(archivoLog,"[ERROR] Master #%d: Almacenado final fallido", (int*)idMaster);

}

void almacenadoFinalFracaso(Socket unSocket, int idMaster) {
	imprimirError1(archivoLog,"[ERROR] Master #%d: Almacenado final fallido", (int*)idMaster);
	mensajeEnviar(unSocket, FRACASO, NULL, NULO);
}

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinConfigurar() {
	dataBinCrear();
	punteroDataBin = dataBinMapear();
	configuracionCalcularBloques();
}

void dataBinCrear() {
	File archivo = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(archivo == NULL) {
		String comando = string_from_format("truncate -s %s %s", configuracion->tamanioDataBin, configuracion->rutaDataBin);
		system(comando);
		memoriaLiberar(comando);
	}
	else
		fileCerrar(archivo);
}

Puntero dataBinMapear() {
	Puntero Puntero;
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirError(archivoLog, "[ERROR] Fallo el open()");
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirError(archivoLog, "[ERROR] Fallo el fstat()");
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	dataBinTamanio = estadoArchivo.st_size;
	Puntero = mmap(NULO, dataBinTamanio, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, descriptorArchivo, NULO);
	if (Puntero == MAP_FAILED) {
		imprimirError(archivoLog, "[ERROR] Fallo el mmap(), corran por sus vidas");
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	close(descriptorArchivo);
	return Puntero;
}

//--------------------------------------- Funciones de Bloque -------------------------------------

Bloque bloqueBuscar(Entero numeroBloque) {
	Bloque bloque = punteroDataBin + (BLOQUE * numeroBloque);
	return bloque;
}

Bloque getBloque(Entero numeroBloque) {
	Bloque bloque = bloqueBuscar(numeroBloque);
	imprimirMensaje2(archivoLog, "[DATABIN] Lectura en el bloque N°%i de %s", (int*)numeroBloque, configuracion->nombreNodo);
	return bloque;
}

//--------------------------------------- Funciones Para Zombies -------------------------------------

void zombieLimpiar(int sig) {
  int saved_errno = errno;
  while(waitpid((pid_t)(WAIT_ANY), NULO, WNOHANG) > NULO);
  errno = saved_errno;
}

void zombieDetector() {
	struct sigaction sa;
	sa.sa_handler = &zombieLimpiar;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sa, NULO) == ERROR) {
	  perror(NULO);
	  exit(EXIT_FAILURE);
	}
}
