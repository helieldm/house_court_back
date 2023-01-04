#include "RFID.hpp"

void alarm_on();
void alarm_off();

MFRC522 mfrc522(0x28, RST_PIN);

bool RFID::unlocked = false;

void RFID::RFID_set_unlocked(bool val) {
    RFID::unlocked = val;
}

void RFID::RFID_setup() {
    mfrc522.PCD_Init();
    show_reader_details();
    RFID_task();
}

void RFID::RFID_loop(void *p) {
    String reading = "";
    String password = "";
    bool alarm = false;

    while (true) {
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {};
        reading = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
            reading += String(mfrc522.uid.uidByte[i]);
        }

        password = reading;
        if (password == "137114131226") {
            alarm = false;
            alarm_off();
            RFID::RFID_set_unlocked(true);
        } else if (password != "") {
            alarm = true;
            alarm_on();
            RFID::RFID_set_unlocked(false);
        }
       vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void RFID::show_reader_details() {
    byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
    Serial.print(F("MFRC522 Software Version: 0x"));
    Serial.print(v, HEX);

    if (v == 0X91)
        Serial.print(F(" = v1.0"));
    else if (v == 0x92)
        Serial.print(F(" = v2.0"));
    else
        Serial.print(F(" (unknown)"));
    Serial.println("");
    if ((v == 0x00) || (v == 0xFF))
        Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
}

bool RFID::RFID_unlocked() {
    return(RFID::unlocked);
}

void RFID_task() {
  xTaskCreate(
    RFID::RFID_loop,
    "ON UTILISE LA RFID", 	 	 
    3000, 	 	 
    NULL, 	 	 
    1, 	 	 
    NULL 	 
    );
}