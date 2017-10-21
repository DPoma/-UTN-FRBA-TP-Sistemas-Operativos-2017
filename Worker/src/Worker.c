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

/*int main(void) {
	workerIniciar();
	socketListenerWorker = socketCrearListener(configuracion->puertoWorker);
	imprimirMensajeUno(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto: %s)", configuracion->puertoWorker);
	while(estadoWorker)
		socketAceptarConexion();
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	return EXIT_SUCCESS;
}*/

void workerCrearHijo(Socket unSocket) {
	int pid = fork();
	if(pid == 0) {
	imprimirMensaje(archivoLog, "[CONEXION] Esperando mensajes de Master");
	Mensaje* mensaje = mensajeRecibir(unSocket);
	char* codigo;
	int sizeCodigo;
	char* origen;
	int sizeOrigen;
	char* destino;
	int sizeDestino;
	switch(mensaje->header.operacion){
			case -1:
				imprimirMensaje(archivoLog, "[EJECUCION] Murio el Master"); //revisar si deve morir por este caso
				estadoWorker=0;
				break;
			case 1: //Etapa Transformacion
			{
				int origenB; //si el origen es un numero de bloque esto lo facilitaria, revisar
				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&origen,mensaje->datos + sizeof(int32_t)+sizeCodigo, sizeof(int32_t));
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo , sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo, sizeDestino);
				transformar(codigo,origenB,destino);
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
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 2:{ //Etapa Reduccion Local

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				reduccionLocal(codigo,origen,destino);
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
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 3:{ //Etapa Reduccion Global

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//reduccionGlobal();
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
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
			case 4:{ //Almacenamiento Definitivo

				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t));
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				//almacenar();
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
				free(codigo);
				free(origen);
				free(destino);
				break;
			}
		}
	//printf("Mensaje: %s\n", (String)mensaje->datos);
	}
	else if(pid > 0)
		puts("PADRE ACEPTO UNA CONEXION");
	else
		puts("ERROR");
}

//1er etapa
int transformar(char* codigo,int origen,char* destino){
	if (origen>tamanioArchData){
		return -1;
	}
	char* buffer=NULL;
	FILE* arch;
	arch = fopen(configuracion->rutaDataBin,"r");
	fseek(arch,MB*origen,SEEK_SET);
	int i;
	char c;
	int cc = 0;
	for(i=0;cc<=MB;i++){
		c = fgetc(arch);
			buffer = realloc(buffer,sizeof(char)*(cc+1));
			buffer[cc]=c;
			cc++;
	}
	/*int j;
	for(j=0;j<=size;j++){
		c = fgetc(arch);
		srtcat(buffer,c);
	}*/
	fclose(arch);
	//doy privilegios a script
	char*commando=NULL;
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char*command=NULL;
	strcat(command,"cat");
	strcat(command,buffer);
	strcat(command,"|");
	strcat(command,codigo);
	strcat(command,"| sort >");
	strcat(command,destino);
	system(command);
	free (command);
	//resuelto por parametro en vez de cat
	/*strcat(command,codigo);
	strcat(command,buffer);//suponiendo que el script requiere un buffer como parametro
	strcat(command,"| sort >");
	strcat(command,destino);
	system(command);
	free (command);*/
	return 0;
}

//2da etapa
int reduccionLocal(char* codigo,char* origen,char* destino){
	locOri* listaOri;
	listaOri = getOrigenesLocales(origen);
	char* apendado;
	apendado = appendL(listaOri);
	//doy privilegios a script
	char*commando=NULL;
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char*command=NULL;
	strcat(command,"cat");
	strcat(command,apendado);
	strcat(command,"|");
	strcat(command,codigo);
	strcat(command,">");
	strcat(command,destino);
	system(command);
	free (command);
	free (apendado);
	int i=0;
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
	int i;
	int size = sizeof(int32_t);
	int sizeOri=0;
	for(i=0;(i+1)==origenes->cant;i++){
		memcpy(&sizeOri, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		memcpy(&oris[i],origen + size, sizeOri);
		size= size + sizeOri;
		origenes->ruta[i] =oris[i];
	}
	return origenes;
}

char* appendL(locOri* origen){
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
	char* rutaArchAppend;
	//cargo  primer registro de cada archivo
	for(i=0;i==origen->cant;i++){
		VRegistros[i]=NULL;
		VLineasiguiente[i]=1;
		arch = fopen(origen->ruta[i],"r");
		/*fseek(arch,MB*origen,SEEK_SET);
		int i;
		char c;
		int cc = 0;*/
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
				again=1;
			}
			while(c!="\n"){
				c = fgetc(arch);
				buffer = realloc(buffer,sizeof(char)*(cc+1));
				buffer[cc]=c;
				cc++;
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
int reduccionGlobal(char* codigo,char* origen,char* destino){
	lGlobOri* listaOri;
	listaOri = getOrigenesGlobales(origen);
	char* apendado;
	apendado = appendG(listaOri);
	//doy privilegios a script
	char*commando=NULL;
	strcat(commando,"chmod 0755");
	strcat(commando,codigo);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char*command=NULL;
	strcat(command,"cat");
	strcat(command,apendado);
	strcat(command,"|");
	strcat(command,codigo);
	strcat(command,">");
	strcat(command,destino);
	system(command);
	free (command);
	free (apendado);
	int i=0;
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
	int size = sizeof(int32_t);
	int sizeOri=0;
	int sizeIP=0;
	for(i=0;(i-1)==origenes->cant;i++){
		memcpy(&sizeOri, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		memcpy(((globOri*) oris[i])->ruta,origen + size, sizeOri);
		size= size + sizeOri;
		memcpy(&sizeIP, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
		memcpy(((globOri*) oris[i])->ip,origen + size, sizeOri);
		size= size + sizeIP;
		memcpy(((globOri*) oris[i])->puerto, origen + size , sizeof(int32_t));
		size= size + sizeof(int32_t);
	}
	origenes->oris = oris;
	return origenes;
}

char* appendG(lGlobOri* origenes){
	char* rutaArchAppend;
	return rutaArchAppend;
}

void socketAceptarConexion() {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(socketListenerWorker, ID_MASTER);
	if(nuevoSocket != ERROR) {
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
		workerCrearHijo(nuevoSocket);
	}
}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSytem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoWorker, archivoConfigStringDe(archivoConfig, "PUERTO_WORKER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

char* agregarBarraCero(char* data, int tamanio)
{
	char* path = malloc(tamanio+1);
	memcpy(path, data, tamanio);
	path[tamanio] = '\0';
	return path;
}

void configuracionImprimir(Configuracion* configuracion) {
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] Ruta archivo data.bin: %s", configuracion->rutaDataBin);
	imprimirMensajeUno(archivoLog, "[CONFIGURACION] tamanio data.bin: %dMB", tamanioArchData);

}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "IP_PROPIO";
	campos[4] = "PUERTO_WORKER";
	campos[5] = "RUTA_DATABIN";
}

void funcionSenial(int senial) {
	puts("");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	exit(0);
}

void workerIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	archivoConfigObtenerCampos();
	senialAsignarFuncion(SIGINT, funcionSenial);
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	char* texto;
	strcat(texto,"stat -c%s ");
	strcat(texto, configuracion->rutaDataBin);
	tamanioArchData = system(texto)/MB;
	configuracionImprimir(configuracion);
	estadoWorker = 1;
	free(texto);
}
