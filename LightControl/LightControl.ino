#include <RCSwitch.h>
#include <SPI.h>
#include <EthernetV2_0.h>
#include <PubSubClient.h>

#define MQTT_CLIENTID "lightSwitchArduino"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_WILLTOPIC	"clients/LightSwitcharduino/"
#define MQTT_WILLMESSAGE	"dead"
//#define TRUE		(1)

// Update these with values suitable for your network.
byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xCB };
byte server[] = { 192, 168, 5, 148 };
IPAddress ip(192,168,5,134);

const int RCPin = 7;
const int StatLED = 13;

RCSwitch mySwitch = RCSwitch();

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

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
  
  if (client.connect(MQTT_CLIENTID)) {
   //client.publish(willtopic, NULL, 0, TRUE);
    client.subscribe("arduino/lightControl");
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }
  
    mySwitch.enableTransmit(RCPin);
  // mySwitch.setPulseLength(320);
}

void loop() 
{
    if(!client.connected())
  {
     char willtopic[128] = MQTT_WILLTOPIC;
    char ipstr[20];
  
  	sprintf(ipstr, "%03x:%03x:%03x:%03x",
		ip[0], ip[1], ip[2], ip[3]);
    
      client.connect(MQTT_CLIENTID);
      
   //client.publish(willtopic, NULL, 0, TRUE);
    client.subscribe("arduino/lightControl");
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);  
  }


client.loop();  
}

void callback(char* topic, byte* payload, unsigned int length) 
{
      char buff[length + 1];
	int n;

	for (n = 0; n < length; n++) {
		buff[n] = payload[n];
	}
	buff[n] = '\0';

	Serial.println(buff);

	  if (!strcmp(topic, "arduino/lightControl")) 
    {
      if(length==5||length==6)
      {
        //int number = atoi( buff[1] );
        int number = buff[1] - '0';
        int channel = buff[0] - '@';
        //Serial.println(number);       
        //Serial.println(channel);

        char feedback[length];  
        char feedbackTopic[256];  
        sprintf(feedbackTopic, "arduino/lightControl/%c%i",buff[0], number);

        if(strstr(buff, "ON"))
        {
           mySwitch.switchOn(channel, number);
           delay(250);
           mySwitch.switchOn(channel, number);
           sprintf(feedback, "%c%i_ON",(channel + '@'), number);
           client.publish(feedbackTopic, "ON");
        }
        else
        {
          mySwitch.switchOff(channel, number);
           delay(250);
           mySwitch.switchOff(channel, number);
           sprintf(feedback, "%c%i_OFF",(channel + '@'), number);
           client.publish(feedbackTopic, "OFF");
        }         
       Serial.println( feedback);
	    }
    }
}
