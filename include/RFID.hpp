#ifndef RFID_H_
#define RFID_H_

#include <Wire.h>
#include "MFRC522_I2C.h"

#define RST_PIN 12

class RFID {
    private:
        static void RFID_set_unlocked(bool val);

    public:
        static bool unlocked;
        void RFID_setup();
        static void RFID_loop(void *p);
        void show_reader_details();
        bool RFID_unlocked();
};

void RFID_task();

#endif // RFID_H_