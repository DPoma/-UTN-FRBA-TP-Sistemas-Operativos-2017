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
	pidPadre = getpid();
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
		imprimirMensaje(archivoLog, ROJO"[ERROR] Error en el fork(), estas jodido"BLANCO);
}

void masterRealizarOperacion(Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	socketCerrar(listenerMaster);
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
	imprimirMensaje(archivoLog, "[CONEXION] Un Master se desconecto");
}

void workerDesconectar(Socket unSocket) {
	socketCerrar(unSocket);
	imprimirMensaje(archivoLog, "[CONEXION] Un Worker se desconecto");
}

void workerFinalizar() {
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
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void configuracionIniciar() {
	configuracionIniciarLog();
	configuracionIniciarCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	dataBinConfigurar();
	senialAsignarFuncion(SIGINT, configuracionSenial);
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
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
}

void configuracionCalcularBloques() {
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el open()"BLANCO);
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el fstat()"BLANCO);
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	dataBinTamanio = estadoArchivo.st_size;
	dataBinBloques = (Entero)ceil((double)dataBinTamanio/(double)BLOQUE);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Cantidad de bloques: %d", (int*)dataBinBloques);
}

void configuracionSenial(int senial) {
	puts("");
	if(getpid() == pidPadre) {
		estadoWorker = DESACTIVADO;
		shutdown(listenerMaster, SHUT_RDWR);
	}
}

//--------------------------------------- Funciones de Transformacion -------------------------------------

void transformacion(Mensaje* mensaje, Socket unSocket) {
	Transformacion* transformacion = memoriaAlocar(sizeof(Transformacion));
	transformacionObtenerScript(transformacion, mensaje);
	String pathScript = transformacionCrearScript(transformacion);
	int estado = ACTIVADO;
	while(estado) {
		Mensaje* otroMensaje = mensajeRecibir(unSocket);
		switch(otroMensaje->header.operacion) {
			case DESCONEXION: masterDesconectar(unSocket); estado = DESACTIVADO; break;
			case TRANSFORMACION: transformacionProcesarBloque(transformacion, mensaje, otroMensaje, unSocket, pathScript); break;
			case EXITO: transformacionFinalizar(unSocket, &estado); break;
		}
		mensajeDestruir(otroMensaje);
	}
	fileLimpiar(pathScript);
	memoriaLiberar(pathScript);
	transformacionDestruir(transformacion);
}

int transformacionEjecutar(Transformacion* transformacion, String pathScript) {
	String pathBloque = transformacionCrearBloque(transformacion);
	String pathDestino = string_from_format("%s%s", RUTA_TEMP, transformacion->nombreResultado);
	String comando = string_from_format("chmod 0777 %s", pathScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado != ERROR) {
		comando = string_from_format("cat %s | %s | sort > %s", pathBloque, pathScript, pathDestino);
		resultado = system(comando);
		memoriaLiberar(comando);
	}
	fileLimpiar(pathBloque);
	memoriaLiberar(pathDestino);
	memoriaLiberar(pathBloque);
	return resultado;
}

void transformacionFinalizar(Socket unSocket, int* estado) {
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] Master #(id?): Transformaciones realizadas con exito"BLANCO);
	socketCerrar(unSocket);
	*estado = DESACTIVADO;
}

void transformacionFinalizarBloque(int resultado, Socket unSocket, Entero numeroBloque) {
	if(resultado != ERROR)
		transformacionExito(numeroBloque, unSocket);
	else
		transformacionFracaso(numeroBloque, unSocket);
}

void transformacionExito(Entero numeroBloque, Socket unSocket) {
	imprimirMensaje1(archivoLog,"[TRANSFORMACION] Master #(id?): Operacion finalizada con exito en bloque N°%d", (int*)numeroBloque);
	mensajeEnviar(unSocket, EXITO, &numeroBloque, sizeof(Entero));
}

