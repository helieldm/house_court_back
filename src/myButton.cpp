#include "myButton.h"

void MyButton::btn_task() {
    pinMode(pin, INPUT);
    xTaskCreate(
    btn_loop,
    "Button press", 	 	 
    1000, 	 	 
    this, 	 	 
    1, 	 	 
    NULL 	 
    );
}

void MyButton::btn_loop(void *pvParameters) {
    unsigned long long int t = 0;
    MyButton *that = (MyButton *)pvParameters;
    while (true) {
        Serial.println(digitalRead(that->pin));
        t = 0; // reset timer
        while(digitalRead(that->pin) == 0){
            Serial.println("YA");
            t++;
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        if (t > 10) { // btn was pressed for more than 10 ms
            Serial.println("YI");
            that->callback_fn();            
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}