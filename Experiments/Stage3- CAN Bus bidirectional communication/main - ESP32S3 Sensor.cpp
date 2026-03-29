#include <Arduino.h>
#include <Wire.h>               // I2C library for sensor and OLED
#include <Adafruit_GFX.h>       // Graphics library
#include <Adafruit_SSD1306.h>   // OLED Driver
#include <VL53L0X.h>            // Laser Distance Sensor library
#include <ESP32-TWAI-CAN.hpp>   // Specialized CAN (TWAI) library for ESP32

// Hardware Pin Definitions
#define CAN_TX_PIN 17           // ESP32 Internal CAN TX
#define CAN_RX_PIN 18           // ESP32 Internal CAN RX
const int LED_HEARTBEAT = 13;   // Status LED

// Initialize OLED: 128x64 pixels, using Wire1 (second I2C bus)
Adafruit_SSD1306 display(128, 64, &Wire1, -1);
VL53L0X sensor;                 // Laser sensor object
CanFrame rxFrame;               // Buffer to hold incoming CAN messages

// Global State Variables
String motorStatus = "STOP";    // Current movement command
uint16_t currentDistance = 0;   // Distance in millimeters
unsigned long lastSendTime = 0; // Timing for CAN TX
uint8_t tx_seq = 0;             // Sequence counter

void setup() {
    Serial.begin(115200);
    pinMode(LED_HEARTBEAT, OUTPUT);

    // Initialize two separate I2C buses (ESP32 specialty)
    Wire.begin(8, 9);           // Bus 0: Pins 8 & 9 for the Laser Sensor
    Wire1.begin(38, 39);        // Bus 1: Pins 38 & 39 for the OLED Display

    // Start OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    // Start Laser Sensor on Bus 0
    sensor.setBus(&Wire);
    if (sensor.init()) {
        sensor.setTimeout(30);       // Limit wait time to keep code fast
        sensor.startContinuous(33);  // Take readings every 33ms
    }

    // Initialize ESP32's built-in CAN hardware (TWAI) at 500kbps
    if (ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN)) {
        Serial.println("CAN Online");
    }
}

void loop() {
    // 1. RECEIVE: Look for motor commands (ID 0x456) sent by the Brain
    if (ESP32Can.readFrame(rxFrame)) {
        if (rxFrame.identifier == 0x456) {
            // Reconstruct the character array from the raw CAN data bytes
            char receivedChars[9]; 
            int len = rxFrame.data_length_code; // Get how many bytes were sent
            for(int i = 0; i < len; i++) {
                receivedChars[i] = (char)rxFrame.data[i];
            }
            receivedChars[len] = '\0'; // Add null terminator to make it a valid C-string
            
            String cmd = String(receivedChars);

            // Compare the word to update the robot's logic
            if (cmd == "Forward")  motorStatus = "FORWARD";
            else if (cmd == "Backward") motorStatus = "BACKWARD";
            else if (cmd == "Left")     motorStatus = "LEFT";
            else if (cmd == "Right")    motorStatus = "RIGHT";
            else if (cmd == "Stop")     motorStatus = "STOP";

            // Flash the LED every time a command is received
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }

    // 2. SEND: Broadcast the Laser distance to the Brain at 20Hz (every 50ms)
    if (millis() - lastSendTime >= 50) { 
        lastSendTime = millis();
        currentDistance = sensor.readRangeContinuousMillimeters();

        CanFrame txFrame;
        txFrame.identifier = 0x123;         // ESP32 Sensor ID
        txFrame.data_length_code = 3;       // Sending 3 bytes total
        txFrame.data[0] = (currentDistance >> 8) & 0xFF; // High byte of 16-bit int
        txFrame.data[1] = currentDistance & 0xFF;        // Low byte of 16-bit int
        txFrame.data[2] = tx_seq++;                     // Sequence counter
        ESP32Can.writeFrame(txFrame);
    }

    // 3. DISPLAY: Update the physical OLED screen at 10Hz (every 100ms)
    static unsigned long lastOled = 0;
    if (millis() - lastOled >= 100) { 
        lastOled = millis();
        display.clearDisplay();
        display.setCursor(0,0);
        // Print formatted strings to the screen
        display.printf("Dist: %d mm\n", currentDistance);
        display.printf("CMD: %s\n", motorStatus.c_str());
        display.printf("Loop: %d", tx_seq);
        display.display(); // Push the buffer to the actual hardware
    }
}