#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "arduino_secrets.h"

#define RG15_BAUDRATE      9650
#define RG15_READ_TIMEOUT  500
#define RG15_EVENT_TIMEOUT 60
#define RG15_BUFFER_SIZE   150

#if defined(ESP8266)
#define CACHED_FUNCTION_ATTR ICACHE_RAM_ATTR
#elif defined(ESP32)
#define CACHED_FUNCTION_ATTR IRAM_ATTR
#else
#define CACHED_FUNCTION_ATTR
#endif

#if ARDUINO_ARCH_ESP8266
#define WIFI
#include <SoftwareSerial.h>
//info config
const String software_version = "\"ESP8266_Wind_Vane_v1\"";
//Weather Sensor Pins ESP8266 V3 PCB 
const byte windSpeedPin = D4;
const byte windDirPin = A0;
//const byte rainPin = D3;
const byte rxPin = D6;
const byte txPin = D5;
const byte ledPin = LED_BUILTIN;
SoftwareSerial RainSerial(rxPin, txPin); // RX | TX
#elif ARDUINO_ARCH_RP2040 
//info config
const String software_version = "\"RP2040_Wind_Vane_v1\"";
#if ARDUINO_WIZNET_5500_EVB_PICO
#define ETHERNET
//#define BME280
//#define DALLASTEMP
//Weather Sensor Pins RP PICO V2 PCB 
const byte windSpeedPin = 1;
const byte windDirPin = 26;
//const byte rainPin = 13;
const byte rxPin = 11;
const byte txPin = 12;
const byte ledPin = 22;
#define ONE_WIRE_BUS 0
#elif ARDUINO_RASPBERRY_PI_PICO_W 
#define WIFI
//Weather Sensor Pins RP PICO V1 PCB 
const byte windSpeedPin = 27;
const byte windDirPin = 26;
//const byte rainPin = 20;
const byte rxPin = 16;
const byte txPin = 17;
const byte ledPin = 2;
#endif
#include <SoftwareSerial.h>
SerialPIO RainSerial(rxPin, txPin,128); // RX | TX
//SoftwareSerial RainSerial(txPin, rxPin); // RX | TX
//#define RainSerial Serial1
#else
  #error "Unsupported Hardware"
#endif  // target detection

#if defined(WIFI)
#include <ESP8266WiFi.h>
int status = WL_IDLE_STATUS;
WiFiClient netClient;
#elif defined(ETHERNET)
#include <SPI.h>
#include <Ethernet.h>
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetClient netClient;
#else
  #error "Need WIFI or Ethernet"
#endif

#if defined(DALLASTEMP)
//https://github.com/pstolarz/Arduino-Temperature-Control-Library
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress outsideThermometer;
#endif

#if defined(BME280)
//https://github.com/adafruit/Adafruit_BME280_Library
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
Adafruit_BME280 bme;
#endif

#define MQTT_CLIENTID "weather"
//#define MQTT_SERVER "192.168.5.148"
//#define MQTT_WILLTOPIC  "clients/weatherArduino3/"
#define MQTT_WILLMESSAGE  "dead"
#define TRUE    (1)

#define INTERVAL 30                                                 // Intervall of sending in seconds

struct RG15 {
  float acc;
  float event;
  float total;
  float rate;
  uint16_t time = RG15_EVENT_TIMEOUT;
  uint8_t init_step;
} Rg15;

// Update these with values suitable for your network.
//byte server[] = { 192, 168, 5, 148 };
IPAddress server(192,168,5,148);

/////////////////////////////////////////////////////////////////////
PubSubClient client(netClient);
unsigned int windcnt = 0;
//unsigned int raincnt = 0;
unsigned long lastSend;

//float rainAcc = 0.0F;
//float rainEventAcc = 0.0F;
//float rainTotalAcc = 0.0F;
//float rainIPH = 0.0F;
//String rainUnits;
String clientid;
void CACHED_FUNCTION_ATTR cntWindSpeed();
void CACHED_FUNCTION_ATTR cntRain();

//////////////// SETUP //////////////////////////////////////////////
void setup() {
#if defined(ETHERNET)
  Ethernet.init(17);  // W5500-EVB-Pico
#endif
  Serial.begin(115200);
  RainSerial.begin(RG15_BAUDRATE);
  Rg15.init_step = 5;
  //RainSerial.write('c');
  //RainSerial.write('\n');
  // pin for Wind speed
  pinMode(windSpeedPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(windSpeedPin), cntWindSpeed, RISING);
  //pinMode(rainPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(rainPin), cntRain, RISING);
  pinMode(windDirPin, INPUT);
  pinMode(ledPin, OUTPUT);

  clientid = String(MQTT_CLIENTID) + GetID();

#if defined(DALLASTEMP)
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0))
  {
    Serial.println("Unable to find address for Dallas Temperature  Device 0");
  }
  sensors.setResolution(insideThermometer, 9);
