#include <OneWire.h>

// the value of the 'other' resistor
#define SERIESRESISTOR 10000  

#define delayMillis 30000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

char endLine1 = '\n';
char endLine2 = '\r';
char endLine3 = '\r\n';

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

const int openPin = 2;
const int closedPin = 4;
const int relayPin = 6;
const int photoPin = A5;
const int StatLED = 5;
const int stdDoorPin = 7;

int prevOpenValue = LOW;    // open sensor Prev value
int prevClosedValue = LOW;   // close sensor Prev value
int prevDoorValue = LOW;   /// standard door sensor Prev value

//int samples[NUMSAMPLES];
int prevPhotoValue = 0;
float prevTemperatureValue = 150;

OneWire  ds(3);// on pin 3 (a 4.7K resistor is necessary)

void setup()
{
  // start serial port at 9600 bps:
  Serial.begin(9600);

  pinMode(openPin, INPUT);   // Open Sensor
  pinMode(closedPin, INPUT);   // Closed Senser
  pinMode(relayPin, OUTPUT);  // Switch Relay 
 pinMode(StatLED, OUTPUT);  // LED 
 pinMode(stdDoorPin, INPUT);   // Stand Door Senser
 digitalWrite(StatLED, LOW);
 
 inputString.reserve(200);
}

void loop()
{
   if (stringComplete)
  {
    if(inputString.indexOf("OpenDoor") >=0)
    {
      OpenDoor();      
      //println("OpenDoor;successfully done");
      Serial.println("OpenDoor_done");
    }
    if(inputString.indexOf("Status") >=0)
    {
      SendDoorReadings();    
      //println("Status;successfully done");
      //Serial.println("Status_done");
    }
    if(inputString.indexOf("Temp") >=0)
    {
      SendTempReadings();    
      //println("Status;successfully done");
      //Serial.println("Status_done");
    }
    if(inputString.indexOf("Photo") >=0)
    {
      SendPhotoReadings();
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
    //Serial.flush();
  }
  
   thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;
    //SendTempReadings(); 
     //SendPhotoReadings();
     SendTempPhotoReadings();
  } 

   //read sensors 
  int newOpenValue = digitalRead(openPin); 
  int newClosedValue = digitalRead(closedPin);
  int newDoorValue = digitalRead(stdDoorPin);
  
  if(newOpenValue!=prevOpenValue||newClosedValue!=prevClosedValue||newDoorValue!=prevDoorValue)
{
  prevOpenValue=newOpenValue;
  prevClosedValue=newClosedValue;
  prevDoorValue=newDoorValue;
  SendDoorReadings();
}

  float newTemperatureValue = ReadTemperature();
  
  if(abs(prevTemperatureValue-newTemperatureValue)>0.1 )
{
  prevTemperatureValue=newTemperatureValue;
  //SendTempReadings();
}

int newPhotoValue = ReadPhotoValue();
  
  if(abs(prevPhotoValue-newPhotoValue)>5 )
{
  prevPhotoValue=newPhotoValue;
  //SendPhotoReadings();
}
  //delay(100);
  //Serial.flush();
}

void SendDoorReadings() 
{
  String stringStat =  "STAT,"+String(prevOpenValue)+","+String(prevClosedValue)+","+String(prevDoorValue);
  Serial.println(stringStat);
}

void SendTempPhotoReadings() 
{
  char buffer[5];
  String s = dtostrf(prevTemperatureValue, 4, 1, buffer);
  String stringStat =  "TEMP,"+String(s)+ ",PHOTO,"+String(prevPhotoValue);
  Serial.println(stringStat);
}

void SendTempReadings() 
{
  char buffer[5];
  String s = dtostrf(prevTemperatureValue, 4, 1, buffer);
  String stringStat =  "TEMP,"+String(s);
  Serial.println(stringStat);
}

void SendPhotoReadings() 
{
  String stringStat =  "PHOTO,"+String(prevPhotoValue);
  Serial.println(stringStat);
}

void OpenDoor() {
  digitalWrite(relayPin, HIGH);
  delay(1000);
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

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available())
  {
    // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    // handle different newLine character
    if (inChar == endLine1)  //\n
    {
      stringComplete = true;
    }
    else if((inChar == endLine2) ||  //\r
            (inChar == endLine3))    //\r\n
    {
      delay(5);
      if(Serial.available())
      {
        inChar = (char)Serial.read();
        inputString += inChar;
        if(inChar == endLine1)  //also ingsesamt==\n\r
        {
          stringComplete = true;
        }
      }
      else
      {
        stringComplete = true;
      }
    }
  }
}
