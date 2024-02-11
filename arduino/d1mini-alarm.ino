/*  ********************************************* 
 *  Birth alarm
 *  Utilizing Sparkfun's ADXL345 Library
 *  
 *  Development Environment Specifics:
 *  Arduino 2.2.1
 *  esp bordmanager version 2.4.2
 *  
 *  Hardware Specifications:
 *  SparkFun ADXL345 on ESP8266 D1 Mini
 * 
 * ADXL345 INT1 on PIN D5 / 15
 *         VNC on 5V
 *         SDA on D2
 *         SCL on D1
 *         GND on GND
 *  *********************************************/

#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          // https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson // install v5.0.8

#include <SparkFun_ADXL345.h>     // SparkFun ADXL345 Library
#include <ESP8266HTTPClient.h>

#include "user_interface.h"       // include required for LIGHT_SLEEP_T

/**
 * debug defnitions
 */
#define DEBUG 1
#if DEBUG == 1
  #define debug(x) Serial.print(x);
  #define debugln(x) Serial.println(x);
#else
  #define debug(x)
  #define debugln(x)
#endif

ADXL345 adxl = ADXL345();             // use adxl with i2c

int interruptPin = D5;                // Setup pin 2 to be the interrupt pin (d0 for most esp Boards)
int actInt = 0;
int inactive = 0;
int batteryPin = A0;
float lowVoltageThreshold = 3.73;     // est. 20% remaining battery cap.

String serverName = "https://push.birth-alarm.com/message";

// Set web server port number to 80
WiFiServer server(80);
//flag for saving data
bool shouldSaveConfig = false;
// Auxiliar variables to store the current output state
String outputState = "off";
// Assign output variables to GPIO pins
char output[2] = "5";
char pushToken[20];
char horseName[40];
// Variable to store the HTTP request
String header;

// milliseconds, to pause between push calls
unsigned long lastTime = 0;
unsigned long timerDelay = 2500;

int messageSent = 0;
int activeCylces = 0;
int sleepAfterXActiveCycles = 10;

void ADXL_ISR();

/**
 * builds wifi connection
 */
void connectToWifi() {
  debugln("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    debug(".");
  }
  debugln("");
  debug("Connected to WiFi network with IP Address: ");
  debugln(WiFi.localIP());
}

/**
 * to be calledin loop
 */
void sendAlarm(String alarmType) {
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClientSecure client;
      HTTPClient http;

      client.setInsecure();

      String serverPath = serverName + "?token=" + pushToken + "&";
      
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath);
      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded", false, true);
      // Data to send with HTTP POST
      String httpRequestData = "Error";
      debugln(alarmType);
      if(alarmType == "birthalarm") {
        String horseNameS = "";
        horseNameS.concat(horseName);
        String titleMessageParams = "title=Geburt-Alarm!&message=";
        httpRequestData = titleMessageParams + horseNameS + " hat sich hin gelegt!&priority=10&";
      }
      if(alarmType == "lowbattery") {
        String horseNameS = "";
        horseNameS.concat(horseName);
        String titleMessageParams = "title=Niedriger Akkustand!&message=";
        httpRequestData = titleMessageParams + "Bitte lade den Akku für " + horseNameS + " Geburtenmelder&priority=10&";
      }
      // Send HTTP POST request
      debug(serverPath);
      debug(httpRequestData);
      int httpResponseCode = http.POST(httpRequestData);
      if (httpResponseCode > 0) {
        debug("HTTP Response code: ");
        debugln(httpResponseCode);
        String payload = http.getString();
        debugln(payload);
      }
      else {
        debug("Error code: ");
        debugln(httpResponseCode);
      }
      // Free resources
      http.end();
      messageSent = 1;
      // clear active cylcles
      activeCylces = 0;
    } else {
      debugln("WiFi Disconnected - reconnect");
      WiFi.forceSleepWake();
      WiFi.begin();
      connectToWifi();
    }
  }
}

