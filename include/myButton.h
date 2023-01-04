#ifndef BTN_CLS
#define BTN_CLS
#include <Arduino.h>

class MyButton {

private: 

    void (*callback_fn)();
    int pin;
    static void btn_loop(void *pvParameters);

public:
    MyButton(int _pin, void (*_callback_fn)()): pin(_pin), callback_fn(_callback_fn) {};
    void btn_task();

};

#endif // BTN_CLS