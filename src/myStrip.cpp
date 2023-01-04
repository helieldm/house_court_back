#include "MyStrip.h"

namespace {
    template<class T>
    int myArrLen(T arr[]){
        return sizeof(arr)/sizeof(T);
    }
}

void MyStrip::random_color(bool yes) {
    std::srand(std::time(nullptr));
    uint32_t val = std::rand();
    if (yes) {
        for (int i = 0; i < numPixels(); i += 1) {
            setPixelColor(i, val); 
            show();
        }
    } else {
        for (int i = 0; i < numPixels(); i += 1) {
        setPixelColor(i, 0); 
        show();    
        }
    }
}

void MyStrip::colorWipe(uint32_t color, int wait){
    for(int i = 0; i < numPixels(); i++) { // For each pixel in strip...
        setPixelColor(i, color);       //  Set pixel's color (in RAM)
        show();                        //  Update strip to match
        delay(wait);                     //  Pause for a moment
    }
}

void MyStrip::rainbow(int wait) {
    for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
        for(int i=0; i<numPixels(); i++) { // For each pixel in strip...
            int pixelHue = firstPixelHue + (i * 65536L / numPixels());
            setPixelColor(i, gamma32(ColorHSV(pixelHue)));
        }
        show(); // Update strip with new contents
        delay(wait);  // Pause for a moment
    }
}