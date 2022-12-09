#include "led.h"

void Led::on() {
    digitalWrite(pin, HIGH);
    flag = true;
}

void Led::off() {
    digitalWrite(pin, LOW);
    flag = false;
}

void Led::toggle() {
    flag ? off() : on(); // if it's on, turn it off
}