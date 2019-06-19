#ifndef PAGE_INDEX_H_
#define PAGE_INDEX_H_

#include "ESP32Pix.h"

void get_index_vals(AsyncWebServerRequest *request) {
    String values = "";
    values += "name_ver|div|" + (String)SOFT_NAME + " " + (String)SOFT_VER + "\n";
    values += "eth_ip|div|" + configData.ethIPAddress + "\n";
	values += "uni_first|div|" + (String)configData.univStart + "\n";
	values += "uni_last|div|" + (String)(configData.univStart + configData.univCount -1) + "\n";
	values += "num_packets|div|" + (String)packets + "\n";
    request->send(200, "text/plain", values);
}

#endif
