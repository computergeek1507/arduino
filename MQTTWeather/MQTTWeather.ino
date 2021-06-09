#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

#define MQTT_CLIENTID "weatherArduino"
//#define MQTT_SERVER "192.168.5.148"
#define MQTT_WILLTOPIC  "clients/weatherArduino/"
#define MQTT_WILLMESSAGE  "dead"
#define TRUE    (1)

// Update these with values suitable for your network.
byte mac[] = { 0x00, 0xA2, 0xDA, 0x00, 0x6C, 0x62 };
//byte server[] = { 192, 168, 5, 148 };
IPAddress ip(192,168,5,127);
IPAddress server(192,168,5,148);

const int StatLED = 13;

const byte WSPEED = 3;

// analog I/O pins
const byte WDIR = A0;

// analog I/O pins
const byte RG11_Pin = 2;

#define Bucket_Size 0.01 // bucket size to trigger tip count

//Global Variables
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
long lastSecond; //The millis counter to see when a second rolls by
byte seconds; //When it hits 60, increase the current minute
byte seconds_2m; //Keeps track of the "wind speed/dir avg" over last 2 minutes array of data
byte minutes; //Keeps track of where we are in various arrays of data
byte minutes_10m; //Keeps track of where we are in wind gust/dir over last 10 minutes array of data


volatile unsigned long tipCount; // rain bucket tip counter used in interrupt routine
volatile unsigned long globalTipCount; 

volatile unsigned long lastTipCount;

volatile unsigned long contactTime; // timer to manage any rain contact bounce in interrupt routine

long lastWindCheck = 0;
volatile long lastWindIRQ = 0;
volatile byte windClicks = 0;

byte windspdavg[120]; //120 bytes to keep track of 2 minute average

#define WIND_DIR_AVG_SIZE 120
int winddiravg[WIND_DIR_AVG_SIZE]; //120 ints to keep track of 2 minute average
float windgust_10m[10]; //10 floats to keep track of 10 minute max
int windgustdirection_10m[10]; //10 ints to keep track of 10 minute max

//These are all the weather values that wunderground expects:
int winddir = 0; // [0-360 instantaneous wind direction]
float windspeedmph = 0; // [mph instantaneous wind speed]

int windADC = 0; 

float windgustmph = 0; // [mph current wind gust, using software specific time period]
int windgustdir = 0; // [0-360 using software specific time period]
float windspdmph_avg2m = 0; // [mph 2 minute average wind speed mph]
int winddir_avg2m = 0; // [0-360 2 minute average wind direction]
float windgustmph_10m = 0; // [mph past 10 minutes wind gust mph ]
int windgustdir_10m = 0; // [0-360 past 10 minutes wind gust direction]

float rainpermin = 0; 

EthernetClient ethClient;
PubSubClient client(server, 1883, ethClient);

void wspeedIRQ()
// Activated by the magnet in the anemometer (2 ticks per rotation), attached to input D3
{
    if (millis() - lastWindIRQ > 15) // Ignore switch-bounce glitches less than 10ms (142MPH max reading) after the reed switch closes
    {
        lastWindIRQ = millis(); //Grab the current time
        windClicks++; //There is 1.492MPH for each click per second.
    }
}

// Interrupt handler routine that is triggered when the rg-11 detects rain
void isr_rg() {

  if((millis() - contactTime) > 15 ) { // debounce of sensor signal
    tipCount++;
    globalTipCount++;
    contactTime = millis();
  }
}

void setup() 
{
  //Serial.begin(9600);

 // pinMode(6,OUTPUT);

//Serial.println("starting");
    //crappy seeed studio ethernet shield needs to be manually reset
 // digitalWrite(6,LOW);  //Reset W5200
 // delay(200);
 // digitalWrite(6,HIGH);  
 //delay(500);       // wait W5200 work
 tipCount = 0;
 lastTipCount=0;

   pinMode(StatLED, OUTPUT);  // LED 
  digitalWrite(StatLED, LOW);

  pinMode(WSPEED, INPUT_PULLUP); // input from wind meters windspeed sensor
  
    pinMode(RG11_Pin, INPUT_PULLUP);
  
      seconds = 0;
    lastSecond = millis();  
  char willtopic[128] = MQTT_WILLTOPIC;
  Ethernet.begin(mac, ip);
   //Serial.println(Ethernet.localIP());
  
 if (client.connect(MQTT_CLIENTID, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE))
 {
    client.publish(willtopic, "Online!");
    digitalWrite(StatLED, HIGH);
  }  

    attachInterrupt(digitalPinToInterrupt(RG11_Pin), isr_rg, FALLING);

//attachInterrupt(digitalPinToInterrupt(WindSensor_Pin), isr_rotation, FALLING);
    attachInterrupt(digitalPinToInterrupt(WSPEED), wspeedIRQ, FALLING);

    // turn on interrupts
    interrupts();
    //Serial.println("Weather Shield online!");
}

