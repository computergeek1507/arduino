#include <PubSubClient.h>


#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NewPing.h>

#define MQTT_CLIENTID "garageArduino"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_USER	NULL
#define MQTT_PASS	NULL
#define MQTTQOS2	NULL
#define MQTT_WILLTOPIC	"clients/arduino/"
#define MQTT_WILLMESSAGE	"dead"
#define TRUE		(1)

// Update these with values suitable for your network.
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
byte server[] = { 192, 168, 5, 148 };
byte ip[] = { 192, 168, 5, 122 };

// set this to the number of milliseconds delay
// this is 30 seconds
#define delayMillis 30000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

// the value of the 'other' resistor
#define SERIESRESISTOR 10000

const int openPin = A0;
const int closedPin = A1;
const int relayPin = 6;
//const int photoPin = A5;
const int StatLED = 5;
const int stdDoorPin = 7;

const int car1Pin = 16;//A2
const int car2Pin = 17;//A3

const int  resolution = 9;
unsigned long lastTempRequest = 0;
int  delayInMillis = 0;
//float temperature = 0.0;

int prevOpenValue = 2;    // open sensor Prev value
int prevClosedValue = 2;   // close sensor Prev value
int prevDoorValue = HIGH;   /// standard door sensor Prev value

int prevOpenValueRaw = 0;    // open sensor Prev value
int prevClosedValueRaw = 0;   // close sensor Prev value

//int samples[NUMSAMPLES];
//int prevPhotoValue = 0;
float prevTemperatureValue = 150;

unsigned int prevCar1 = 0;
unsigned int prevCar2 = 0;

OneWire  ds(3);// on pin 3 (a 4.7K resistor is necessary)
DallasTemperature sensors(&ds);
DeviceAddress outsideThermometer;

#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing car1Ping(car1Pin, car1Pin, MAX_DISTANCE); // NewPing setup of pin and maximum distance.
NewPing car2Ping(car2Pin, car2Pin, MAX_DISTANCE); // NewPing setup of pin and maximum distance.

// Callback function header
void callback(char* topic, byte* payload, unsigned int length);

EthernetClient ethClient;
PubSubClient client(server, 1883, callback, ethClient);

void setup()
{
  // pinMode(openPin, INPUT);   // Open Sensor
  //pinMode(closedPin, INPUT);   // Closed Senser
  pinMode(relayPin, OUTPUT);  // Switch Relay
  pinMode(StatLED, OUTPUT);  // LED
  pinMode(stdDoorPin, INPUT);   // Stand Door Senser
  digitalWrite(StatLED, LOW);
  //Serial.begin(9600);
  char willtopic[128] = MQTT_WILLTOPIC;
  char ipstr[20];
  Ethernet.begin(mac, ip);

  sprintf(ipstr, "%03x:%03x:%03x:%03x",
          ip[0], ip[1], ip[2], ip[3]);

  if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE)) {
    client.publish(willtopic, NULL, 0, TRUE);
    //client.publish("arduino/hello","hello world");
    client.subscribe("arduino/garagepushbutton");
    client.publish(willtopic, ipstr);
    digitalWrite(StatLED, HIGH);
  }
  sensors.begin();
  sensors.getAddress(outsideThermometer, 0);
  sensors.setResolution(outsideThermometer, resolution);

  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  delayInMillis = 750 / (1 << (12 - resolution));
  lastTempRequest = millis();
}

void loop()
{
  if (!client.connected())
  {
    char willtopic[128] = MQTT_WILLTOPIC;
    char ipstr[20];
    sprintf(ipstr, "%03x:%03x:%03x:%03x",
            ip[0], ip[1], ip[2], ip[3]);

    if (client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASS, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE)) {
      client.publish(willtopic, NULL, 0, TRUE);
      //client.publish("arduino/hello","hello world");
      client.subscribe("arduino/garagepushbutton");
      client.publish(willtopic, ipstr);
      digitalWrite(StatLED, HIGH);
    }
  }

  thisMillis = millis();

  if (thisMillis - lastTempRequest >= delayInMillis) // waited long enough??
  {
    float newTemperatureValue = ReadTemperature();

    if (abs(prevTemperatureValue - newTemperatureValue) > 0.1 )
    {
      if (newTemperatureValue != 0.0)
      {
        prevTemperatureValue = newTemperatureValue;
      }
    }
    sensors.requestTemperatures();
    lastTempRequest = thisMillis;
  }

  if (thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;
    SendTempReadings();
    //SendPhotoReadings();
    //SendOpenDoorReading();
    //SendClosedDoorReading();
    //SendStandardDoorReading();
    //totalCount++;
    //Serial.println(totalCount,DEC);
    SendCar1Reading();
    SendCar2Reading();
  }

  int newDoorValue = digitalRead(stdDoorPin);

  if (newDoorValue != prevDoorValue)
  {
    prevDoorValue = newDoorValue;
    SendStandardDoorReading();
  }

  prevOpenValueRaw = ReadAnalogValue(openPin);
  int newOpenValue = Evaluate(prevOpenValueRaw);
  prevClosedValueRaw = ReadAnalogValue(closedPin);
  int newClosedValue = Evaluate(prevClosedValueRaw);

  if (prevOpenValue != newOpenValue)
  {
    prevOpenValue = newOpenValue;
    SendOpenDoorReading();
  }
  if (prevClosedValue != newClosedValue)
  {
    prevClosedValue = newClosedValue;
    SendClosedDoorReading();
  }

  unsigned int newCar1Value = (car1Ping.ping_median(3) / US_ROUNDTRIP_CM);
  unsigned int newCar2Value = (car2Ping.ping_median(3) / US_ROUNDTRIP_CM);
  //unsigned int newCar1Value = car1Ping.ping_cm();
  //unsigned int newCar2Value = car2Ping.ping_cm();
  if( newCar1Value != 0 )
  {
    if (abs(prevCar1 - newCar1Value) > 11 )
    {
      prevCar1 = newCar1Value;
      SendCar1Reading();
    }
  }
  if( newCar2Value != 0 )
  {
    if (abs(prevCar2 - newCar2Value) > 11 )
    {
      prevCar2 = newCar2Value;
      SendCar2Reading();
    }
  }

  //int newPhotoValue = ReadAnalogValue(photoPin);

  //  if(abs(prevPhotoValue-newPhotoValue)>5 )
  //{
  //  prevPhotoValue=newPhotoValue;
  //SendPhotoReadings();
  //}

  client.loop();
  delay(1);
}

