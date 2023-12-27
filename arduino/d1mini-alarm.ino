#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>

// Assign a unique ID to this sensor at the same time
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);


const char* ssid = "YOUR ROUTER SSID";
const char* password = "YOUR-WLAN-PASSWORD";
const char* gotifyAppToken = "YOUR APP TOKEN FROM GOTIFY"

//Your Domain name with URL path or IP address with path
String serverName = "http://your-gotify-host.com/message";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
//unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;


void displaySensorDetails(void)
{
  sensor_t sensor;
  accel.getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.print ("Sensor: "); Serial.println(sensor.name);
  Serial.print ("Driver Ver: "); Serial.println(sensor.version);
  Serial.print ("Unique ID: "); Serial.println(sensor.sensor_id);
  Serial.print ("Max Value: "); Serial.print(sensor.max_value); Serial.println(" m/s^2");
  Serial.print ("Min Value: "); Serial.print(sensor.min_value); Serial.println(" m/s^2");
  Serial.print ("Resolution: "); Serial.print(sensor.resolution); Serial.println(" m/s^2"); 
  Serial.println("------------------------------------");
  Serial.println("");
  delay(500);
}

void displayDataRate(void)
{
  Serial.print ("Data Rate: "); 

  switch(accel.getDataRate())
  {
    case ADXL345_DATARATE_3200_HZ:
      Serial.print ("3200 "); 
      break;
    case ADXL345_DATARATE_1600_HZ:
      Serial.print ("1600 "); 
      break;
    case ADXL345_DATARATE_800_HZ:
      Serial.print ("800 "); 
      break;
    case ADXL345_DATARATE_400_HZ:
      Serial.print ("400 "); 
      break;
    case ADXL345_DATARATE_200_HZ:
      Serial.print ("200 "); 
      break;
    case ADXL345_DATARATE_100_HZ:
      Serial.print ("100 "); 
      break;
    case ADXL345_DATARATE_50_HZ:
      Serial.print ("50 "); 
      break;
    case ADXL345_DATARATE_25_HZ:
      Serial.print ("25 "); 
      break;
    case ADXL345_DATARATE_12_5_HZ:
      Serial.print ("12.5 "); 
      break;
    case ADXL345_DATARATE_6_25HZ:
      Serial.print ("6.25 "); 
      break;
    case ADXL345_DATARATE_3_13_HZ:
      Serial.print ("3.13 "); 
      break;
    case ADXL345_DATARATE_1_56_HZ:
      Serial.print ("1.56 "); 
      break;
    case ADXL345_DATARATE_0_78_HZ:
      Serial.print ("0.78 "); 
      break;
    case ADXL345_DATARATE_0_39_HZ:
      Serial.print ("0.39 "); 
      break;
    case ADXL345_DATARATE_0_20_HZ:
      Serial.print ("0.20 "); 
      break;
    case ADXL345_DATARATE_0_10_HZ:
      Serial.print ("0.10 "); 
      break;
    default:
      Serial.print ("???? "); 
      break;
  } 
  Serial.println(" Hz"); 
}

void displayRange(void)
{
  Serial.print ("Range: +/- "); 

  switch(accel.getRange())
  {
    case ADXL345_RANGE_16_G:
      Serial.print ("16 "); 
      break;
    case ADXL345_RANGE_8_G:
      Serial.print ("8 "); 
      break;
    case ADXL345_RANGE_4_G:
      Serial.print ("4 "); 
      break;
    case ADXL345_RANGE_2_G:
      Serial.print ("2 "); 
      break;
    default:
      Serial.print ("?? "); 
      break;
  } 
  Serial.println(" g"); 
}

int WLANLED = 15;
int ALARMLED = 13;

void setup(void) 
{
  
  pinMode(WLANLED, OUTPUT);
  pinMode(ALARMLED, OUTPUT);
  digitalWrite(WLANLED, LOW);
  digitalWrite(ALARMLED, LOW);


 Serial.begin(9600);
 
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  digitalWrite(WLANLED, HIGH);

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");

  
  Serial.println("Accelerometer Test"); Serial.println("");
 
  /// Initialise the sensor
  if(!accel.begin())
  {
    // There was a problem detecting the ADXL345 ... check your connections
    Serial.println("Ooops, no ADXL345 detected ... Check your wiring!");
    while(1);
  }

  /* Set the range to whatever is appropriate for your project */
  accel.setRange(ADXL345_RANGE_16_G);
  // displaySetRange(ADXL345_RANGE_8_G);
  // displaySetRange(ADXL345_RANGE_4_G);
  // displaySetRange(ADXL345_RANGE_2_G);

  // Display some basic information on this sensor
  displaySensorDetails();

  /* Display additional settings (outside the scope of sensor_t) */
  displayDataRate();
  displayRange();
  Serial.println("");
}

void loop(void) 
{
  /* Get a new sensor event */ 
  sensors_event_t event; 
  accel.getEvent(&event);

  // Send an HTTP POST request depending on timerDelay
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      if(event.acceleration.x < -5 || event.acceleration.x > 5) {
        digitalWrite(ALARMLED, HIGH);
        WiFiClientSecure client;
        HTTPClient http;

        // this should later be replaced with te root certificate of your host
        client.setInsecure();

        String serverPath = serverName + "?token=" + gotifyAppToken + "&";
        
        // Your Domain name with URL path or IP address with path
        http.begin(client, serverPath);
        // Specify content-type header
        http.addHeader("Content-Type", "application/x-www-form-urlencoded", false, true);
        // Data to send with HTTP POST
        String httpRequestData = "title=Birth Alarm&message=Check your horse&";
        // Send HTTP POST request
        Serial.print(serverPath);
        int httpResponseCode = http.POST(httpRequestData);
        Serial.print(httpRequestData);
        if (httpResponseCode > 0) {
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
      } else {
        digitalWrite(ALARMLED, LOW);
      }
    }
    else {
      Serial.println("WiFi Disconnected");
      
      digitalWrite(WLANLED, LOW);
    }
    lastTime = millis();
  }

  /* Display the results (acceleration is measured in m/s^2) */
  Serial.print("X: "); Serial.print(event.acceleration.x); Serial.print(" ");
  Serial.print("Y: "); Serial.print(event.acceleration.y); Serial.print(" ");
  Serial.print("Z: "); Serial.print(event.acceleration.z); Serial.print(" ");Serial.println("m/s^2 ");
  delay(500);
}