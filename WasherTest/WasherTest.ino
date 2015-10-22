#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define MQTT_CLIENTID "washerArduino"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_USER	NULL
#define MQTT_PASS	NULL
#define MQTTQOS2	NULL
#define MQTT_WILLTOPIC	"clients/arduino/"
#define MQTT_WILLMESSAGE	"dead"
#define TRUE		(1)

// Update these with values suitable for your network.
byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x00, 0x6C, 0x85 };
byte server[] = { 192, 168, 5, 148 };
byte ip[] = { 192, 168, 5, 128 }; 

// set this to the number of milliseconds delay
// this is 30 seconds
#define delayMillis 30000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

const int StatLED = 13;
const int washPin = A0;
int prevWashValue = 0;

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void setup()
{
     // pinMode(openPin, INPUT);   // Open Sensor
  //pinMode(closedPin, INPUT);   // Closed Senser

 pinMode(StatLED, OUTPUT);  // LED 

 digitalWrite(StatLED, LOW);
  Serial.begin(9600);
  char willtopic[128] = MQTT_WILLTOPIC;
 char ipstr[20];
  Ethernet.begin(mac, ip);
  
  	sprintf(ipstr, "%03x:%03x:%03x:%03x",
		ip[0], ip[1], ip[2], ip[3]);
  
  
  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE)) {
   client.publish(willtopic, NULL, 0, TRUE);
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }

}

void loop()
{  
  if(!client.connected())
  {
    char willtopic[128] = MQTT_WILLTOPIC;
    char ipstr[20];
    sprintf(ipstr, "%03x:%03x:%03x:%03x",
		ip[0], ip[1], ip[2], ip[3]);
    
      if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE)) {
   client.publish(willtopic, NULL, 0, TRUE);

    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }
  
  }
  
  thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;

    SendPhotoReadings();
    //totalCount++;
    //Serial.println(totalCount,DEC);
  } 

int newWashValue = ReadAnalogValue(washPin);
  
  if(abs(prevWashValue-newWashValue)>5 )
{
  prevWashValue=newWashValue;
  //SendPhotoReadings();
}
  client.loop();
}

void SendPhotoReadings() 
{
   String photoValue = String(prevWashValue);
     char photoChar[photoValue.length()+1]; 
    photoValue.toCharArray(photoChar, photoValue.length()+1);

    if(!client.publish("arduino/washLED", photoChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
  
  String stringStat =  "WASH,"+String(prevWashValue);
  Serial.println(stringStat);

}

void callback(char* topic, byte* payload, unsigned int length) {
      char buff[256];
	int n;

	for (n = 0; (n < length) && (n < sizeof(buff) - 1); n++) {
		buff[n] = payload[n];
	}
	buff[n] = '\0';
}

int ReadAnalogValue(int pinNumber)
{
  int loopValue = 0;
  
  //average 10 readings for kicks
  int i = 0;
  for (i = 0; i < 10; i++)  
  {
     int readValue = analogRead(pinNumber); 
     loopValue = loopValue + readValue; 
       delay(2);  
  }
  
  return ( loopValue/i);
}

int Evaluate(int reading)
{
  if(reading < 800){
    return 1;
  }
   return 0;
 }

String BoolToOpenHAB(int stat)
{
  if(stat == 1){
    return "ON";
  }
   return "OFF";
 }
