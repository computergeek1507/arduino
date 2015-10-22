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


int prevSensorValue = 0;        // value read from the pot

void setup() {
  pinMode(led, OUTPUT);
  pinMode(homelinkOne, OUTPUT);

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
 
