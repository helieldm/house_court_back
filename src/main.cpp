#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <esp_websocket_client.h>
#include <time.h>

// Our classes
#include "led.h"
#include "myStrip.h"
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

void destroy();

const char* SSID = "IOT";
const char* PWD = "40718804";

const char* BEGIN = "BEGIN";
const char* END = "END";
const char* SEP = ";";
const char* DHT_STR = "DHT";

const esp_websocket_client_config_t ws_cfg = {
  .uri = "ws://192.168.0.100:5196",
};

esp_websocket_client_handle_t ws_cli;
esp_event_handler_t wsHandler;

time_t curr_time;
time_t setup_time;

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

String temperature="";
String humiidity="";
String reading="";

MyStrip strip(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Led y_led = Led(Y_LED_PIN);

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);

  int timeout_counter = 0;
  int status = WL_IDLE_STATUS;
  
  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    status = WiFi.status();
    delay(500);
    timeout_counter++;
    if(timeout_counter >= CONNECTION_TIMEOUT*6){
      destroy();
      ESP.restart();
    }
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  mylcd.print(WiFi.localIP());
}

void led_loop(void *p) {
    while(true){
      y_led.toggle();
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }; 
  }

void led_task() {
  xTaskCreate(
    led_loop,
    "Toggle led", 	 	 
    1000, 	 	 
    NULL, 	 	 
    1, 	 	 
    NULL 	 
    );
}

void read_dht(void * parameter) {
   while(true){
    humiidity = String(dht.readHumidity());
    temperature = String(dht.readTemperature());

    reading = "BEGIN;"+ WiFi.macAddress() + ";DHT;" + humiidity + ";" + temperature + ";END";

    esp_websocket_client_send(ws_cli,reading.c_str(),reading.length(),2000);

    Serial.print( "fAlarm " );
    Serial.print(uxTaskGetStackHighWaterMark( NULL ));
    Serial.println();
    Serial.flush();

    // delay the task
     vTaskDelay(60000 / portTICK_PERIOD_MS);
   }
}

void dht_task() {	 	 
  xTaskCreate(	 	 
  read_dht, 	 	 
  "Read sensor data", 	 	 
  2000, 	 	 
  NULL, 	 	 
  1, 	 	 
  NULL 	 	 
  );	 	 
}

void openWindow() {
    Serial.println("Open the window");
    ledcWrite(channel_PWM, 120);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 100;
}

void closeWindow() {
    Serial.println("Closed the window");
    ledcWrite(channel_PWM, 60);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 60;
}

void openDoor() {
    Serial.println("Open the door");
    ledcWrite(channel_PWM2, 120);
}

void closeDoor() {
    Serial.println("Close the door");
    ledcWrite(channel_PWM2, 20);
}

void handleMessage(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  char * message = (char *)data->data_ptr;
  if (strcmp(message,""))
  {
    Serial.print("Data received length : ");
    Serial.println(data->data_len);
    Serial.print("Data received : ");
    Serial.println((char *)data->data_ptr);
  }
}

void configWebsocket(){
    // Setup websocket
    ws_cli = esp_websocket_client_init(&ws_cfg);
    esp_websocket_client_start(ws_cli);

    wsHandler = esp_event_handler_t(handleMessage);
    
    esp_websocket_register_events(ws_cli,WEBSOCKET_EVENT_DATA, wsHandler, (void *)ws_cli);
    
    String init = "BEGIN;" + WiFi.macAddress() + ";REGISTER;END";
  
    esp_websocket_client_send(ws_cli,init.c_str(),init.length(),2000);

}

void setup() {

  time(&setup_time);

  Serial.begin(115200);

  delay(1000);

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
  configWebsocket();
  dht_task();
  led_task(); 	 

  strip.begin();
  strip.show();
  strip.setBrightness(25);
  strip.setPixelColor(0, strip.Color(251,0,0));

}

void loop() {
  strip.rainbow(10);
}   

void destroy() {
  esp_websocket_client_stop(ws_cli);
  esp_websocket_client_destroy(ws_cli);
  WiFi.disconnect();
}