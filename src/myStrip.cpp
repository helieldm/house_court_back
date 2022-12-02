#include "MyStrip.h"

void MyStrip::colorWipe(uint32_t color, int wait){
    for(int i=0; i<s.numPixels(); i++) { // For each pixel in strip...
        s.setPixelColor(i, color);       //  Set pixel's color (in RAM)
        s.show();                        //  Update strip to match
        delay(wait);                     //  Pause for a moment
    }
}

void MyStrip::colorWipe(int rgb[], int wait){
    if (rgb == nullptr) {
        return colorWipe(s.Color(0, 0, 0), wait);
    }
    else if (myArrLen(rgb) != 3){
        return colorWipe(s.Color(0, 0, 0), wait);
    }
    return colorWipe(s.Color(rgb[0], rgb[1], rgb[2]), wait);   
}
