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
	socketListenerWorker = socketCrearListener(configuracion->puertoMaster);
	imprimirMensaje1(archivoLog, "[CONEXION] Esperando conexiones de Master (Puerto: %s)", configuracion->puertoMaster);
	while(estadoWorker)
		socketAceptarConexion();
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	return EXIT_SUCCESS;
}

void workerCrearHijo(Socket unSocket) {
	int ppid = fork();
	if(ppid == 0) {
		imprimirMensaje(archivoLog, "[CONEXION] Esperando mensajes de Master");
		Mensaje* mensaje = mensajeRecibir(unSocket);
		char* codigo;
		int sizeCodigo;
		switch(mensaje->header.operacion){
			case -1:
			{
				imprimirMensaje(archivoLog, ("[EJECUCION] Tuve problemas para comunicarme con el Master (Pid hijo: %d)", ppid)); //el hijo fallo en comunicarse con el master
				mensajeEnviar(unSocket, DESCONEXION, NULL, 0); //MANDA AL MASTER QUE FALLO
				free(mensaje);
				break;
			}
			case TRANSFORMACION: //Etapa Transformacion
			{
				imprimirMensaje(archivoLog, "[CONEXION] recibo script master transformacion");
				codigo = malloc(mensaje->header.tamanio);
				memcpy(codigo,mensaje->datos, mensaje->header.tamanio);
				sizeCodigo=mensaje->header.tamanio;
				while(true){
					imprimirMensaje(archivoLog, "[CONEXION] Esperando bloques a transformar");
					Mensaje* mensaje = mensajeRecibir(unSocket);
					if (mensaje->header.operacion==EXITO){
						imprimirMensaje(archivoLog, "[CONEXION] Fin de envio de bloques a transformar");
						break;}
					else{
						int subpid = fork();
						imprimirMensaje(archivoLog, "[CONEXION] [SUBFORK] llega op. de transformacion");
						if(subpid == 0) {
							int origen;
							char destino[12];
							int bytes;
							memcpy(&origen,mensaje->datos, sizeof(int32_t));
							memcpy(&bytes, mensaje->datos + sizeof(int32_t), sizeof(int32_t));
							memcpy(destino,mensaje->datos + sizeof(int32_t)*2, TEMPSIZE);
							int result = transformar(codigo,sizeCodigo,origen,bytes,destino);
							if(result==-1){
								imprimirMensaje1(archivoLog,"[TRASFORMACION] Fracaso la transformacion del bloque %d",origen);
								char respuesta[INTSIZE];
								memcpy(respuesta,&origen,INTSIZE);
								mensajeEnviar(unSocket, FRACASO, respuesta, INTSIZE+1);
							}
							else{
								imprimirMensaje1(archivoLog,"[TRASFORMACION] transformacion del bloque %d exitosa",origen);
								char respuesta[INTSIZE];
								memcpy(respuesta,&origen,INTSIZE);
								mensajeEnviar(unSocket, EXITO, respuesta, INTSIZE+1);
							}
							free(mensaje);
							break;
						}
						else if(subpid > 0){
							puts("SUBPADRE ACEPTO UNA CONEXION");
							free(mensaje);
						}
						else{
							puts("ERROR");
							free(mensaje);
						}
					}
				}
				break;
			}
			case REDUCLOCAL:{ //Etapa Reduccion Local
				char* origen;
				int sizeOrigen;
				char* destino; //los temporales siempre miden 12, le podes mandar un char[12] aca y listo
				int sizeDestino;
				memcpy(&sizeCodigo, mensaje->datos, sizeof(int32_t));
				memcpy(&codigo,mensaje->datos + sizeof(int32_t), sizeCodigo);
				memcpy(&sizeOrigen, mensaje->datos+ sizeof(int32_t) +sizeCodigo, sizeof(int32_t));
				memcpy(&origen,mensaje->datos + sizeof(int32_t)*2+sizeCodigo, sizeOrigen);
				memcpy(&sizeDestino, mensaje->datos + sizeof(int32_t)*2 + sizeCodigo + sizeOrigen, sizeof(int32_t)); //es 12
				memcpy(&destino,mensaje->datos + sizeof(int32_t)*3+sizeCodigo+ sizeOrigen, sizeDestino);
				int result=reduccionLocal(codigo,sizeCodigo,origen,destino);
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
				mensajeEnviar(unSocket, EXITO, NULL, 0);
				free(codigo);
				free(origen);
				free(destino);
				free(mensaje);
				break;
			}
			case REDUCGLOBAL:{ //Etapa Reduccion Global
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
				int result = reduccionGlobal(codigo,sizeCodigo,origen,destino);
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
				mensajeEnviar(unSocket, EXITO, NULL, 0);
				free(codigo);
				free(origen);
				free(destino);
				free(mensaje);
				break;
			}
			case ALMACENADO:{ //Almacenamiento Definitivo
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
				c=NULL;
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
				break;
			}
			case PASAREG:{ //PasaRegistro
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
				break;
			}
			case ABORTAR:{ //Aborto
				imprimirMensaje(archivoLog, ("[EJECUCION] El Master me aborto (Pid hijo: %d)", pid)); //el hijo fallo en comunicarse con el master
				mensajeEnviar(unSocket, FRACASO, NULL, 0); //MANDA AL MASTER QUE FALLO
				free(mensaje);
				break;
			}
		}
	}
	else if(ppid > 0){
		puts("PADRE ACEPTO UNA CONEXION");
	}
	else
		puts("ERROR");
}

