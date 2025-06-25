#include <SPI.h>
#include <LoRa.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h> 
#define JSON_CONFIG_FILE "/test_config.json"

#include <Keypad.h>

#include <Preferences.h>
Preferences preferences;

#include <HTTPClient.h>

#define WM_NOHELP
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
WiFiManager wm;
WiFiManagerParameter custom_fpp_server("server", "FPP Master", "192.168.5.198", 15);
WiFiManagerParameter custom_subnet("subnet", "Controller Subnet ie'192.168.5'", "192.168.5", 11);
WiFiManagerParameter custom_playlist("playlist", "Playlist to start", "Main", 100);

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

const byte ROWS = 4; // rows
const byte COLS = 3; // columns
//define the symbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 22, 23, 17}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {32, 33, 12}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

String inputText;

String FPP_server;
String controller_subnet;
String playlist;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  preferences.begin("LoRa-KEYPAD", false);

  FPP_server = preferences.getString("fpp_server", "192.168.5.198");
  if(!FPP_server.isEmpty()){
    custom_fpp_server.setValue(FPP_server.c_str(),15);
  }
  controller_subnet = preferences.getString("controller_subnet", "192.168.5");
  if(!controller_subnet.isEmpty()){
    custom_subnet.setValue(controller_subnet.c_str(),11);
  }
  playlist = preferences.getString("playlist", "Main");
  if(!playlist.isEmpty()){
    custom_playlist.setValue(playlist.c_str(),100);
  }

  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Controller Keypad");
  display.display();

  Serial.println("Controller Keypad");

  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
  //WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP    
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
    wm.addParameter(&custom_fpp_server);
    wm.addParameter(&custom_subnet);
    wm.addParameter(&custom_playlist);
    wm.setConfigPortalBlocking(false);
    wm.setParamsPage(true);
    std::vector<const char *> menuIds  = {"wifi","param","info","update"};
    wm.setMenu(menuIds);
    wm.setTitle("Controller Keypad");
    wm.setSaveParamsCallback(saveParamsCallback);

    //automatically connect using saved credentials if they exist
    //If connection fails it starts an access point with the specified name
    if(wm.autoConnect("KeypadAP")){
        display.setCursor(0,20);
        display.print("Wifi Connected " + wm.getWiFiSSID(false));
        display.setCursor(0,30);
        display.print("Wifi IP " + WiFi.localIP().toString());
        display.display();
        wm.startWebPortal();
    }
    else {
        display.setCursor(0,20);
        display.print("Wifi Portal started" + wm.getWiFiSSID(false));
        display.display();
    }
}

void loop() {
  char key1 = keypad.getKey();

  if (key1 != NO_KEY ){
    if(key1 == '*') {
      inputText = "";
      DisplayText();
    } else if(key1 == '#' ) {
      SendText();
      inputText = "";
    } else {
      inputText += key1;
      DisplayText();
    }
  }
  wm.process();
  delay(10);
}
void DisplayText()
{
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Controller Keypad");
  display.setTextSize(1);
  display.setCursor(0,30);
  display.print(inputText);
  display.display();
}

void SendText()
{
  Serial.print("Sending packet: ");
  Serial.println(inputText);

  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(inputText);
  LoRa.endPacket();
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Controller Keypad");
  display.setCursor(0,40);
  display.setTextSize(1);
  display.print("LoRa packet sent:" + inputText);
  //display.setCursor(0,30);
  //display.print(inputText);
  String message;
  if(SendWIFICommand(inputText, message)) {
    display.setCursor(0,50);
    display.setTextSize(1);
    display.print("WIFI packet sent:" + inputText);
  }else{
    display.setCursor(0,50);
    display.setTextSize(1);
    display.print("WIFI Failed " + message);
  }
  display.display();
}

void saveParamsCallback () {
  Serial.println("Get Params:");
  Serial.print(custom_fpp_server.getID());
  Serial.print(" : ");
  Serial.println(custom_fpp_server.getValue());
  FPP_server = custom_fpp_server.getValue();
  preferences.putString("fpp_server", FPP_server);

  Serial.println("Get Params:");
  Serial.print(custom_subnet.getID());
  Serial.print(" : ");
  Serial.println(custom_subnet.getValue());
  controller_subnet = custom_subnet.getValue();
  preferences.putString("controller_subnet", controller_subnet);

  playlist = custom_playlist.getValue();
  preferences.putString("playlist", playlist);
}

