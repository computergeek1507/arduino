/*
* 
* ESP32Pix.ino
* 
* Project: ESP32Pix  - 8 Port ESP32 Pixel Controller
* Copyright (c) 2019 Scott Hanson
* 
* Credit to Shelby Merrick - For the ESPixelStick and E131 Library
* http://www.forkineye.com
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/

//#define FASTLED_SHOW_CORE 0
#define FASTLED_ESP32_I2S true
//#include <ESPAsyncE131.h>
#include <E131.h>
#include <FastLED.h>
#include <ETH.h>

#include <WiFi.h>
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


E131 e131;

// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
//ESPAsyncE131 e131(UNIVERSE_COUNT);

CRGB leds[NUM_PORTS][1];

#include "utils.h"

AsyncWebServer web(HTTP_PORT); /* Web Server */

uint32_t lastPacket;

TaskHandle_t TaskA;

static bool eth_connected = false;

uint32_t    startTime;
//uint32_t    refreshTime;    // Time until we can refresh after starting a TX
//uint32_t    maxLenth;      // Max Lenth

//inline bool canRefresh() 
//{
//    return (micros() - startTime) >= refreshTime;
//}

void WiFiEvent(WiFiEvent_t event)
{
	switch (event) {
	case SYSTEM_EVENT_ETH_START:
		Serial.println("ETH Started");
		//set eth hostname here
		ETH.setHostname("esp32-ethernet");
		break;
	case SYSTEM_EVENT_ETH_CONNECTED:
		Serial.println("ETH Connected");
		break;
	case SYSTEM_EVENT_ETH_GOT_IP:
		Serial.print("ETH MAC: ");
		Serial.print(ETH.macAddress());
		Serial.print(", IPv4: ");
		Serial.print(ETH.localIP());
		if (ETH.fullDuplex()) {
		Serial.print(", FULL_DUPLEX");
		}
		Serial.print(", ");
		Serial.print(ETH.linkSpeed());
		Serial.println("Mbps");
		eth_connected = true;
		break;
	case SYSTEM_EVENT_ETH_DISCONNECTED:
		Serial.println("ETH Disconnected");
		eth_connected = false;
		break;
	case SYSTEM_EVENT_ETH_STOP:
		Serial.println("ETH Stopped");
		eth_connected = false;
		break;
	default:
		break;
	}
}

void setup()
{
	pinMode(LED_PIN, OUTPUT);
	digitalWrite(LED_PIN, LOW); //turn onboard LED off
	Serial.begin(115200);
	SPIFFS.begin();

	readConfig();

	delay(10);  

	wifiConnect();

	IPAddress ipaddr;
	IPAddress ipsub;
	IPAddress ipgate;
	if (!ipaddr.fromString(configData.ethIPAddress) || !ipsub.fromString(configData.ethIPSubmask) || !ipgate.fromString(configData.ethIPGateway))
	{
		// invalid addresss
		Serial.println("Invalid Ethernet Addresses");
		configData.ethIPAddress = "192.168.1.206";
		configData.ethIPSubmask = "255.255.255.0";
		configData.ethIPGateway = "192.168.1.1";
		ipaddr.fromString(configData.ethIPAddress);
		ipsub.fromString(configData.ethIPSubmask);
		ipgate.fromString(configData.ethIPGateway);
		saveConfig();
	}

	ETH.config(ipaddr,ipgate,ipsub);

	WiFi.onEvent(WiFiEvent);
	ETH.begin();

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

	e131.initUnicast();

#ifdef HARDWARE_V1
	FastLED.addLeds<NEOPIXEL, 5>(leds[0], configData.ports[0].pixCount);
	FastLED.addLeds<NEOPIXEL, 4>(leds[1], configData.ports[1].pixCount);
	FastLED.addLeds<NEOPIXEL, 2>(leds[2], configData.ports[2].pixCount);
	FastLED.addLeds<NEOPIXEL, 0>(leds[3], configData.ports[3].pixCount);

	FastLED.addLeds<NEOPIXEL, 13>(leds[4], configData.ports[4].pixCount);
	FastLED.addLeds<NEOPIXEL, 14>(leds[5], configData.ports[5].pixCount);
	FastLED.addLeds<NEOPIXEL, 15>(leds[6], configData.ports[6].pixCount);
	FastLED.addLeds<NEOPIXEL, 16>(leds[7], configData.ports[7].pixCount);
#else
	FastLED.addLeds<NEOPIXEL, 5>(leds[0], configData.ports[0].pixCount);
	FastLED.addLeds<NEOPIXEL, 39>(leds[1], configData.ports[1].pixCount);
	FastLED.addLeds<NEOPIXEL, 36>(leds[2], configData.ports[2].pixCount);
	FastLED.addLeds<NEOPIXEL, 35>(leds[3], configData.ports[3].pixCount);

	FastLED.addLeds<NEOPIXEL, 33>(leds[4], configData.ports[4].pixCount);
	FastLED.addLeds<NEOPIXEL, 13>(leds[5], configData.ports[5].pixCount);
	FastLED.addLeds<NEOPIXEL, 16>(leds[6], configData.ports[6].pixCount);
	FastLED.addLeds<NEOPIXEL, 32>(leds[7], configData.ports[7].pixCount);
#endif

	PowerUpDone();

 //xTaskCreatePinnedToCore(
//	Task1,                  /* pvTaskCode */
//	"Workload1",            /* pcName */
//	1000,                   /* usStackDepth */
//	NULL,                   /* pvParameters */
//	1,                      /* uxPriority */
//	&TaskA,                 /* pxCreatedTask */
//	0);
}

