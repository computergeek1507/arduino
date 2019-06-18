#ifndef ESP32PIX_H_
#define ESP32PIX_H_

#define CONFIG_MAX_SIZE 4096
#define HTTP_PORT 80

#define NUM_PORTS 8
#define LED_PIN 4

const char SOFT_NAME[] = "ESP32PIX";
const char SOFT_VER[] = "0.1";
const char CONFIG_FILE_NAME[] = "/config.json";

typedef struct {
unsigned int  pixCount;
String ethIPAddress;
String ethIPSubmask;
String ethIPGateway;
unsigned int startUni;
unsigned int startChan;
unsigned int endUni;
unsigned int endChan;
} portConfig;

typedef struct {
String ethIPAddress;
String ethIPSubmask;
String ethIPGateway;
unsigned int univStart;
unsigned int univCount;
unsigned int univSize = 510;
unsigned int brightness = 100;

portConfig ports[NUM_PORTS];

} espConfig;

espConfig configData;
unsigned int packets = 0;

#endif 