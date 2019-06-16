/**
 * ESP32Pix
 * 
 * Control Pixels Stuff
 * 
  Copyright (c) 2019 Scott Hanson.
  This software is released under the MIT License.
  https://opensource.org/licenses/MIT
*/

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

// Adjusting WebServer class with between ESP8266 and ESP32.
typedef WebServer WiFiWebServer;

AutoConnect  portal;
AutoConnectConfig config;
WiFiClient   wifiClient;

String  ethIPAddress;
String  ethIPSubmask;
unsigned int  universeStart = 1;
unsigned int  universeCount = 8;
unsigned int  universeSize = 510;
unsigned int  brightness = 100;

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
  portal.aux("/config_setting")->saveElement(param, { "ethipaddress", "ethipsubmask", "universestart", "universecount", "universesize", "brightness" });
  param.close();

  // Echo back saved parameters to AutoConnectAux page.
  AutoConnectText&  echo = aux["parameters"].as<AutoConnectText>();
  echo.value = "Ethernet IP Address: " + ethIPAddress + "<br>";
  echo.value += "Ethernet IP Submask: " + ethIPSubmask + "<br>";
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

  //portal.on("/setting", JSONSettings);
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
    //portal.on("/", handleRoot2);
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
}

void loop() {
  portal.handleClient();
}