void PowerUpDone() 
{
	LEDS.showColor(CRGB(255, 0, 0)); //turn all pixels on red
	delay(1000);
	LEDS.showColor(CRGB(0, 255, 0)); //turn all pixels on green
	delay(1000);
	LEDS.showColor(CRGB(0, 0, 255)); //turn all pixels on blue
	delay(1000);
	FastLED.clear(); //clear pixels
	FastLED.show();
}

void Task1( void * parameter )
{
	//for (;;)
	//{
	//	LEDS.show();
	//	LEDS.delay(10);
		//delay(10);
	//}
}

void loop()
{
	if(configData.testMode == 0)
	{
		if(e131.parsePacket()) 
		{
			if (e131.universe >= configData.univStart && e131.universe < configData.univStart + configData.univCount)
			{
				packets++;
				digitalWrite(LED_PIN, HIGH); //turn onboard LED on
				Serial.print("Universe:");
				Serial.println(e131.universe);
				lastPacket = millis();
				//int x = e131.universe - universeStart;

				for (int i = 0; i < NUM_PORTS; i++)
				{
					if ((e131.universe >= configData.ports[i].startUni) && (e131.universe <= configData.ports[i].endUni)) 
					{
						uint8_t uniOffset = (e131.universe - configData.ports[i].startUni);

						//memcpy(leds[i],e131.data, universeSize);
						/* Offset the channel if required for the first universe */
						uint16_t offset = 0;
						if (e131.universe == configData.ports[i].startUni)
							offset = configData.ports[i].startChan - 1;

						/* Find start of data based off the Universe */
						uint16_t dataStart = uniOffset * configData.univSize;

						/* Caculate how much data we need for this buffer */
						uint16_t dataStop = configData.ports[i].chanCount;
						if ((dataStart + configData.univSize) < dataStop)
							dataStop = dataStart + configData.univSize;

						/* Set the data */
						uint16_t buffloc = 0;
						uint16_t length = dataStop - dataStart;
						
						//for (int i = dataStart; i < dataStop; i++) 
						//{
						//	pixels.setValue(i, e131.data[buffloc + offset]);

						//	buffloc++;
						//}
						Serial.print("port: ");
						Serial.print(i);
						Serial.print(" dataStart: ");
						Serial.print(dataStart);
						Serial.print(" dataStop: ");
						Serial.print(dataStop);
						Serial.print(" length: ");
						Serial.print(length);
						Serial.println("");
						memcpy(leds[i] + dataStart ,e131.data + offset, length);
					}
				//memcpy(leds[x],e131.data, universeSize);
				}
			}
		}
		
		if(millis() > startTime + REFRESH)
		{
			startTime = millis();
			//print_time(time_1);
			LEDS.show();
		}

		if ((millis() - lastPacket) > DATA_TIMEOUT)
		{
			lastPacket = millis();
			if(configData.testMode == 0)
			{
				FastLED.clear(); //clear pixels
			}
			digitalWrite(LED_PIN, LOW); //turn onboard LED off
		}
	}
	else
	{
		testMode();
	}
}

void wifiConnect()
{
	//reset networking
	WiFi.softAPdisconnect(true);
	WiFi.disconnect();          
	delay(1000);
	//check for stored credentials
	if(SPIFFS.exists(WIFI_FILE_NAME))
	{
		const char * _ssid = "", *_pass = "";
		File configFile = SPIFFS.open(WIFI_FILE_NAME, "r");
		if(configFile)
		{
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
	}
	else 
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

void rebootController(AsyncWebServerRequest *request)
{
	// Returns the redirect response. The page is reloaded and its contents
	// are updated to the state after deletion.
	//request->sendHeader("Location", String("http://") + web.client().localIP().toString() + String("/"));
	//request->send(302, "text/plain", "");
	//web.client().flush();
	//web.client().stop();
	//Serial.println("Restarting Controller");
	ESP.restart();
}

void setTestMode(AsyncWebServerRequest *request)
{
	Serial.println("setTestMode called");
	if (request->params())
	{
		for (uint8_t i = 0; i < request->params(); i++)
		{
			AsyncWebParameter *p = request->getParam(i);
			Serial.println(p->name());
			Serial.println(p->value());
			if (p->name() == "mode") configData.testMode = p->value().toInt();
		}
		saveConfig();

		request->send(200);
	}
	else
	{
		request->send(400);
	}
}

void configToJSON(String &json)
{
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

void recalcConfig()
{
	for(int i =0;i < NUM_PORTS;i++)
	{
		unsigned int numchan = configData.ports[i].pixCount * 3;
		unsigned int numunv = numchan / configData.univSize;
		unsigned int endChanel = numchan % configData.univSize;
		configData.ports[i].endUni = configData.ports[i].startUni + numunv;
		configData.ports[i].endChan = endChanel;
		configData.ports[i].chanCount = numchan;
	}
}

void saveConfig()
{
	recalcConfig();
	String json;
	configToJSON(json);
	File file = SPIFFS.open(CONFIG_FILE_NAME, "w");
	if (file)
	{
		file.println(json);
	}
}

void jsonToConfig(DynamicJsonDocument &doc)
{
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
void readConfig()
{
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

