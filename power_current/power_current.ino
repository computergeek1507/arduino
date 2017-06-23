// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"                   // Include Emon Library

#include <SPI.h>
#include <EthernetV2_0.h>
#include <PubSubClient.h>

#define MQTT_CLIENTID "PowerArduino"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_WILLTOPIC  "clients/PowerArduino/"
#define MQTT_WILLMESSAGE  "dead"
//#define TRUE    (1)

// Update these with values suitable for your network.
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xDB };
byte server[] = { 192, 168, 5, 148 };
IPAddress ip(192,168,5,133);

void callback(char* topic, byte* payload, unsigned int length);
const int StatLED = 9;

// set this to the number of milliseconds delay
// this is 10 seconds
#define delayMillis 10000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

EnergyMonitor emon1;                   // Create an instance
EnergyMonitor emon2;                   // Create an instance

double prevCurrent1Value = 0.0;
double prevCurrent2Value = 0.0;

void setup()
{  
  Serial.begin(9600);
  pinMode(6,OUTPUT);
  // disable SD card if one in the slot
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

    //crappy seeed studio ethernet shield needs to be manually reset
  digitalWrite(6,LOW);  //Reset W5200
  delay(200);
  digitalWrite(6,HIGH);  
  delay(500);       // wait W5200 work
 
  pinMode(StatLED, OUTPUT);  // LED 
  digitalWrite(StatLED, LOW);
  
  char willtopic[128] = MQTT_WILLTOPIC;
  char ipstr[20];
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  
  sprintf(ipstr, "%03x:%03x:%03x:%03x",
    ip[0], ip[1], ip[2], ip[3]);
  
  if (client.connect(MQTT_CLIENTID))
  {
   //client.publish(willtopic, NULL, 0, TRUE);
   // client.subscribe("arduino/lightControl");
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }
  
  emon1.current(1, 60.6);             // Current: input pin, calibration.
  emon2.current(2, 60.6);             // Current: input pin, calibration.
}

void loop()
{
  if(!client.connected())
  {
    char willtopic[128] = MQTT_WILLTOPIC;
    char ipstr[20];
    sprintf(ipstr, "%03x:%03x:%03x:%03x",
    ip[0], ip[1], ip[2], ip[3]);
    
    if (client.connect(MQTT_CLIENTID)) 
    {
      //client.publish(willtopic, NULL, 0, TRUE);
      client.publish(willtopic, ipstr);
      digitalWrite(StatLED, HIGH);
    }  
  }
  
  thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;

    SendPowerReading1();
    SendPowerReading2();
    //totalCount++;
    //Serial.println(totalCount,DEC);
  } 
  prevCurrent1Value = emon1.calcIrms(1480); 
  prevCurrent2Value = emon2.calcIrms(1480); 
 // delay(100);
  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  char buff[256];
  int n;

  for (n = 0; (n < length) && (n < sizeof(buff) - 1); n++)
  {
    buff[n] = payload[n];
  }
  buff[n] = '\0';
}

void SendPowerReading1() 
{
  char buffer[5];
  String value = dtostrf(prevCurrent1Value, 4, 1, buffer);
  char tempChar[value.length() + 1];
  value.toCharArray(tempChar, value.length() + 1);

  if (!client.publish("arduino/PowerCurrentOne", tempChar))
  {
    //   Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    // Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
}
void SendPowerReading2() 
{
  char buffer[5];
  String value = dtostrf(prevCurrent2Value, 4, 1, buffer);
  char tempChar[value.length() + 1];
  value.toCharArray(tempChar, value.length() + 1);

  if (!client.publish("arduino/PowerCurrentTwo", tempChar))
  {
    //   Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    // Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
}
