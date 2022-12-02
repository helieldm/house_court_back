#ifndef LED_CLS
#define LED_CLS
#include <Arduino.h>

class Led {

private: 

    int pin;
    bool flag = false;

public:

    Led(int p): pin(p) {}

    void on();
    void off();
    void toggle();

};

#endif // LED_CLS