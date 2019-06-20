#include <WiFi.h>
//#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <Update.h>

#include "ESP32Pix.h"
#include "page_index.h"
#include "page_input.h"
#include "page_pixel.h"
#include "page_wifi.h"

AsyncWebServer web(HTTP_PORT);     /* Web Server */

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  SPIFFS.begin();
  
  readConfig();

  wifiConnect();

  //server.on("/",[](){server.send_P(200,"text/html", webpage);});
  //server.on("/ethernet_settings", HTTP_POST, handleEthernetSettingsUpdate);
  //server.on("/ethernet_settings", HTTP_GET, handleEthernetSettingsGet);
  
  //server.on("/global_settings", HTTP_POST, handleGlobalSettingsUpdate);
  //server.on("/global_settings", HTTP_GET, handleGlobalSettingsGet);
  //server.on("/all_settings", HTTP_GET, handleAllSettingsGet);

  web.on("/indexvals", HTTP_GET, get_index_vals);
  web.on("/inputvals", HTTP_GET, get_input_vals);
  web.on("/pixelvals", HTTP_GET, get_pixel_vals);
  web.on("/wifivals", HTTP_GET, get_wifi_vals);

  //web.on("/index.html", HTTP_POST, send_index_html);
  web.on("/inputs.html", HTTP_POST, send_input_vals);
  web.on("/pixels.html", HTTP_POST, send_pixel_vals);
  web.on("/wifi.html", HTTP_POST, send_wifi_vals);
  //web.on("/sdcard.html", HTTP_POST, send_sdcard_html);
  
  web.on("/settest", HTTP_POST, setTestMode);

  web.serveStatic("/configfile", SPIFFS, CONFIG_FILE_NAME);

   web.on("/reboot", HTTP_GET, rebootController);
   web.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    web.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Page not found");
    });
	
  
  web.begin();
}

void loop()
{
  //server.handleClient();
}

