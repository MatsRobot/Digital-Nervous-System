#include <Arduino.h>
#include <SPI.h>
#include <CAN.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

#define CAN_CS_PIN 10
#define CAN_INT_PIN 2
#define SONAR_RX_PIN 3 
#define SONAR_TX_PIN 4
#define LED_HEARTBEAT 5

LiquidCrystal_I2C lcd(0x27, 20, 4); 
SoftwareSerial sonarSerial(SONAR_RX_PIN, SONAR_TX_PIN);

int currentDistCm = 0;
uint8_t nano_seq = 0;
unsigned long lastCanTx = 0;
int lastLcdState = -1; // Tracks previous state to prevent flickering

void setup() {
    Serial.begin(115200);
    sonarSerial.begin(9600);
    pinMode(LED_HEARTBEAT, OUTPUT);

    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Node 0x111 Active");

    CAN.setPins(CAN_CS_PIN, CAN_INT_PIN);
    CAN.setClockFrequency(8E6); 
    if (!CAN.begin(500E3)) {
        lcd.setCursor(0, 1);
        lcd.print("CAN FAIL");
        while (1);
    }
}

void handleIncomingCAN() {
    int packetSize = CAN.parsePacket();
    if (packetSize && CAN.packetId() == 0x500) {
        int newState = CAN.read();
        
        // Only update LCD if the state actually changed (Debounce)
        if (newState != lastLcdState) {
            lcd.setCursor(0, 3);
            if (newState == 1) {
                lcd.print("Brain CMD Received!");
            } else {
                lcd.print("                    "); // Clear line
            }
            lastLcdState = newState;
        }
    }
}

void loop() {
    // 1. Check for CAN commands AS FAST AS POSSIBLE
    handleIncomingCAN();

    // 2. Read Sonar (Only if data is waiting)
    if (sonarSerial.available() > 0) {
        char c = sonarSerial.read();
        if (c == 'R') {
            // Use a short non-blocking wait for digits
            unsigned long startRead = millis();
            String rStr = "";
            while(millis() - startRead < 40 && rStr.length() < 3) {
                if(sonarSerial.available()) {
                    char d = sonarSerial.read();
                    if(isDigit(d)) rStr += d;
                }
            }
            if (rStr.length() > 0) {
                currentDistCm = (int)(rStr.toInt() * 2.54);
                lcd.setCursor(0, 1);
                lcd.print("Dist: "); lcd.print(currentDistCm); lcd.print(" cm   ");
            }
        }
    }

    // 3. Transmit Status (Every 100ms)
    if (millis() - lastCanTx >= 100) {
        lastCanTx = millis();
        CAN.beginPacket(0x111);
        CAN.write(currentDistCm & 0xFF);
        CAN.write(nano_seq++);
        if (CAN.endPacket()) {
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }
}