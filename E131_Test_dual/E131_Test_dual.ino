/*
* E131_Test.ino - Simple sketch to listen for E1.31 data on an ESP32 
*                  and print some statistics.
*
* Project: E131 ESP32 Controller
* Copyright (c) 2019 Scott Hanson
*
*  This program is provided free for you to use in any way that you wish,
*  subject to the laws and regulations where you are using it.  Due diligence
*  is strongly suggested before using this code.  Please give credit where due.
*
*  The Author makes no warranty of any kind, express or implied, with regard
*  to this program or the documentation contained in this document.  The
*  Author shall not be liable in any event for incidental or consequential
*  damages in connection with, or arising out of, the furnishing, performance
*  or use of these programs.
*
*/
//#define FASTLED_SHOW_CORE 0
#define FASTLED_ESP32_I2S true
//#include <ESPAsyncE131.h>
#include <E131.h>
#include <FastLED.h>
#include <ETH.h>

TaskHandle_t TaskA, TaskB;

//#define LED_PIN D4
//#define LED_PIN 12

//#define UNIVERSE 1                      // First DMX Universe to listen for
//#define UNIVERSE_COUNT 2                // Total number of Universes to listen for, starting at UNIVERSE

//#define START_CHAN 1                // Total number of Universes to listen for, starting at UNIVERSE
//#define PIXEL_COUNT 170                // Total number of Universes to listen for, starting at UNIVERSE

#define NUM_STRIPS 8
#define NUM_LEDS_PER_STRIP 170

const int universe = 1;
const int universeCount = 8;

//boolean LEDState = true;

E131                e131;

// ESPAsyncE131 instance with UNIVERSE_COUNT buffer slots
//ESPAsyncE131 e131(UNIVERSE_COUNT);

CRGB leds[NUM_STRIPS][NUM_LEDS_PER_STRIP];

    
//int Pins[16]={12,2,4,5,0,13,14,15,16,17,18,19,21,22,23,25};


static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event)
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void setup() {
    Serial.begin(115200);
    delay(10);

    WiFi.onEvent(WiFiEvent);
    ETH.begin();
    
    // Make sure you're in station mode    
    //WiFi.mode(WIFI_STA);

    //Serial.println("");
    //Serial.print(F("Connecting to "));
    //Serial.print(ssid);

    //if (passphrase != NULL)
    //    WiFi.begin(ssid, passphrase);
    //else
    //    WiFi.begin(ssid);

    //while (WiFi.status() != WL_CONNECTED) {
     //   delay(500);
     //   Serial.print(".");
    //}
      pinMode(2, OUTPUT); //set onboard LED to output
  digitalWrite(2, LOW); //turn onboard LED off

    Serial.println("");
    //Serial.print(F("Connected with IP: "));
    //Serial.println(WiFi.localIP());

    e131.initUnicast();
    // Choose one to begin listening for E1.31 data
    //if (e131.begin(E131_UNICAST))                               // Listen via Unicast
    //    Serial.println(F("Listening for data..."));
    //else 
    //    Serial.println(F("*** e131.begin failed ***"));

    FastLED.addLeds<NEOPIXEL, 5>(leds[0], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 4>(leds[1], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 2>(leds[2], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 0>(leds[3], NUM_LEDS_PER_STRIP);
    
    FastLED.addLeds<NEOPIXEL, 13>(leds[4], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 14>(leds[5], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 15>(leds[6], NUM_LEDS_PER_STRIP);
    FastLED.addLeds<NEOPIXEL, 16>(leds[7], NUM_LEDS_PER_STRIP);

    //FastLED.setBrightness(BRIGHTNESS );
    
    //FastLED.addLeds<NEOPIXEL, 16>(leds[8], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 17>(leds[9], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 18>(leds[10], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 19>(leds[11], NUM_LEDS_PER_STRIP);
    
    //FastLED.addLeds<NEOPIXEL, 21>(leds[12], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 22>(leds[13], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 23>(leds[14], NUM_LEDS_PER_STRIP);
    //FastLED.addLeds<NEOPIXEL, 25>(leds[15], NUM_LEDS_PER_STRIP);


//FastLED.addLeds<NEOPIXEL, A4>(leds[15], NUM_LEDS_PER_STRIP); 
PowerUpDone();

 xTaskCreatePinnedToCore(
   Task1,                  /* pvTaskCode */
   "Workload1",            /* pcName */
   1000,                   /* usStackDepth */
   NULL,                   /* pvParameters */
   1,                      /* uxPriority */
   &TaskA,                 /* pxCreatedTask */
   0);  

}

void loop() {

if(e131.parsePacket()) {
  Serial.print("Universe:");
  Serial.println(e131.universe);
        if (e131.universe >= universe && e131.universe < universe + universeCount) {
          int x = e131.universe - universe;
          
            //for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
             //   int j = i * 3;
            //    leds[x][i]=CRGB(e131.data[j], e131.data[j+1], e131.data[j+2]);
            //}
            memcpy(leds[x],e131.data, 510);
            
            digitalWrite(2, HIGH); //turn LED on
        }
    }
  
    digitalWrite(2, LOW);
}

void PowerUpDone() 
{
   LEDS.showColor(CRGB(255, 0, 0)); //turn all pixels on red
   delay(1000);
   LEDS.showColor(CRGB(0, 255, 0)); //turn all pixels on green
   delay(1000);
   LEDS.showColor(CRGB(0, 0, 255)); //turn all pixels on blue
   delay(1000);
   FastLED.clear(); //clear pixels
   FastLED.show();
}


void Task1( void * parameter )
{
 for (;;) {
   LEDS.show();
   //LEDS.delay(10);
   delay(10) ;
 }
}
