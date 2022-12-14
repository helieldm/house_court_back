#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#include <analogWrite.h>
#include "xht11.h"
#include <ESP32Tone.h>
// #include <ESP32_Servo.h>

#define BUZZER_PIN 25
//#define WINDOW_S_PIN 5 // Window ervo pin
//#define WINDOW_D_PIN 13 // Door servo pin
#define WATER_PIN 34
#define FAN_PIN1 19
#define FAN_PIN2 18
#define Y_LED_PIN 12  //Define the yellow led pin to 12
#define GAS_PIN 23
#define PYRO_PIN 14
#define NEO_PIXEL_PIN 26
#define NEO_PIXEL_COUNT 4

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(NEO_PIXEL_COUNT, NEO_PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Create the server object on the http port, and define the ssid and pw of our network interface
WiFiServer server(80); 
const char* SSID = "OnePlus 8"; 
const char* PASSWORD = "azertyui";

// Screen and humidity detector
LiquidCrystal_I2C mylcd(0x27,16,2); // I2C address is 0x27, 16 char long, 2 lines

// okay so xht is the chinese lib that communicates with the dht11............ 
xht11 xht(17); // pin 17 
unsigned char dht[4] = {0, 0, 0, 0};//Only the first 32 bits of data are received, not the parity bits

//Servo Wservo;
//Servo Dservo;

//Servo channel
int channel_PWM = 13;
int channel_PWM2 = 10;
int freq_PWM = 50; 
int resolution_PWM = 10;
const int PWM_Pin1 = 5;
const int PWM_Pin2 = 13;

int door_deg = 0;
int door_diff = 0;

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
//  Wservo.attach(WINDOW_S_PIN);
//  Dservo.attach(WINDOW_D_PIN);
//  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
//    clock_prescale_set(clock_div_1);
//  #endif
//    // END of Trinket-specific code.
//  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
//  strip.show();            // Turn OFF all pixels ASAP
//  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("TCP server started");
  MDNS.addService("http", "tcp", 80);
  mylcd.setCursor(0, 0);
  mylcd.print("ip:");
  mylcd.setCursor(0, 1);
  mylcd.print(WiFi.localIP());  //LCD displays ip adress
}

