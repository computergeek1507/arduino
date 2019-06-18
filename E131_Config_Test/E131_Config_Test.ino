

#include <WiFi.h>
#include <WebServer.h>
#include "SPIFFS.h"
#include <FS.h>
#include <ArduinoJson.h>

uint8_t pin_led = 2;

WebServer server(80);

char* mySsid = "ESP32Pix";
char* password = "password";

IPAddress local_ip(192, 168, 11, 4);
IPAddress gateway(192, 168, 11, 1);
IPAddress netmask(255, 255, 255, 0);

char webpage[] PROGMEM = R"=====(
<html>
<head>
<title>ESP32Pix</title>
</head>
<body onload="loadFunction()">
  <form id="MainForm">
  <div>
  <table align="center" border="1px" rules="none" cellpadding="5px">
      <tr>
        <td>Ethernet&nbsp;IP&nbsp;Address</td>      
        <td><input value="192.168.1.206" id="etherIPaddress" type="text" minlength="7" maxlength="15" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"></td>
      </tr>
      <tr>
        <td>Ethernet Subnet Mask </td> 
        <td><input value="255.255.255.0" id="etherIPsubnet" type="text" minlength="7" maxlength="15" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"></td>
      </tr>
      <tr>
        <td>Ethernet Gateway</td>
        <td><input value="192.168.1.1" id="etherIPgateway" type="text" minlength="7" maxlength="15" pattern="^((\d{1,2}|1\d\d|2[0-4]\d|25[0-5])\.){3}(\d{1,2}|1\d\d|2[0-4]\d|25[0-5])$"></td>
      </tr>
      <tr>
        <td colspan="2" align="center"><button class="primary" id="saveEthbtn" type="button" onclick="saveEthernet()">Save Ethernet Settings</button></td>
      </tr>
      <tr>
        <td>Start Universe</td> 
        <td><input value="1" id="startUniverse" type="number" min="1" max="64000"></td>
      </tr>
      <tr>
        <td>Number of Universes</td> 
        <td><input value="8" id="numberUniverse" type="number"  min="1" max="17"></td>
      </tr>
      <tr>
        <td>Universe Size</td> 
        <td><input value="510" id="sizeUniverse" type="number"  min="510" max="512"></td>
      </tr>
	    <tr>
        <td>Brightness</td> 
        <td><input value="100" id="brightness" type="number"  min="1" max="100"></td>
      </tr>
      <tr>
        <td colspan="2" align="center"><button class="primary" id="saveGlbbtn" type="button" onclick="saveGlobals()">Save Settings</button></td>
      </tr>
      <tr>
        <td colspan="2" align="center"><button class="primary" id="rebootbtn" type="button" onclick="reboot()">Reboot</button></td>
      </tr>
      </table>
      </div>
  </form>
</body>
<script>

function saveEthernet()
{
  console.log("saveEthernet button was clicked!");
  var etherIPaddress = document.getElementById("etherIPaddress").value;
  var etherIPsubnet = document.getElementById("etherIPsubnet").value;
  var etherIPgateway = document.getElementById("etherIPgateway").value;
  var data = {etherIPaddress:etherIPaddress, etherIPsubnet:etherIPsubnet, etherIPgateway:etherIPgateway};
  var xhr = new XMLHttpRequest();
  var url = "/ethernet_settings";
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // Typical action to be performed when the document is ready:
      if(xhr.responseText != null){
        console.log(xhr.responseText);
      }
    }
  };
  xhr.open("POST", url, true);
  xhr.send(JSON.stringify(data));
};

function saveGlobals()
{
  console.log("saveGlobals button was clicked!");
  var startUniverse = document.getElementById("startUniverse").value;
  var numberUniverse = document.getElementById("numberUniverse").value;
  var sizeUniverse = document.getElementById("sizeUniverse").value;
  var brightness = document.getElementById("brightness").value;
  var data = {startUniverse:startUniverse, numberUniverse:numberUniverse, sizeUniverse:sizeUniverse, brightness:brightness};
  var xhr = new XMLHttpRequest();
  var url = "/global_settings";
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      // Typical action to be performed when the document is ready:
      if(xhr.responseText != null){
        console.log(xhr.responseText);
      }
    }
  };
  xhr.open("POST", url, true);
  xhr.send(JSON.stringify(data));
};

function reboot()
{
  console.log("reboot button was clicked!");
  
  var xhr = new XMLHttpRequest();
  var url = "/reboot";
  xhr.open("GET", url, true);
  xhr.send();
};

function readSetting(){
  console.log("readEthernet was clicked!");
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
      var txt = this.responseText;
      var obj = JSON.parse(txt);
        document.getElementById("etherIPaddress").value = obj.etherIPaddress;
        document.getElementById("etherIPsubnet").value = obj.etherIPsubnet;
        document.getElementById("etherIPgateway").value = obj.etherIPgateway;
        document.getElementById("startUniverse").value = obj.startUniverse;
        document.getElementById("numberUniverse").value = obj.numberUniverse;
        document.getElementById("sizeUniverse").value = obj.sizeUniverse;
        document.getElementById("brightness").value = obj.brightness;
      }  
    };
   xhttp.open("GET", "all_settings", true);
   xhttp.send();
};