#endif
#if defined(BME280)
  if (!bme.begin(0x76)) 
  {
    Serial.println("Could not find a valid BME280 sensor");
  }
#endif 
  delay(10);
  InitNetwork();
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
  payload += "\"Sensors\":"; payload += "\"Wind Vane v2\"";
  payload += "}";

  // Send the payload
  char attributes[100];
  payload.toCharArray( attributes, 100 );
  Serial.println(payload);
  client.publish( ("arduino/" + clientid + "/info").c_str(), attributes );
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
    digitalWrite(ledPin, HIGH); 
    //readRainSensor();
    getAndSendTemperatureAndHumidityData();
#if defined(DALLASTEMP)
    sensors.requestTemperatures();
#endif
    digitalWrite(ledPin, LOW); 
  }

  if ( millis() - lastSend > 1000 ) { // Update and send only after delay
    digitalWrite(ledPin, HIGH); 
    Rg15Poll();

    digitalWrite(ledPin, LOW); 
  }

  client.loop();        //keep connected, look for messages from server
  ArduinoOTA.handle();
  delay(10);
}

//////////////// Functions //////////////////////////////////////////
/*void readRainSensor()
{
  String response = RainSerial.readStringUntil('\n');
  if (response.startsWith("Acc")) {
    char acc[7], eventAcc[7], totalAcc[7], rInt[7], unit[4];
    sscanf (response.c_str(), "%*s %s %[^,] , %*s %s %*s %*s %s %*s %*s %s", &acc, &unit, &eventAcc, &totalAcc, &rInt);

    rainAcc = atof (acc);
    rainEventAcc = atof (eventAcc);
    rainTotalAcc = atof (totalAcc);
    rainIPH = atof (rInt);
    rainUnits = unit;
  }
}*/
void getAndSendTemperatureAndHumidityData()
{
  Serial.println("Collecting Weather data.");

  //Calculate Wind Speed (clicks/interval * 2,4 kmh)
  float ws = (windcnt/INTERVAL) * 2.4;
  windcnt = 0;
  //Calculate Rain
  //float r = (raincnt/2)*0.2794;
  //raincnt = 0;
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
#if defined(DALLASTEMP)
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Outside Temp: ");
  Serial.print(tempC);
  Serial.print(" *C ");
#endif

#if defined(BME280)
  float tempCBME = bme.readTemperature();
  Serial.print("Temp: ");
  Serial.print(tempCBME);
  Serial.print(" *C ");
  float humBME = bme.readHumidity();
  Serial.print("Humidity: ");
  Serial.print(humBME);
  Serial.print(" % ");
#endif

  Serial.print("Windspeed: ");
  Serial.print(ws);
  Serial.print(" km/h ");
  Serial.print("Wind Direction: ");
  Serial.print(wd);
  Serial.print(" ");
  //Serial.print("Rain: ");
  //Serial.print(r);
  //Serial.print(" mm ");
  //Serial.print(" Accumulation: ");
  //Serial.print(rainAcc,3);  
  //Serial.print(rainUnits);
  //Serial.print(" Event Accumulation: ");
  //Serial.print(rainEventAcc,3);  
  //Serial.print(rainUnits);
  //Serial.print(" Total Accumulation: ");
  //Serial.print(rainTotalAcc,3);  
  //Serial.print(rainUnits);
  //Serial.print(" IPH: ");
  //Serial.print(rainIPH, 3);
  //Serial.print(" IPH\n");

  String windspeed = String(ws);
  String winddir = String(wd);
  String winddirADC = String(dirpin);
  //String rain = String(r);

  //String rain_Acc = String(rainAcc);
  //String rain_Event_Acc = String(rainEventAcc);
  //String rain_Total_Acc = String(rainTotalAcc);
  //String rain_IPH = String(rainIPH);

#if defined(DALLASTEMP)
  String temp_c = String(tempC);
#endif

#if defined(BME280)
  String temp_c_bme = = String(tempCBME);
  String hum_bme = String(humBME);
#endif

  // Just debug messages
  Serial.print( "Sending Data -> " );

  // Prepare a JSON payload string
  String payload = "{";
#if defined(DALLASTEMP)
  payload += "\"outsidetemperature\":"; payload += temp_c; payload += ",";
#endif
#if defined(BME280)
  payload += "\"temperature\":"; payload += temp_c_bme; payload += ",";
  payload += "\"humidity\":"; payload += hum_bme; payload += ",";
#endif
  payload += "\"windspeed\":"; payload += windspeed; payload += ",";
  payload += "\"winddirection\":\""; payload += winddir; payload += "\",";
  payload += "\"winddirectionadc\":"; payload += winddirADC; //payload += ",";
  //payload += "\"rain\":"; payload += rain; payload += ",";
  //payload += "\"rainAcc\":"; payload += rain_Acc; payload += ",";
  //payload += "\"rainEventAcc\":"; payload += rain_Event_Acc; payload += ",";
  //payload += "\"rainTotalAcc\":"; payload += rain_Total_Acc; payload += ",";
  //payload += "\"rainIPH\":"; payload += rain_IPH; payload += ",";
  //payload += "\"rainUnits\":\""; payload += rainUnits;  payload += "\"";
  payload += "}";

  // Send payload
  char attributes[1024];
  payload.toCharArray( attributes, 1024 );
  client.publish( ("arduino/" + clientid + "/wind").c_str(), attributes );
  Serial.println( attributes );

  lastSend = millis();
}

