#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class MyStrip {

    Adafruit_NeoPixel s; // strip object contained in the singleton

public:

    MyStrip(int count, int pin, int wth): s(count, pin, wth) {}

    void colorWipe(uint32_t color, int wait);

    void colorWipe(int[3] rgb, int wait);


};

void MyStrip::colorWipe(uint32_t color, int wait){
    for(int i=0; i<s.numPixels(); i++) { // For each pixel in strip...
        s.setPixelColor(i, color);       //  Set pixel's color (in RAM)
        s.show();                        //  Update strip to match
        delay(wait);                     //  Pause for a moment
    }
}

void MyStrip::colorWipe(int[] rgb, int wait){
    if (rgb == nullptr) {
        colorWipe(strip.Color(0, 0, 0), wait)
        return;
    };
    return colorWipe(strip.Color(rgb[0],rgb[1],rgb[2]), wait);
    

}
