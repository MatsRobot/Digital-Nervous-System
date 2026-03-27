#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VL53L0X.h>
#include <ESP32-TWAI-CAN.hpp>

// Hardware Pin Definitions from circuit diagram
#define CAN_TX_PIN 17
#define CAN_RX_PIN 18
const int LED_HEARTBEAT = 13;

// Display and Sensor Objects
// OLED is on Bus 1 (Pins 38/39)
Adafruit_SSD1306 display(128, 64, &Wire1, -1);
VL53L0X sensor;
CanFrame rxFrame;

// Global State Variables
String motorStatus = "STOP";
uint16_t currentDistance = 0;
unsigned long lastSendTime = 0;
uint8_t tx_seq = 0;

void setup() {
    Serial.begin(115200);
    pinMode(LED_HEARTBEAT, OUTPUT);

    // Initialize I2C0 for sensor (8,9) and I2C1 for OLED (38,39)
    Wire.begin(8, 9);     
    Wire1.begin(38, 39);   

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    sensor.setBus(&Wire);
    if (sensor.init()) {
        sensor.setTimeout(30);       // Short timeout to keep loop fast
        sensor.startContinuous(33);  // Continuous measurement mode
    }

    // Start CAN Bus at 500kbps
    if (ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN)) {
        Serial.println("CAN Online");
    }
}

void loop() {
    // 1. RECEIVE: Parse word commands sent by laptop
    if (ESP32Can.readFrame(rxFrame)) {
        if (rxFrame.identifier == 0x456) {
            // Convert byte array from CAN message back into a character array
            char receivedChars[9]; 
            int len = rxFrame.data_length_code;
            for(int i = 0; i < len; i++) {
                receivedChars[i] = (char)rxFrame.data[i];
            }
            receivedChars[len] = '\0'; // Add null terminator for string safety
            
            String cmd = String(receivedChars);

            // Update status based on word matches
            if (cmd == "Forward")  motorStatus = "FORWARD";
            else if (cmd == "Backward") motorStatus = "BACKWARD";
            else if (cmd == "Left")     motorStatus = "LEFT";
            else if (cmd == "Right")    motorStatus = "RIGHT";
            else if (cmd == "Stop")     motorStatus = "STOP";

            // Flash heartbeat LED on receipt
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }

    // 2. SEND: Transmit distance to laptop at 20Hz
    if (millis() - lastSendTime >= 50) { 
        lastSendTime = millis();
        currentDistance = sensor.readRangeContinuousMillimeters();

        CanFrame txFrame;
        txFrame.identifier = 0x123;
        txFrame.data_length_code = 3;
        txFrame.data[0] = (currentDistance >> 8) & 0xFF; // High byte
        txFrame.data[1] = currentDistance & 0xFF;        // Low byte
        txFrame.data[2] = tx_seq++;                     // Seq counter
        ESP32Can.writeFrame(txFrame);
    }

    // 3. DISPLAY: Visual feedback on OLED at 10Hz
    static unsigned long lastOled = 0;
    if (millis() - lastOled >= 100) { 
        lastOled = millis();
        display.clearDisplay();
        display.setCursor(0,0);
        display.printf("Dist: %d mm\n", currentDistance);
        display.printf("CMD: %s\n", motorStatus.c_str());
        display.printf("Loop: %d", tx_seq);
        display.display();
    }
}