void transformacionFracaso(Entero numeroBloque, Socket unSocket) {
	imprimirMensaje1(archivoLog,"[TRANSFORMACION] Master #(id?): Operacion fallida en bloque N°%d", (int*)numeroBloque);
	mensajeEnviar(unSocket, FRACASO, &numeroBloque, sizeof(Entero));
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
	memcpy(transformacion->nombreResultado, datos+sizeof(Entero)*2, 12);
}

void transformacionProcesarBloque(Transformacion* transformacion, Mensaje* mensaje, Mensaje* otroMensaje, Socket unSocket, String pathScript) {
	transformacionObtenerBloque(transformacion, otroMensaje->datos);
	pid_t pid = fork();
	if(pid == 0) {
		int resultado = transformacionEjecutar(transformacion, pathScript);
		transformacionFinalizarBloque(resultado, unSocket, transformacion->numeroBloque);
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
	String path = string_from_format("%sScriptTransf%d", RUTA_TEMP, (int)pid);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(transformacion->script, sizeof(char), transformacion->scriptSize, file);
	fileCerrar(file);
	path = string_from_format("%s./ScriptTransf%d", RUTA_TEMP, (int)pid);
	return path;
}

String transformacionCrearBloque(Transformacion* transformacion) {
	String path = string_from_format("%s%sBloque", RUTA_TEMP, transformacion->nombreResultado);
	File file = fileAbrir(path, ESCRITURA);
	Puntero puntero = getBloque(transformacion->numeroBloque);
	fwrite(puntero, sizeof(char), transformacion->bytesUtilizados, file);
	fileCerrar(file);
	return path;
}

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket) {
	ReduccionLocal* reduccion = reduccionLocalRecibirDatos(mensaje->datos);
	String temporales = reduccionLocalObtenerTemporales(reduccion);
	int resultado = reduccionLocalEjecutar(reduccion, temporales);
	reduccionLocalDestruir(reduccion);
	reduccionLocalFinalizar(resultado, unSocket);
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
	return reduccion;
}

int reduccionLocalEjecutar(ReduccionLocal* reduccion, String temporales) {
	String archivoReduccion = string_from_format("%s%s", RUTA_TEMP, reduccion->nombreResultado);
	String archivoScript = reduccionLocalCrearScript(reduccion);
	String comando = string_from_format("chmod 0777 %s", archivoScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado != ERROR) {
		comando = string_from_format("sort -m %s | cat | %s > %s", temporales, archivoScript, archivoReduccion);
		resultado = system(comando);
	}
	fileLimpiar(archivoScript);
	memoriaLiberar(comando);
	memoriaLiberar(archivoReduccion);
	memoriaLiberar(archivoScript);
	memoriaLiberar(temporales);
	return resultado;
}

void reduccionLocalFinalizar(int resultado, Socket unSocket) {
	if(resultado != ERROR)
		reduccionLocalExito(unSocket);
	else
		reduccionLocalFracaso(unSocket);
}

void reduccionLocalDestruir(ReduccionLocal* reduccion) {
	memoriaLiberar(reduccion->nombresTemporales);
	memoriaLiberar(reduccion->script);
	memoriaLiberar(reduccion);
}

void reduccionLocalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,AMARILLO"[AVISO] Master #(id?): Reduccion local realizada con exito"BLANCO);
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void reduccionLocalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog, AMARILLO"[AVISO] Master #(id?): Reduccion local fallida"BLANCO);
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

String reduccionLocalCrearScript(ReduccionLocal* reduccion) {
	String path = string_from_format("%sscriptReducL%s", RUTA_TEMP, reduccion->nombreResultado);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(reduccion->script, sizeof(char), reduccion->scriptSize-1, file);
	fileCerrar(file);
	path = string_from_format("%s./scriptReducL%s", RUTA_TEMP, reduccion->nombreResultado);
	return path;
}

