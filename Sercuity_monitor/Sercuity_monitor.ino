/*
  Analog input, analog output, serial output
 
 Reads an analog input pin
 prints the results to the serial monitor.

 
 */
 
char endLine1 = '\n';
char endLine2 = '\r';
char endLine3 = '\r\n';

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete


// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = A0;  // Analog input pin that the LED is attached to
const int led = 13;

const int homelinkOne = 2;
const int plugOneOn = 3;
const int plugOneOff = 5;
const int plugTwoOn = 6;
const int plugTwoOff = 9;
const int plugThreeOn = 10;
const int plugThreeOff = 11;

int prevSensorValue = 0;        // value read from the pot

void setup() {
  pinMode(led, OUTPUT);
  pinMode(homelinkOne, OUTPUT);
  pinMode(plugOneOn, OUTPUT);
  pinMode(plugOneOff, OUTPUT);
  pinMode(plugTwoOn, OUTPUT);
  pinMode(plugTwoOff, OUTPUT);
  pinMode(plugThreeOn, OUTPUT);
  pinMode(plugThreeOff, OUTPUT);
  // initialize serial communications at 9600 bps:
  Serial.begin(9600); 
  inputString.reserve(200);
  
  //cycle for openhab
  Serial.println("Door_Closed");
  delay(500); 
  Serial.println("Door_Opened");
  delay(500); 
}

void loop() 
{  
   if (stringComplete)
  {
    if(inputString.indexOf("L11") >=0)
    {
      LightOneON(); 
      delay(200); 
      LightOneON(); 
    }
    if(inputString.indexOf("L10") >=0)
    {
      LightOneOFF();
      delay(200); 
      LightOneOFF();
    }
    if(inputString.indexOf("L21") >=0)
    {
      LightTwoON(); 
      delay(200); 
      LightTwoON();
    }
    if(inputString.indexOf("L20") >=0)
    {
      LightTwoOFF();
      delay(300); 
      LightTwoOFF();
    }
    if(inputString.indexOf("L31") >=0)
    {
      LightThreeON(); 
      delay(200); 
      LightThreeON();
    }
    if(inputString.indexOf("L30") >=0)
    {
      LightThreeOFF();
      delay(200); 
      LightThreeOFF();
    }
    
    if(inputString.indexOf("GD1") >=0)
    {
      GarageDoorOpen();
    }
    // clear the string:
    inputString = "";
    stringComplete = false;
    //Serial.flush();
  }
    
  // read the analog in value:
  digitalWrite(led, LOW);
  int loopValue = 0;
  
  //average 5 readings for kicks
  int i = 0;
  for (i = 0; i < 5; i++)  
  {
     int readValue = analogRead(analogInPin); 
     loopValue = loopValue + readValue;          
  }
  
  int newSensorValue = loopValue/i;
  if(abs(prevSensorValue-newSensorValue)>201){
  
  if(newSensorValue>201)
  {
    Serial.println("Door_Closed");
    digitalWrite(led, LOW);
  }
  else
  {
    Serial.println("Door_Opened");
    digitalWrite(led, HIGH);
  }
  prevSensorValue=newSensorValue;
  }
  // Serial.flush();
  //int sensorValue = analogRead(analogInPin);            

  // print the results to the serial monitor:
  //Serial.print("sensor = " );                       
  //Serial.println(sensorValue);      

  //delay(100);                     
}

   void LightOneON()
   {
     digitalWrite(plugOneOn, HIGH);
     //Serial.println("plugOneOn");
     delay(300); 
     digitalWrite(plugOneOn, LOW);
     
   }
   void LightOneOFF()
   {
     digitalWrite(plugOneOff, HIGH);
     //Serial.println("plugOneOff");
     delay(300);
     digitalWrite(plugOneOff, LOW);
   }
  void LightTwoON()
   {
     digitalWrite(plugTwoOn, HIGH);
     //Serial.println("plugTwoOn");
     delay(300); 
     digitalWrite(plugTwoOn, LOW);
   }
   void LightTwoOFF()
   {
     digitalWrite(plugTwoOff, HIGH);
     //Serial.println("plugTwoOff");
     delay(300); 
     digitalWrite(plugTwoOff, LOW);
   }
   void LightThreeON()
   {
     digitalWrite(plugThreeOn, HIGH);
     //Serial.println("plugThreeOn");
     delay(300); 
     digitalWrite(plugThreeOn, LOW);
   }
   void LightThreeOFF()
   {
       digitalWrite(plugThreeOff, HIGH);
       //Serial.println("plugThreeOff");
       delay(300); 
       digitalWrite(plugThreeOff, LOW);
   }   
   
   void GarageDoorOpen()
   {
       digitalWrite(homelinkOne, HIGH);
       //Serial.println("plugThreeOff");
       delay(4000); 
       digitalWrite(homelinkOne, LOW);
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
 
