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
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de Worker -------------------------------------

void workerIniciar() {
	configuracionIniciar();
	estadoWorker = ACTIVADO;
}

void workerAtenderProcesos() {
	listenerMaster = socketCrearListener(configuracion->puertoMaster);
	while(estadoWorker)
		workerAceptarMaster();
}

void workerAceptarMaster() {
	Socket nuevoSocket = socketAceptar(listenerMaster, ID_MASTER);
	if(nuevoSocket != ERROR)
		masterAtenderOperacion(nuevoSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el accept(), hoy no es tu dia papu");
}

void masterAtenderOperacion(Socket unSocket) {
	pid_t pid = fork();
	if(pid == 0)
		masterRealizarOperacion(unSocket);
	else if(pid > 0)
		socketCerrar(unSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el fork(), estas jodido");
}

void masterRealizarOperacion(Socket unSocket) {
	socketCerrar(listenerMaster);
	Mensaje* mensaje = mensajeRecibir(unSocket);
	switch(mensaje->header.operacion) {
		case DESCONEXION: //TODO imprimirMensaje(archivoLog, "[AVISO] El Master se desconecto");break;
		case TRANSFORMACION: transformacion(mensaje, unSocket); break;
		case REDUCCION_LOCAL: reduccionLocal(mensaje, unSocket); break;
		case REDUCCION_GLOBAL: reduccionGlobal(mensaje, unSocket); break;
		case ALMACENADO_FINAL: almacenadoFinal(mensaje, unSocket); break;
		case CONEXION_WORKER: reduccionGlobalConOtroWorker(mensaje, unSocket);
	}
	mensajeDestruir(mensaje);
	exit(EXIT_SUCCESS);
}

void workerFinalizar() {
	imprimirMensaje(archivoLog, "[CONEXION] Un Master se desconecto");
	exit(EXIT_FAILURE);
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
	//senialAsignarFuncion(SIGINT, configuracionSenial);
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
		imprimirMensaje(archivoLog, "[ERROR] Fallo el open()");
		perror("open");
		exit(EXIT_FAILURE);
	}
	struct stat estadoArchivo;
	if (fstat(descriptorArchivo, &estadoArchivo) == ERROR) {
		imprimirMensaje(archivoLog, "[ERROR] Fallo el fstat()");
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
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
}

//--------------------------------------- Funciones de Transformacion -------------------------------------

void transformacion(Mensaje* mensaje, Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	Transformacion* transformacion = memoriaAlocar(sizeof(Transformacion));
	transformacionRecibirScript(transformacion, mensaje);
	int estado = ACTIVADO;
	while(estado) {
		Mensaje* mensaje = mensajeRecibir(unSocket);
		switch(mensaje->header.operacion) {
		case DESCONEXION: workerFinalizar(); break;
		case TRANSFORMACION: transformacionRecibirBloque(transformacion, unSocket, mensaje->datos); break;
		case EXITO: transformacionFinalizar(unSocket, &estado); break;
		}
	}
	transformacionDestruir(transformacion);
}

int transformacionEjecutar(Transformacion* transformacion) {
	String pathBloque = transformacionCrearBloque(transformacion);
	String pathScript = transformacionCrearScript(transformacion);
	String pathDestino = string_from_format("%s%s", RUTA_TEMP, transformacion->nombreResultado);
	String comando = string_from_format("cat %s | sh %s | sort > %s", pathBloque, pathScript, pathDestino);
	int resultado = system(comando);
	fileLimpiar(pathBloque);
	fileLimpiar(pathScript);
	memoriaLiberar(pathScript);
	memoriaLiberar(pathDestino);
	memoriaLiberar(pathBloque);
	return resultado;
}

void transformacionFinalizar(Socket unSocket, int* estado) {
	imprimirMensaje(archivoLog, "[CONEXION] Fin de envio de bloques a transformar");
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
	imprimirMensaje1(archivoLog,"[TRANSFORMACION] Transformacion exitosa en bloque N°%i de Master d", (int*)numeroBloque);
	mensajeEnviar(unSocket, EXITO, &numeroBloque, sizeof(Entero));
}

void transformacionFracaso(Entero numeroBloque, Socket unSocket) {
	imprimirMensaje(archivoLog,"[TRANSFORMACION] transformacion Fracaso");
	mensajeEnviar(unSocket, FRACASO, &numeroBloque, sizeof(Entero));
}

void transformacionDestruir(Transformacion* transformacion) {
	memoriaLiberar(transformacion->script);
	memoriaLiberar(transformacion);
}

void transformacionRecibirScript(Transformacion* transformacion, Mensaje* mensaje) {
	transformacion->scriptSize = mensaje->header.tamanio;
	transformacion->script = memoriaAlocar(transformacion->scriptSize);
	memcpy(transformacion->script, mensaje->datos, transformacion->scriptSize);
}

void transformacionObtenerBloque(Transformacion* transformacion, Puntero datos) {
	memcpy(&transformacion->numeroBloque, datos, sizeof(Entero));
	memcpy(&transformacion->bytesUtilizados, datos+sizeof(Entero), sizeof(Entero));
	memcpy(transformacion->nombreResultado, datos+sizeof(Entero)*2, 12);
}

void transformacionRecibirBloque(Transformacion* transformacion, Socket unSocket, Puntero datos) {
	transformacionObtenerBloque(transformacion, datos);
	pid_t pid = fork();
	if(pid == 0) {
		int resultado = transformacionEjecutar(transformacion);
		transformacionFinalizarBloque(resultado, unSocket, transformacion->numeroBloque);
		exit(EXIT_SUCCESS);
	}
}

String transformacionCrearScript(Transformacion* transformacion) {
	String path = string_from_format("%sscriptTemporal%i", RUTA_TEMP, transformacion->numeroBloque);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(transformacion->script, sizeof(char), transformacion->scriptSize, file);
	fileCerrar(file);
	return path;
}

String transformacionCrearBloque(Transformacion* transformacion) {
	String path = string_from_format("%sbloqueTemporal%i", RUTA_TEMP, transformacion->numeroBloque);
	File file = fileAbrir(path, ESCRITURA);
	Puntero puntero = getBloque(transformacion->numeroBloque);
	fwrite(puntero, sizeof(char), transformacion->bytesUtilizados, file);
	fileCerrar(file);
	return path;
}

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	ReduccionLocal reduccion = reduccionLocalRecibirDatos(mensaje->datos);
	String temporales = reduccionLocalObtenerTemporales(reduccion);
	int resultado = reduccionLocalEjecutar(reduccion, temporales);
	reduccionLocalFinalizar(resultado, unSocket);
}

ReduccionLocal reduccionLocalRecibirDatos(Puntero datos) {
	ReduccionLocal reduccion;
	memcpy(&reduccion.scriptSize, datos, sizeof(Entero));
	reduccion.script = memoriaAlocar(reduccion.scriptSize);
	memcpy(reduccion.script, datos+sizeof(Entero), reduccion.scriptSize);
	memcpy(&reduccion.cantidadTemporales,datos+INTSIZE+reduccion.scriptSize,INTSIZE);//origen
	reduccion.nombresTemporales = memoriaAlocar(reduccion.cantidadTemporales*TEMPSIZE);
	memcpy(reduccion.nombresTemporales, datos+INTSIZE*2+reduccion.scriptSize, reduccion.cantidadTemporales*TEMPSIZE);
	memcpy(reduccion.nombreResultado, datos+INTSIZE*2+reduccion.scriptSize+reduccion.cantidadTemporales*TEMPSIZE, TEMPSIZE);
	return reduccion;
}

int reduccionLocalEjecutar(ReduccionLocal reduccion, String temporales) {
	String archivoApareado = string_from_format("%s%sApareado", RUTA_TEMP, reduccion.nombreResultado);
	String archivoReduccion = string_from_format("%s%s", RUTA_TEMP, reduccion.nombreResultado);
	String archivoScript = reduccionLocalCrearScript(reduccion);
	String comando = string_from_format("chmod 0755 %s", archivoScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado != ERROR) {
		comando = string_from_format("sort -m %s | cat | %s > %s", temporales, archivoScript, archivoReduccion);
		resultado = system(comando);
	}
	fileLimpiar(archivoScript);
	fileLimpiar(archivoApareado);
	memoriaLiberar(comando);
	memoriaLiberar(archivoApareado);
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

void reduccionLocalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,"[REDUCCION LOCAL] La operacion termino con exito");
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void reduccionLocalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,"[REDUCCION LOCAL] La operacion fracaso");
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

