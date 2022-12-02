#include "MyStrip.h"

namespace {
    template<class T>
    int myArrLen(T arr[]){
        return sizeof(arr)/sizeof(T);
    }
}


void MyStrip::colorWipe(uint32_t color, int wait){
    for(int i = 0; i < numPixels(); i++) { // For each pixel in strip...
        setPixelColor(i, color);       //  Set pixel's color (in RAM)
        show();                        //  Update strip to match
        delay(wait);                     //  Pause for a moment
    }
}

void MyStrip::colorWipe(uint32_t rgb[], int wait){
    if (rgb == nullptr) {
        return colorWipe(Color(0, 0, 0), wait);
    }
    else if (myArrLen(rgb) != 3){
        return colorWipe(Color(0, 0, 0), wait);
    }
    return colorWipe(Color(rgb[0], rgb[1], rgb[2]), wait);   
}
