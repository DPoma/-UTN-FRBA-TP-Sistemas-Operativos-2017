/*
 ============================================================================
 Name        : Master.c
 Author      : Dario Poma
 Version     : 1.0
 Copyright   : Todos los derechos reservados papu
 Description : Proceso YAMA
 ============================================================================
 */

#include "Master.h"

char* leerArchivo(FILE* archivo) {
	fseek(archivo, 0, SEEK_END);
	long posicion = ftell(archivo);
	fseek(archivo, 0, SEEK_SET);
	char *texto = malloc(posicion + 1);
	fread(texto, posicion, 1, archivo);
	texto[posicion] = '\0';
	return texto;
}

FILE* archivoAbrir(char* path) {
	FILE* archivo = fopen(path, "r");
	return archivo;
}

int archivoValido(FILE* archivo) {
	return archivo != NULL;
}

void archivoCerrar(FILE* archivo) {
	fclose(archivo);
}

bool esUnArchivo(char* c) {
	return string_contains(c, ".");
}


void enviarArchivo(FILE* archivo) {
	char* texto = leerArchivo(archivo);
	archivoCerrar(archivo);
	mensajeEnviar(socketYAMA, 4, texto, strlen(texto)+1);
	mensajeEnviar(socketWorker, 4, texto, strlen(texto)+1);
	free(texto);
}


char* leerCaracteresEntrantes() {
	int i, caracterLeido;
	char* cadena = malloc(1000);
	for(i = 0; (caracterLeido= getchar()) != '\n'; i++)
		cadena[i] = caracterLeido;
	cadena[i] = '\0';
	return cadena;
}


int main(void) {
	system("clear");
	imprimirMensajeProceso("# PROCESO MASTER");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	socketYAMA = socketCrearCliente(configuracion->ipYAMA, configuracion->puertoYAMA);
	printf("Conectado a YAMA en IP: %s | Puerto %s\n", configuracion->ipYAMA, configuracion->puertoYAMA);
	socketWorker = socketCrearCliente("127.0.0.1", "5050");
	printf("Conectado a Master en IP: 127.0.0.1 | Puerto: 5050\n");
	estado = 1;
	senialAsignarFuncion(SIGINT, funcionSenial);
	while(estado){
		printf("Ingrese la ruta de un archivo: ");
		char* ruta = leerCaracteresEntrantes();
		FILE* archivo = archivoAbrir(ruta);
		if(archivoValido(archivo)) {
			if(estado)
				enviarArchivo(archivo);
		}
		else
		imprimirMensajeProceso("ERROR: Archivo invalido");
	}
	socketCerrar(socketYAMA);
	socketCerrar(socketWorker);
	return 0;

}


Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipYAMA, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	strcpy(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigImprimir(Configuracion* configuracion) {
	puts("DATOS DE CONFIGURACION");
	puts("----------------------------------------------------------------");
	printf("IP YAMA: %s\n", configuracion->ipYAMA);
	printf("Puerto YAMA: %s\n", configuracion->puertoYAMA);
	puts("----------------------------------------------------------------");
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void funcionSenial(int senial) {
	estado = 0;
	puts("");
	imprimirMensajeProceso("# PROCESO MASTER FINALIZADO");
	puts("Aprete enter para salir");
	puts("----------------------------------------------------------------");
}