String reduccionLocalObtenerTemporales(ReduccionLocal* reduccion) {
	String temporales = stringCrear((TEMPSIZE+stringLongitud(RUTA_TEMP))*reduccion->cantidadTemporales + reduccion->cantidadTemporales);
	int indice;
	for(indice=0; indice < reduccion->cantidadTemporales; indice++) {
		String buffer = reduccion->nombresTemporales+TEMPSIZE*indice;
		stringConcatenar(temporales, RUTA_TEMP);
		stringConcatenar(temporales, buffer);
		stringConcatenar(temporales, " ");
	}
	return temporales;
}

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobal(Mensaje* mensaje, Socket unSocket) {
	imprimirMensaje1(archivoLog, AMARILLO"[AVISO] Master #id: %s es el encargado de la reduccion global"BLANCO, configuracion->nombreNodo);
	ReduccionGlobal* reduccion = reduccionGlobalRecibirDatos(mensaje->datos);
	int resultado = reduccionGlobalAparearTemporales(reduccion);
	if(resultado != ERROR)
		resultado = reduccionGlobalEjecutar(reduccion);
	reduccionGlobalDestruir(reduccion);
	reduccionGlobalFinalizar(resultado, unSocket);
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
	return reduccion;
}

int reduccionGlobalAparearTemporales(ReduccionGlobal* reduccion) {
	imprimirMensaje(archivoLog,"[REDUCCION GLOBAL] Realizando apareo de archivos");
	Lista listaApareados = listaCrear();
	int resultado = reduccionGlobalAlgoritmoApareo(reduccion, listaApareados);
	listaDestruirConElementos(listaApareados, (Puntero)memoriaLiberar);
	return resultado;
}

int reduccionGlobalEjecutar(ReduccionGlobal* reduccion) {
	imprimirMensaje(archivoLog,"[REDUCCION GLOBAL] Apareo de archivos finalizado");
	String archivoScript = reduccionGlobalCrearScript(reduccion);
	String archivoSalida = string_from_format("%s%s", RUTA_TEMP, reduccion->nombreResultado);
	String comando = string_from_format("chmod 0777 %s", archivoScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado != ERROR) {
		comando = string_from_format("sort %s | cat | %s > %s", reduccion->pathApareo, archivoScript, archivoSalida);
		resultado = system(comando);
	}
	fileLimpiar(archivoScript);
	memoriaLiberar(comando);
	memoriaLiberar(archivoSalida);
	memoriaLiberar(archivoScript);
	return resultado;
}

void reduccionGlobalFinalizar(int resultado, Socket unSocket) {
	if(resultado != ERROR)
		reduccionGlobalExito(unSocket);
	else
		reduccionGlobalFracaso(unSocket);
}

void reduccionGlobalDestruir(ReduccionGlobal* reduccion) {
	memoriaLiberar(reduccion->nodos);
	memoriaLiberar(reduccion->script);
	memoriaLiberar(reduccion->pathApareo);
	memoriaLiberar(reduccion);
}

void reduccionGlobalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,AMARILLO"[AVISO] Master #(id?): Reduccion global realizada con exito"BLANCO);
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void reduccionGlobalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,AMARILLO"[AVISO] Master #(id?): Reduccion global fallida"BLANCO);
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