String reduccionLocalCrearScript(ReduccionLocal reduccion) {
	String path = string_from_format("%s./scriptTemporal", RUTA_TEMP);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(reduccion.script, sizeof(char), reduccion.scriptSize-1, file);
	fileCerrar(file);
	return path;
}

String reduccionLocalObtenerTemporales(ReduccionLocal reduccion) {
	String temporales = stringCrear((TEMPSIZE+stringLongitud(RUTA_TEMP))*reduccion.cantidadTemporales + reduccion.cantidadTemporales-1);
	int indice;
	for(indice=0; indice < reduccion.cantidadTemporales; indice++) {
		String buffer = reduccion.nombresTemporales+TEMPSIZE*indice;
		stringConcatenar(temporales, RUTA_TEMP);
		stringConcatenar(temporales, buffer);
		stringConcatenar(temporales, " ");
	}
	return temporales;
}

//--------------------------------------- Funciones de Reduccion Global -------------------------------------

void reduccionGlobalControlLineas(Lista listaApareados) {

	bool buscarLineaVacia(Apareo* apareo) {
		return apareo->linea == NULL;
	}

	listaEliminarDestruyendoPorCondicion(listaApareados, (Puntero)buscarLineaVacia, memoriaLiberar);
}

Apareo* reduccionGlobalLineaMinima(Apareo* unApareo, Apareo* otroApareo) {
	int indice;
	int longitudLineaMasCorta = stringLongitud(unApareo->linea) < stringLongitud(otroApareo->linea)? stringLongitud(unApareo->linea): stringLongitud(otroApareo->linea);
	for(indice = 0; indice < longitudLineaMasCorta; indice++) {
		if(unApareo->linea[indice] != otroApareo->linea[indice])
			break;
	}
	if(unApareo->linea[indice] < otroApareo->linea[indice])
		return unApareo;
	return otroApareo;
}

