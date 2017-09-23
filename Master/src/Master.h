/*
 * Master.h

 *
 *  Created on: 15/9/2017
 *      Author: Dario Poma
 */

#include "../../Biblioteca/src/Biblioteca.c"

#define RUTA_CONFIG "../MasterConfig.conf"
#define RUTA_LOG "../../../MasterLog.log"

typedef struct {
	char ipYAMA[50];
	char puertoYAMA[50];
	char ipWorker[50];
	char puertoWorker[50];
} Configuracion;

String campos[2];
Configuracion* configuracion;
ArchivoLog archivoLog;
Socket socketYAMA;
Socket socketWorker;
int estadoMaster;

Configuracion* configuracionLeerArchivoConfig(ArchivoConfig archivoConfig);
void archivoConfigObtenerCampos();

bool masterActivado();
bool masterDesactivado();
void masterActivar();
void masterDesactivar();
