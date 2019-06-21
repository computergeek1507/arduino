#ifndef PAGE_WIFI_H_
#define PAGE_WIFI_H_

#include "ESP32Pix.h"

void send_wifi_vals(AsyncWebServerRequest *request) {
    if (request->params()) {
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
            if (p->name() == "softapname") configData.apName = p->value();
            if (p->name() == "softappass") configData.apPass = p->value();
			if (p->name() == "hostname") configData.hostName = p->value(); 
        }
        saveConfig();

        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
}

void get_wifi_vals(AsyncWebServerRequest *request) {
    String values = "";
    values += "softapname|input|" + configData.apName + "\n";
    values += "softappass|input|" + configData.apPass + "\n";
    values += "hostname|input|" + configData.hostName + "\n";
    request->send(200, "text/plain", values);
}

#endif