void reduccionGlobalAlgoritmoApareo(Lista listaApareados, String pathResultado) {
	File archivoResultado = fileAbrir(pathResultado, ESCRITURA);
	if(archivoResultado == NULL)
		puts("ARCHIVO NULO");
	int indice;
	for(indice=0; indice < listaCantidadElementos(listaApareados); indice++) {
		Apareo* apareo = listaObtenerElemento(listaApareados, indice);
		apareo->linea = reduccionGlobalObtenerLinea(apareo->socketWorker);
	}
	Apareo* apareo = listaPrimerElemento(listaApareados);
	while(!listaEstaVacia(listaApareados)) {
		for(indice=0; indice < listaCantidadElementos(listaApareados); indice++) {
			apareo = reduccionGlobalLineaMinima(apareo, listaObtenerElemento(listaApareados, indice));
		}
		fwrite(apareo->linea, sizeof(char), stringLongitud(apareo->linea), archivoResultado);
		memoriaLiberar(apareo->linea);
		apareo->linea = reduccionGlobalObtenerLinea(apareo->socketWorker);
		reduccionGlobalControlLineas(listaApareados);
	}
	fileCerrar(archivoResultado);
	listaDestruir(listaApareados);
}

String reduccionGlobalGenerarArchivo(ReduccionGlobal reduccion) {
	Lista listaApareados = listaCrear();
	ReduccionGlobalNodo pedido;
	int indice;
	for(indice=0; indice < reduccion.cantidadNodos; indice++) {
		pedido = reduccion.nodos[indice];
		Apareo* apareo = memoriaAlocar(sizeof(Apareo));
		apareo->socketWorker = socketCrearCliente(pedido.nodo.ip, pedido.nodo.port, ID_MASTER);
		mensajeEnviar(apareo->socketWorker, CONEXION_WORKER, pedido.temporal, TEMPSIZE);
		listaAgregarElemento(listaApareados, apareo);
	}
	String pathResultado = string_from_format("%s%sApareo", RUTA_TEMP, reduccion.nombreResultado);
	reduccionGlobalAlgoritmoApareo(listaApareados, pathResultado);
	return pathResultado;
}

