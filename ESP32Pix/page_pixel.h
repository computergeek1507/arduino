#ifndef PAGE_PIXEL_H_
#define PAGE_PIXEL_H_

#include "ESP32Pix.h"

void get_pixel_vals(AsyncWebServerRequest *request) {
    String values = "";

for(int i = 0; i<configData.univCount; i++)
{
	int unv = configData.univStart + i;
	values += "port1uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port2uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port3uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port4uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port5uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port6uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port7uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "port8uni|opt|" + (String)unv + "|" + (String)i + "\n";
	values += "serialuni|opt|" + (String)unv + "|" + (String)i + "\n";
}
for(int j = 0; j<NUM_PORTS; j++)
{
	const String name =  "port"+(String)(j+1);
	values += name +"pix|input|" + (String)configData.ports[j].pixCount + "\n";
	if(configData.ports[j].startUni >= configData.univStart && configData.ports[j].startUni <= (configData.univStart + configData.univCount))
		values += name + "uni|input|" + (String)(configData.ports[j].startUni - configData.univStart) + "\n";
	values += name + "chan|input|" + (String)configData.ports[j].startChan + "\n";

	values += name +"endchan|div|#" + (String)configData.ports[j].endUni + ":" + (String)configData.ports[j].endChan + "\n";
	values += name +"bright|input|" + (String)configData.ports[j].brightness + "\n";

}
	if(configData.serialUni >= configData.univStart && configData.serialUni <= (configData.univStart + configData.univCount))
		values += "serialuni|input|" + (String)(configData.serialUni - configData.univStart) + "\n";
    values += "testmode|input|" + (String)configData.testMode + "\n";
    request->send(200, "text/plain", values);
}

void send_pixel_vals(AsyncWebServerRequest *request) {
    if (request->params()) {
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
			Serial.println(p->name());
			Serial.println(p->value());
			for(int j = 0; j<NUM_PORTS; j++)
			{
				const String name =  "port"+(String)(j+1);
				if (p->name() == name + "pix") configData.ports[j].pixCount = p->value().toInt();
				if (p->name() == name + "uni") configData.ports[j].startUni = configData.univStart + p->value().toInt();
				if (p->name() == name + "chan") configData.ports[j].startChan = p->value().toInt();
				if (p->name() == name + "bright") configData.ports[j].brightness = p->value().toInt();
			}
			if (p->name() == "serialuni") configData.serialUni = configData.univStart + p->value().toInt();

        }
        saveConfig();

        AsyncWebServerResponse *response = request->beginResponse(303);
        response->addHeader("Location", request->url());
        request->send(response);
    } else {
        request->send(400);
    }
}

#endif
