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
	listenerWorker = socketCrearListener(configuracion->puertoWorker);
	pid_t pid = fork();
	if(pid == 0)
		workerAtenderWorkers();
	else if(pid > 0)
		workerAtenderMasters();
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el fork(), estas jodido");
}

void workerAtenderMasters() {
	while(estadoWorker) {
		socketCerrar(listenerWorker);
		workerAceptarMaster();
	}
}

void workerAtenderWorkers() {
	while(estadoWorker) {
		socketCerrar(listenerMaster);
		workerAceptarWorker();
	}
}

void workerAceptarMaster() {
	Socket nuevoSocket = socketAceptar(listenerMaster, ID_MASTER);
	if(nuevoSocket != ERROR)
		masterAtenderOperacion(nuevoSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el accept(), hoy no es tu dia papu");
}

void workerAceptarWorker() {
	Socket nuevoSocket = socketAceptar(listenerWorker, ID_WORKER);
	if(nuevoSocket != ERROR)
		workerAtenderOperacion(nuevoSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el accept(), hoy no es tu dia papu");
}

void masterAtenderOperacion(Socket unSocket) {
	pid_t pid = fork();
	if(pid == 0) {
		socketCerrar(listenerMaster);
		masterRealizarOperacion(unSocket);
	}
	else if(pid > 0)
		socketCerrar(unSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el fork(), estas jodido");
}

void masterRealizarOperacion(Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	Mensaje* mensaje = mensajeRecibir(unSocket);
	switch(mensaje->header.operacion) {
		case DESCONEXION: imprimirMensaje(archivoLog, "[AVISO] El Master se desconecto"); break;
		case TRANSFORMACION: transformacion(mensaje, unSocket); break;
		case REDUCCION_LOCAL: reduccionLocal(mensaje, unSocket); break;
		case REDUCCION_GLOBAL: reduccionGlobal(mensaje, unSocket); break;
		case ALMACENADO_FINAL: almacenadoFinal(mensaje, unSocket); break;
	}
	mensajeDestruir(mensaje);
	exit(EXIT_SUCCESS);
}

void workerAtenderOperacion(Socket socketWorker) {
	Mensaje* mensaje = mensajeRecibir(socketWorker);
	String pathReduccionLocal = string_from_format("%s%s", RUTA_TEMP, (String)mensaje->datos);
	mensajeDestruir(mensaje);
	File archivoReduccionLocal = fileAbrir(pathReduccionLocal, LECTURA);
	String buffer = stringCrear(BLOQUE);
	while(fgets(buffer, BLOQUE, archivoReduccionLocal) != NULL)
		mensaje = mensajeRecibir(socketWorker);
		if(mensaje->header.operacion == PEDIR_LINEA)
			mensajeEnviar(socketWorker, NULO, buffer, stringLongitud(buffer));
	fileCerrar(archivoReduccionLocal);
}

//--------------------------------------- Funciones de Configuracion -------------------------------------

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSytem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
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
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Esperando conexiones de Woker (Puerto: %s)", configuracion->puertoWorker);
}

void configuracionIniciarCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "IP_PROPIA";
	campos[4] = "PUERTO_MASTER";
	campos[5] = "PUERTO_WORKER";
	campos[6] = "RUTA_DATABIN";
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
	Transformacion* transformacion = memoriaAlocar(sizeof(Transformacion));
	transformacionRecibirScript(transformacion, mensaje);
	while(ACTIVADO) {
		Mensaje* mensaje = mensajeRecibir(unSocket);
		if (mensaje->header.operacion==EXITO) {
			imprimirMensaje(archivoLog, "[CONEXION] Fin de envio de bloques a transformar");
			socketCerrar(unSocket);
			break;
		}
		else {
			transformacionRecibirBloque(transformacion, mensaje->datos);
			transformacionCrearNieto(transformacion, unSocket);
		}
	}
	transformacionDestruir(transformacion);
}

int transformacionEjecutar(Transformacion* transformacion) {
	String pathBloque = transformacionCrearBloqueTemporal(transformacion);
	String pathScript = transformacionCrearScriptTemporal(transformacion);
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

void transformacionTerminar(int resultado, Socket unSocket, Entero numeroBloque) {
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

void transformacionRecibirScript(Transformacion* transformacion, Mensaje* mensaje) {
	transformacion->scriptSize = mensaje->header.tamanio;
	transformacion->script = memoriaAlocar(transformacion->scriptSize);
	memcpy(transformacion->script, mensaje->datos, transformacion->scriptSize);
}

void transformacionRecibirBloque(Transformacion* transformacion, Puntero datos) {
	memcpy(&transformacion->numeroBloque, datos, sizeof(Entero));
	memcpy(&transformacion->bytesUtilizados, datos+sizeof(Entero), sizeof(Entero));
	memcpy(transformacion->nombreResultado, datos+sizeof(Entero)*2, 12);
}

void transformacionCrearNieto(Transformacion* transformacion, Socket unSocket) {
	int estado;
	pid_t pid = fork();
	if(pid == 0) {
		int resultado = transformacionEjecutar(transformacion);
		transformacionTerminar(resultado, unSocket, transformacion->numeroBloque);
		exit(EXIT_SUCCESS);
	}
	else if(pid > 0)
		waitpid(pid, &estado, NULO);
}

void transformacionDestruir(Transformacion* transformacion) {
	memoriaLiberar(transformacion->script);
	memoriaLiberar(transformacion);
}

String transformacionCrearBloqueTemporal(Transformacion* transformacion) {
	String path = string_from_format("%sbloqueTemporal%i", RUTA_TEMP, transformacion->numeroBloque);
	File file = fileAbrir(path, ESCRITURA);
	Puntero puntero = getBloque(transformacion->numeroBloque);
	fwrite(puntero, sizeof(char), transformacion->bytesUtilizados, file);
	fileCerrar(file);
	return path;
}

String transformacionCrearScriptTemporal(Transformacion* transformacion) {
	String path = string_from_format("%sscriptTemporal%i", RUTA_TEMP, transformacion->numeroBloque);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(transformacion->script, sizeof(char), transformacion->scriptSize, file);
	fileCerrar(file);
	return path;
}

//--------------------------------------- Funciones de Reduccion Local -------------------------------------

void reduccionLocal(Mensaje* mensaje, Socket unSocket) {
	ReduccionLocal reduccion = reduccionLocalRecibirTemporales(mensaje->datos);
	String temporales = reduccionLocalObtenerTemporales(reduccion);
	int resultado = reduccionLocalEjecutar(reduccion, temporales);
	reduccionLocalTerminar(resultado, unSocket);
}

ReduccionLocal reduccionLocalRecibirTemporales(Puntero datos) {
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

void reduccionLocalTerminar(int resultado, Socket unSocket) {
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
	int indice;
	for(indice=0; indice < listaCantidadElementos(listaApareados); indice++) {
		Apareo* apareo = listaObtenerElemento(listaApareados, indice);
		apareo->linea = reduccionGlobalObtenerLinea(apareo->socketWorker);
	}
	Apareo* apareo = listaPrimerElemento(listaApareados);
	while(!listaEstaVacia(listaApareados)) {
		for(indice=0; indice < listaCantidadElementos(listaApareados); indice++)
			apareo = reduccionGlobalLineaMinima(apareo, listaObtenerElemento(listaApareados, indice));
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
	for(indice=0; indice < reduccion.cantidadWorkers; indice++) {
		pedido = reduccion.nodos[indice];
		if(stringIguales(pedido.nodo.ip, "0")) {
			Apareo* apareo = memoriaAlocar(sizeof(Apareo));
			apareo->socketWorker = socketCrearCliente(pedido.nodo.ip, pedido.nodo.port, ID_WORKER);
			mensajeEnviar(apareo->socketWorker, ENVIAR_TEMPORAL, pedido.temporal, TEMPSIZE);
			listaAgregarElemento(listaApareados, apareo);
		}
	}
	String pathResultado = string_from_format("%s%s", RUTA_TEMP, reduccion.nombreResultado);
	reduccionGlobalAlgoritmoApareo(listaApareados, pathResultado);
	return pathResultado;
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
		exit(EXIT_FAILURE);
	String linea = stringCrear(mensaje->header.tamanio);
	mensajeDestruir(mensaje);
	return linea;
}

ReduccionGlobal reduccionGlobalRecibirDatos(Puntero datos) {
	ReduccionGlobal reduccion;
	memcpy(&reduccion.scriptSize, (Entero*)datos, sizeof(Entero));
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
	ReduccionGlobal reduccion = reduccionGlobalRecibirDatos(mensaje->datos);
	String pathApareado = reduccionGlobalGenerarArchivo(reduccion);
	int resultado = reduccionGlobalEjecutar(reduccion, pathApareado);
	reduccionGlobalTerminar(resultado, unSocket);
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
		comando = string_from_format("%s %s > %s", reduccion.script, pathApareado, archivoSalida);
		resultado = system(comando);
	}
	fileLimpiar(archivoScript);
	memoriaLiberar(comando);
	memoriaLiberar(archivoSalida);
	memoriaLiberar(archivoScript);
	return resultado;
}

void reduccionGlobalTerminar(int resultado, Socket unSocket) {
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

void almacenadoFracaso(Socket unSocket) {
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] La operacion se realizo con exito");
	mensajeEnviar(unSocket, EXITO, NULL, 0);
}

void almacenadoExitoso(Socket unSocket) {
	imprimirMensaje(archivoLog,"[ALMACENADO FINAL] La operacion fracaso");
	mensajeEnviar(unSocket, FRACASO, NULL, 0);
}

void almacenadoTerminar(int resultado, Socket unSocket) {
	if(resultado == EXITO)
		almacenadoExitoso(unSocket);
	else
		almacenadoFracaso(unSocket);
}

void almacenadoFinal(Mensaje* mensaje, Socket socketMaster) {
	Socket socketFileSystem =socketCrearCliente(configuracion->ipFileSytem, configuracion->puertoFileSystem, ID_WORKER);
	mensajeEnviar(socketFileSystem, ALMACENADO_FINAL, mensaje->datos, mensaje->header.tamanio);
	Mensaje* mensajeOperacion = mensajeRecibir(socketFileSystem);
	almacenadoTerminar(mensajeOperacion->header.operacion, socketMaster);
	mensajeDestruir(mensajeOperacion);
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

/*
//2da etapa
int reduccionLocal(char* codigo,int sizeCodigo,char* origen,char* destino){
	locOri* listaOri;
	listaOri = getOrigenesLocales(origen);
	int i=0;
	char* archApendado = appendL(listaOri);
	char* patharchdes = string_from_format("%s%s",RUTA_TEMPS,destino);
	//char* fileScript=transformacionScriptTemporal(codigo, origen, sizeCodigo);
	//doy privilegios a script
	char* commando = string_from_format("chmod 0755 %s",fileScript);
	system(commando);
	memoriaLiberar(commando);
	//paso buffer a script y resultado script a sort
	char* command = string_from_format("cat %s | sh %s | sort > %s",archApendado,fileScript,patharchdes);
	system(command);
	fileLimpiar(archApendado);
	fileLimpiar(fileScript);
	memoriaLiberar (command);
	memoriaLiberar(patharchdes);
	while(listaOri->ruta[i]!=NULL){
		memoriaLiberar(listaOri->ruta[i]);
		i++;
	}
	return 0;
}


locOri* getOrigenesLocales(char* origen){
	locOri* origenes;
	memcpy(&origenes->cant, origen, sizeof(int32_t));
	char* oris [origenes->cant];
	char temporis[12];
	int i;
	int size = sizeof(int32_t);
	int sizeOri=0;
	for(i=0;(i+1)==origenes->cant;i++){
		oris[i]=NULL;
		memcpy(&sizeOri, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		//memcpy(&oris[i],origen + size, sizeOri);
		memcpy(&temporis,origen + size, sizeOri);
		size= size + sizeOri;
		oris[i] = realloc(oris[i],sizeof(RUTA_TEMPS)+16);
		strcat(oris[i],RUTA_TEMPS);
		strcat(oris[i],temporis);
		strcat(oris[i],".bin");
		origenes->ruta[i] =oris[i];
	}
	return origenes;
}

char* appendL(locOri* origen){
	char* rutaArchAppend = string_from_format("/home/utnso/Escritorio/appendtemp%s", pid);
	char* VRegistros[origen->cant];
	int VLineasiguiente[origen->cant];
	FILE* arch;
	int var;
	int cantEOF=0;
	int allend=0;
	int resultado;
	int i;
	int again=0;
	char c;
	int cc = 0;
	char* buffer=NULL;
	//cargo  primer registro de cada archivo
	for(i=0;i==origen->cant;i++){
		VRegistros[i]=NULL;
		VLineasiguiente[i]=1;
		arch = fopen(origen->ruta[i],"r");
		fseek(arch,MB*origen,SEEK_SET);
		int i;
		char c;
		int cc = 0
		c = fgetc(arch);
		if(c=="\n"){
			VLineasiguiente[i] = VLineasiguiente[i] +1;
			i--;
		}
		while(c!="\n"){
			buffer = realloc(buffer,sizeof(char)*(cc+1));
			buffer[cc]=c;
			cc++;
			c = fgetc(arch);
		}
		if(buffer!=NULL){
			VRegistros[i]= realloc(VRegistros[i],sizeof(char)*(cc+1));
			VRegistros[i]= buffer;
			cc=0;
			free(buffer);
		}
		c=NULL;
		close(arch);
	}
	i=0;
	//control de EOF de todos los archivos
	for(i=0;i==origen->cant;i++){
		if(VRegistros[i]== NULL){
			cantEOF++;
		}
	}
	if(cantEOF==origen->cant){
		allend=1;
	}
	cantEOF=0;
	//comienzo de apareo
	i=0;
	while(allend==0){
		//funcion de posicion donde esta el registro de mayor orden
		resultado = registroMayorOrden(VRegistros,origen->cant);
		//registro mayor a archivo apareo definitivo
		arch = fopen(rutaArchAppend,"w");
		fwrite(VRegistros[resultado], sizeof(char), sizeof(VRegistros[resultado]), arch);
		fwrite("\n", sizeof(char), 1, arch);
		close(arch);
		free(VRegistros[resultado]);
		VRegistros[resultado]=NULL;
		//remplazo registro
		VLineasiguiente[resultado] = VLineasiguiente[resultado]+1;
		arch = fopen(origen->ruta[resultado],"r");
		while(again==0){
			again=1;
			for(i=0;i==VLineasiguiente[resultado];i++){
				while(c!="\n"){
					c = fgetc(arch);
				}
				c=NULL;
			}
			c = fgetc(arch);
			if(c=="\n"){
				VLineasiguiente[resultado] = VLineasiguiente[resultado]+1;
				again=0;
			}
			while(c!="\n"){
				buffer = realloc(buffer,sizeof(char)*(cc+1));
				buffer[cc]=c;
				cc++;
				c = fgetc(arch);
			}
			if(buffer!=NULL){
				VRegistros[resultado]= realloc(VRegistros[resultado],sizeof(char)*(cc+1));
				VRegistros[resultado]= buffer;
				cc=0;
				free(buffer);
			}
			c=NULL;
		}
		close(arch);
		i=0;
		//control de EOF de todos los archivos
		for(i=0;i==origen->cant;i++){
			if(VRegistros[i]== NULL){
				cantEOF++;
			}
		}
		if(cantEOF==origen->cant){
			allend=1;
		}
		cantEOF=0;
	}
	return rutaArchAppend;
}

int registroMayorOrden(char** VRegistros,int cant){
	int posicion=-1;
	int flag=0;
	int i;
	int resultado;
	for(i=0;i==cant;i++){
		if(VRegistros[i]!=NULL){
			if(flag==0){
				posicion=i;
				flag=1;
			}
			else{
				resultado = strcmp(VRegistros[posicion],VRegistros[i]);
				if(resultado<1){
					posicion=i;
				}
			}
		}
	}
	return posicion;
}

//3ra etapa
int reduccionGlobal(char* codigo, int sizeCodigo,char* origen,char* destino){
	lGlobOri* listaOri;
	listaOri = getOrigenesGlobales(origen);
	char* archApendado = appendG(listaOri);
	int i=0;
	char* patharchdes = string_from_format("%s%s",RUTA_TEMPS,destino);
	String fileScript=transformacionScriptTemporal( codigo, origen, sizeCodigo);
	//doy privilegios a script
	char* commando = string_from_format("chmod 0755 %s",fileScript);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char* command = string_from_format("cat %s | sh %s | sort > %s",archApendado,fileScript,patharchdes);
	system(command);
	fileLimpiar(archApendado);
	fileLimpiar(fileScript);
	free (command);
	free(patharchdes);
	while(((globOri*)listaOri->oris[i])->ruta!=NULL){
		free(((globOri*)listaOri->oris[i])->ruta);
		i++;
	}
	i=0;
	while(((globOri*)listaOri->oris[i])->ip!=NULL){
		free(((globOri*)listaOri->oris[i])->ip);
		i++;
	}
	//for(i=0;;i++)
	//free (listaOri->oris);
	//for(i=0;(i-1)==origenes->cant;i++){
	return 0;
}


lGlobOri* getOrigenesGlobales(char* origen){
	lGlobOri* origenes;
	memcpy(&origenes->cant, origen, sizeof(int32_t));
	globOri** oris [origenes->cant];
	int i;
	char temporis[12];
	int size = sizeof(int32_t);
	int sizeOri=0;
	int sizeIP=0;
	for(i=0;(i-1)==origenes->cant;i++){
		memcpy(&sizeOri, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		memcpy(&temporis,origen + size, sizeOri);
		size= size + sizeOri;
		((globOri*) oris[i])->ruta = realloc(((globOri*) oris[i])->ruta,sizeof(RUTA_TEMPS)+16);
		strcat(((globOri*) oris[i])->ruta,RUTA_TEMPS);
		strcat(((globOri*) oris[i])->ruta,temporis);
		strcat(((globOri*) oris[i])->ruta,".bin");
		memcpy(&sizeIP, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		memcpy(((globOri*) oris[i])->ip,origen + size, sizeOri);
		size= size + sizeIP;
		memcpy(&((globOri*) oris[i])->puerto, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
	}
	origenes->oris = oris;
	return origenes;
}

char* appendG(lGlobOri* origenes){
	char* rutaArchAppend = string_from_format("/home/utnso/Escritorio/appendtemp%s", pid);
	char* VRegistros[origenes->cant];
	int VLineasiguiente[origenes->cant];
	FILE* arch;
	int var;
	int cantEOF=0;
	int allend=0;
	int resultado;
	int i;
	int again=0;
	char c;
	int cc = 0;
	int bufferSize;
	char* buffer=NULL;
	Socket socketClientWorker;
	int local;
	//cargo  primer registro de cada archivo
	for(i=0;i==origenes->cant;i++){
		//me fijo si el archivo el local o remoto
		local=0;
		VRegistros[i]=NULL;
		VLineasiguiente[i]=1;
			//etapa local
		if(((globOri*)origenes->oris[i])->ip=="0"){ //VER SI SE CAMBIA
			local=1;
			arch = fopen(((globOri*)origenes->oris[i])->ruta,"r");
			c = fgetc(arch);
			if(c=="\n"){
				VLineasiguiente[i] = VLineasiguiente[i] +1;
				i--;
			}
			while(c!="\n"){
				buffer = realloc(buffer,sizeof(char)*(cc+1));
				buffer[cc]=c;
				cc++;
				c = fgetc(arch);
			}
			close(arch);
			c=NULL;
		}
			//etapa remota
		if (local==0){
		imprimirMensaje2(archivoLog, "[CONEXION] Realizando conexion con Worker (IP: %s | Puerto %s)", ((globOri*)origenes->oris[i])->ip, ((globOri*)origenes->oris[i])->puerto);
		socketClientWorker = socketCrearCliente(((globOri*)origenes->oris[i])->ip,((globOri*)origenes->oris[i])->puerto,ID_MASTER);
		imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con Worker");
		//serializo
		int mensajeSize = sizeof(int32_t)*2 + sizeof(((globOri*)origenes->oris[i])->ruta);
		char* mensajeData = malloc(mensajeSize);
		char* puntero = mensajeData;
		memcpy(puntero, sizeof(((globOri*)origenes->oris[i])->ruta), sizeof(int32_t));
		puntero += sizeof(int32_t);
		memcpy(puntero, ((globOri*)origenes->oris[i])->ruta, sizeof(((globOri*)origenes->oris[i])->ruta));
		puntero += sizeof(((globOri*)origenes->oris[i])->ruta);
		memcpy(puntero, VLineasiguiente[i], sizeof(int32_t));
		//envio y recibo
		mensajeEnviar(socketClientWorker,PASAREG,mensajeData,mensajeSize);
		free(mensajeData);
		free(puntero);
		Mensaje* mensaje =mensajeRecibir(socketClientWorker);
		//deserializo
		memcpy(&bufferSize, mensaje, sizeof(int32_t));
		memcpy(&buffer, mensaje + sizeof(int32_t), sizeof(bufferSize));
		memcpy(&VLineasiguiente[i], mensaje + sizeof(int32_t) + sizeof(bufferSize), sizeof(int32_t));
		free(mensaje);
		}

		//etapa local
		VRegistros[i]=NULL;
		VLineasiguiente[i]=1;
		fseek(arch,MB*origen,SEEK_SET);
		int i;
		char c;
		int cc = 0;
		c = fgetc(arch);
		if(c=="\n"){
			VLineasiguiente[resultado] = VLineasiguiente[resultado] +1;
			i--;
		}
		while(c!="\n"){
			c = fgetc(arch);
			buffer = realloc(buffer,sizeof(char)*(cc+1));
			buffer[cc]=c;
			cc++;
		}

		if(buffer!=NULL){
			VRegistros[i]= realloc(VRegistros[i],sizeof(char)*(cc+1));
			VRegistros[i]= buffer;
			cc=0;
			free(buffer);
		}
	}
	i=0;
	//control de EOF de todos los archivos
	for(i=0;i==origenes->cant;i++){
		if(VRegistros[i]== NULL){
			cantEOF++;
		}
	}
	if(cantEOF==origenes->cant){
		allend=1;
	}
	cantEOF=0;
	i=0;
	//comienzo de apareo
	while(allend==0){
		local=0;
		//funcion de posicion donde esta el registro de mayor orden
		resultado = registroMayorOrden(VRegistros,origenes->cant);
		//registro mayor a archivo apareo definitivo
		arch = fopen(rutaArchAppend,"w");
		fwrite(VRegistros[resultado], sizeof(char), sizeof(VRegistros[resultado]), arch);
		fwrite("\n", sizeof(char), 1, arch);
		close(arch);
		free(VRegistros[resultado]);
		VRegistros[resultado]=NULL;
		//remplazo registro
		VLineasiguiente[resultado] = VLineasiguiente[resultado]+1;
			//etapa local
		if(((globOri*)origenes->oris[resultado])->ip=="0"){ //VER SI SE CAMBIA
			local=1;
			arch = fopen(((globOri*)origenes->oris[resultado])->ruta,"r");
			while(again==0){
				again=1;
				for(i=0;i==VLineasiguiente[resultado];i++){
					while(c!="\n"){
						c = fgetc(arch);
					}
					c=NULL;
				}
				c = fgetc(arch);
				if(c=="\n"){
					VLineasiguiente[resultado] = VLineasiguiente[resultado]+1;
					again=0;
				}
				while(c!="\n"){
					buffer = realloc(buffer,sizeof(char)*(cc+1));
					buffer[cc]=c;
					cc++;
					c = fgetc(arch);
				}
				c=NULL;
			}
			close(arch);
		}
			//etapa remota
		if (local==0){
		imprimirMensaje2(archivoLog, "[CONEXION] Realizando conexion con Worker (IP: %s | Puerto %s)", ((globOri*)origenes->oris[resultado])->ip, ((globOri*)origenes->oris[resultado])->puerto);
		socketClientWorker = socketCrearCliente(((globOri*)origenes->oris[resultado])->ip,((globOri*)origenes->oris[resultado])->puerto,ID_MASTER);
		imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con Worker");
		//serializo
		int mensajeSize = sizeof(int32_t)*2 + sizeof(((globOri*)origenes->oris[resultado])->ruta);
		char* mensajeData = malloc(mensajeSize);
		char* puntero = mensajeData;
		memcpy(puntero, sizeof(((globOri*)origenes->oris[resultado])->ruta), sizeof(int32_t));
		puntero += sizeof(int32_t);
		memcpy(puntero, ((globOri*)origenes->oris[resultado])->ruta, sizeof(((globOri*)origenes->oris[resultado])->ruta));
		puntero += sizeof(((globOri*)origenes->oris[resultado])->ruta);
		memcpy(puntero, VLineasiguiente[resultado], sizeof(int32_t));
		//envio y recibo
		mensajeEnviar(socketClientWorker,PASAREG,mensajeData,mensajeSize);
		free(mensajeData);
		free(puntero);
		Mensaje* mensaje =mensajeRecibir(socketClientWorker);
		//deserializo
		memcpy(&bufferSize, mensaje, sizeof(int32_t));
		memcpy(&buffer, mensaje + sizeof(int32_t), sizeof(bufferSize));
		memcpy(&VLineasiguiente[resultado], mensaje + sizeof(int32_t) + sizeof(bufferSize), sizeof(int32_t));
		free(mensaje);
		}
		if(buffer!=NULL){
			VRegistros[resultado]= realloc(VRegistros[resultado],sizeof(char)*(cc+1));
			VRegistros[resultado]= buffer;
			cc=0;
			free(buffer);
		}
		i=0;
		//control de EOF de todos los archivos
		for(i=0;i==origenes->cant;i++){
			if(VRegistros[i]== NULL){
				cantEOF++;
			}
		}
		if(cantEOF==origenes->cant){
			allend=1;
		}
		cantEOF=0;
	}

	return rutaArchAppend;
}

datosReg* PasaRegistro(char* ruta,int NroReg){
	datosReg* Reg;
	char* buffer;
	FILE* arch;
	char c;
	int cc;
	int again=0;
	int i;
	arch = fopen(ruta,"r");
	while(again==0){
		again=1;
		for(i=0;i==NroReg;i++){
			while(c!="\n"){
				c = fgetc(arch);
			}
			c=NULL;
		}
		c = fgetc(arch);
		if(c=="\n"){
			NroReg = NroReg+1;
			again=0;
		}
		while(c!="\n"){
			buffer = realloc(buffer,sizeof(char)*(cc+1));
			buffer[cc]=c;
			cc++;
			c = fgetc(arch);
		}
		c=NULL;
	}
	close(arch);
	Reg->sizebuffer=cc;
	Reg->buffer = malloc(Reg->sizebuffer);
	Reg->buffer=buffer;
	Reg->NumReg=NroReg+1;
	return Reg;
}
*/
