#include <Arduino.h>              // Base Arduino framework
#include <SPI.h>                  // Required for SPI communication with the CAN Shield
#include <CAN.h>                  // Library for the MCP2515 CAN controller
#include <Wire.h>                 // I2C library for the LCD display
#include <LiquidCrystal_I2C.h>    // Driver for I2C-based LCD screens
#include <SoftwareSerial.h>       // Allows serial communication on digital pins

// --- PIN DEFINITIONS ---
#define CAN_CS_PIN 10             // CAN Shield Chip Select pin
#define CAN_INT_PIN 2             // CAN Shield Interrupt pin
#define SONAR_RX_PIN 3            // Serial Receive from MaxSonar
#define SONAR_TX_PIN 4            // Serial Transmit to MaxSonar (if needed)
#define LED_HEARTBEAT 5           // LED that toggles on successful CAN transmission

// --- OBJECT INITIALIZATION ---
// Initialize LCD at I2C address 0x27 with 20 columns and 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4); 
// Initialize virtual serial port for the Sonar sensor
SoftwareSerial sonarSerial(SONAR_RX_PIN, SONAR_TX_PIN);

// --- GLOBAL VARIABLES ---
int currentDistCm = 0;            // Stores the latest distance reading in centimeters
uint8_t nano_seq = 0;             // Incremental heartbeat counter (0-255)
unsigned long lastCanTx = 0;      // Timestamp to manage 100ms transmission interval
int lastLcdState = -1;            // Stores previous state to avoid redundant LCD refreshes

void setup() {
    Serial.begin(115200);         // Debug serial for PC monitor
    sonarSerial.begin(9600);      // MaxSonar standard baud rate is 9600
    pinMode(LED_HEARTBEAT, OUTPUT);

    // 1. LCD Setup
    lcd.init();                   // Initialize the LCD hardware
    lcd.backlight();              // Turn on the screen light
    lcd.setCursor(0, 0);          // Column 0, Row 0
    lcd.print("Node 0x111 Active");

    // 2. CAN Shield Setup
    CAN.setPins(CAN_CS_PIN, CAN_INT_PIN); // Configure MCP2515 pins
    CAN.setClockFrequency(8E6);           // Set for 8MHz crystal (common on shields)
    
    // Attempt to start CAN at 500kbps (must match the PC and ESP32 nodes)
    if (!CAN.begin(500E3)) {
        lcd.setCursor(0, 1);
        lcd.print("CAN FAIL");
        while (1);                // Halt if CAN hardware is missing/broken
    }
}

/**
 * Checks for incoming CAN messages specifically addressed to this node.
 * Used here for receiving simple "Brain" status updates from the PC.
 */
void handleIncomingCAN() {
    int packetSize = CAN.parsePacket(); // Check if a packet has arrived
    // If packet exists and ID is 0x500 (PC Command ID for Nano)
    if (packetSize && CAN.packetId() == 0x500) {
        int newState = CAN.read();      // Read the data byte
        
        // --- DEBOUNCE LOGIC ---
        // Only update the physical screen if the status has actually changed
        if (newState != lastLcdState) {
            lcd.setCursor(0, 3);        // Move to the bottom row
            if (newState == 1) {
                lcd.print("Brain CMD Received!");
            } else {
                lcd.print("                    "); // Clear the row with spaces
            }
            lastLcdState = newState;    // Record the new state
        }
    }
}

void loop() {
    // 1. Check for CAN commands AS FAST AS POSSIBLE to ensure responsiveness
    handleIncomingCAN();

    // 2. READ SONAR DATA
    // MaxSonar sensors send strings like "R010" (meaning 10 inches)
    if (sonarSerial.available() > 0) {
        char c = sonarSerial.read();
        if (c == 'R') {                 // 'R' marks the start of a reading
            unsigned long startRead = millis();
            String rStr = "";
            // Non-blocking read: wait up to 40ms for the 3-digit distance string
            while(millis() - startRead < 40 && rStr.length() < 3) {
                if(sonarSerial.available()) {
                    char d = sonarSerial.read();
                    if(isDigit(d)) rStr += d; // Only keep numeric characters
                }
            }
            
            // If we successfully read a number
            if (rStr.length() > 0) {
                // Convert inches to centimeters (1 inch = 2.54 cm)
                currentDistCm = (int)(rStr.toInt() * 2.54);
                
                // Update the LCD with the new distance and sequence
                lcd.setCursor(0, 1);
                lcd.print("Dist: "); lcd.print(currentDistCm); lcd.print(" cm   ");
                lcd.setCursor(0, 2);
                lcd.print("Seq: "); lcd.print(nano_seq); lcd.print("      ");
            }
        }
    }

    // 3. TRANSMIT STATUS (Every 100ms)
    // Sends telemetry back to the PC Dashboard (Node ID 0x111)
    if (millis() - lastCanTx >= 100) {
        lastCanTx = millis();
        
        CAN.beginPacket(0x111);         // Start a packet with ID 0x111
        CAN.write(currentDistCm & 0xFF); // Byte 0: Distance (truncated to 8-bit)
        CAN.write(nano_seq++);           // Byte 1: Heartbeat counter, then increment
        
        if (CAN.endPacket()) {
            // Toggle the LED to provide a visual "blink" on every successful send
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }
}