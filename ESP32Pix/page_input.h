#ifndef PAGE_INPUT_H_
#define PAGE_INPUT_H_

#include "ESP32Pix.h"

void get_input_vals(AsyncWebServerRequest *request) {
    String values = "";
    values += "etherIPaddress|input|" + configData.ethIPAddress + "\n";
    values += "etherIPsubnet|input|" + configData.ethIPSubmask + "\n";
    values += "etherIPgateway|input|" + configData.ethIPGateway + "\n";
	values += "startUniverse|input|" + (String)configData.univStart + "\n";
	values += "numberUniverse|input|" + (String)configData.univCount + "\n";
	values += "sizeUniverse|input|" + (String)configData.univSize + "\n";
	values += "input_mode|input|" + (String)configData.inputMode + "\n";
    request->send(200, "text/plain", values);
}

#endif