function loadFunction(){
  readSetting();
};
</script>
</html>
)=====";


void setup()
{
  pinMode(pin_led, OUTPUT);
  Serial.begin(115200);
  SPIFFS.begin();

  wifiConnect();

  server.on("/",[](){server.send_P(200,"text/html", webpage);});
  server.on("/ethernet_settings", HTTP_POST, handleEthernetSettingsUpdate);
  server.on("/ethernet_settings", HTTP_GET, handleEthernetSettingsGet);
  
  server.on("/global_settings", HTTP_POST, handleGlobalSettingsUpdate);
  server.on("/global_settings", HTTP_GET, handleGlobalSettingsGet);
  server.on("/all_settings", HTTP_GET, handleAllSettingsGet);

  server.on("/reboot", HTTP_GET, rebootController);
  
  server.begin();
}

void loop()
{
  server.handleClient();
}

void handleEthernetSettingsUpdate()
{
  String data = server.arg("plain");
  DynamicJsonDocument doc(1024);
  
  deserializeJson(doc, data);

  File configFile = SPIFFS.open("/ethernet_config.json", "w");
  serializeJson(doc, configFile);
  configFile.close();
  
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
}

void handleEthernetSettingsGet()
{
  File configFile = SPIFFS.open("/ethernet_config.json", "r");

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
        server.send(200, "application/json", output);
      }
    }
    configFile.close();
    return;
  }
    server.send(500, "application/json", "{\"status\" : \"Error\"}");
}

void handleGlobalSettingsUpdate()
{
  String data = server.arg("plain");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, data);

  File configFile = SPIFFS.open("/global_config.json", "w");

  serializeJson(doc, configFile);
  configFile.close();
  
  server.send(200, "application/json", "{\"status\" : \"ok\"}");
}

void handleGlobalSettingsGet()
{
  File configFile = SPIFFS.open("/global_config.json", "r");

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
        server.send(200, "application/json", output);
      }
    }
    configFile.close();
    return;
  }
    server.send(500, "application/json", "{\"status\" : \"Error\"}");
}

void handleAllSettingsGet()
{
  DynamicJsonDocument alldoc(4096);
  //JsonObject root = alldoc.to<JsonObject>(); 
  File etherFile = SPIFFS.open("/ethernet_config.json", "r");
  File configFile = SPIFFS.open("/global_config.json", "r");

  if (!etherFile)
  { 
    Serial.println("No Ethernet File Exist");
  } 
  else 
  {
    size_t size = etherFile.size();
  if ( size == 0 )
  {
    Serial.println("Ethernet File empty !");
  } 
  else 
  {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, etherFile);
    etherFile.close();
    if (error) 
    {
    Serial.println("Ethernet Invalid file");
    } 
    else 
    {
            Serial.println("ethernet loaded");
            combine(alldoc,doc);
      //JsonObject ethernet = root.createNestedObject("ethernet_config");
      //ethernet.set(doc.to<JsonObject>()); 
      //root["ethernet_config"] =doc.to<JsonObject>();
  
    }
  }
    etherFile.close();
  }
  if (!configFile)
  { 
    Serial.println("No Global File Exist");
  } 
  else 
  {
    size_t size = configFile.size();
  if ( size == 0 )
  {
    Serial.println("Global File empty !");
  } 
  else 
  {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();
    if (error) 
    {
    Serial.println("Global Invalid file");
    } 
    else 
    {
      Serial.println("Global loaded");
      combine(alldoc,doc);
      //JsonObject global = root.createNestedObject("global_config");
    //root["global_config"] = doc.to<JsonObject>();
      //global.set(doc.to<JsonObject>()); 
  
    }
  }
    configFile.close();
  }
  String output;
  serializeJson(alldoc, output);       
  server.send(200, "application/json", output);
}

void combine(JsonDocument& dst, const JsonDocument& src) {
    for (auto p : src.as<JsonObject>())
        dst[p.key()] = p.value();
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
          digitalWrite(pin_led,!digitalRead(pin_led));
          if ((unsigned long)(millis() - startTime) >= 5000) break;
        }
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(pin_led,HIGH);
  } else 
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(local_ip, gateway, netmask);
    WiFi.softAP(mySsid, password); 
    digitalWrite(pin_led,LOW);      
  }
  Serial.println("");
  WiFi.printDiag(Serial);
}

void rebootController() {
  // Returns the redirect response. The page is reloaded and its contents
  // are updated to the state after deletion.
  server.sendHeader("Location", String("http://") + server.client().localIP().toString() + String("/"));
  server.send(302, "text/plain", "");
  server.client().flush();
  server.client().stop();
  Serial.println("Restarting Controller");
  ESP.restart();
}
