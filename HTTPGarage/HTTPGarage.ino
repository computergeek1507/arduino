/*
   Web client sketch for IDE v1.0.1 and w5100/w5200
   Uses POST method.
   Posted November 2012 by SurferTim
*/

#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h>

byte mac[] = {  
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 5, 122 }; 
//Change to your server domain
char serverName[] = "192.168.5.148";

// change to your server's port
int serverPort = 8080;

// change to the page on that server
char pageName[] = "/CMD";

EthernetClient client;
//int totalCount = 0; 
// insure params is big enough to hold your variables
//char params[32];

// set this to the number of milliseconds delay
// this is 30 seconds
#define delayMillis 30000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

// the value of the 'other' resistor
#define SERIESRESISTOR 10000  

const int openPin = 2;
const int closedPin = 4;
const int relayPin = 6;
const int photoPin = A5;
const int StatLED = 5;
const int stdDoorPin = 7;

int prevOpenValue = LOW;    // open sensor Prev value
int prevClosedValue = LOW;   // close sensor Prev value
int prevDoorValue = LOW;   /// standard door sensor Prev value

int prevPhotoValue = 0;
float prevTemperatureValue = 150;

OneWire  ds(3);// on pin 3 (a 4.7K resistor is necessary)

void setup() {
  Serial.begin(9600);

  pinMode(openPin, INPUT);   // Open Sensor
  pinMode(closedPin, INPUT);   // Closed Senser
  pinMode(relayPin, OUTPUT);  // Switch Relay 
 pinMode(StatLED, OUTPUT);  // LED 
 pinMode(stdDoorPin, INPUT);   // Stand Door Senser
 digitalWrite(StatLED, LOW);
  //// disable SD SPI
  //pinMode(4,OUTPUT);
  //digitalWrite(4,HIGH);

  Serial.print(F("Starting ethernet..."));
  //if(!Ethernet.begin(mac)) Serial.println(F("failed"));
  //else Serial.println(Ethernet.localIP());
  Ethernet.begin(mac, ip);
  
  delay(2000);
  Serial.println(F("Ready"));
  //digitalWrite(StatLED, HIGH);
}

void loop()
{
  thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;
    SendTempReadings();
    SendPhotoReadings();
    //SendOpenDoorReading();
    //SendClosedDoorReading();
    //SendStandardDoorReading();
    //totalCount++;
    //Serial.println(totalCount,DEC);
  }    
  
     //read sensors 
  int newOpenValue = digitalRead(openPin); 
  int newClosedValue = digitalRead(closedPin);
  int newDoorValue = digitalRead(stdDoorPin);
  
  if(newOpenValue!=prevOpenValue)
{
  prevOpenValue=newOpenValue;
  SendOpenDoorReading();
}

  if(newClosedValue!=prevClosedValue)
{
  prevClosedValue=newClosedValue;
  SendClosedDoorReading();
}

  if(newDoorValue!=prevDoorValue)
{
  prevDoorValue=newDoorValue;
  SendStandardDoorReading();
}

int newPhotoValue = ReadPhotoValue();
  
  if(abs(prevPhotoValue-newPhotoValue)>5 )
{
  prevPhotoValue=newPhotoValue;
  //SendPhotoReadings();
}

  float newTemperatureValue = ReadTemperature();
  
  if(abs(prevTemperatureValue-newTemperatureValue)>0.1 )
{
  prevTemperatureValue=newTemperatureValue;
//  SendTempReadings(); 
}
}

