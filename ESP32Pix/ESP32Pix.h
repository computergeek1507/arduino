#ifndef ESP32PIX_H_
#define ESP32PIX_H_

#define CONFIG_MAX_SIZE 4096
#define HTTP_PORT 80

#define NUM_PORTS 8
#define LED_PIN 4

const char SOFT_NAME[] = "ESP32PIX";
const char SOFT_VER[] = "0.1";
const char CONFIG_FILE_NAME[] = "/config.json";
const char WIFI_FILE_NAME[] = "/wifi.json";

typedef struct {
unsigned int  pixCount = 50;
unsigned int startUni = 1;
unsigned int startChan = 1;
unsigned int endUni = 1;
unsigned int endChan = 150;
unsigned int brightness = 100;
} portConfig;

typedef struct {
String apName = "ESP32PIX";
String apPass = "12345678";
String hostName = "ESP32PIX";
String ethIPAddress = "192.168.1.206";
String ethIPSubmask = "255.255.255.0";
String ethIPGateway = "192.168.1.1";
unsigned int univStart = 1;
unsigned int univCount = 9;
unsigned int univSize = 510;
unsigned int serialUni = 9;
unsigned int testMode = 0;
unsigned int inputMode = 0;

portConfig ports[NUM_PORTS];

} espConfig;

espConfig configData;
unsigned int packets = 0;

void saveConfig();

#endif 