void loop() 
{
  if(!client.connected())
  {
    char willtopic[128] = MQTT_WILLTOPIC;
    client.connect(MQTT_CLIENTID, willtopic, MQTTQOS2, 1, MQTT_WILLMESSAGE);  
    client.publish(willtopic, "Online:");
    digitalWrite(StatLED, HIGH);  
  }

  if(millis() - lastSecond >= 1000)
    {
        digitalWrite(StatLED, LOW);
        lastSecond += 1000;
        if(++seconds_2m > 119) seconds_2m = 0;
        //Calc the wind speed and direction every second for 120 second to get 2 minute average
        float currentSpeed = get_wind_speed();
        windspeedmph = currentSpeed;

        int currentDirection = get_wind_direction();
        windspdavg[seconds_2m] = (int)currentSpeed;
        winddiravg[seconds_2m] = currentDirection;
        //if(seconds_2m % 10 == 0) displayArrays(); //For testing

        //Check to see if this is a gust for the minute
        if(currentSpeed > windgust_10m[minutes_10m])
        {
            windgust_10m[minutes_10m] = currentSpeed;
            windgustdirection_10m[minutes_10m] = currentDirection;
        }

        //Check to see if this is a gust for the day
        if(currentSpeed > windgustmph)
        {
            windgustmph = currentSpeed;
            windgustdir = currentDirection;
        }

        if(++seconds > 59)
        {
            seconds = 0;
            if(++minutes > 59) minutes = 0;
            if(++minutes_10m > 9) minutes_10m = 0;
            windgust_10m[minutes_10m] = 0; //Zero out this minute's gust
            //Report all readings every minute
            printWeather();
        }
        digitalWrite(StatLED, HIGH); 
 }
client.loop(); 
}

//Calculates each of the variables that wunderground is expecting
void calcWeather()
{
    //Calc winddir
    winddir = get_wind_direction();

    //Calc windspeed
    //windspeedmph = get_wind_speed(); //This is calculated in the main loop

    //Calc windspdmph_avg2m
    float temp = 0;
    for(int i = 0 ; i < 120 ; i++)
        temp += windspdavg[i];
    temp /= 120.0;
    windspdmph_avg2m = temp;
	
	lastTipCount = tipCount;
	rainpermin = tipCount * Bucket_Size;
	tipCount = 0;

    //Calc winddir_avg2m, Wind Direction
    //You can't just take the average. Google "mean of circular quantities" for more info
    //We will use the Mitsuta method because it doesn't require trig functions
    //And because it sounds cool.
    //Based on: http://abelian.org/vlf/bearings.html
    //Based on: http://stackoverflow.com/questions/1813483/averaging-angles-again
    long sum = winddiravg[0];
    int D = winddiravg[0];
    for(int i = 1 ; i < WIND_DIR_AVG_SIZE ; i++)
    {
        int delta = winddiravg[i] - D;

        if(delta < -180)
            D += delta + 360;
        else if(delta > 180)
            D += delta - 360;
        else
            D += delta;

        sum += D;
    }
    winddir_avg2m = sum / WIND_DIR_AVG_SIZE;
    if(winddir_avg2m >= 360) winddir_avg2m -= 360;
    if(winddir_avg2m < 0) winddir_avg2m += 360;

    //Calc windgustmph_10m
    //Calc windgustdir_10m
    //Find the largest windgust in the last 10 minutes
    windgustmph_10m = 0;
    windgustdir_10m = 0;
    //Step through the 10 minutes
    for(int i = 0; i < 10 ; i++)
    {
        if(windgust_10m[i] > windgustmph_10m)
        {
            windgustmph_10m = windgust_10m[i];
            windgustdir_10m = windgustdirection_10m[i];
        }
    }
}

//Returns the instataneous wind speed
float get_wind_speed()
{
    float deltaTime = millis() - lastWindCheck; //750ms

    deltaTime /= 1000.0; //Covert to seconds

    float windSpeed = (float)windClicks / deltaTime; //3 / 0.750s = 4

    windClicks = 0; //Reset and start watching for new wind
    lastWindCheck = millis();

    windSpeed *= 1.492; //4 * 1.492 = 5.968MPH

    /* Serial.println();
     Serial.print("Windspeed:");
     Serial.println(windSpeed);*/

    return(windSpeed);
}

//Read the wind direction sensor, return heading in degrees
int get_wind_direction()
{
    //int adc;

    int adc = analogRead(WDIR); // get the current reading from the sensor

    // The following table is ADC readings for the wind direction sensor output, sorted from low to high.
    // Each threshold is the midpoint between adjacent headings. The output is degrees for that ADC reading.
    // Note that these are not in compass degree order! See Weather Meters datasheet for more information.
    windADC = adc;

    if (adc < 380) return (113);
    if (adc < 393) return (68);
    if (adc < 414) return (90);
    if (adc < 456) return (158);
    if (adc < 508) return (135);
    if (adc < 551) return (203);
    if (adc < 615) return (180);
    if (adc < 680) return (23);
    if (adc < 746) return (45);
    if (adc < 801) return (248);
    if (adc < 833) return (225);
    if (adc < 878) return (338);
    if (adc < 913) return (0);
    if (adc < 940) return (293);
    if (adc < 967) return (315);
    if (adc < 990) return (270);
    return (-1); // error, disconnected?
}

