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
#include "myButton.h"
#include "RFID.hpp"

#define CONNECTION_TIMEOUT 10

#define BUZZER_PIN 25

#define WATER_PIN 34
#define FAN_PIN1 19
#define FAN_PIN2 18
#define Y_LED_PIN 12  
#define GAS_PIN 23
#define PYRO_PIN 14
#define NEO_PIXEL_PIN 26
#define NEO_PIXEL_COUNT 4
#define BTN_PIN_1 16
#define BTN_PIN_2 27

void destroy();

const char *SSID = "Helie";
const char *PWD = "Proutprout";

const char *BEGIN = "BEGIN";
const char *END = "END";
const char *SEP = ";";
const char *DHT_STR = "DHT";

const esp_websocket_client_config_t ws_cfg = {
  .uri = "ws://192.168.38.244:5196",
  .path = "/",
  .transport = WEBSOCKET_TRANSPORT_OVER_TCP
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

float temperature=0;
float humidity=0;
String reading="";
String keepAlive="";

bool flag_LEDs = true;
bool flag_window = true;
bool flag_door = true;
bool flag_buzzer = true;
bool flag_fan = true;

MyStrip strip(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

Led y_led = Led(Y_LED_PIN);
RFID my_rfid = RFID();

int menu_index = 0;
const String menus_l1[6] = {"Allumer/Eteindre", "Allumer/Eteindre", "Ouvrir/Fermer", "Ouvrir/Fermer", "Allumer/Eteindre", "Allumer/Eteindre"};
const String menus_l2[6] = {"Led jaune", "Strip", "Fenetre", "porte", "Alarme", "Ventilateur"};

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);

  int timeout_counter = 0;
  int status = WL_IDLE_STATUS;
  
  WiFi.mode(WIFI_STA);

  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(std::to_string(status).c_str());
    status = WiFi.status();
    delay(500);
    timeout_counter++;
    if(timeout_counter >= CONNECTION_TIMEOUT*5 || status == WL_CONNECT_FAILED){
      destroy();
      ESP.restart();
    }
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  mylcd.print(WiFi.localIP());
}

void fan_on();
void fan_off();
void open_window();
void close_window();
void read_dht(void * parameter) {
   while(true){
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();

    if (humidity > 80 or temperature > 30) {
      fan_on();
      open_window();
    } else {
      fan_off();
      close_window();
    }

    reading = "BEGIN;"+ WiFi.macAddress() + ";DHT;" + String(humidity) + ";" + String(temperature) + ";END\0";

    String door_state = flag_door ? "OPEN" : "CLOSE";
    String door_sensor = "BEGIN;"+ WiFi.macAddress() + ";STATE;DOOR;" + door_state + ";END\0";

    String WINDOW_state = flag_window ? "OPEN" : "CLOSE";
    String WINDOW_sensor = "BEGIN;"+ WiFi.macAddress() + ";STATE;WINDOW;" + WINDOW_state + ";END\0";

    String vents_state = flag_fan ? "ON" : "OFF";
    String vents_sensor = "BEGIN;"+ WiFi.macAddress() + ";STATE;VENTS;" + vents_state + ";END\0";

    String alarm_state = flag_buzzer ? "ON" : "OFF";
    String alarm_sensor = "BEGIN;"+ WiFi.macAddress() + ";STATE;ALARM;" + alarm_state + ";END\0";

    // send reading to API
    esp_websocket_client_send(ws_cli,reading.c_str(),reading.length(),2000);

    esp_websocket_client_send(ws_cli,door_sensor.c_str(),door_sensor.length(),2000);
    esp_websocket_client_send(ws_cli,WINDOW_sensor.c_str(),WINDOW_sensor.length(),2000);
    esp_websocket_client_send(ws_cli,vents_sensor.c_str(),vents_sensor.length(),2000);
    esp_websocket_client_send(ws_cli,alarm_sensor.c_str(),alarm_sensor.length(),2000);

    door_sensor = "";
    WINDOW_sensor = "";
    vents_sensor = "";
    alarm_sensor = "";
    reading = "";

    // delay the task
    vTaskDelay(15000 / portTICK_PERIOD_MS);
   }
}

