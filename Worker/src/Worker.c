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
	workerAtenderMasters();
	workerFinalizar();
	return EXIT_SUCCESS;
}

//--------------------------------------- Funciones de Worker -------------------------------------

void workerIniciar() {
	configuracionCalcularBloques();
	configuracionIniciar();
	estadoWorker = ACTIVADO;
}

void workerAtenderMasters() {
	listenerMaster = socketCrearListener(configuracion->puertoMaster);
	listenerWorker = socketCrearListener(configuracion->puertoWorker);
	while(estadoWorker)
		masterAceptarConexion();
}

void workerFinalizar() {
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
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
	configuracionCalcularBloques();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	senialAsignarFuncion(SIGINT, configuracionSenial);
}

void configuracionIniciarLog() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Nombre: %s", configuracion->nombreNodo);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Cantidad de bloques: %d", (int*)dataBinBloques);
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
}

void configuracionSenial(int senial) {
	puts("");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
}

//--------------------------------------- Funciones de Master -------------------------------------

void masterAceptarConexion() {
	Socket nuevoSocket = socketAceptar(listenerWorker, ID_MASTER);
	if(nuevoSocket != ERROR)
		masterEjecutarOperacion(nuevoSocket);
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el accept(), hoy no es tu dia papu");
}

void masterAtenderOperacion(Socket unSocket) {
	imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
	pid = fork();
	if(pid == 0)
		masterEjecutarOperacion(unSocket);
	else if(pid > 0)
		puts("PADRE ACEPTO UNA CONEXION");
	else
		imprimirMensaje(archivoLog, "[ERROR] Error en el fork(), estas jodido");
}

void masterEjecutarOperacion(Socket unSocket) {
	Mensaje* mensaje = mensajeRecibir(unSocket);
	switch(mensaje->header.operacion){
		case DESCONEXION: imprimirMensaje(archivoLog, "[AVISO] El Master #%d se desconecto"); break;
		case TRANSFORMACION: transformacionIniciar(mensaje->datos, unSocket); break;
		case REDUCCION_LOCAL: break;
		case REDUCCION_GLOBAL: break;
		case ALMACENADO: break;
		case PASAREG: break;
	}
}

//--------------------------------------- Funciones de Transformacion -------------------------------------

Transformacion* transformacionRecibir(Puntero datos) {
	int sizeScript = *(Entero*)datos;
	Transformacion* transformacion = memoriaAlocar(sizeof(Transformacion));
	transformacion->script = memoriaAlocar(sizeScript);
	memcpy(transformacion->script, datos+sizeof(Entero), sizeScript);
	memcpy(&transformacion->numeroBloque, datos+sizeof(Entero)+sizeScript, sizeof(Entero));
	memcpy(&transformacion->bytesUtilizados, datos+sizeof(Entero)*2+sizeScript, sizeof(Entero));
	memcpy(transformacion->archivoTemporal, datos+sizeof(Entero)*3+sizeScript, 12);
	return transformacion;
}

void transformacionIniciar(Mensaje* mensaje, Socket unSocket) {
	imprimirMensaje(archivoLog, "[TRANSFORMACION] Etapa iniciada en Master #%d");
	Transformacion* transformacion = transformacionRecibir(mensaje->datos);
	int resultado = transformacionEjecutar(transformacion);
	if(resultado == OK)
		transformacionExito(transformacion->numeroBloque, unSocket);
	else
		transformacionFracaso(transformacion->numeroBloque, unSocket);
}

int transformacionEjecutar(Transformacion* transformacion) {
	String pathBloque = transformacionBloqueTemporal(transformacion->numeroBloque, transformacion->bytesUtilizados);
	String pathScript = transformacionScriptTemporal(transformacion->script, transformacion->numeroBloque);
	String pathDestino = string_from_format("%s/%s", RUTA_TEMPS, transformacion->archivoTemporal);
	String comando = string_from_format("cat %s | sh %s | sort > %s", pathBloque, pathScript, pathDestino);
	int resultado = system(comando);
	memoriaLiberar(pathScript);
	memoriaLiberar(pathDestino);
	memoriaLiberar(pathBloque);
	return resultado;
}

void transformacionExito(Entero numeroBloque, Socket unSocket) {
	imprimirMensaje1(archivoLog,"[TRANSFORMACION] Transformacion exitosa en bloque N°%i de Master #%d", (int*)numeroBloque);
	mensajeEnviar(unSocket, EXITO, &numeroBloque, sizeof(Entero));

}

void transformacionFracaso(Entero numeroBloque, Socket unSocket) {
	imprimirMensaje(archivoLog,"[TRANSFORMACION] transformacion Fracaso");
	mensajeEnviar(unSocket, FRACASO, &numeroBloque, sizeof(Entero));
}

String transformacionBloqueTemporal(Entero numeroBloque, Entero bytesUtilizados) {
	String path = string_from_format("%s/bloqueTemporal%i", RUTA_TEMPS, numeroBloque);
	File file = fileAbrir(path, ESCRITURA);
	bloqueBuscar(numeroBloque);
	fwrite(punteroDataBin, sizeof(char), bytesUtilizados, file);
	fileCerrar(file);
	return path;
}

