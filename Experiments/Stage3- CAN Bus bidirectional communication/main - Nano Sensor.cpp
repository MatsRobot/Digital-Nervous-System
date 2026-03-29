#include <Arduino.h>            // Base Arduino functions
#include <SPI.h>                // Used for communication with the CAN module
#include <CAN.h>                // Sandeep Mistry CAN library
#include <Wire.h>               // I2C library for the LCD
#include <LiquidCrystal_I2C.h>  // Controls the 20x4 LCD screen
#include <SoftwareSerial.h>     // Creates a serial port on digital pins for the Sonar

// --- Pin Definitions ---
#define CAN_CS_PIN 10           // Chip Select for CAN module
#define CAN_INT_PIN 2           // Interrupt pin for CAN module
#define SONAR_RX_PIN 3          // Receive pin for MaxSonar sensor
#define SONAR_TX_PIN 4          // Transmit pin (unused, but required for library)
#define LED_HEARTBEAT 5         // LED that blinks when data is sent

// Initialize LCD at address 0x27, 20 columns, 4 rows
LiquidCrystal_I2C lcd(0x27, 20, 4); 
// Initialize Serial for Sonar sensor at pins 3 and 4
SoftwareSerial sonarSerial(SONAR_RX_PIN, SONAR_TX_PIN);

// Global variables
int currentDistCm = 0;          // Stores the latest distance reading
uint8_t nano_seq = 0;           // Sequence number (increments every message)
unsigned long lastCanTx = 0;    // Timing variable for sending data
int lastLcdState = -1;          // Stores previous LCD command to prevent screen flickering

void setup() {
    Serial.begin(115200);       // Start hardware serial for debugging
    sonarSerial.begin(9600);    // Start sonar serial (standard for MaxSonar)
    pinMode(LED_HEARTBEAT, OUTPUT); // Configure LED as output

    // Initialize LCD Screen
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Node 0x111 Active");

    // Initialize CAN Bus module
    CAN.setPins(CAN_CS_PIN, CAN_INT_PIN);
    CAN.setClockFrequency(8E6);  // Set to 8MHz (matches most MCP2515 modules)
    if (!CAN.begin(500E3)) {     // Try to start at 500kbps
        lcd.setCursor(0, 1);
        lcd.print("CAN FAIL");
        while (1);               // Halt if CAN fails
    }
}

void handleIncomingCAN() {
    /* Checks if there is a message for this node from the Dashboard */
    int packetSize = CAN.parsePacket();
    // Look specifically for ID 0x500 (Dashboard LCD command)
    if (packetSize && CAN.packetId() == 0x500) {
        int newState = CAN.read(); // Read the first byte (1 or 0)
        
        // Optimization: Only update the screen if the value actually changed
        if (newState != lastLcdState) {
            lcd.setCursor(0, 3); // Go to the 4th line
            if (newState == 1) {
                lcd.print("Brain CMD Received!");
            } else {
                lcd.print("                    "); // Send spaces to clear the line
            }
            lastLcdState = newState; // Update history
        }
    }
}

void loop() {
    // 1. RECEIVE: Check for commands from the Brain as fast as possible
    handleIncomingCAN();

    // 2. SENSOR: Read Sonar data from Serial
    if (sonarSerial.available() > 0) {
        char c = sonarSerial.read();
        if (c == 'R') { // MaxSonar messages start with 'R' (e.g., "R072")
            unsigned long startRead = millis();
            String rStr = "";
            // Timeout loop to collect the 3 digits following 'R'
            while(millis() - startRead < 40 && rStr.length() < 3) {
                if(sonarSerial.available()) {
                    char d = sonarSerial.read();
                    if(isDigit(d)) rStr += d;
                }
            }
            if (rStr.length() > 0) {
                // Convert inches to cm (approx 2.54 multiplier)
                currentDistCm = (int)(rStr.toInt() * 2.54);
                lcd.setCursor(0, 1);
                lcd.print("Dist: "); lcd.print(currentDistCm); lcd.print(" cm   ");
            }
        }
    }

    // 3. TRANSMIT: Send distance to the CAN bus every 100ms (10Hz)
    if (millis() - lastCanTx >= 100) {
        lastCanTx = millis();
        CAN.beginPacket(0x111);        // Start a packet with ID 0x111
        CAN.write(currentDistCm & 0xFF); // Send distance (low byte)
        CAN.write(nano_seq++);         // Send sequence number then increment
        if (CAN.endPacket()) {         // Finalize and send
            // Toggle LED to show successful transmission
            digitalWrite(LED_HEARTBEAT, !digitalRead(LED_HEARTBEAT));
        }
    }
}