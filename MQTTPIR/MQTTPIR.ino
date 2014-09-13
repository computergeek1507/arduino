#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <Wire.h>

#define MQTT_CLIENTID "pirArduino"
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
byte ip[] = { 192, 168, 5, 118 }; 

// set this to the number of milliseconds delay
// this is 30 seconds
#define delayMillis 30000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

int tmp102Address = 0x48;
float oldfahrenheit = 0;

const int motionPin = 7;
const int StatLED = 13;

int prevMotionValue = LOW;    // Motion Prev value

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void setup()
{
 pinMode(motionPin, INPUT);   // Stand Door Senser
 digitalWrite(StatLED, LOW);
  // disable SD SPI
  pinMode(4,OUTPUT);
  //digitalWrite(4,HIGH);
  Serial.begin(9600);
  
  char willtopic[128] = MQTT_WILLTOPIC;
 char ipstr[20];
  Ethernet.begin(mac, ip);
  
  	sprintf(ipstr, "%03x:%03x:%03x:%03x",
		ip[0], ip[1], ip[2], ip[3]);
  
  
  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE)) {
   client.publish(willtopic, NULL, 0, TRUE);
    client.publish("arduino2/hello","hello world");
    //client.subscribe("/arduino/garagepushbutton");
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
    client.publish("arduino2/hello","hello world");
    //client.subscribe("/arduino/garagepushbutton");
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }
  
  }
  //client.subscribe("/arduino/garagepushbutton");
  thisMillis = millis();
  
    if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;
    SendTempReading();
  } 


  int newMotionValue = digitalRead(motionPin); 

  
  if(newMotionValue!=prevMotionValue)
{
  prevMotionValue=newMotionValue;
  SendMotionReading();
   //delay(100);
}

  //float celsius = getTemperature();
  //Serial.print("Celsius: ");
  //Serial.println(celsius);
  
  
    float loopValue = 0;
  
  //average 10 readings for kicks
  int i = 0;
  for (i = 0; i < 5; i++)  
  {
     float readValue = getTemperature();      
     loopValue = loopValue + readValue;        
  }
  
  float celsius = loopValue/((float)i);

  float fahrenheit = (1.8 * celsius) + 32.0; 
  
  // if(abs(oldfahrenheit-fahrenheit)>.05)
  //{
    oldfahrenheit=fahrenheit;
    //SendTempReading();
    //Serial.print("Fahrenheit: ");
 // Serial.println(fahrenheit);
//}

  client.loop();
}
void SendMotionReading() 
{

    String value = BoolToOpenHAB(prevMotionValue);    
    char tempChar[value.length()+1]; 
    value.toCharArray(tempChar, value.length()+1);
    
   
    if(!client.publish("/arduino/pirmotion", tempChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else{
      Serial.print(F("Pass "));
      digitalWrite(StatLED, HIGH);
    }
}

void SendTempReading() 
{
 char buffer[5];
    String value = dtostrf(oldfahrenheit, 4, 1, buffer);
    char tempChar[value.length()+1]; 
    value.toCharArray(tempChar, value.length()+1);
    
   
    if(!client.publish("/arduino/indoortemp", tempChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else{
      Serial.print(F("Pass "));
      digitalWrite(StatLED, HIGH);
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
      char buff[256], echostring[512];
	int n, on;

	for (n = 0; (n < length) && (n < sizeof(buff) - 1); n++) {
		buff[n] = payload[n];
	}
	buff[n] = 0;

	//sprintf(echostring, "%s: %s", topic, buff);

	Serial.println(echostring);

	/*
	 * Echo
	 */
	//client.publish("arduino/echo", echostring);
/*
	if (!strcmp(topic, "/arduino/garagepushbutton")) {
		on = (*buff == '1') ? 1 : 0;

		if (on) {
			OpenDoor();
		} 
	}
 */
	client.publish("arduino/hello2", "hello");
}

String BoolToOpenHAB(int stat){
  //inverse logic because of PULLUPS are Default
  if(stat == 1){
    return "ON";
  }
   return "OFF";
 }

float getTemperature(){
  Wire.requestFrom(tmp102Address,2); 

  byte MSB = Wire.read();
  byte LSB = Wire.read();

  //it's a 12bit int, using two's compliment for negative
  int TemperatureSum = ((MSB << 8) | LSB) >> 4; 

  float celsius = TemperatureSum*0.0625;
  return celsius;
}
