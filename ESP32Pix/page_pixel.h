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
}
for(int j = 0; j<NUM_PORTS; j++)
{
	const String name =  "port"+(String)(j+1);
	values += name +"pix|input|" + (String)configData.ports[j].pixCount + "\n";
	for(int i = 0; i<configData.univCount; i++)
	{
		int unv = configData.univStart + i;
		values += name +"uni|opt|" + (String)unv + "|" + (String)i + "\n";
	}
	values += name + "uni|input|" + (String)(configData.ports[j].startUni - configData.univStart) + "\n";
	values += name + "chan|input|" + (String)configData.ports[j].startChan + "\n";

	values += name +"endchan|div|#" + (String)configData.ports[j].endUni + ":" + (String)configData.ports[j].endChan + "\n";
	values += name +"bright|input|" + (String)configData.ports[j].brightness + "\n";

}
    values += "testmode|input|" + (String)configData.testMode + "\n";
    request->send(200, "text/plain", values);
}

#endif
