#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define CONNECTION_TIMEOUT 10

#define BUZZER_PIN 25

#define WATER_PIN 34
#define FAN_PIN1 19
#define FAN_PIN2 18
#define Y_LED_PIN 12  //Define the yellow led pin to 12
#define GAS_PIN 23
#define PYRO_PIN 14
#define NEO_PIXEL_PIN 26
#define NEO_PIXEL_COUNT 4

// Web server running on port 80
WebServer server(80);

const char *SSID = "DESKTOP-97OMHOF 2648";
const char *PWD = "L613s3%4";

StaticJsonDocument<250> jsonDocument;
char buffer[250];

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Screen and humidity detector
LiquidCrystal_I2C mylcd(0x27,16,2); // I2C address is 0x27, 16 char long, 2 lines

DHT dht(17, DHT11);

//Servo channel
int channel_PWM = 13;
int channel_PWM2 = 10;
int freq_PWM = 50; 
int resolution_PWM = 10;
const int PWM_Pin1 = 5;
const int PWM_Pin2 = 13;

int door_deg = 0;
int door_diff = 0;

// env variable
float temperature;
float humidity;

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);

  int timeout_counter = 0;
  
  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    timeout_counter++;
    if(timeout_counter >= CONNECTION_TIMEOUT*2){
      ESP.restart();
    }
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  mylcd.print(WiFi.localIP());
}

void create_json(char *tag, float value, char *unit) {  
  jsonDocument.clear();  
  jsonDocument["type"] = tag;
  jsonDocument["value"] = value;
  jsonDocument["unit"] = unit;
  serializeJson(jsonDocument, buffer);
}

void add_json_object(char *tag, float value, char *unit) {
  JsonObject obj = jsonDocument.createNestedObject();
  obj["type"] = tag;
  obj["value"] = value;
  obj["unit"] = unit; 
}

void read_sensor_data(void * parameter) {
   for (;;) {
     Serial.println("Read sensor data");

     humidity = dht.readHumidity();

     temperature = dht.readTemperature();


     Serial.println("Temperature : ");
     Serial.println(temperature);

     Serial.println("Humidity : ");
     Serial.println(humidity);

 
     // delay the task
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}

void setup_task() {	 	 
  xTaskCreate(	 	 
  read_sensor_data, 	 	 
  "Read sensor data", 	 	 
  1000, 	 	 
  NULL, 	 	 
  1, 	 	 
  NULL 	 	 
  );	 	 
}

void getTemperature() {
  Serial.println("Get temperature");
  create_json("temperature", temperature, "°C");
  server.send(200, "application/json", buffer);
}
 
void getHumidity() {
  Serial.println("Get humidity");
  create_json("humidity", humidity, "%");
  server.send(200, "application/json", buffer);
}

void openWindow() {
    Serial.println("Open the window");
    ledcWrite(channel_PWM, 120);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 100;
    jsonDocument.clear();
    jsonDocument["windowState"] = "opened";
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void closeWindow() {
    Serial.println("Closed the window");
    ledcWrite(channel_PWM, 60);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 60;
    jsonDocument.clear();
    jsonDocument["windowState"] = "closed";
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void openDoor() {
    Serial.println("Open the door");
    ledcWrite(channel_PWM2, 120);
    jsonDocument.clear();
    jsonDocument["doorState"] = "opened";
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void closeDoor() {
    Serial.println("Close the door");
    ledcWrite(channel_PWM2, 20);
    jsonDocument.clear();
    jsonDocument["doorState"] = "closed";
    serializeJson(jsonDocument, buffer);
    server.send(200, "application/json", buffer);
}

void getEnv() {
  Serial.println("Get env");
  jsonDocument.clear();
  add_json_object("temperature", temperature, "°C");
  add_json_object("humidity", humidity, "%");
  serializeJson(jsonDocument, buffer);
  server.send(200, "application/json", buffer);
}

void handlePost() {
  if (server.hasArg("plain") == false) {
    //handle error here
  }
  String body = server.arg("plain");
  deserializeJson(jsonDocument, body);
  
  // Respond to the client
  server.send(200, "application/json", "{}");
}

void setup_routing() {	 	 
  server.on("/temperature", getTemperature);	 	  	 
  server.on("/humidity", getHumidity);	 	 
  server.on("/env", getEnv);
  server.on("/window/open", openWindow);
  server.on("/window/close", closeWindow);
  server.on("/door/open", openDoor);
  server.on("/door/close", closeDoor);
  server.on("/led", HTTP_POST, handlePost);	 	 
  	 	 
  // start server	 	 
  server.begin();	 	 
}

void setup() {
  Serial.begin(115200);
  mylcd.init();
  mylcd.backlight();
  pinMode(Y_LED_PIN, OUTPUT);
  pinMode(FAN_PIN1, OUTPUT);
  pinMode(FAN_PIN2, OUTPUT);
  ledcSetup(5, 1200, 8);//Set the frequency of LEDC channel 1 to 1200 and PWM resolution to 8, that is duty cycle to 256.  
  ledcAttachPin(FAN_PIN2, 5);  //Bind the LEDC channel 1 to the specified left motor pin GPIO26 for output.  
  pinMode(WATER_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GAS_PIN, INPUT);
  pinMode(PYRO_PIN, INPUT);
  ledcSetup(channel_PWM, freq_PWM, resolution_PWM); //Set servo channel and frequency as well as PWM resolution.
  ledcSetup(channel_PWM2, freq_PWM, resolution_PWM);
  ledcAttachPin(PWM_Pin1, channel_PWM);  //Binds the LEDC channel to the specified IO port for output
  ledcAttachPin(PWM_Pin2, channel_PWM2);  //Binds the LEDC channel to the specified IO port for output

  dht.begin(); 

  mylcd.setCursor(0, 0);
  mylcd.print("Ip address :");
  mylcd.setCursor(0, 1); 	

  connectToWiFi(); 
  setup_task();	 	 
  setup_routing(); 	 	 

}

void loop() {
  server.handleClient();
}