bool SendWIFICommand ( String number, String &message )
{
  if(WiFi.status() != WL_CONNECTED) {
    message = "Wifi Not Connected";
    return false;
  }
  if(FPP_server.isEmpty()){
    Serial.println("FPP_server is empty");
    message = "FPP_server is empty";
    return false;
  }

  if(controller_subnet.isEmpty()){
    Serial.println("controller_subnet is empty");
    message = "controller_subnet is empty";
    return false;
  }
  if(number == "400")
  {
    return StartPlaylist();
  }
  if(number == "500")
  {
    return StopPlaylist();
  }
  if(number == "501")
  {
    return StopGracefullyPlaylist();
  }
  if(number == "999")
  {
    return TurnOffAllController();
  }
  if(number == "998")
  {
    return TurnOnAllController();
  }
  //http://192.168.5.198/api/command/Controller%20Set%20Test%20Mode%20Off%20Auto/192.168.8.179
  return ToggleController(number);
}

bool ToggleController ( String number )
{
  //Controller Set All Test Mode Off
  //http://192.168.5.198/api/command/Controller%20Toggle%20Test%20Mode/192.168.8.179
  String url = "http://" + FPP_server + "/api/command/Controller%20Toggle%20Test%20Mode/" + controller_subnet + "." + number;
  return PostDataToFPP(url);
}

bool TurnOnAllController ()
{
  //Controller Set All Test Mode Off
  String url = "http://" + FPP_server + "/api/command/Controller%20Set%20All%20Test%20Mode%20On";
  return PostDataToFPP(url);
}

bool TurnOffAllController ()
{
  //Controller Set All Test Mode Off
  String url = "http://" + FPP_server + "/api/command/Controller%20Set%20All%20Test%20Mode%20Off";
  return PostDataToFPP(url);
}

bool StartPlaylist ()
{
  //Controller Set All Test Mode Off
  String url = "http://" + FPP_server + "/api/playlist/"+ playlist +"/start";
  return PostDataToFPP(url);
}

bool StopPlaylist ()
{
  String url = "http://" + FPP_server + "/api/playlists/stop";
  return PostDataToFPP(url);
}

bool StopGracefullyPlaylist ()
{
  String url = "http://" + FPP_server + "/api/playlists/stopgracefully";
  return PostDataToFPP(url);
}

bool PostDataToFPP ( String httpRequestData )
{
  //http://192.168.5.198/api/command/Controller%20Set%20Test%20Mode%20Off%20Auto/192.168.8.179
  //WiFiClient client;
  HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      //http.begin(client, serverName);
    // Your Domain name with URL path or IP address with path
  http.begin(httpRequestData.c_str());

  // If you need Node-RED/server authentication, insert user and password below
  //http.setAuthorization("REPLACE_WITH_SERVER_USERNAME", "REPLACE_WITH_SERVER_PASSWORD");

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
  return httpResponseCode == 200;
}
/*
void saveConfigFile()
// Save Config in JSON format
{
  Serial.println(F("Saving configuration..."));
  
  // Create a JSON document
  StaticJsonDocument<512> json;
  json["testString"] = testString;
  json["testNumber"] = testNumber;
 
  // Open config file
  File configFile = SPIFFS.open(JSON_CONFIG_FILE, "w");
  if (!configFile)
  {
    // Error, file did not open
    Serial.println("failed to open config file for writing");
  }
 
  // Serialize JSON data to write to file
  serializeJsonPretty(json, Serial);
  if (serializeJson(json, configFile) == 0)
  {
    // Error writing file
    Serial.println(F("Failed to write to file"));
  }
  // Close file
  configFile.close();
}
 
bool loadConfigFile()
// Load existing configuration file
{
  // Uncomment if we need to format filesystem
  // SPIFFS.format();
 
  // Read configuration from FS json
  Serial.println("Mounting File System...");
 
  // May need to make it begin(true) first time you are using SPIFFS
  if (SPIFFS.begin(false) || SPIFFS.begin(true))
  {
    Serial.println("mounted file system");
    if (SPIFFS.exists(JSON_CONFIG_FILE))
    {
      // The file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open(JSON_CONFIG_FILE, "r");
      if (configFile)
      {
        Serial.println("Opened configuration file");
        StaticJsonDocument<512> json;
        DeserializationError error = deserializeJson(json, configFile);
        serializeJsonPretty(json, Serial);
        if (!error)
        {
          Serial.println("Parsing JSON");
 
          strcpy(testString, json["testString"]);
          testNumber = json["testNumber"].as<int>();
 
          return true;
        }
        else
        {
          // Error loading JSON data
          Serial.println("Failed to load json config");
        }
      }
    }
  }
  else
  {
    // Error mounting file system
    Serial.println("Failed to mount FS");
  }
 
  return false;
}
*/