void wifiConnect()
{
  //reset networking
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();          
  delay(1000);
  //check for stored credentials
  if(SPIFFS.exists(WIFI_FILE_NAME)){
    const char * _ssid = "", *_pass = "";
    File configFile = SPIFFS.open(WIFI_FILE_NAME, "r");
    if(configFile){
       DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();

      if(error)
      {
        _ssid = doc["ssid"];
        _pass = doc["password"];
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED) 
        {
          delay(500);
          Serial.print(".");
          digitalWrite(LED_PIN,!digitalRead(LED_PIN));
          if ((unsigned long)(millis() - startTime) >= 5000) break;
        }
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(LED_PIN,HIGH);
  } else 
  {
    WiFi.mode(WIFI_AP);
    //WiFi.softAPConfig(local_ip, gateway, netmask);
    WiFi.softAP(configData.apName.c_str(), configData.apPass.c_str());
	Serial.println(WiFi.localIP()); 
    digitalWrite(LED_PIN,LOW);
  }
  Serial.println("");
  WiFi.printDiag(Serial);
}

void rebootController(AsyncWebServerRequest *request) {
  // Returns the redirect response. The page is reloaded and its contents
  // are updated to the state after deletion.
  //web.sendHeader("Location", String("http://") + web.client().localIP().toString() + String("/"));
  //web.send(302, "text/plain", "");
  //web.client().flush();
  //web.client().stop();
  //Serial.println("Restarting Controller");
  ESP.restart();
}

void setTestMode(AsyncWebServerRequest *request) {
	Serial.println("setTestMode called");
if (request->params()) {
        for (uint8_t i = 0; i < request->params(); i++) {
            AsyncWebParameter *p = request->getParam(i);
			Serial.println(p->name());
			Serial.println(p->value());
            if (p->name() == "mode") configData.testMode = p->value().toInt();
        }
        saveConfig();

        request->send(200);
    } else {
        request->send(400);
    }
}

void configToJSON(String &json) {
    //doc
    DynamicJsonDocument doc(CONFIG_MAX_SIZE);
    //ethernet
    JsonObject ethernet = doc.createNestedObject("ethernet");
    ethernet["ipaddress"] = configData.ethIPAddress;
	ethernet["ipsubmask"] = configData.ethIPSubmask;
	ethernet["ipgateway"] = configData.ethIPGateway;
	//wifi
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["apname"] = configData.apName;
	wifi["appassword"] = configData.apPass;
	wifi["hostname"] = configData.hostName;

	//input
    JsonObject input = doc.createNestedObject("input");
    input["universestart"] = configData.univStart;
	input["universecount"] = configData.univCount;
	input["universesize"] = configData.univSize;
	input["serialuniverse"] = configData.serialUni;
	input["inputMode"] = configData.inputMode;
	input["testMode"] = configData.testMode;
	
	for(int i =0;i < NUM_PORTS;i++)
	{
		JsonObject port = doc.createNestedObject("port" + (String)(i+1));
		port["pixelcount"] = configData.ports[i].pixCount;
		port["startuniverse"] = configData.ports[i].startUni;
		port["startchannel"] = configData.ports[i].startChan;
		//port["enduniverse"] = configData.ports[i].endUni;
		//port["endchannel"] = configData.ports[i].endChan;
		port["brightness"] = configData.ports[i].brightness;
	}

    serializeJson(doc,json);
}

void recalcConfig() {
	for(int i =0;i < NUM_PORTS;i++)
	{
		unsigned int numchan = configData.ports[i].pixCount * 3;
		unsigned int numunv = numchan / configData.univSize;
		unsigned int endChanel = numchan % configData.univSize;
		configData.ports[i].endUni = configData.ports[i].startUni + numunv;
		configData.ports[i].endChan = endChanel;
	}
}

void saveConfig() {
	recalcConfig();
	String json;
	configToJSON(json);
	File file = SPIFFS.open(CONFIG_FILE_NAME, "w");
    if (file) {
        file.println(json);
    }
}

void jsonToConfig(DynamicJsonDocument &doc) {
    //ethernet
    JsonObject ethernet = doc["ethernet"];
    configData.ethIPAddress = ethernet["ipaddress"].as<String>();
	configData.ethIPSubmask = ethernet["ipsubmask"].as<String>();
	configData.ethIPGateway = ethernet["ipgateway"].as<String>();
	//wifi
    JsonObject wifi = doc["wifi"];
    configData.apName = wifi["apname"].as<String>();;
	configData.apPass = wifi["appassword"].as<String>();;
	configData.hostName = wifi["hostname"].as<String>();

	//input
    JsonObject input = doc["input"];
    configData.univStart = input["universestart"];
	configData.univCount = input["universecount"];
	configData.univSize = input["universesize"];
	configData.serialUni = input["serialuniverse"];
	configData.inputMode = input["inputMode"];
	configData.testMode = input["testMode"];
	
	for(int i =0;i < NUM_PORTS;i++)
	{
		JsonObject port = doc["port" + (String)(i+1)];
		configData.ports[i].pixCount=port["pixelcount"];
		configData.ports[i].startUni=port["startuniverse"];
		configData.ports[i].startChan=port["startchannel"];
		//configData.ports[i].endUni=port["enduniverse"];
		//configData.ports[i].endChan=port["endchannel"];
		configData.ports[i].brightness=port["brightness"];
	}
	
	recalcConfig();
}
void readConfig() {
Serial.println("LoadConfigSettings Called !");
  File configFile = SPIFFS.open(CONFIG_FILE_NAME, "r");
  if (!configFile)
  {
    Serial.println("No File Exist");
  } 
  else 
  {
      size_t size = configFile.size();
      if ( size == 0 )
      {
        Serial.println("File empty !");
      } 
      else 
      {
		  	DynamicJsonDocument doc(CONFIG_MAX_SIZE);
			DeserializationError error = deserializeJson(doc,configFile);
			configFile.close();
       
        if (error) 
        {
          Serial.println("Invalid file");
        } 
        else 
        {
			Serial.println("Reading Json..");
			jsonToConfig(doc);
		}
	  }
    }
}