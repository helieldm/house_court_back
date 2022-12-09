#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class MyStrip: public Adafruit_NeoPixel {

    // use the parent classes
    using Adafruit_NeoPixel::Adafruit_NeoPixel;

public:

    void colorWipe(uint32_t color, int wait);
    void rainbow(int wait);

};
