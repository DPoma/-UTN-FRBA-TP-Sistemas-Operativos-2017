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
	pantallaLimpiar("clear");
	imprimirMensajeProceso("# PROCESO MASTER");
	archivoLog = archivoLogCrear(RUTA_LOG, "Master");
	archivoConfigObtenerCampos();
	configuracion = configuracionCrear(RUTA_CONFIG, (void*)configuracionLeerArchivoConfig, campos);
	strcpy(configuracion->ipWorker, IP_LOCAL);
	strcpy(configuracion->puertoWorker, "5050");
	printf("[CONEXION] Estableciendo Conexion con YAMA (IP: %s | Puerto %s)\n", configuracion->ipYAMA, configuracion->puertoYAMA);
	log_info(archivoLog, "[CONEXION] Estableciendo Conexion con YAMA (IP: %s | Puerto %s)\n", configuracion->ipYAMA, configuracion->puertoYAMA);
	socketYAMA = socketCrearCliente(configuracion->ipYAMA, configuracion->puertoYAMA, ID_MASTER);
	printf("[CONEXION] Conexion existosa con YAMA\n");
	log_info(archivoLog, "[CONEXION] Conexion exitosa con YAMA\n");
	printf("[CONEXION] Estableciendo Conexion con Worker (IP: %s | Puerto: %s)\n", configuracion->ipWorker, configuracion->puertoWorker);
	log_info(archivoLog, "[CONEXION] Estableciendo Conexion con Worker (IP: %s | Puerto: %s)\n", configuracion->ipWorker, configuracion->puertoWorker);
	socketWorker = socketCrearCliente(configuracion->ipWorker, configuracion->puertoWorker, ID_MASTER);
	printf("[CONEXION] Estableciendo Conexion con Worker\n");
	log_info(archivoLog, "[CONEXION] Conexion exitosa con Worker\n");
	estadoMaster = 1;
	senialAsignarFuncion(SIGINT, funcionSenial);
	while(estadoMaster);
	socketCerrar(socketYAMA);
	socketCerrar(socketWorker);
	return EXIT_SUCCESS;

}

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig) {
	Configuracion* configuracion = malloc(sizeof(Configuracion));
	strcpy(configuracion->ipYAMA, archivoConfigStringDe(archivoConfig, "IP_YAMA"));
	strcpy(configuracion->puertoYAMA, archivoConfigStringDe(archivoConfig, "PUERTO_YAMA"));
	archivoConfigDestruir(archivoConfig);
	return configuracion;
}

void archivoConfigObtenerCampos() {
	campos[0] = "IP_YAMA";
	campos[1] = "PUERTO_YAMA";
}

void funcionSenial(int senial) {
	estadoMaster = 0;
	puts("");
}