int reduccionGlobalRealizarConexiones(ReduccionGlobal* reduccion, Lista listaApareados) {
	int resultado = OK;
	int indice;
	for(indice=0; indice < reduccion->cantidadNodos; indice++) {
		Apareo* apareo = memoriaAlocar(sizeof(Apareo));
		apareo->socketWorker = socketCrearCliente(reduccion->nodos[indice].nodo.ip, reduccion->nodos[indice].nodo.port, ID_MASTER);
		if(apareo->socketWorker == ERROR) {
			resultado = ERROR;
			break;
		}
		resultado = mensajeEnviar(apareo->socketWorker, CONEXION_WORKER, reduccion->nodos[indice].temporal, TEMPSIZE);
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
		apareo->linea = reduccionGlobalEncargadoPedirLinea(apareo->socketWorker);
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
	if(apareo->linea == NULL)
		puts("LLEGO LINEA NULA");
	int resultado = OK;
	fwrite(apareo->linea, sizeof(char), stringLongitud(apareo->linea), archivoResultado);
	memoriaLiberar(apareo->linea);
	apareo->linea = reduccionGlobalEncargadoPedirLinea(apareo->socketWorker);
	if(stringIguales(apareo->linea, "ERROR"))
		resultado = ERROR;
	return resultado;
}

int reduccionGlobalAlgoritmoApareo(ReduccionGlobal* reduccion, Lista listaApareados) {
	int resultado = reduccionGlobalRealizarConexiones(reduccion, listaApareados);
	if(resultado != ERROR) {
		reduccion->pathApareo = string_from_format("%s%sAp", RUTA_TEMP, reduccion->nombreResultado);
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
	int longitudLineaMasCorta;
	if(stringLongitud(unApareo->linea) < stringLongitud(otroApareo->linea))
		longitudLineaMasCorta = stringLongitud(unApareo->linea);
	else
		longitudLineaMasCorta = stringLongitud(otroApareo->linea);
	int indice;
	for(indice = 0; indice < longitudLineaMasCorta; indice++)
		if(unApareo->linea[indice] != otroApareo->linea[indice])
			break;
	int acomodarCriterioSort(char c){
		if(c>='a'&&c<='z')
			return c+100;
		if((c>='A'&&c<='Z') || (c>='0'&&c<='9'))
			return c;
		return 0;
	}
	if(acomodarCriterioSort(unApareo->linea[indice]) < acomodarCriterioSort(otroApareo->linea[indice]))
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

int reduccionGlobalEsperarPedido(Socket socketWorker, Puntero buffer) {
	int resultado = OK;
	Mensaje* mensaje = mensajeRecibir(socketWorker);
	if(mensaje->header.operacion == PEDIR_LINEA)
		resultado = reduccionGlobalEnviarRespuesta(socketWorker, buffer);
	else {
		workerDesconectar(socketWorker);
		resultado = ERROR;
	}
	mensajeDestruir(mensaje);
	return resultado;
}

void reduccionGlobalEnviarLinea(Mensaje* mensaje, Socket socketWorker) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Worker conectado existosamente");
	int resultado = OK;
	String pathReduccionLocal= string_from_format("%s%s", RUTA_TEMP, (String)mensaje->datos);
	File archivoReduccionLocal = fileAbrir(pathReduccionLocal, LECTURA);
	memoriaLiberar(pathReduccionLocal);
	String buffer = stringCrear(BLOQUE);
	while(fgets(buffer, BLOQUE, archivoReduccionLocal)) {
		resultado = reduccionGlobalEsperarPedido(socketWorker, buffer);
		if(resultado == ERROR)
			break;
		memoriaLiberar(buffer);
		buffer = stringCrear(BLOQUE);
	}
	memoriaLiberar(buffer);
	if(resultado != ERROR)
		reduccionGlobalEsperarPedido(socketWorker, NULL);
	fileCerrar(archivoReduccionLocal);
}

String reduccionGlobalCopiarLinea(Mensaje* mensaje) {
	if(mensaje->datos == NULL)
		return NULL;
	String linea = stringCrear(mensaje->header.tamanio);
	memcpy(linea, mensaje->datos, mensaje->header.tamanio);
	return linea;
}

String reduccionGlobalEncargadoPedirLinea(Socket unSocket) {
	String linea = NULL;
	int resultado = mensajeEnviar(unSocket, PEDIR_LINEA, NULL, NULO);
	if(resultado == ERROR)
			return "ERROR";
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensaje->header.operacion == ENVIAR_LINEA)
		linea = reduccionGlobalCopiarLinea(mensaje);
	else
		workerDesconectar(unSocket);
	mensajeDestruir(mensaje);
	return linea;
}

void reduccionGlobalDestruirLineaNula(Lista listaApareados) {
	bool buscarLineaVacia(Apareo* apareo) {return apareo->linea == NULL;}
	listaEliminarDestruyendoPorCondicion(listaApareados, (Puntero)buscarLineaVacia, memoriaLiberar);
}

String reduccionGlobalCrearScript(ReduccionGlobal* reduccion) {
	String path = string_from_format("%sscriptReducG%s", RUTA_TEMP, reduccion->nombreResultado);
	File file = fileAbrir(path , ESCRITURA);
	memoriaLiberar(path);
	fwrite(reduccion->script, sizeof(char), reduccion->scriptSize-1, file);
	fileCerrar(file);
	path = string_from_format("%s./scriptReducG%s", RUTA_TEMP, reduccion->nombreResultado);
	return path;
}

//--------------------------------------- Funciones de Almacenado Final -------------------------------------

void almacenadoFinal(Mensaje* mensaje, Socket socketMaster) {
	String pathArchivo = string_from_format("%s%s", RUTA_TEMP, mensaje->datos);
	int resultado = almacenadoFinalEjecutar(pathArchivo, mensaje->datos+TEMPSIZE);
	memoriaLiberar(pathArchivo);
	almacenadoFinalFinalizar(resultado, socketMaster);
}

int almacenadoFinalEjecutar(String pathArchivo, String pathYama) {
	Socket socketFileSystem =  almacenadoFinalConectarAFileSystem();
	int resultado = almacenadoFinalEnviarArchivo(pathArchivo, pathYama, socketFileSystem);
	imprimirMensaje1(archivoLog,"[ALMACENADO FINAL] Guardando archivo en %s", pathYama);
	Mensaje* mensaje = mensajeRecibir(socketFileSystem);
	resultado = mensaje->header.operacion;
	mensajeDestruir(mensaje);
	return resultado;
}

int almacenadoFinalEnviarArchivo(String pathArchivo, String pathYama, Socket socketFileSystem) {
	mensajeEnviar(socketFileSystem, ALMACENAR_PATH, pathYama, stringLongitud(pathYama)+1);
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
	}
	mensajeEnviar(socketFileSystem, ALMACENADO_FINAL, NULL, NULO);
	memoriaLiberar(bloqueWorker);
	memoriaLiberar(buffer);
	fileCerrar(file);
	return estado;
}

