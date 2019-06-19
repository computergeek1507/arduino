

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

AsyncWebServer web(HTTP_PORT);     /* Web Server */

//char* mySsid = "ESP32Pix";
char* password = "password";

IPAddress local_ip(192, 168, 11, 4);
IPAddress gateway(192, 168, 11, 1);
IPAddress netmask(255, 255, 255, 0);



void setup()
{
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  SPIFFS.begin();

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

  //web.on("/index.html", HTTP_POST, send_index_html);
  //web.on("/inputs.html", HTTP_POST, send_inputs_html);
  //web.on("/pixels.html", HTTP_POST, send_pixels_html);
  //web.on("/sdcard.html", HTTP_POST, send_sdcard_html);

  //web.serveStatic("/configfile", SPIFFS, "/config.json");

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
  if(SPIFFS.exists("/wifi.json")){
    const char * _ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/wifi.json", "r");
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
    WiFi.softAP(configData.apName.c_str(), password);
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
