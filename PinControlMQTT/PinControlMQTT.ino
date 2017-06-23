#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define MQTT_CLIENTID "lightSwitchArduino"
//#define MQTT_SERVER "192.168.0.22"
#define MQTT_WILLTOPIC  "clients/LightSwitcharduino/"
#define MQTT_WILLMESSAGE  "dead"
#define TRUE    (1)

// Update these with values suitable for your network.
byte mac[]    = {  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x07 };
//byte server[] = { 192, 168, 0, 22 };
IPAddress ip(192, 168, 0, 50);
IPAddress server(192, 168, 5, 22);

const int StatLED = 13;

void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void callback(char* topic, byte* payload, unsigned int length) 
{
  char buff[length + 1];
  int n;

  for (n = 0; n < length; n++) 
  {
    buff[n] = payload[n];
  }
  buff[n] = '\0';

  if (!strcmp(topic, "arduino/lightControl")) 
  {
    if(length==3)
    {
      //int number = atoi( buff[1] );
      int number = buff[2] - '0';
      int channel = buff[0] - '0';
      Serial.println(number);       
      Serial.println(channel);
      char feedbackTopic[256];  
      sprintf(feedbackTopic, "arduino/lightControl/%i",channel);
      pinMode(channel, OUTPUT);  // output
      digitalWrite(channel, number);

      if(number==1)
      {
        client.publish(feedbackTopic, "ON");
      }
      else
      {
        client.publish(feedbackTopic, "OFF");
      }
    }
  }
}

void setup() 
{
  Serial.begin(9600);
  pinMode(StatLED, OUTPUT);  // LED 
  digitalWrite(StatLED, LOW);
  
  char willtopic[128] = MQTT_WILLTOPIC;
  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  
  if (client.connect(MQTT_CLIENTID, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE))
  {
    //client.publish(willtopic, NULL, 0, TRUE);
    client.subscribe("arduino/lightControl");
    digitalWrite(StatLED, HIGH);
  }
}

void loop() 
{
  if(!client.connected())
  {
    char willtopic[128] = MQTT_WILLTOPIC;    
    client.connect(MQTT_CLIENTID, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE);      
    //client.publish(willtopic, NULL, 0, TRUE);
    client.subscribe("arduino/lightControl");
    digitalWrite(StatLED, HIGH);  
  }
  client.loop();  
}

