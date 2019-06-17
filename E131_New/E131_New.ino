/**
 * ESP32Pix
 * 
 * Control Pixels Stuff
 * 
  Copyright (c) 2019 Scott Hanson.
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

//#define FASTLED_SHOW_CORE 0
#define FASTLED_ESP32_I2S true
//#include <ESPAsyncE131.h>
#include <E131.h>
#include <FastLED.h>
#include <ETH.h>

#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#define GET_CHIPID()  ((uint16_t)(ESP.getEfuseMac()>>32))

#include <FS.h>
#include <AutoConnect.h>

#define PARAM_FILE      "/param.json"
#define AUX_CONFIGSETTING "/config_setting"
#define AUX_CONFIGSAVE    "/config_save"
#define AUX_CONFIGCLEAR   "/config_clear"
#define REBOOT_LINK   "/reboot"

TaskHandle_t TaskA, TaskB;

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 170

typedef WebServer WiFiWebServer;

E131                e131;

CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

AutoConnect  portal;
AutoConnectConfig config;
WiFiClient   wifiClient;

String  ethIPAddress;
String  ethIPSubmask;
String  ethIPGateway;
unsigned int  universeStart = 1;
unsigned int  universeCount = 8;
unsigned int  universeSize = 510;
unsigned int  brightness = 100;

static bool eth_connected = false;

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

void LoadConfigSettings() {
  Serial.println("LoadConfigSettings Called !");
  File configFile = SPIFFS.open(PARAM_FILE, "r");
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
        DynamicJsonDocument doc(AUTOCONNECT_JSONPSRAM_SIZE);
        DeserializationError error = deserializeJson(doc, configFile);
        configFile.close();
       
        if (error) 
        {
          Serial.println("Invalid file");
        } 
        else 
        {
          Serial.println("Reading Json..");
          JsonArray array = doc.as<JsonArray>();
          for(JsonVariant v : array) {
            JsonObject val = v.as<JsonObject>();
            const String name = val["name"];
            const String value = val["value"];
            if(name == "ethipaddress")
            {
              ethIPAddress = value;
            }
            else if(name == "ethipsubmask")
            {
              ethIPSubmask = value;
            }
            else if(name == "ethipgateway")
            {
              ethIPGateway = value;
            }
            else if(name == "universestart")
            {
              universeStart = value.toInt();
            }
            else if(name == "universecount")
            {
              universeCount = value.toInt();
            }
            else if(name == "universesize")
            {
              universeSize = value.toInt();
            }
            else if(name == "brightness")
            {
              brightness = value.toInt();
            }              
          }
        }
      }
      configFile.close();
    }
}

String loadParams(AutoConnectAux& aux, PageArgument& args) {
  (void)(args);
  File param = SPIFFS.open(PARAM_FILE, "r");
  if (param) {
    aux.loadElement(param);
    param.close();
  }
  else
    Serial.println(PARAM_FILE " open failed");
  return String("");
}

String saveParams(AutoConnectAux& aux, PageArgument& args) {
  ethIPAddress = args.arg("ethipaddress");
  ethIPAddress.trim();

  ethIPSubmask = args.arg("ethipsubmask");
  ethIPSubmask.trim();

  ethIPGateway = args.arg("ethipgateway");
  ethIPGateway.trim();
  
  String sUniverseStart = args.arg("universestart");
  universeStart = sUniverseStart.toInt();

  String sUniverseCount = args.arg("universecount");
  universeCount = sUniverseCount.toInt();

  String sUniverseSize = args.arg("universesize");
  universeSize = sUniverseSize.toInt();

  String sBrightness = args.arg("brightness");
  brightness = sBrightness.toInt();
 
  // The entered value is owned by AutoConnectAux of /config_setting.
  // To retrieve the elements of /mqtt_setting, it is necessary to get
  // the AutoConnectAux object of /mqtt_setting.
  File param = SPIFFS.open(PARAM_FILE, "w");
  portal.aux("/config_setting")->saveElement(param, { "ethipaddress", "ethipsubmask", "ethipgateway", "universestart", "universecount", "universesize", "brightness" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Ethernet IP Address: " + ethIPAddress + "<br>";
  echo.value += "Ethernet IP Submask: " + ethIPSubmask + "<br>";
  echo.value += "Ethernet IP Gateway: " + ethIPGateway + "<br>";
  echo.value += "Universe Start: " + String(universeStart) + "<br>";
  echo.value += "Universe Count: " + String(universeCount) + "<br>";
  echo.value += "Universe Size: " + String(universeSize) + "<br>";
  echo.value += "Brightness: " + String(brightness) + "<br>";

  return String("");
}

void handleRoot() {
    String  content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "</head>"
    "<body>"
    "Ethernet IP Address: " + ethIPAddress + "<br>"
    "Ethernet IP Submask: " + ethIPSubmask + "<br>"
    "Ethernet IP Gateway: " + ethIPGateway + "<br>"
    "Universe Start: " + String(universeStart) + "<br>"
    "Universe Count: " + String(universeCount) + "<br>"
    "Universe Size: " + String(universeSize) + "<br>"
    "Brightness: " + String(brightness) + "<br>"    
    "<p style=\"padding-top:10px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"
    "</body>"
    "</html>";

  WiFiWebServer&  webServer = portal.host();
  webServer.send(200, "text/html", content);
}

void DisplayJsonSettings() {
  Serial.println("DisplayJsonSettings Called !");
    File configFile = SPIFFS.open(PARAM_FILE, "r");
 WiFiWebServer&  webServer = portal.host();
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
      DynamicJsonDocument doc(AUTOCONNECT_JSONPSRAM_SIZE);
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();
     
      if (error) 
      {
        Serial.println("Invalid file");
      } 
      else 
      {
        Serial.println("Json loaded");
        String output;
        serializeJson(doc, output);        
        webServer.send(200, "application/json", output);
      }
    }
    configFile.close();
    //return;
  }
    webServer.send(500, "application/json", "{\"status\" : \"Error\"}");
}

// Clear
void handleClearChannel() {
  // Returns the redirect response. The page is reloaded and its contents
  // are updated to the state after deletion.
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
}

void rebootController() {
  // Returns the redirect response. The page is reloaded and its contents
  // are updated to the state after deletion.
  WiFiWebServer&  webServer = portal.host();
  webServer.sendHeader("Location", String("http://") + webServer.client().localIP().toString() + String("/"));
  webServer.send(302, "text/plain", "");
  webServer.client().flush();
  webServer.client().stop();
  Serial.println("Restarting Controller");
  ESP.restart();
}

// Load AutoConnectAux JSON from SPIFFS.
bool loadAux(const String auxName) {
  bool  rc = false;
  String  fn = auxName + ".json";
  File fs = SPIFFS.open(fn.c_str(), "r");
  if (fs) {
    rc = portal.load(fs);
    fs.close();
  }
  else
    Serial.println("SPIFFS open failed: " + fn);
  return rc;
}

String JSONSettings(AutoConnectAux& aux, PageArgument& args) {
 File configFile = SPIFFS.open(PARAM_FILE, "r");
 WiFiWebServer&  webServer = portal.host();

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
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, configFile);
      configFile.close();

      if (error) 
      {
        Serial.println("Invalid file");
      } 
      else 
      {
        Serial.println("Json loaded");
        String output;

         serializeJson(doc, output);
          return output;
      }
    }
    configFile.close();
  }
    return String("");
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  SPIFFS.begin();

  loadAux(AUX_CONFIGSETTING);
  loadAux(AUX_CONFIGSAVE);

  LoadConfigSettings();

  AutoConnectAux* setting = portal.aux(AUX_CONFIGSETTING);
  if (setting) {
    PageArgument  args;
    AutoConnectAux& CONFIG_setting = *setting;
    loadParams(CONFIG_setting, args);
    config.apid = "ESP32Pix";
    config.psk = "12345678";
    //config.retainPortal = true;
    config.hostName = String("ESP32Pix") + "-" + String(GET_CHIPID(), HEX);
    config.bootUri = AC_ONBOOTURI_HOME;
    config.retainPortal = true;
    config.contuineOnDisconnect = true;
    config.immediateStart = true;
    config.homeUri = "/";
    
    portal.config(config);

    portal.on(AUX_CONFIGSETTING, loadParams);
    portal.on(AUX_CONFIGSAVE, saveParams);
  }
  else
    Serial.println("aux. load error");

  Serial.print("WiFi ");
  if (portal.begin()) {
    Serial.println("connected:" + WiFi.SSID());
    Serial.println("IP:" + WiFi.localIP().toString());
  } else {
    Serial.println("connection failed:" + String(WiFi.status()));
  }

  WiFiWebServer&  webServer = portal.host();
  webServer.on("/", handleRoot);
  webServer.on("/setting", DisplayJsonSettings);
  webServer.on(AUX_CONFIGCLEAR, handleClearChannel);
  webServer.on(AUX_CONFIGCLEAR, handleClearChannel);
  webServer.on(REBOOT_LINK, rebootController);


  IPAddress ipaddr;
  if (ipaddr.fromString(ethIPAddress)) {
    // it was a valid address, do something with it 
  }

  IPAddress ipsub;
  if (ipsub.fromString(ethIPSubmask)) {
    // it was a valid address, do something with it 
  }

  IPAddress ipgate;
  if (ipgate.fromString(ethIPGateway)) {
    // it was a valid address, do something with it 
  }

  ETH.config(ipaddr,ipgate,ipsub);
  WiFi.onEvent(WiFiEvent);
  ETH.begin();
  e131.initUnicast();

  FastLED.addLeds<NEOPIXEL, 5>(leds[0], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 4>(leds[1], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 2>(leds[2], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 0>(leds[3], NUM_LEDS_PER_STRIP);
  
  FastLED.addLeds<NEOPIXEL, 13>(leds[4], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 14>(leds[5], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 15>(leds[6], NUM_LEDS_PER_STRIP);
  FastLED.addLeds<NEOPIXEL, 16>(leds[7], NUM_LEDS_PER_STRIP);

  FastLED.setBrightness(brightness );
  PowerUpDone();

 xTaskCreatePinnedToCore(
   Task1,                  /* pvTaskCode */
   "Workload1",            /* pcName */
   1000,                   /* usStackDepth */
   NULL,                   /* pvParameters */
   1,                      /* uxPriority */
   &TaskA,                 /* pxCreatedTask */
   0);  
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
 for (;;) {
   LEDS.show();
   //LEDS.delay(10);
   delay(10) ;
 }
}

void loop() {
  if(e131.parsePacket()) {
  Serial.print("Universe:");
  Serial.println(e131.universe);
        if (e131.universe >= universeStart && e131.universe < universeStart + universeCount) {
          int x = e131.universe - universeStart;
          
            //for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
             //   int j = i * 3;
            //    leds[x][i]=CRGB(e131.data[j], e131.data[j+1], e131.data[j+2]);
            //}
            memcpy(leds[x],e131.data, universeSize);
        }
    }
  portal.handleClient();
}
