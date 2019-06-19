#ifndef PAGE_INPUT_H_
#define PAGE_INPUT_H_

#include "ESP32Pix.h"

void send_input_vals(AsyncWebServerRequest *request) {
    if (request->params()) {
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "etherIPaddress") configData.ethIPAddress = p->value();
            if (p->name() == "etherIPsubmask") configData.ethIPSubmask = p->value();
			if (p->name() == "etherIPgateway") configData.ethIPGateway = p->value(); 
            if (p->name() == "startUniverse") configData.univStart = p->value().toInt();
            if (p->name() == "numberUniverse") configData.univCount = p->value().toInt();
            if (p->name() == "sizeUniverse") configData.univSize = p->value().toInt();
            if (p->name() == "input_mode") configData.inputMode = p->value().toInt();
        }
        saveConfig();

        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
}

void get_input_vals(AsyncWebServerRequest *request) {
    String values = "";
    values += "etherIPaddress|input|" + configData.ethIPAddress + "\n";
    values += "etherIPsubmask|input|" + configData.ethIPSubmask + "\n";
    values += "etherIPgateway|input|" + configData.ethIPGateway + "\n";
	values += "startUniverse|input|" + (String)configData.univStart + "\n";
	values += "numberUniverse|input|" + (String)configData.univCount + "\n";
	values += "sizeUniverse|input|" + (String)configData.univSize + "\n";
	values += "input_mode|input|" + (String)configData.inputMode + "\n";
    request->send(200, "text/plain", values);
}

#endif