Socket almacenadoFinalConectarAFileSystem() {
	imprimirMensaje2(archivoLog,"[ALMACENADO FINAL] Conectando a File System (IP:%s | Puerto:%s)", configuracion->ipFileSystem, configuracion->puertoFileSystemWorker);
	Socket socketFileSystem =socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystemWorker, ID_WORKER);
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] Conexion existosa con File System");
	return socketFileSystem;
}


void almacenadoFinalFinalizar(int resultado, Socket unSocket) {
	if(resultado == EXITO)
		almacenadoFinalExito(unSocket);
	else
		almacenadoFinalFracaso(unSocket);
}

void almacenadoFinalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,AMARILLO"[AVISO] Master #(id?): Almacenado final realizado con exito"BLANCO);
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void almacenadoFinalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,AMARILLO"[AVISO] Master #(id?): Almacenado final fallido"BLANCO);
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinConfigurar() {
	dataBinAbrir();
	punteroDataBin = dataBinMapear();
	configuracionCalcularBloques();
}

void dataBinAbrir() {
	dataBin = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(dataBin == NULL) {
		imprimirMensaje(archivoLog,ROJO"[ERROR] No se pudo abrir el archivo data.bin"BLANCO);
		exit(EXIT_FAILURE);
	}
	fileCerrar(dataBin);
}

Puntero dataBinMapear() {
	Puntero Puntero;
	int descriptorArchivo = open(configuracion->rutaDataBin, O_CLOEXEC | O_RDWR);
	if (descriptorArchivo == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el open()"BLANCO);
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el fstat()"BLANCO);
		perror("fstat");
		exit(EXIT_FAILURE);
	}
	dataBinTamanio = estadoArchivo.st_size;
	Puntero = mmap(0, dataBinTamanio, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, descriptorArchivo, 0);
	if (Puntero == MAP_FAILED) {
		imprimirMensaje(archivoLog, ROJO"[ERROR] Fallo el mmap(), corran por sus vidas"BLANCO);
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
	imprimirMensaje1(archivoLog, "[DATABIN] El bloque N°%i fue leido", (int*)numeroBloque);
	return bloque;
}
