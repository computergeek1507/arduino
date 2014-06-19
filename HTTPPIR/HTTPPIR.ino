/*
   Web client sketch for IDE v1.0.1 and w5100/w5200
   Uses POST method.
   Posted November 2012 by SurferTim
*/

#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {  
  0x90, 0xA2, 0xDA, 0x00, 0x6C, 0x85 };
byte ip[] = { 192, 168, 5, 118 }; 
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


const int motionPin = 7;

int prevMotionValue = LOW;    // Motion Prev value

void setup() {
  Serial.begin(9600);

 pinMode(motionPin, INPUT);   // Stand Door Senser
 //digitalWrite(StatLED, LOW);
  // disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

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
     //read sensors 
  int newMotionValue = digitalRead(motionPin); 

  
  if(newMotionValue!=prevMotionValue)
{
  prevMotionValue=newMotionValue;
  SendMotionReading();
   delay(1000);
}

}
void SendMotionReading() 
{
   String standardValue = "MotionSensor="+BoolToOpenHAB(prevMotionValue);
     char standardChar[standardValue.length()+1]; 
    standardValue.toCharArray(standardChar, standardValue.length()+1);

    //sprintf(params,value2); 
    if(!postPage(serverName,serverPort,pageName,standardChar)) 
    {
      Serial.print(F("Fail "));
      //digitalWrite(StatLED, LOW);
    }
    else
    {
     Serial.print(F("Pass "));
     //digitalWrite(StatLED, HIGH);
    }
  
  String stringStat =  "STAT,"+String(prevMotionValue);
  Serial.println(stringStat);

}

String BoolToOpenHAB(int stat){
  if(stat == 0){
    return "OFF";
  }
   return "ON";
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