//Prints the various variables directly to the port
//I don't like the way this function is written but Arduino doesn't support floats under sprintf
void printWeather()
{
    calcWeather(); //Go calc all the various sensors
    SendWindDirection();
    SendWindSpeedMPH();
    SendWindGustDirection();
    SendWindGustSpeedMPH();
    SendWindSpeedMPHavg2m();
    SendWindDirectionavg2m();
    SendWindGustDirection10m();
    SendWindGustSpeedMPH10m();
    SendWindADC();
    SendRainPerMin();
    SendRainCount();
    SendRaining();
    SendGlobalRainCount();
}

void SendWindADC() 
{
    char windADCChar[16]; 
    itoa(windADC, windADCChar, 10);
    client.publish("arduino/weather/windadc", windADCChar);
    //Serial.print("windadc:");
    //Serial.println(windADCChar);
}

void SendWindDirection() 
{
    char windDirChar[16]; 
    itoa(winddir, windDirChar, 10);
    client.publish("arduino/weather/winddir", windDirChar);
    //Serial.print("winddir:");
    //Serial.println(windDirChar);
}

void SendWindSpeedMPH() 
{
   char winSpeedChar[256]; 
   dtostrf(windspeedmph, 3, 2, winSpeedChar);
   client.publish("arduino/weather/windspeedmph", winSpeedChar);
   //Serial.print("windspeedmph:");
   //Serial.println(winSpeedChar);
}

void SendWindGustDirection() 
{
    char winDirChar[16]; 
    itoa(windgustdir, winDirChar, 10);
    client.publish("arduino/weather/windgustdir", winDirChar);
    //Serial.print("windgustdir:");
    //Serial.println(winDirChar);
}

void SendWindGustSpeedMPH() 
{
   char winSpeedChar[256]; 
   dtostrf(windgustmph, 3, 2, winSpeedChar);
   client.publish("arduino/weather/windgustspeedmph", winSpeedChar);
   //Serial.print("windgustmph:");
   //Serial.println(winSpeedChar);
}

void SendWindSpeedMPHavg2m() 
{
   char winSpeedChar[256]; 
   dtostrf(windspdmph_avg2m, 3, 2, winSpeedChar);
   client.publish("arduino/weather/windspeedmphavg2m", winSpeedChar);
   //Serial.print("windspdmph_avg2m:");
   //Serial.println(winSpeedChar);
}

void SendWindDirectionavg2m() 
{
    char winDirChar[16]; 
    itoa(winddir_avg2m, winDirChar, 10);
    client.publish("arduino/weather/winddiravg2m", winDirChar);
    //Serial.print("winddir_avg2m:");
    //Serial.println(winDirChar);
}

void SendWindGustDirection10m() 
{
    char winDirChar[16]; 
    itoa(windgustdir_10m, winDirChar, 10);
    client.publish("arduino/weather/windgustdir10m", winDirChar);
    //Serial.print("windgustdir10m:");
    //Serial.println(winDirChar);
}

void SendWindGustSpeedMPH10m() 
{
   char winSpeedChar[256]; 
   dtostrf(windgustmph_10m, 3, 2, winSpeedChar);
   client.publish("arduino/weather/windgustspeedmph10m", winSpeedChar);
   //Serial.print("windgustspeedmph10m:");
   //Serial.println(winSpeedChar);
}

void SendRainPerMin() 
{
   char rainChar[256]; 
   dtostrf(rainpermin, 3, 2, rainChar);
   client.publish("arduino/weather/rainpermin", rainChar);
   //Serial.print("windspdmph_avg2m:");
   //Serial.println(winSpeedChar);
}

void SendRainCount() 
{
    char rainChar[16]; 
    itoa(lastTipCount, rainChar, 10);
    client.publish("arduino/weather/raincount", rainChar);
    //Serial.print("windgustdir:");
    //Serial.println(winDirChar);
}

void SendGlobalRainCount() 
{
    char rainChar[16]; 
    itoa(globalTipCount, rainChar, 10);
    client.publish("arduino/weather/globalraincount", rainChar);
    //Serial.print("windgustdir:");
    //Serial.println(winDirChar);
}

void SendRaining() 
{
   if(lastTipCount>0)
   {

      client.publish("arduino/weather/raining", "ON");
   }
  else
  {
    client.publish("arduino/weather/raining", "OFF");
  }
 
    //Serial.print("windgustdir:");
    //Serial.println(winDirChar);
}