void reduccionGlobalConOtroWorker(Mensaje* mensaje, Socket socketWorker) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Worker conectado existosamente");
	String pathReduccionLocal = string_from_format("%s%s", RUTA_TEMP, (String)mensaje->datos);
	mensajeDestruir(mensaje);
	File archivoReduccionLocal = fileAbrir(pathReduccionLocal, LECTURA);
	String buffer = stringCrear(BLOQUE);
	while(fgets(buffer, BLOQUE, archivoReduccionLocal)) {
		mensaje = mensajeRecibir(socketWorker);
		if(mensaje->header.operacion == PEDIR_LINEA) {
			//printf("%s", buffer);
			mensajeEnviar(socketWorker, 14, buffer, stringLongitud(buffer)+1);
		}
	}
	mensaje = mensajeRecibir(socketWorker);
	if(mensaje->header.operacion == PEDIR_LINEA) {
		mensajeEnviar(socketWorker, EXITO, NULL, NULO);
	}
	fileCerrar(archivoReduccionLocal);
}

String reduccionGlobalObtenerLinea(Socket unSocket) {
	mensajeEnviar(unSocket, PEDIR_LINEA, NULL, 0);
	Mensaje* mensaje = mensajeRecibir(unSocket);
	if(mensaje->header.operacion == EXITO) {
		mensajeDestruir(mensaje);
		socketCerrar(unSocket);
		return NULL;
	}
	if(mensaje->header.operacion == DESCONEXION)
		//todo desconexion
		exit(EXIT_FAILURE);
	String linea = stringCrear(mensaje->header.tamanio);
	memcpy(linea, mensaje->datos, mensaje->header.tamanio);
	printf("Recibi %s", linea);
	mensajeDestruir(mensaje);
	return linea;
}

ReduccionGlobal reduccionGlobalRecibirDatos(Puntero datos) {
	ReduccionGlobal reduccion;
	memcpy(&reduccion.scriptSize, (Entero*)datos, sizeof(Entero));
	reduccion.script = memoriaAlocar(reduccion.scriptSize);
	memcpy(reduccion.script, datos+sizeof(Entero), reduccion.scriptSize);
	memcpy(&reduccion.cantidadNodos, datos+sizeof(Entero)+reduccion.scriptSize, sizeof(Entero));
	reduccion.nodos = memoriaAlocar(reduccion.cantidadNodos*sizeof(ReduccionGlobalNodo));
	int indice;
	for(indice = 0; indice < reduccion.cantidadNodos; indice++)
		reduccion.nodos[indice] = *(ReduccionGlobalNodo*)(datos+sizeof(Entero)*2+reduccion.scriptSize+sizeof(ReduccionGlobalNodo)*indice);
	memcpy(reduccion.nombreResultado, datos+sizeof(Entero)*2+reduccion.scriptSize+sizeof(ReduccionGlobalNodo)*reduccion.cantidadNodos, TEMPSIZE);
	return reduccion;
}

void reduccionGlobal(Mensaje* mensaje, Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	ReduccionGlobal reduccion = reduccionGlobalRecibirDatos(mensaje->datos);
	String pathApareado = reduccionGlobalGenerarArchivo(reduccion);
	int resultado = reduccionGlobalEjecutar(reduccion, pathApareado);
	reduccionGlobalFinalizar(resultado, unSocket);
}

String reduccionGlobalCrearScript(ReduccionGlobal reduccion) {
	String path = string_from_format("%s./scriptTemporal", RUTA_TEMP);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(reduccion.script, sizeof(char), reduccion.scriptSize-1, file);
	fileCerrar(file);
	return path;
}