void SendOpenDoorReading() 
{
      String openValue = "GarageDoorOpened="+BoolToOpenHAB(prevOpenValue);
      char openChar[openValue.length()+1]; 
    openValue.toCharArray(openChar, openValue.length()+1);

    //sprintf(params,value);  
    if(!postPage(serverName,serverPort,pageName,openChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else{
     Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
  
  
  String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  Serial.println(stringStat);
}

void SendClosedDoorReading() 
{
   String closedValue = "GarageDoorClosed="+BoolToOpenHAB(prevClosedValue);
     char closedChar[closedValue.length()+1]; 
    closedValue.toCharArray(closedChar, closedValue.length()+1);

    //sprintf(params,value2); 
    if(!postPage(serverName,serverPort,pageName,closedChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
  
  String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  Serial.println(stringStat);

}

void SendStandardDoorReading() 
{
   String standardValue = "GarageStdDoor="+BoolToOpenHAB(prevDoorValue);
     char standardChar[standardValue.length()+1]; 
    standardValue.toCharArray(standardChar, standardValue.length()+1);

    //sprintf(params,value2); 
    if(!postPage(serverName,serverPort,pageName,standardChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
  
  String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue);
  Serial.println(stringStat);

}

void SendPhotoReadings() 
{
   String photoValue = "Daylight="+String(prevPhotoValue);
     char photoChar[photoValue.length()+1]; 
    photoValue.toCharArray(photoChar, photoValue.length()+1);

    //sprintf(params,value2); 
    if(!postPage(serverName,serverPort,pageName,photoChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else
    {
     Serial.print(F("Pass "));
     digitalWrite(StatLED, HIGH);
    }
  
  String stringStat =  "PHOTO,"+String(prevPhotoValue);
  Serial.println(stringStat);

}

void SendTempReadings() 
{
 char buffer[5];
    String s = dtostrf(prevTemperatureValue, 4, 1, buffer);
    String value = "Temperature="+s;    
    char tempChar[value.length()+1]; 
    value.toCharArray(tempChar, value.length()+1);
    
    //sprintf(params,tempChar);     
    if(!postPage(serverName,serverPort,pageName,tempChar)) 
    {
      Serial.print(F("Fail "));
      digitalWrite(StatLED, LOW);
    }
    else{
      Serial.print(F("Pass "));
      digitalWrite(StatLED, HIGH);
    }
}

void OpenDoor() {
  digitalWrite(relayPin, HIGH);
  delay(500);
  digitalWrite(relayPin, LOW);
}

int ReadPhotoValue()
{
  int loopValue = 0;
  
  //average 5 readings for kicks
  int i = 0;
  for (i = 0; i < 10; i++)  
  {
     int readValue = analogRead(photoPin); 
     loopValue = loopValue + readValue;          
  }
  
  return ( loopValue/i);
}

float ReadTemperature(){
 byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  
  if ( !ds.search(addr)) 
  {
    //Serial.println("No more addresses.");
    //Serial.println();
    ds.reset_search();
    delay(250);
    return 0;
  }
  
  //Serial.print("ROM =");
  //for( i = 0; i < 8; i++) {
  //  Serial.write(' ');
  //  Serial.print(addr[i], HEX);
  //}

  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return 0;
  }
  //Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return 0;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  //Serial.print("  Data = ");
 // Serial.print(present, HEX);
  //Serial.print(" ");
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    //Serial.print(" ");
  }
  //Serial.print(" CRC=");
  //Serial.print(OneWire::crc8(data, 8), HEX);
  //Serial.println();

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;

  return fahrenheit;
//return 1;
  //return steinhart;
}

String BoolToOpenHAB(int stat){
  //inverse logic because of PULLUPS are Default
  if(stat == 0){
    return "ON";
  }
   return "OFF";
 }

byte postPage(char* domainBuffer,int thisPort,char* page,char* thisData)
{
  int inChar;
  char outBuf[64];

  Serial.print(F("connecting..."));

  if(client.connect(domainBuffer,thisPort))
  {
    Serial.println(F("connected"));

    // send the header
    sprintf(outBuf,"POST %s HTTP/1.1",page);
    client.println(outBuf);
    sprintf(outBuf,"Host: %s",domainBuffer);
    client.println(outBuf);
    client.println(F("Connection: close\r\nContent-Type: application/x-www-form-urlencoded"));
    sprintf(outBuf,"Content-Length: %u\r\n",strlen(thisData));
    client.println(outBuf);

    // send the body (variables)
    client.print(thisData);
  } 
  else
  {
    Serial.println(F("failed"));
    return 0;
  }

  int connectLoop = 0;

  while(client.connected())
  {
    while(client.available())
    {
      inChar = client.read();
      Serial.write(inChar);
      connectLoop = 0;
    }

    delay(1);
    connectLoop++;
    if(connectLoop > 10000)
    {
      Serial.println();
      Serial.println(F("Timeout"));
      client.stop();
    }
  }

  Serial.println();
  Serial.println(F("disconnecting."));
  client.stop();
  return 1;
}