#if defined(WIFI)
void InitNetwork()
{
  Serial.println("Connecting to AP ...");
  // attempt to connect to WiFi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(SECRET_SSID, SECRET_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
#elif defined(ETHERNET)
void InitNetwork()
{
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
      while (true) {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
}
#endif

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
#if defined(WIFI)
    status = WiFi.status();
    if ( status != WL_CONNECTED) {
      WiFi.begin(SECRET_SSID, SECRET_PASS);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println("Connected to AP");
    }
#elif defined(ETHERNET)

#endif
    Serial.print("Connecting to MQTT Server as ");
    Serial.print(clientid);
    Serial.print("...");
    // Attempt to connect (clientId, username, password)

    if ( client.connect(clientid.c_str(), NULL, NULL) ) {
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
void CACHED_FUNCTION_ATTR cntWindSpeed() {
  windcnt++;
}

void CACHED_FUNCTION_ATTR cntRain() {
  //raincnt++;
}


#if defined(ESP8266)
String GetID()
{
  return String(ESP.getChipId());
}
#elif defined(ESP32)
String GetID()
{
  char macValue[17]; // Don't forget one byte for the terminating NULL...
  uint64_t mac = ESP.getEFuseMac();
  uint32_t hi = mac >> 32;
  uint32_t lo = mac;
  sprintf(macValue, "%08x%08x", hi, lo);
  return macValue;
}
#elif defined(ARDUINO_ARCH_RP2040)
String GetID()
{
  return rp2040.getChipID();
}
#endif

float CharToFloat(const char *str)
{
  // simple ascii to double, because atof or strtod are too large
  char strbuf[24];

  strlcpy(strbuf, str, sizeof(strbuf));
  char *pt = strbuf;
  if (*pt == '\0') { return 0.0f; }

  while ((*pt != '\0') && isspace(*pt)) { pt++; }  // Trim leading spaces

  signed char sign = 1;
  if (*pt == '-') { sign = -1; }
  if (*pt == '-' || *pt == '+') { pt++; }          // Skip any sign

  float left = 0;
  if (*pt != '.') {
    left = atoi(pt);                               // Get left part
    while (isdigit(*pt)) { pt++; }                 // Skip number
  }

  float right = 0;
  if (*pt == '.') {
    pt++;
    uint32_t max_decimals = 0;
    while ((max_decimals < 8) && isdigit(pt[max_decimals])) { max_decimals++; }
    pt[max_decimals] = '\0';                       // Limit decimals to float max of 8
    right = atoi(pt);                              // Decimal part
    while (isdigit(*pt)) {
      pt++;
      right /= 10.0f;
    }
  }

  float result = left + right;
  if (sign < 0) {
    return -result;                                // Add negative sign
  }
  return result;
}

bool Rg15ReadLine(char* buffer) {
  // All lines are terminated with a carriage return (\r or 13) followed by a new line (\n or 10)
  uint32_t i = 0;
  uint32_t cmillis = millis();
  while (RainSerial.available() ) {
    char c = RainSerial.read();
    if (c == 10) { break; }                  // New line ends the message

    if ((c >= 32) && (c < 127)) {            // Accept only valid characters
      buffer[i++] = c;
      if (i == RG15_BUFFER_SIZE -1) { break; }  // Overflow
    }
  delay(10);
    //if ((millis() - cmillis) > RG15_READ_TIMEOUT) {
      //AddLog(LOG_LEVEL_DEBUG, PSTR("HRG: Timeout"));
      //Serial.println( "HRG: Timeout" );
      //return false;
    //}
  }
  buffer[i] = '\0';
      Serial.print( "HRG: Read '" );
      Serial.print( buffer );
      Serial.println( "'" );


  return true;
}

bool Rg15Parse(char* buffer, const char* item, float* result) {
  char* start = strstr(buffer, item);
  if (start != nullptr) {
    char* end = strstr(start, " mm");        // Metric (mm or mmph)
    if (end == nullptr) {
      end = strstr(start, " i");             // Imperial (in or iph)
    }
    if (end != nullptr) {
      char tmp = end[0];
      end[0] = '\0';
      *result = CharToFloat(start + strlen(item));
      end[0] = tmp;
      return true;
    }
  }
  return false;
}

bool Rg15Process(char* buffer) {
  // Process payloads like:
  // Acc  0.01 mm, EventAcc  2.07 mm, TotalAcc 54.85 mm, RInt  2.89 mmph
  // Acc 0.001 in, EventAcc 0.002 in, TotalAcc 0.003 in, RInt 0.004 iph
  // Acc 0.001 mm, EventAcc 0.002 mm, TotalAcc 0.003 mm, RInt 0.004 mmph, XTBTips 0, XTBAcc 0.01 mm, XTBEventAcc 0.02 mm, XTBTotalAcc 0.03 mm
  if (buffer[0] == 'A' && buffer[1] == 'c' && buffer[2] == 'c') {
    Rg15Parse(buffer, "Acc", &Rg15.acc);
    Rg15Parse(buffer, "EventAcc", &Rg15.event);
    Rg15Parse(buffer, "TotalAcc", &Rg15.total);
    Rg15Parse(buffer, "RInt", &Rg15.rate);

    if (Rg15.acc > 0.0f) {
      Rg15.time = RG15_EVENT_TIMEOUT;        // We have some data, so the rain event is on-going
    }
    return true;
  }
  return false;
}

/*********************************************************************************************/

void Rg15Poll(void) {
  bool publish = false;

  if (!RainSerial.available()) {
    if (Rg15.time) {                         // Check if the rain event has timed out, reset rate to 0
      Rg15.time--;
      if (!Rg15.time) {
        Rg15.acc = 0;
        Rg15.rate = 0;
        publish = true;
      }
    }
  } else {
    char rg15_buffer[RG15_BUFFER_SIZE];      // Read what's available
    while (RainSerial.available()) {
      Rg15ReadLine(rg15_buffer);
      if (Rg15Process(rg15_buffer)) {        // Do NOT use "publish = Rg15Process(rg15_buffer)"
        publish = true;
      }
    }
  }

  if (publish) {
    MqttPublishRainSensor();
  }

//       Units: I = Imperial (in)          or M = Metric (mm)
//  Resolution: H = High (0.001)           or L = Low (0.01)
//        Mode: P = Request mode (Polling) or C = Continuous mode - report any change
//     Request: R = Read available data once
  char init_commands[] = "R CLI  ";          // Indexed by Rg15.init_step

  if (Rg15.init_step) {
    Rg15.init_step--;

    char cmnd = init_commands[Rg15.init_step];
    if (cmnd != ' ') {
      RainSerial.println(cmnd);
    }
  }
}

void MqttPublishRainSensor()
{
  /*
    float acc;
  float event;
  float total;
  float rate;
  */
  Serial.println("Sendind Rain data.");

  Serial.print(" Accumulation: ");
  Serial.print(Rg15.acc,4);  
  Serial.print("in Event Accumulation: ");
  Serial.print(Rg15.event,4);  
  Serial.print("in Total Accumulation: ");
  Serial.print(Rg15.total,4);  
  Serial.print("in Rate: ");
  Serial.print(Rg15.rate, 4);
  Serial.print(" IPH\n");

  String rain_Acc = String(Rg15.acc,3);
  String rain_Event_Acc = String(Rg15.event,3);
  String rain_Total_Acc = String(Rg15.total,3);
  String rain_IPH = String(Rg15.rate,3);

  // Just debug messages
  Serial.print( "Sending Data -> " );

  // Prepare a JSON payload string
  String payload = "{";
  payload += "\"rainAcc\":"; payload += rain_Acc; payload += ",";
  payload += "\"rainEventAcc\":"; payload += rain_Event_Acc; payload += ",";
  payload += "\"rainTotalAcc\":"; payload += rain_Total_Acc; payload += ",";
  payload += "\"rainIPH\":"; payload += rain_IPH; payload;
  payload += "}";

  // Send payload
  char attributes[1024];
  payload.toCharArray( attributes, 1024 );
  client.publish( ("arduino/" + clientid + "/rain").c_str(), attributes );
  Serial.println( attributes );

}

