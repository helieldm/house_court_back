#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace {
    template<class T>
    int myArrLen(T arr[]){
        return sizeof(arr)/sizeof(T);
    }
}

class MyStrip {

    Adafruit_NeoPixel s; // strip object contained in the singleton

public:

    MyStrip(int count, int pin, int wth): s(count, pin, wth) {}

    void colorWipe(uint32_t color, int wait);

    void colorWipe(int rgb[3], int wait);


};