int reduccionGlobalEjecutar(ReduccionGlobal reduccion, String pathApareado) {
	String archivoScript = reduccionGlobalCrearScript(reduccion);
	String archivoSalida = string_from_format("%s%s", RUTA_TEMP, reduccion.nombreResultado);
	String comando = string_from_format("chmod 0755 %s", archivoScript);
	int resultado = system(comando);
	memoriaLiberar(comando);
	if(resultado != ERROR) {
		comando = string_from_format("cat %s | %s > %s", pathApareado, archivoScript, archivoSalida);
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

void reduccionGlobalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,"[REDUCCION GLOBAL] La operacion termino con exito");
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void reduccionGlobalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,"[REDUCCION GLOBAL] La operacion fracaso");
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

//--------------------------------------- Funciones de Almacenado Final -------------------------------------

void almacenadoFinal(Mensaje* mensaje, Socket socketMaster) {
	String pathArchivo = string_from_format("%s%s", RUTA_TEMP, mensaje->datos);
	Entero tamanioPathArchivo = stringLongitud(pathArchivo)+1;
	Entero tamanioPathYama = stringLongitud(mensaje->datos+TEMPSIZE)+1;
	String buffer = stringCrear(tamanioPathArchivo+tamanioPathYama+2*sizeof(Entero));
	memcpy(buffer, &tamanioPathArchivo, sizeof(Entero));
	memcpy(buffer+sizeof(Entero), pathArchivo, tamanioPathArchivo);
	memcpy(buffer+sizeof(Entero)+tamanioPathArchivo, &tamanioPathYama, sizeof(Entero));
	memcpy(buffer+sizeof(Entero)*2+tamanioPathArchivo, mensaje->datos+TEMPSIZE, tamanioPathYama);
	int tamanioBuffer = sizeof(Entero)*2+tamanioPathArchivo+tamanioPathYama;
	int resultado = almacenadoFinalEnviar(buffer, tamanioBuffer, mensaje->datos+TEMPSIZE);
	almacenadoFinalFinalizar(resultado, socketMaster);
	memoriaLiberar(buffer);
	memoriaLiberar(pathArchivo);
}

int almacenadoFinalEnviar(Puntero buffer, int tamanio, String pathYama) {
	imprimirMensaje2(archivoLog,"[ALMACENADO FINAL] Estableciendo conexion con el File System (IP:%s | Puerto:%s)", configuracion->ipFileSystem, configuracion->puertoFileSystemWorker);
	Socket socketFileSystem =socketCrearCliente(configuracion->ipFileSystem, configuracion->puertoFileSystemWorker, ID_WORKER);
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] Conexion existosa con el File System");
	mensajeEnviar(socketFileSystem, ALMACENADO_FINAL, buffer, tamanio);
	imprimirMensaje1(archivoLog,"[ALMACENADO FINAL] Guardando %s", pathYama);
	Mensaje* mensajeOperacion = mensajeRecibir(socketFileSystem);
	int resultado = mensajeOperacion->header.operacion;
	mensajeDestruir(mensajeOperacion);
	return resultado;
}

void almacenadoFinalFinalizar(int resultado, Socket unSocket) {
	if(resultado == EXITO)
		almacenadoFinalExito(unSocket);
	else
		almacenadoFinalFracaso(unSocket);
}

void almacenadoFinalExito(Socket unSocket) {
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] La operacion se realizo con exito");
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void almacenadoFinalFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] La operacion fracaso");
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

BloqueWorker bloqueBuscar(Entero numeroBloque) {
	BloqueWorker bloque = punteroDataBin + (BLOQUE * numeroBloque);
	return bloque;
}

BloqueWorker getBloque(Entero numeroBloque) {
	BloqueWorker bloque = bloqueBuscar(numeroBloque);
	imprimirMensaje1(archivoLog, "[DATABIN] El bloque N°%i fue leido", (int*)numeroBloque);
	return bloque;
}
