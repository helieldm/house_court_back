#include "led.h"

void Led::on() {
    digitalWrite(pin, HIGH);
    Serial.println("turn on the LED");
    flag = true;
}

void Led::off() {
    digitalWrite(pin, LOW);
    Serial.println("turn off the LED");
    flag = false;
}

void Led::toggle() {
    flag ? off() : on(); // if it's on, turn it off
}