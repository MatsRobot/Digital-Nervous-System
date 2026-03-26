#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <VL53L0X.h>
#include <ESP32-TWAI-CAN.hpp> // HW-accelerated CAN library for ESP32

// -----------------------------------------------------------------
// PIN ASSIGNMENTS (Matches full circuit diagram)
// -----------------------------------------------------------------
#define SDA0 8          // I2C Bus 0 Data (Sensor)
#define SCL0 9          // I2C Bus 0 Clock (Sensor)
#define SDA1 38         // I2C Bus 1 Data (OLED)
#define SCL1 39         // I2C Bus 1 Clock (OLED)
#define CAN_TX_PIN 17   // CAN Transceiver Transmit
#define CAN_RX_PIN 18   // CAN Transceiver Receive
const int LED_HEARTBEAT = 13;

// -----------------------------------------------------------------
// GLOBAL OBJECTS
// -----------------------------------------------------------------
// OLED initialized on Wire1 (I2C Bus 1)
Adafruit_SSD1306 display(128, 64, &Wire1, -1);
VL53L0X sensor;         // ToF Sensor instance
CanFrame rxFrame;       // Buffer to hold incoming CAN messages

void setup() {
    Serial.begin(115200);
    pinMode(LED_HEARTBEAT, OUTPUT);

    // 1. INITIALIZE COMMUNICATION BUSES
    Wire.begin(SDA0, SCL0);   // Start Sensor I2C Bus
    Wire1.begin(SDA1, SCL1);  // Start Display I2C Bus

    // 2. INITIALIZE PERIPHERALS
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    
    sensor.setBus(&Wire);     // Assign sensor to Bus 0
    sensor.init();
    sensor.startContinuous(); // Enable continuous high-speed sampling

    // 3. INITIALIZE CAN CONTROLLER (TWAI)
    // Setup at 500kbps using the S3's internal hardware controller
    if (ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN)) {
        Serial.println("CAN Bus Initialized Successfully!");
    } else {
        Serial.println("CAN Bus Initialization Failed!");
    }
}

void loop() {
    // Collect distance from sensor (Local Data)
    uint16_t distance = sensor.readRangeContinuousMillimeters();

    // -------------------------------------------------------------
    // STEP 1: TRANSMIT DATA VIA CAN-BUS
    // -------------------------------------------------------------
    CanFrame txFrame;
    txFrame.identifier = 0x123;         // ID for this specific sensor node
    txFrame.extd = 0;                  // Standard 11-bit ID format
    txFrame.data_length_code = 2;       // We are sending 2 bytes (16-bit value)
    
    // Manual bit-shifting to fit a 16-bit number into two 8-bit CAN slots
    txFrame.data[0] = (distance >> 8) & 0xFF; // Shift right 8 bits for High Byte
    txFrame.data[1] = distance & 0xFF;        // Mask for the Low Byte
    
    ESP32Can.writeFrame(txFrame);      // Push the data onto the physical bus

    // -------------------------------------------------------------
    // STEP 2: LISTEN FOR INCOMING COMMANDS
    // -------------------------------------------------------------
    String lastRx = "None";
    if (ESP32Can.readFrame(rxFrame)) {  // Check if a message is waiting
        // Log the ID of whatever node just messaged us
        lastRx = "ID: 0x" + String(rxFrame.identifier, HEX);
    }

    // -------------------------------------------------------------
    // STEP 3: UPDATE USER INTERFACE (OLED)
    // -------------------------------------------------------------
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("CAN NODE ACTIVE");
    display.println("---------------");
    display.print("Dist: "); display.print(distance); display.println(" mm");
    display.println("");
    display.println("Last RX (CAN):");
    display.println(lastRx);           // Show diagnostic info on screen
    display.display();

    // Visual Heartbeat: Toggle LED state every 100ms
    digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
    delay(100); 
}