void setupGyroSensor() {
  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(4);            // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 
   
  adxl.setActivityXYZ(1, 0, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(125);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)
 
  adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(70);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 0); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
 
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(200);       // 1.25 ms per increment
 
  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment
 
  // Setting all interupts to take place on INT1 pin
  adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.
  
  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(0);
  adxl.doubleTapINT(0);
  adxl.singleTapINT(0);
  
  attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);
}

void setup(){
  Serial.begin(9600);
  delay(1000);
  debugln("------------");
  debugln("BIRTH ALARM");
  debugln("------------");

  setupWifiManager();
  delay(2500);
  setupGyroSensor();
}

/****************** MAIN CODE ******************/
/*     Accelerometer Readings and Interrupt    */
void loop() {
  if(inactive == 1) {
    debugln("sleep now...");
    inactive = 3; // sleep mode
    lightSleep();
  }

  if(inactive == 0) {
    delay(2500);
    // restet inactive state
    // or go to sleep if we reached max active cycles
    if(messageSent == 1 || activeCylces > sleepAfterXActiveCycles) {
      debugln("*** INACTIVITY ***");
      //add code here to do when inactivity is sensed
      inactive = 1;
      lastTime = millis();
      messageSent = 0;
      activeCylces = 0;
    }

    debugln("*** ACTIVITY ***"); 
    // Accelerometer Readings
    int x,y,z;   
    adxl.readAccel(&x, &y, &z);         // Read the accelerometer values and store them in variables declared above x,y,z
    debugln(x)

    // send alarm
    if(x > 94 || x < -94) {
      // send alarm
      sendAlarm("birthalarm");
    } else {
      // increment active cylces to prevent hanging active when not reaching thresholds
      activeCylces = activeCylces + 1;
    }
    //add code here to do when activity is sensed
    checkBattery();
  }
}

void lightSleep(){
  WiFi.mode(WIFI_OFF);
  delay(100);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  gpio_pin_wakeup_enable(interruptPin, GPIO_PIN_INTR_HILEVEL);
  wifi_fpm_do_sleep(0xFFFFFFF);
}

void ICACHE_RAM_ATTR ADXL_ISR() {
  byte interrupts = adxl.getInterruptSource();
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)) {
    debugln("wake!")
    inactive = 0;
  }
}

//callback notifying us of the need to save config for wifi
void saveConfigCallback () {
  debugln("Should save config");
  shouldSaveConfig = true;
}

void checkBattery() {
  int rawValue = analogRead(batteryPin);
  float voltage = 4.2 * (rawValue/ 1023.0);  // calculate voltage: ref v / 10bit 
  debugln("Current battery voltage: " + String(voltage) + "V");

  if (voltage < lowVoltageThreshold) {
    debugln("send warning for low voltage!");
    sendAlarm("lowbattery");
  }
}

void setupWifiManager() {
  //read configuration from FS json
  debugln("mounting FS...");

  if (SPIFFS.begin()) {
    debugln("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      debugln("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        debugln("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          debugln("\nparsed json");
          strcpy(output, json["output"]);
          strcpy(pushToken, json["pushToken"]);
          strcpy(horseName, json["horseName"]);
        } else {
          debugln("failed to load json config");
        }
      }
    }
  } else {
    debugln("failed to mount FS");
  }
  //end read
  WiFiManagerParameter custom_output("output", "output", output, 2);
  WiFiManagerParameter custom_pushToken("pushToken", "Push token", pushToken, 20);
  WiFiManagerParameter custom_horseName("horseName", "Horse name", horseName, 40);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  // set custom ip for portal
  //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_output);
  wifiManager.addParameter(&custom_pushToken);
  wifiManager.addParameter(&custom_horseName);
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("AutoConnectAP");
  // or use this for auto generated name ESP + ChipID
  //wifiManager.autoConnect();
  
  // if you get here you have connected to the WiFi
  debugln("Connected.");
  
  strcpy(output, custom_output.getValue());
  strcpy(pushToken, custom_pushToken.getValue());
  strcpy(horseName, custom_horseName.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    debugln("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["output"] = output;
    json["pushToken"] = pushToken;
    json["horseName"] = horseName;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      debugln("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  
  server.begin();
}
