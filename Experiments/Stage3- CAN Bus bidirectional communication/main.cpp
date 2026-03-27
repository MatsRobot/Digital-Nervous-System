#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VL53L0X.h>
#include <ESP32-TWAI-CAN.hpp>

// --- PIN DEFINITIONS ---
#define CAN_TX_PIN 17          // GPIO pin for CAN Transmit
#define CAN_RX_PIN 18          // GPIO pin for CAN Receive
const int LED_HEARTBEAT = 13;   // Status LED pin

// --- GLOBAL OBJECTS ---
// Display using I2C Bus 1 (pins 38/39)
Adafruit_SSD1306 display(128, 64, &Wire1, -1); 
VL53L0X sensor;                // Distance sensor object
CanFrame rxFrame;              // Container for incoming CAN messages

// --- STATE VARIABLES ---
String globalLastRx = "None";   // Stores the sequence number received from PC
uint16_t currentDistance = 0;  // Stores the latest distance reading in mm
unsigned long lastSendTime = 0; // Timer to track when we last sent a CAN message
uint8_t tx_seq = 0;            // Local counter to track outgoing message order

void setup() {
    Serial.begin(115200);      // Initialize Serial for debugging
    pinMode(LED_HEARTBEAT, OUTPUT);

    // Initialize the two separate I2C buses on the ESP32-S3
    Wire.begin(8, 9);          // Bus 0: Pins 8 & 9 (connected to Sensor)
    Wire1.begin(38, 39);       // Bus 1: Pins 38 & 39 (connected to OLED)

    // Start OLED. SSD1306_SWITCHCAPVCC generates 3.3V internally for the display
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    sensor.setBus(&Wire);      // Tell sensor to use Bus 0
    if (!sensor.init()) {
        Serial.println("Failed to detect sensor!");
    } else {
        // Optimization: Reduce the sensor's "wait time" so it doesn't freeze the code
        sensor.setTimeout(30);          // Max 30ms wait if no target is seen
        sensor.startContinuous(33);    // Take a new measurement every 33ms
    }

    // Initialize CAN bus at 500kbps (Industry standard speed)
    if (ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN)) {
        Serial.println("CAN Online");
    }
}

void loop() {
    // SECTION 1: HIGH-SPEED RECEIVE
    // We check for incoming CAN frames every loop iteration (thousands of times per second)
    if (ESP32Can.readFrame(rxFrame)) {
        // Check if the message ID matches the one sent by the Python script (0x456)
        if (rxFrame.identifier == 0x456) {
            // Data index 2 contains the PC's sequence number
            globalLastRx = String(rxFrame.data[2]); 
            // Toggle the LED to visually confirm a message was received
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }

    // SECTION 2: TIMED SENSOR READ & TRANSMIT
    // We only send data at a fixed interval (50ms / 20Hz) to avoid flooding the bus
    static unsigned long lastTx = 0;
    if (millis() - lastTx >= 50) { 
        lastTx = millis();
        
        // Grab the latest distance from the sensor buffer
        currentDistance = sensor.readRangeContinuousMillimeters();

        CanFrame txFrame;              // Create a new frame
        txFrame.identifier = 0x123;    // ID for the PC to look for
        txFrame.data_length_code = 3;  // Sending 3 bytes of data
        
        // Split 16-bit distance into two 8-bit bytes (High byte / Low byte)
        txFrame.data[0] = (currentDistance >> 8) & 0xFF;
        txFrame.data[1] = currentDistance & 0xFF;
        // Add our own sequence number so the PC can detect if it missed a packet
        txFrame.data[2] = tx_seq++;
        
        ESP32Can.writeFrame(txFrame);  // Push the message onto the CAN bus
    }

    // SECTION 3: THROTTLED OLED UPDATE
    // Updating the screen is the slowest task. We do it only at 10Hz (100ms)
    // to ensure the CPU has enough time to handle CAN and Sensor tasks.
    static unsigned long lastOled = 0;
    if (millis() - lastOled >= 100) { 
        lastOled = millis();
        display.clearDisplay();        // Wipe screen buffer
        display.setCursor(0,0);
        display.printf("Dist: %d mm\n", currentDistance);
        display.printf("PC Seq: %s\n", globalLastRx.c_str());
        display.printf("My Seq: %d", tx_seq);
        display.display();             // Send buffer to physical screen via I2C
    }
}