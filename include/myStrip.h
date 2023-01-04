#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <cstdlib>
#include <ctime>
class MyStrip: public Adafruit_NeoPixel {

    // use the parent classes
    using Adafruit_NeoPixel::Adafruit_NeoPixel;

public:

    void colorWipe(uint32_t color, int wait);
    void rainbow(int wait);
    void random_color(bool yes);

};
