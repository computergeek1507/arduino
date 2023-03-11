#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

#include "arduino_secrets.h"

#define MQTT_CLIENTID "weatherArduino2"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_WILLTOPIC  "clients/weatherArduino2/"
#define MQTT_WILLMESSAGE  "dead"
#define TRUE    (1)

#define INTERVAL 30                                                 // Intervall of sending in seconds

// Update these with values suitable for your network.
//byte server[] = { 192, 168, 5, 148 };
IPAddress server(192,168,5,148);

//info config
const String software_version = "\"ESP8266_Wind_Vane_v1\"";
//weather sensors
const byte windSpeedPin = D4;
const byte windDirPin = A0;
const byte rainPin = D3;
/////////////////////////////////////////////////////////////////////
WiFiClient wifiClient;
PubSubClient client(wifiClient);
int status = WL_IDLE_STATUS;
unsigned int windcnt = 0;
unsigned int raincnt = 0;
unsigned long lastSend;

void ICACHE_RAM_ATTR cntWindSpeed();
void ICACHE_RAM_ATTR cntRain();

//////////////// SETUP //////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  // pin for Wind speed
  pinMode(windSpeedPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(windSpeedPin), cntWindSpeed, RISING);
  pinMode(rainPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(rainPin), cntRain, RISING);
  pinMode(windDirPin, INPUT);
  
  delay(10);
  InitWiFi();
  client.setServer( server, 1883 );

  // send device attributes
  if ( !client.connected() ) {
    reconnect();
  }  

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"Firmware\":"; payload += software_version; payload += ",";
  payload += "\"Sensors\":"; payload += "Wind Vane v2";
  payload += "}";
  
  // Send the payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  Serial.println(payload);
  client.publish( "arduino/weather2/info", attributes );
  Serial.println("Send device attributes.");
  
  lastSend = millis() - INTERVAL*1000;
}

//////////////// LOOP //////////////////////////////////////////////
void loop() {
  // check connection
  if ( !client.connected() ) {
    reconnect();
  }

  if ( millis() - lastSend > INTERVAL*1000 ) { // Update and send only after delay
    getAndSendTemperatureAndHumidityData();    
  }

  client.loop();        //keep connected, look for messages from server
  ArduinoOTA.handle();
}

//////////////// Functions //////////////////////////////////////////

void getAndSendTemperatureAndHumidityData()
{
  Serial.println("Collecting Weather data.");

  //Calculate Wind Speed (clicks/interval * 2,4 kmh)
  float ws = (windcnt/INTERVAL) * 2.4;
  windcnt = 0;
  //Calculate Rain
  float r = (raincnt/2)*0.2794;
  raincnt = 0;
  // get wind direction
  float dirpin = analogRead(windDirPin)*(3.3 / 1023.0);
  String wd = "other";
  
  if(dirpin > 2.60 &&  dirpin < 2.70 ){
    wd = "N";
  }
  if(dirpin > 1.60 &&  dirpin < 1.70 ){
    wd = "NE";
  }
  if(dirpin > 0.30 &&  dirpin < 0.40 ){
    wd = "E";
  }
  if(dirpin > 0.60 &&  dirpin < 0.70 ){
    wd = "SE";
  }
  if(dirpin > 0.96 &&  dirpin < 1.06 ){
    wd = "S";
  }
  if(dirpin > 2.10 &&  dirpin < 2.20 ){
    wd = "SW";
  }
  if(dirpin > 3.10 &&  dirpin < 3.25 ){
    wd = "W";
  }
  if(dirpin > 2.95 &&  dirpin < 3.05 ){
    wd = "NW";
  }    

  Serial.print("Windspeed: ");
  Serial.print(ws);
  Serial.print(" km/h ");
  Serial.print("Wind Direction: ");
  Serial.print(wd);
  Serial.print(" ");
  Serial.print("Rain: ");
  Serial.print(r);
  Serial.print(" mm ");
  

  String windspeed = String(ws);
  String winddir = String(wd);
  String winddirADC = String(dirpin);
  String rain = String(r);

  // Just debug messages
  Serial.print( "Sending Data : [" );
  Serial.print( windspeed ); Serial.print( "," );
  Serial.print( winddir ); Serial.print( "," );
  Serial.print( rain );
  Serial.print( "]   -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"windspeed\":"; payload += windspeed; payload += ",";
  payload += "\"winddirection\":"; payload += winddir; payload += ",";
  payload += "\"winddirectionadc\":"; payload += winddirADC; payload += ",";
  payload += "\"rain\":"; payload += rain;
  payload += "}";

  // Send payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  client.publish( "arduino/weather2/status", attributes );
  Serial.println( attributes );

  lastSend = millis();
}

void InitWiFi()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  WiFi.mode(WIFI_STA);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(SECRET_SSID, SECRET_PASS);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
    Serial.print("Connecting to MQTT Server ...");
    // Attempt to connect (clientId, username, password)
    if ( client.connect(MQTT_CLIENTID, NULL, NULL) ) {
      Serial.println( "[DONE]" );
    } else {
      Serial.print( "[FAILED] [ rc = " );
      Serial.print( client.state() );
      Serial.println( " : retrying in 5 seconds]" );
      // Wait 5 seconds before retrying
      delay( 5000 );
    }
  }
}

// Interupts
void ICACHE_RAM_ATTR cntWindSpeed() {
  windcnt++;
}

void ICACHE_RAM_ATTR cntRain() {
  raincnt++;
}
