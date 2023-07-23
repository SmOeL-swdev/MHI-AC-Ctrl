
/*
	Simpel update WebServer library
	V.01 23-JUL-2023
	E.Kosse
*/
#include "Arduino.h"
#include "MHI-WebServer.h"
#include "support.h"
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;


void WServer::Init(void){

    httpUpdater.setup(&httpServer, UPDATE_PATH, FW_USER, FW_PASSWORD);
    httpServer.begin();
}

void WServer::Handle(void){    
   httpServer.handleClient();
}