void keep_alive(void * parameter) {
  keepAlive = "BEGIN;"+ WiFi.macAddress() + ";END\0";
  while(true){
    Serial.println("HIHIHII : " + keepAlive);
    esp_websocket_client_send(ws_cli,keepAlive.c_str(),keepAlive.length(),2000);
    // delay the task
     vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void dht_task() {	 	 
  xTaskCreate(	 	 
  read_dht, 	 	 
  "Read sensor data", 	 	 
  5000, 	 	 
  NULL, 	 	 
  1, 	 	 
  NULL 	 	 
  );	 	 
}

void keep_alive_task() {	 	 
  xTaskCreate(	 	 
  keep_alive, 	 	 
  "Keep alive", 	 	 
  5000, 	 	 
  NULL, 	 	 
  1, 	 	 
  NULL 	 	 
  );	 	 
}

void btn2_loop(void *p);
void btn1_loop(void *p) {
    unsigned int t = 0;
    bool locked = false;
    bool unlocked = false;
    while (true) {
      if (!my_rfid.RFID_unlocked()) {  
        vTaskDelay(10 / portTICK_PERIOD_MS);
        unlocked = false;
        if (locked) continue;
        mylcd.clear();
        mylcd.print("Deverouillez la");
        mylcd.setCursor(0, 1); 	
        mylcd.print("maison!");
        locked = true;
        continue;
      } else {   
        locked = false;
        if (!unlocked){
          mylcd.clear();
          mylcd.print(menus_l1[menu_index]);
          mylcd.setCursor(0, 1); 	
          mylcd.print(menus_l2[menu_index]);
          unlocked = true;
        }
      }
      if (digitalRead(BTN_PIN_1) == 0) {
        t++;
      } else if (t > 10 && my_rfid.RFID_unlocked()) {
        t = 0;
        menu_index++;
        if (menu_index > 5) menu_index = 0;   

        mylcd.clear();
        mylcd.print(menus_l1[menu_index]);
        mylcd.setCursor(0, 1); 	
        mylcd.print(menus_l2[menu_index]);
                    
      } else {
        t = 0;
      }
      
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void btn1_task() {
  xTaskCreate(
    btn1_loop,
    "Button 1 press", 	 	 
    2000, 	 	 
    NULL, 	 	 
    1, 	 	 
    NULL 	 
    );
}

void btn2_task() {
  xTaskCreate(
    btn2_loop,
    "Button 2 press", 	 	 
    2000, 	 	 
    NULL, 	 	 
    1, 	 	 
    NULL 	 
    );
}

void open_window() {
  if (!flag_window) return;
  flag_window = false;
    Serial.println("Open the window");
    ledcWrite(channel_PWM, 120);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 100;
}

void close_window() {
  if (!flag_window) return;
  flag_window = true;
    Serial.println("Closed the window");
    ledcWrite(channel_PWM, 60);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180°.  
    door_deg = 60;
}

void open_door() {
  if (!flag_door) return;
  flag_door = false;
  // TODO : On arrive bien la mais la porte s'ouvre pas 
  Serial.println("Open the door");
  ledcWrite(channel_PWM2, 120);
}

void close_door() {
  if (!flag_door) return;
  flag_door = true;
  // TODO : On arrive bien la mais la porte se ferme pas 
  Serial.println("Close the door");
  ledcWrite(channel_PWM2, 20);
}

void fan_on() {
  if (!flag_fan) return;
  flag_fan = false;
  digitalWrite(FAN_PIN1, LOW); //pwm = 0
  ledcWrite(5, 100); //The LEDC channel 1 is bound to the specified left motor output PWM value of 100.
}

void fan_off(){
  if (flag_fan) return;
  flag_fan = true;
  digitalWrite(FAN_PIN1, LOW); //pwm = 0
  ledcWrite(5, 0); //The LEDC channel 1 is bound to the specified left motor output PWM value of 0.
}

void alarm_on(){
  if (!flag_buzzer) return;
  flag_buzzer = false;
  tone(BUZZER_PIN,392);
}

void alarm_off(){
  if (flag_buzzer) return;
  flag_buzzer = true;
  noTone(BUZZER_PIN);
}


void flipflop_LED() {
  y_led.toggle();
}

void flipflop_fan() {
  if (flag_fan) {
    fan_on();
  } else {
    fan_off();
  }
}

void flipflop_LEDs() {
  if (flag_LEDs) {
    strip.random_color(flag_LEDs);
    flag_LEDs = false;
  } else {
    strip.random_color(flag_LEDs);
    flag_LEDs = true;
  }
}

void flipflop_window() {
  if (flag_window) {
    open_window();
  } else {
    close_window();
  }
}

void flipflop_door() {
  if (flag_door) {
    open_door();
  } else {
    close_door();
  }
}

void flipflop_buzzer() {
  if (flag_buzzer) {
    alarm_on();
  } else {
    alarm_off();
  }
}

void btn2_loop(void *p) {
  void (*tab[6])(void) = {flipflop_LED, flipflop_LEDs, flipflop_window, flipflop_door, flipflop_buzzer, flipflop_fan};
  int count = 0;

  while (true) {
    if (digitalRead(BTN_PIN_2) == 0)
      count += 1;
    else if (count > 10 && my_rfid.RFID_unlocked()) {
      Serial.print(menu_index);
      tab[menu_index]();
      count = 0;
    } else
      count = 0;
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void handle_message(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data){
  esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
  char * message = (char *)data->data_ptr;
  if (strcmp(message,""))
  {

    Serial.print("Data received length : ");
    Serial.println(data->data_len);
    Serial.print("Data received : ");
    Serial.println((char *)data->data_ptr);

    if (strstr(message, "VENTS") != NULL) {
      Serial.println("VENTS TOGGLE");
      if(strstr(message, "ON") != NULL){
        fan_on();  
      } else if (strstr(message, "OFF") != NULL)
      {
        fan_off();
      }
    }

    if (strstr(message, "WINDOW") != NULL) {
      Serial.println("WINDOW TOGGLE");
      if(strstr(message, "OPEN") != NULL){
        open_window();  
      } else if (strstr(message, "CLOSE") != NULL)
      {
        close_window();
      }
    }

    if (strstr(message, "DOOR") != NULL) {
      Serial.println("DOOR TOGGLE");
      if(strstr(message, "OPEN") != NULL){
        open_door();  
      } else if (strstr(message, "CLOSE") != NULL)
      {
        close_door();
      }
    }

    if (strstr(message, "ALARM") != NULL) {
      Serial.println("ALARM TOGGLE");
      if(strstr(message, "ON") != NULL){
        alarm_on();  
      } else if (strstr(message, "OFF") != NULL)
      {
        alarm_off();
      }
    }
  }
}

void config_websocket(){
    // Setup websocket

    Serial.println("Start ws config");
    ws_cli = esp_websocket_client_init(&ws_cfg);
    esp_websocket_client_start(ws_cli);

    wsHandler = esp_event_handler_t(handle_message);
    
    esp_websocket_register_events(ws_cli, WEBSOCKET_EVENT_DATA, wsHandler, (void *)ws_cli);

    if(esp_websocket_client_is_connected(ws_cli)) Serial.println("Connected! Yay.");

    String init = "BEGIN;" + WiFi.macAddress() + ";REGISTER;END";
    //String init = "BEGIN";


    esp_websocket_client_send_text(ws_cli,init.c_str(), init.length(), 2000);

    Serial.println("Finish ws config");
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
  pinMode(BTN_PIN_1, INPUT);
  pinMode(BTN_PIN_2, INPUT);

  dht.begin(); 

  mylcd.setCursor(0, 0);
  mylcd.print("Ip address :");
  mylcd.setCursor(0, 1); 	

  connectToWiFi();
  delay(500);
  config_websocket();
  dht_task();
  keep_alive_task(); 	 
  btn1_task();
  btn2_task();
  
  strip.begin();
  strip.show();
  strip.setBrightness(25);
  strip.setPixelColor(0, strip.Color(251,0,0));

  my_rfid.RFID_setup();
  RFID::unlocked = false;
}

void loop() {
}   

void destroy() {
  esp_websocket_client_stop(ws_cli);
  esp_websocket_client_destroy(ws_cli);
  WiFi.disconnect();
}