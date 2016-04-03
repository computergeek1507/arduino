#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define wifi_ssid "SSID"
#define wifi_password "PASSWORD"
#define mqtt_server "192.168.5.148"
#define topicFront "ESP/frontdoor"
#define topicSide "ESP/sidedoor"

int prevFrontDoorValue = HIGH; //Start In default state to prevent false triggers if micro resets
int prevSideDoorValue = HIGH;
const int frontDoor = 1; //Digial Pin of ESP8266
const int sideDoor = 3; //Digial Pin of ESP8266

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
    //Serial.begin(115200); //No serial as Pins are used as Digital IO
     pinMode(frontDoor, INPUT);
     pinMode(sideDoor, INPUT);
    setup_wifi();
    client.setServer(mqtt_server, 1883);
}
void setup_wifi() {
    delay(10);
    // We start by connecting to a WiFi network
   // Serial.println();
    //Serial.print("Connecting to ");
    //Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
    }
    //Serial.println("");
    //Serial.println("WiFi connected");
    //Serial.println("IP address: ");
    //Serial.println(WiFi.localIP());
}
void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        //Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("DoorBell")) { // Just Client Name
            //Serial.println("connected");
        } else {
            //Serial.print("failed, rc=");
            //Serial.print(client.state());
            //Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void pubMQTT(String topic,int topic_val){
    //Serial.print("Newest topic " + topic + " value:");
   // Serial.println(String(topic_val).c_str());
    client.publish(topic.c_str(), BoolToOpenHAB(topic_val).c_str(), true);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  int newFrontDoorValue = digitalRead(frontDoor);
    
  if(newFrontDoorValue!=prevFrontDoorValue)
  {
    prevFrontDoorValue=newFrontDoorValue;
    pubMQTT(topicFront,prevFrontDoorValue);
  }

  int newSideDoorValue = digitalRead(sideDoor);
    
  if(newSideDoorValue!=prevSideDoorValue)
  {
    prevSideDoorValue=newSideDoorValue;
    pubMQTT(topicSide,prevSideDoorValue);
  }
}

//Convert Digial State of Pins to openHAB switch statuses
String BoolToOpenHAB(int stat)
{
  if(stat == 0){
    return "ON";
  }
   return "OFF";
}