void SendCar1Reading()
{
  String closedValue = String(prevCar1);
  char closedChar[closedValue.length() + 1];
  closedValue.toCharArray(closedChar, closedValue.length() + 1);
  if (!client.publish("arduino/garagecar1", closedChar) )
  {
    //Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    //Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
  // String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  // Serial.println(stringStat);
}

void SendCar2Reading()
{
  String closedValue = String(prevCar2);
  char closedChar[closedValue.length() + 1];
  closedValue.toCharArray(closedChar, closedValue.length() + 1);
  if (!client.publish("arduino/garagecar2", closedChar) )
  {
    // Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    // Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }

  // String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  // Serial.println(stringStat);
}

void SendOpenDoorReading()
{
  String openValue = BoolToOpenHAB(prevOpenValue);
  char openChar[openValue.length() + 1];
  openValue.toCharArray(openChar, openValue.length() + 1);

  if (!client.publish("arduino/garageopen", openChar))
  {
    //Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    //Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
  // String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  // Serial.println(stringStat);

   String strOpenValue = String(prevOpenValueRaw);
    char strOpenChar[strOpenValue.length()+1];
    strOpenValue.toCharArray(strOpenChar, strOpenValue.length()+1);

    if(!client.publish("arduino/garageopenraw", strOpenChar))
    {
      //Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     //Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
}

void SendClosedDoorReading()
{
  String closedValue = BoolToOpenHAB(prevClosedValue);
  char closedChar[closedValue.length() + 1];
  closedValue.toCharArray(closedChar, closedValue.length() + 1);
  if (!client.publish("arduino/garageclose", closedChar) )
  {
    //Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    //Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
  //  String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  // Serial.println(stringStat);

    String strClosedValue = String(prevClosedValueRaw);
    char strClosedChar[strClosedValue.length()+1];
    strClosedValue.toCharArray(strClosedChar, strClosedValue.length()+1);

    if(!client.publish("arduino/garagecloseraw", strClosedChar))
    {
      //Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     //Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
}

void SendStandardDoorReading()
{
  String standardValue = BoolToOpenHAB(prevDoorValue);
  char standardChar[standardValue.length() + 1];
  standardValue.toCharArray(standardChar, standardValue.length() + 1);

  if (!client.publish("arduino/garagestddoor", standardChar) )
  {
    //Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else
  {
    //Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
  // String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  // Serial.println(stringStat);
}
/*
  void SendPhotoReadings()
  {
   String photoValue = String(prevPhotoValue);
     char photoChar[photoValue.length()+1];
    photoValue.toCharArray(photoChar, photoValue.length()+1);

    if(!client.publish("arduino/garagephoto", photoChar))
    {
      //Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     //Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }

  // String stringStat =  "PHOTO,"+String(prevPhotoValue);
  // Serial.println(stringStat);

  }
*/
void SendTempReadings()
{
  char buffer[5];
  String value = dtostrf(prevTemperatureValue, 4, 1, buffer);
  char tempChar[value.length() + 1];
  value.toCharArray(tempChar, value.length() + 1);

  if (!client.publish("arduino/garagetemp", tempChar))
  {
    //   Serial.print(F("Fail "));
    digitalWrite(StatLED, LOW);
  }
  else {
    // Serial.print(F("Pass "));
    digitalWrite(StatLED, HIGH);
  }
}

void OpenDoor() {
  digitalWrite(relayPin, HIGH);
  delay(1000);
  digitalWrite(relayPin, LOW);
}

void callback(char* topic, byte* payload, unsigned int length) {
  char buff[256], echostring[512];
  int n, on;

  for (n = 0; (n < length) && (n < sizeof(buff) - 1); n++) {
    buff[n] = payload[n];
  }
  buff[n] = 0;

  //sprintf(echostring, "%s: %s", topic, buff);

  //	Serial.println(echostring);

  /*
     Echo
  */
  //client.publish("arduino/echo", echostring);
  if (!strcmp(topic, "arduino/garagepushbutton"))
  {
    on = (*buff == '1') ? 1 : 0;
    if (on)
    {
      OpenDoor();
    }
  }
  //client.publish("arduino/hello", "hello");
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

  return ( loopValue / i);
}

float ReadTemperature() {

  //celsius = (float)raw / 16.0;
  //fahrenheit = celsius * 1.8 + 32.0;

  // sensors.requestTemperatures();

  return sensors.getTempF(outsideThermometer);
  //return sensors.getTempFByIndex(0);
  //return 1;
  //return steinhart;
}

int Evaluate(int reading)
{
  if (reading < 850) {
    return 1;
  }
  return 0;
}

int EvaluateCar(unsigned int reading)
{
  if (reading < 205) {
    return 1;
  }
  return 0;
}

String BoolToOpenHAB(int stat)
{
  if (stat == 1) {
    return "ON";
  }
  return "OFF";
}