//1er etapa
int transformar(char* codigo, int sizeCodigo,int origen,int bytes ,char destino[TEMPSIZE]){
	imprimirMensaje(archivoLog,"[TRASFORMACION] Comienzo a transformar");
	if (origen>=dataBinBloques){
		imprimirMensaje(archivoLog,"[TRASFORMACION] se busca leer un bloque mayor al tamaño de bloques archivo, abortando transformacion");
		return -1;
	}
	char* patharchdes = string_from_format("%s%s",RUTA_TEMPS,destino);
	imprimirMensaje1(archivoLog,"[TRASFORMACION] Ruta temporal: %s",patharchdes);
	imprimirMensaje1(archivoLog,"[TRASFORMACION] N° de bloque a tranfsormar: %d",origen);
	imprimirMensaje1(archivoLog,"[TRASFORMACION] cant de bytes a transformar: %d",bytes);
	String fileBloque=transformacionBloqueTemporal(origen, bytes);
	String fileScript=transformacionScriptTemporal( codigo, origen, sizeCodigo);
	//doy privilegios a script
	char* commando = string_from_format("chmod 0755 %s",fileScript);
	system(commando);
	free (commando);
	//paso buffer a script y resultado script a sort
	char* command = string_from_format("cat %s | sh %s | sort > %s",fileBloque,fileScript,patharchdes);
	system(command);
	fileLimpiar(fileBloque);
	fileLimpiar(fileScript);
	free (command);
	free(patharchdes);
	return 0;
}

String transformacionBloqueTemporal(int origen, int bytes) {
	String path = string_from_format("%s/bloqueTemporal%i", RUTA_TEMPS, origen);
	File file = fileAbrir(path, ESCRITURA);
	Puntero puntero = getBloque(origen);
	fwrite(puntero, sizeof(char), bytes, file);
	fileCerrar(file);
	return path;
}

String transformacionScriptTemporal(char* codigo,int origen, int sizeCodigo) {
	String path = string_from_format("%s/scriptTemporal%i", RUTA_TEMPS, origen);
	File file = fileAbrir(path , ESCRITURA);
	fwrite(codigo, sizeof(char), sizeCodigo, file);
	fileCerrar(file);
	return path;
}

BloqueWorker bloqueBuscar(Entero numeroBloque) {
	BloqueWorker bloque = punteroDataBin + (BLOQUE * numeroBloque);
	return bloque;
}

BloqueWorker getBloque(Entero numeroBloque) {
	BloqueWorker bloque = bloqueBuscar(numeroBloque);
	imprimirMensaje1(archivoLog, "[DATABIN] Se lee bloque N°%i", (int*)numeroBloque);
	return bloque;
}

//2da etapa
int reduccionLocal(char* codigo,int sizeCodigo,char* origen,char destino[TEMPSIZE]){
	locOri* listaOri;
	listaOri = getOrigenesLocales(origen);
	int i=0;
	char* archApendado = appendL(listaOri);
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
	while(listaOri->ruta[i]!=NULL){
		free(listaOri->ruta[i]);
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
		/*fseek(arch,MB*origen,SEEK_SET);
		int i;
		char c;
		int cc = 0;*/
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

void socketAceptarConexion() {
	Socket nuevoSocket;
	nuevoSocket = socketAceptar(socketListenerWorker, ID_MASTER);
	printf("%d",nuevoSocket);
	puts("");
	if(nuevoSocket != ERROR) {
		imprimirMensaje(archivoLog, "[CONEXION] Proceso Master conectado exitosamente");
		workerCrearHijo(nuevoSocket);
	}
	else{
		imprimirMensaje(archivoLog, ROJO"[ERROR] NO SE PUDO CONECTAR CON EL PROCESO MASTER"BLANCO);
	}
}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = memoriaAlocar(sizeof(Configuracion));
	stringCopiar(configuracion->ipFileSytem, archivoConfigStringDe(archivoConfig, "IP_FILESYSTEM"));
	stringCopiar(configuracion->puertoFileSystem, archivoConfigStringDe(archivoConfig, "PUERTO_FILESYSTEM"));
	stringCopiar(configuracion->nombreNodo, archivoConfigStringDe(archivoConfig, "NOMBRE_NODO"));
	stringCopiar(configuracion->puertoMaster, archivoConfigStringDe(archivoConfig, "PUERTO_MASTER"));
	stringCopiar(configuracion->rutaDataBin, archivoConfigStringDe(archivoConfig, "RUTA_DATABIN"));
	stringCopiar(configuracion->ipPropia, archivoConfigStringDe(archivoConfig, "IP_PROPIA"));
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
	imprimirMensaje1(archivoLog, "[CONFIGURACION] Nombre Nodo: %s", configuracion->nombreNodo);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] IP: %s", configuracion->ipPropia);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] PUERTO MASTER: %s", configuracion->puertoMaster);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] IP FILESYSTEM: %s", configuracion->ipFileSytem);
	imprimirMensaje1(archivoLog, "[CONFIGURACION] PUERTO FILESYSTEM: %s", configuracion->puertoFileSystem);
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_FILESYSTEM";
	campos[1] = "PUERTO_FILESYSTEM";
	campos[2] = "NOMBRE_NODO";
	campos[3] = "IP_PROPIA";
	campos[4] = "PUERTO_MASTER";
	campos[5] = "RUTA_DATABIN";
}

void configuracionSenial(int senial) {
	puts("");
	imprimirMensaje(archivoLog, "[EJECUCION] Proceso Worker finalizado");
	exit(0);
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

void workerIniciar() {
	pantallaLimpiar();
	imprimirMensajeProceso("# PROCESO WORKER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Worker");
	archivoConfigObtenerCampos();
	senialAsignarFuncion(SIGINT, configuracionSenial);
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	configuracionImprimir(configuracion);
	dataBinConfigurar();
	estadoWorker = 1;
}