void loop() {
  WiFiClient client = server.available();
  if (!client) {
      return;
  }
  while(client.connected() && !client.available()){
      delay(1);
  }
  String req = client.readStringUntil('\r');
  int addr_start = req.indexOf(' ');
  int addr_end = req.indexOf(' ', addr_start + 1);
  if (addr_start == -1 || addr_end == -1) {
      Serial.print("Invalid request: ");
      Serial.println(req);
      return;
  }
  req = req.substring(addr_start + 1, addr_end);
  Serial.println(req);  // WARNING : removed weird string assignation, to check when we actually get to test with the house

  String s;
  if (req == "/")  //Browser accesses address can read the information sent by the client.println(s);
  {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>ESP32 ip:"; // this is disgusting
      s += ipStr;
      s += "</html>\r\n\r\n";
      Serial.println("Sending 200");
      client.println(s);  //Send the string S, then you can read the information when visiting the address of E smart home using the browser.
  }
  if(req == "/led/on") //Browser accesses address ip address/led/on
  {
    client.println("turn on the LED");
    digitalWrite(Y_LED_PIN, HIGH);
  }
  if(req == "/led/off") //Browser accesses address ip address/led/off
  {
    client.println("turn off the LED");
    digitalWrite(Y_LED_PIN, LOW);
  }
    if(req == "/window/more"){
    
    door_diff += 5;
    ledcWrite(channel_PWM, door_deg + door_diff);
    client.println(door_diff);



  }
  if(req == "/window/less"){
    
    door_diff -= 5;
    ledcWrite(channel_PWM, door_deg + door_diff);
    client.println(door_diff);



  }

  if(req == "/window/on")
  {
    client.println("open the window");
    ledcWrite(channel_PWM, 120);  //The high level of 20ms is about 2.5ms, that is, 2.5/20*1024, at this time, the servo angle is 180??.  
    door_deg = 100;
    //Wservo.write(175);
  }
  if(req == "/window/off")
  {
    client.println("close the window");
    ledcWrite(channel_PWM, 60);  //The high level of 20ms is about 0.5ms???that is, 0.5/20*1024???at this time, the servo angle is 0??.
    door_deg = 60;
    //Wservo.write(0);
  }
  if(req == "/music/on")
  {
    //client.println("play music");
  }
  if(req == "/music/off")
  {
    client.println("play music");
    birthday();
    noTone(BUZZER_PIN,0);
  }
  if(req == "/buz/on")
  {
    client.println("buzzer");
    tone(BUZZER_PIN,392,250,0);
    Serial.println("1");
  }
  if(req == "/buz/off")
  {
    client.println("off");
    noTone(BUZZER_PIN,0);
  }
  if(req == "/door/on")
  {
    client.println("open the door");
    ledcWrite(channel_PWM2, 120);
//    Dservo.write(180);
  }
  if(req == "/door/off")
  {
    client.println("close the door");
     ledcWrite(channel_PWM2, 20);
//    Dservo.write(0);
  }
  if(req == "/fan/on")
  {
    client.println("turn on the fan");
    digitalWrite(FAN_PIN1, LOW); //pwm = 0
    ledcWrite(5, 100); //The LEDC channel 1 is bound to the specified left motor output PWM value of 100.
  }
  if(req == "/fan/off")
  {
    client.println("turn off the fan");
    digitalWrite(FAN_PIN1, LOW); //pwm = 0
    ledcWrite(5, 0); //The LEDC channel 1 is bound to the specified left motor output PWM value of 0.
  }
  if(req == "/red/on")
  {
    client.println("red on");
    colorWipe(strip.Color(255,   0,   0), 50);
  }
  if(req == "/red/off")
  {
    client.println("red off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/orange/on")
  {
    client.println("oringe on");
    colorWipe(strip.Color(200,   100,   0), 50);
  }
  if(req == "/orange/off")
  {
    client.println("oringe off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/yellow/on")
  {
    client.println("yellow on");
    colorWipe(strip.Color(200,   200,   0), 50);
  }
  if(req == "/yellow/off")
  {
    client.println("yellow off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/green/on")
  {
    client.println("green on");
    colorWipe(strip.Color(0,   255,   0), 50);
  }
  if(req == "/green/off")
  {
    client.println("green off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/cyan/on")
  {
    client.println("cyan on");
    colorWipe(strip.Color(0,   100,   255), 50);
  }
  if(req == "/cyan/off")
  {
    client.println("cyan off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/blue/on")
  {
    client.println("blue on");
    colorWipe(strip.Color(0,   0,   255), 50);
  }
  if(req == "/blue/off")
  {
    client.println("blue off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/purple/on")
  {
    client.println("purple on");
    colorWipe(strip.Color(100,   0,   255), 50);
  }
  if(req == "/purple/off")
  {
    client.println("purple off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/white/on")
  {
    client.println("white on");
    colorWipe(strip.Color(255,   255,   255), 50);
  }
  if(req == "/white/off")
  {
    client.println("white off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/sfx1/on")
  {
    client.println("sfx1 on");
    rainbow(10);
  }
  if(req == "/sfx1/off")
  {
    client.println("sfx1 off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }
  if(req == "/sfx2/on")
  {
    client.println("sfx2 on");
    theaterChaseRainbow(50);
  }
  if(req == "/sfx2/off")
  {
    client.println("sfx2 off");
    colorWipe(strip.Color(0,   0,   0), 50);
  }


  if(req == "/rain/on")
  {
    int rainVal = analogRead(WATER_PIN);
    client.println(rainVal);
  }
  if(req == "/rain/off")
  {
    client.println("off");
  }
  if(req == "/gas/on")
  {
    boolean gasVal = analogRead(GAS_PIN);
    if(gasVal == 0)
    {
      client.println("safety");
    }
    else
    {
      client.println("dangerous");
    }
  }
  if(req == "/gas/off")
  {
    client.println("off");
  }
  if(req == "/body/on")
  {
    boolean pyroelectric_val = digitalRead(PYRO_PIN);
    if(pyroelectric_val == 1)
    {
      client.println("someone");
    }
    else
    {
      client.println("no one");
    }
  }
  if(req == "/body/off")
  {
    client.println("off");
  }
  if(req == "/temp/on")
  {
    if (xht.receive(dht)) { //Returns true when checked correctly
      Serial.print("Temp:");
      Serial.print(dht[2]); //The integral part of temperature, DHT [3] is the fractional part
      Serial.println("C");
      delay(200);
    } else {    //Read error
      Serial.println("sensor error");
    }
    client.println(dht[2]);
    delay(1000);  //It takes 1000ms to wait for the device to read
  }
  if(req == "/temp/off")
  {
    client.println("off");
  }
  if(req == "/humidity/on")
  {
    if (xht.receive(dht)) { //Returns true when checked correctly
      Serial.print("Temp:");
      Serial.print(dht[0]); //The integral part of temperature, DHT [3] is the fractional part
      Serial.println("%");
      delay(200);
    } else {    //Read error
      Serial.println("sensor error");
    }
    client.println(dht[0]);
    delay(1000);  //It takes 1000ms to wait for the device to read
  }
  if(req == "/humidity/off")
  {
    client.println("off");
  }

  
  //client.stop();
}


void birthday()
{
  tone(BUZZER_PIN,294,250,0);  //The four parameters are pin, frequency, delay and channel 
  tone(BUZZER_PIN,440,250,0);
  tone(BUZZER_PIN,392,250,0);
  tone(BUZZER_PIN,532,250,0);
  tone(BUZZER_PIN,494,250,0);
  tone(BUZZER_PIN,392,250,0);
  tone(BUZZER_PIN,440,250,0);
  tone(BUZZER_PIN,392,250,0);
  tone(BUZZER_PIN,587,250,0);
  tone(BUZZER_PIN,532,250,0);
  tone(BUZZER_PIN,392,250,0);
  tone(BUZZER_PIN,784,250,0);
  tone(BUZZER_PIN,659,250,0);
  tone(BUZZER_PIN,532,250,0);
  tone(BUZZER_PIN,494,250,0);
  tone(BUZZER_PIN,440,250,0);
  tone(BUZZER_PIN,698,250,0);
  tone(BUZZER_PIN,659,250,0);
  tone(BUZZER_PIN,532,250,0);
  tone(BUZZER_PIN,587,250,0);
  tone(BUZZER_PIN,532,500,0);
  noTone(BUZZER_PIN,0);  //??????
}

void colorWipe(uint32_t color, int wait) {
  for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

// Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show(); // Update strip with new contents
    delay(wait);  // Pause for a moment
  }
}

// Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
void theaterChaseRainbow(int wait) {
  int firstPixelHue = 0;     // First pixel starts at red (hue 0)
  for(int a=0; a<30; a++) {  // Repeat 30 times...
    for(int b=0; b<3; b++) { //  'b' counts from 0 to 2...
      strip.clear();         //   Set all pixels in RAM to 0 (off)
      // 'c' counts up from 'b' to end of strip in increments of 3...
      for(int c=b; c<strip.numPixels(); c += 3) {
        int      hue   = firstPixelHue + c * 65536L / strip.numPixels();
        uint32_t color = strip.gamma32(strip.ColorHSV(hue)); // hue -> RGB
        strip.setPixelColor(c, color); // Set pixel 'c' to value 'color'
      }
      strip.show();                // Update strip with new contents
      delay(wait);                 // Pause for a moment
      firstPixelHue += 65536 / 90; // One cycle of color wheel over 90 frames
    }
  }
}