String transformacionScriptTemporal(String script, Entero numeroBloque) {
	String path = string_from_format("%s/scriptTemporal%i", RUTA_TEMPS, numeroBloque);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(script, sizeof(char), stringLongitud(script), file);
	fileCerrar(file);
	return path;
}

//--------------------------------------- Funciones de DataBin -------------------------------------

void dataBinConfigurar() {
	dataBinAbrir();
	punteroDataBin = dataBinMapear();
	configuracionCalcularBloques();
}

void dataBinAbrir() {
	dataBin = fileAbrir(configuracion->rutaDataBin, LECTURA);
	if(dataBin == NULL){
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




/* todo
void redulocal() {
	char* origen;
	int sizeOrigen;
	char* destino; //los temporales siempre miden 12, le podes mandar un char[12] aca y listo
	int sizeDestino;
	memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
	memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
	memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
	memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
	memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
	memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
	int result=reduccionLocalEjecutar(codigo,origen,destino);
	if(result==-1){
		mensajeEnviar(unSocket, FRACASO, NULL, 0);
	}
	/*char* buffer = leerArchivo(path,offset,size);
	log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
	if(buffer=="-1"){
		lSend(conexion, NULL, -4, 0);
		log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
		break;
	}
	//enviar el buffer
	lSend(conexion, buffer, 2, sizeof(char)*size);
	free(buffer);
	free(path);*/
/* todo
	mensajeEnviar(unSocket, EXITO, NULL, 0);
	free(codigo);
	free(origen);
	free(destino);
	free(mensaje);
}
*/

/*todo
void reducglobal() {
	char* origen;
					int sizeOrigen;
					char* destino; //los temporales siempre miden 12, le podes mandar un char[12] aca y listo
					int sizeDestino;
					memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
					memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
					memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
					memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
					memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
					memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
					int result = reduccionGlobalEjecutar(codigo,origen,destino);
					if(result==-1){
						mensajeEnviar(unSocket, FRACASO, NULL, 0);
					}
					/*char* buffer = leerArchivo(path,offset,size);
					log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
					if(buffer=="-1"){
						lSend(conexion, NULL, -4, 0);
						log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
						break;32_t
					}
					//enviar el buffer
					lSend(conexion, buffer, 2, sizeof(char)*size);
					free(buffer);
					free(path);*/
/* todo
					mensajeEnviar(unSocket, EXITO, NULL, 0);
					free(codigo);
					free(origen);
					free(destino);
					free(mensaje);
}
*///todo

/* todo
void alamcenado() {
	int sizeNombre;
			char* Nombre;
			int sizeRuta;
			char* Ruta;
			int sizebuffer=0;
			char* buffer;
			char* c;
			memcpy(&sizeNombre, mensaje->datos, sizeof(int32_t));
			memcpy(&Nombre,mensaje->datos + sizeof(int32_t), sizeNombre);
			memcpy(&sizeRuta, mensaje->datos+ sizeof(int32_t) +sizeNombre, sizeof(int32_t));
			memcpy(&Ruta,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeRuta);
			//almacenar();
			//leer el archivo y meterlo en un buffer
			FILE* arch;
			arch = fopen(Ruta,"r");
			c = fgetc(arch);
			while(c!=EOF){
				buffer = realloc(buffer,sizeof(char)*(sizebuffer+1));
				buffer[sizebuffer]=c;
				sizebuffer++;
				c = fgetc(arch);
			}
					/*if(buffer!=NULL){
						VRegistros[i]= realloc(VRegistros[i],sizeof(char)*(sizebuffer+1));
						VRegistros[i]= buffer;
						sizebuffer=0;
						free(buffer);
					}*/
			/*todo c=NULL;
			close(arch);
			//creo socket
			Socket socketFS;
			imprimirMensaje2(archivoLog, "[CONEXION] Realizando conexion con FileSystem (IP: %s | Puerto %s)", configuracion->ipFileSytem, configuracion->puertoFileSystem);
			socketFS = socketCrearCliente(configuracion->ipFileSytem,configuracion->puertoFileSystem,ID_WORKER);
			imprimirMensaje(archivoLog, "[CONEXION] Conexion exitosa con FileSystem");
			//serializo
			int mensajeSize = sizeof(int32_t)*3 + sizeNombre + sizeRuta + sizebuffer;
			char* mensajeData = malloc(mensajeSize);
			char* puntero = mensajeData;
			memcpy(puntero, sizeNombre, sizeof(int32_t));
			puntero += sizeof(int32_t);
			memcpy(puntero, Nombre, sizeNombre);
			puntero += sizeNombre;
			memcpy(puntero, sizeRuta, sizeof(int32_t));
			puntero += sizeof(int32_t);
			memcpy(puntero, Ruta, sizeRuta);
			puntero += sizeRuta;
			memcpy(puntero, sizebuffer, sizeof(int32_t));
			puntero += sizeof(int32_t);
			memcpy(puntero, buffer, sizebuffer);
			//envio
			mensajeEnviar(socketFS,ALMACENADO,mensajeData,mensajeSize); //CAMBIAR 2 Seguramente
			free(mensajeData);
			free(puntero);
			mensajeEnviar(unSocket, ALMACENADO, NULL, 0);
			free(codigo);
			free(mensaje);
			mensajeEnviar(unSocket, EXITO, NULL, 0);
}
todo */

/*todo
void pasaRegistro() {
	int sizeRuta;
	char* ruta;
	int numeroReg;
	memcpy(&sizeRuta, mensaje->datos, sizeof(int32_t));
	memcpy(&ruta,mensaje->datos + sizeof(int32_t), sizeRuta);
	memcpy(&numeroReg, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
	datosReg* Reg = PasaRegistro(ruta,numeroReg);
	/*char* buffer = leerArchivo(path,offset,size);
	log_info(logFile, "[FILE SYSTEM] EL KERNEL PIDE LEER: %s | OFFSET: %i | SIZE: %i", path, offset, size);
	if(buffer=="-1"){
		lSend(conexion, NULL, -4, 0);
		log_error(logFile, "[LEER]: HUBO UN ERROR AL LEER");
		break;
	}
	//enviar el buffer
	lSend(conexion, buffer, 2, sizeof(char)*size);
	free(buffer);
	free(path);*/
	//serializo
/*todo
	int mensajeSize = sizeof(int)*2 +  Reg->sizebuffer;
	char* mensajeData = malloc(mensajeSize);
	char* puntero = mensajeData;
	memcpy(puntero, Reg->sizebuffer, sizeof(int));
	puntero += sizeof(int);
	memcpy(puntero, Reg->buffer, Reg->sizebuffer);
	puntero += Reg->sizebuffer;
	memcpy(puntero, Reg->NumReg, sizeof(int));
	//envio
	mensajeEnviar(unSocket,PASAREG,mensajeData,mensajeSize);
	free(mensajeData);
	free(puntero);
	free(codigo);
	free(mensaje);
}
todo*/

/*//TODO remover
//2da etapa
int reduccionLocalEjecutar(char* codigo,char* origen,char* destino){
	locOri* listaOri;
	listaOri = getOrigenesLocales(origen);
	char* apendado;
	int i;
	apendado = appendL(listaOri);
	char* patharchdes;
	patharchdes = realloc(patharchdes,sizeof(RUTA_TEMPS)+16);
	strcat(patharchdes,RUTA_TEMPS);
	strcat(patharchdes,destino);
	strcat(patharchdes,".bin");
	//doy privilegios a script
	char commando [500];
	for(i=0;i==500;i++){
		commando[i]=NULL;
	}
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char command [500];
	for(i=0;i==500;i++){
		command[i]=NULL;
	}
	strcat(command,"cat");
	strcat(command,apendado);
	strcat(command,"|");
	strcat(command,codigo);
	strcat(command,">");
	strcat(command,patharchdes);
	system(command);
	free (command);
	free(patharchdes);
	free (apendado);
	i=0;
	while(listaOri->ruta[i]!=NULL){
		free(listaOri->ruta[i]);
		i++;
	}
	//for(i=0;;i++)
	//free (listaOri->ruta);
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
		/*fseek(arch,MB*origen,SEEK_SET);
		int i;
		char c;
		int cc = 0;*/
/*todo remover
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
int reduccionGlobalEjecutar(char* codigo,char* origen,char* destino){
	lGlobOri* listaOri;
	listaOri = getOrigenesGlobales(origen);
	char* apendado;
	apendado = appendG(listaOri);
	int i=0;
	char* patharchdes;
	patharchdes = realloc(patharchdes,sizeof(RUTA_TEMPS)+16);
	strcat(patharchdes,RUTA_TEMPS);
	strcat(patharchdes,destino);
	strcat(patharchdes,".bin");
	//doy privilegios a script
	char commando [500];
	for(i=0;i==500;i++){
		commando[i]=NULL;
	}
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char command [500];
	for(i=0;i==500;i++){
		command[i]=NULL;
	}
	strcat(command,"cat");
	strcat(command,apendado);
	strcat(command,"|");
	strcat(command,codigo);
	strcat(command,">");
	strcat(command,patharchdes);
	system(command);
	free (command);
	free (apendado);
	free(patharchdes);
	i=0;
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
		socketClientWorker = socketCrearCliente(((globOri*)origenes->oris[i])->ip,((globOri*)origenes->oris[i])->puerto,ID_WORKER);
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
		/*VRegistros[i]=NULL;
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
		}*/
/*Todo remover
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
		socketClientWorker = socketCrearCliente(((globOri*)origenes->oris[resultado])->ip,((globOri*)origenes->oris[resultado])->puerto,ID_WORKER);
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

char* agregarBarraCero(char* data, int tamanio)
{
	char* path = malloc(tamanio+1);
	memcpy(path, data, tamanio);
	path[tamanio] = '\0';
	return path;
}


//TODO cambiar por alamcenadoFinal algo asi
void workerAtenderWorkers() {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(listenerWorker, ID_WORKER);
	if(nuevoSocket != ERROR) {
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Worker conectado exitosamente");
		workerAtenderOperacion(nuevoSocket);
	}
}
*/ //todo remover
