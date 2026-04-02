#include <Arduino.h>              // Core Arduino framework for ESP32
#include <Wire.h>                 // I2C communication library (for the OLED display)
#include <Adafruit_GFX.h>         // Base graphics library for displays
#include <Adafruit_SSD1306.h>     // Specific driver for the SSD1306 OLED screen
#include <ESP32-TWAI-CAN.hpp>     // ESP32's Two-Way Automotive Interface (CAN-Bus) library

// --- HARDWARE PIN DEFINITIONS ---
#define BT_RX 43                  // Bluetooth Serial Receive pin
#define BT_TX 44                  // Bluetooth Serial Transmit pin
#define CAN_TX_PIN 17             // CAN-Bus Transmit pin
#define CAN_RX_PIN 18             // CAN-Bus Receive pin
#define LED_PIN 13                // Onboard LED for status signaling

// --- IR SENSOR PINS (Collision Detection) ---
#define IR_FL 1                   // Front-Left IR Sensor
#define IR_FR 2                   // Front-Right IR Sensor
#define IR_RL 41                  // Rear-Left IR Sensor
#define IR_RR 42                  // Rear-Right IR Sensor

// --- GLOBAL STATE VARIABLES ---
volatile bool humanInControl = false;    // Tracks if Bluetooth is currently overriding CAN commands
volatile uint8_t heartbeatCounter = 0;   // Incrementing counter sent over CAN to show node is alive
String canStatus = "Idle";               // Stores last command received from CAN-Bus for the display
String btStatus = "No Data";             // Stores last command received from Bluetooth for the display
String activeMove = "Stop";              // The current movement command being executed by motors

// --- WATCHDOG TIMER ---
unsigned long lastMsgTime = 0;           // Timestamp of the last valid command received
const unsigned long TIMEOUT_MS = 2000;   // If no command for 2 seconds, stop the robot for safety

// Initialize OLED Display (128x64 pixels) on the second I2C bus (Wire1)
Adafruit_SSD1306 display(128, 64, &Wire1, -1);

/**
 * Logic to translate string commands into physical Motor signals.
 * Includes "Hard-Stop" safety logic based on IR sensor feedback.
 */
void applyMotors(String cmd) {
    // Check if path is blocked (LOW means the sensor sees an object)
    bool blockFwd = (digitalRead(IR_FL) == LOW || digitalRead(IR_FR) == LOW);
    bool blockBwd = (digitalRead(IR_RL) == LOW || digitalRead(IR_RR) == LOW);

    // Safety Override: Force "Stop" if moving toward an obstacle
    if (cmd == "Forward" && blockFwd) cmd = "Stop";
    if (cmd == "Backward" && blockBwd) cmd = "Stop";

    // --- Motor Driver Pins Control ---
    if (cmd == "Forward") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);   // Motor A Forward
        digitalWrite(7, HIGH); digitalWrite(15, LOW);  // Motor B Forward
        analogWrite(6, 200);   analogWrite(16, 200);   // Speed (0-255)
    } else if (cmd == "Backward") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);  // Motor A Backward
        digitalWrite(7, LOW);  digitalWrite(15, HIGH); // Motor B Backward
        analogWrite(6, 200);   analogWrite(16, 200);   // Speed
    } else if (cmd == "Left") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);  // Motor A Backward
        digitalWrite(7, HIGH); digitalWrite(15, LOW);  // Motor B Forward (Spin Left)
        analogWrite(6, 180);   analogWrite(16, 180);   // Turning Speed
    } else if (cmd == "Right") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);   // Motor A Forward
        digitalWrite(7, LOW);  digitalWrite(15, HIGH); // Motor B Backward (Spin Right)
        analogWrite(6, 180);   analogWrite(16, 180);   // Turning Speed
    } else {
        // Full stop: Set all pins LOW and speed to 0
        digitalWrite(4, LOW); digitalWrite(5, LOW);
        digitalWrite(7, LOW); digitalWrite(15, LOW);
        analogWrite(6, 0);    analogWrite(16, 0);
    }
}

/**
 * TASK 1: Communication (Radio)
 * Handles Bluetooth input, CAN-Bus input, and Telemetry output.
 */
void TaskRadio(void * pvParameters) {
    // Start Bluetooth Serial (Serial1) at 9600 baud
    Serial1.begin(9600, SERIAL_8N1, BT_RX, BT_TX);
    Serial1.setTimeout(50); 
    
    // Start CAN-Bus at 500kbps using defined TX/RX pins
    ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN);
    CanFrame rxFrame;           // Buffer for incoming CAN messages
    unsigned long lastHBTime = 0; // Timer for sending telemetry updates

    for(;;) {
        unsigned long now = millis();

        // 1. BLUETOOTH HANDLING (Highest Priority Override)
        if (Serial1.available() > 0) {
            lastMsgTime = now;           // Update watchdog timer
            humanInControl = true;        // Set mode to HUMAN (blocks CAN input)
            String content = Serial1.readStringUntil('\n'); 
            content.trim();
            if (content.length() > 0) {
                btStatus = content;      // Update UI string
                activeMove = content;    // Set motor command
                digitalWrite(LED_PIN, HIGH); // Signal Bluetooth activity
            }
            while(Serial1.available()) Serial1.read(); // Clear remaining buffer
        } else {
            digitalWrite(LED_PIN, LOW);  // LED off when no BT data
        }

        // 2. CAN BUS HANDLING
        if (ESP32Can.readFrame(rxFrame)) {
            // ID 0x456 is the command ID from the PC Dashboard
            if (rxFrame.identifier == 0x456) {
                String cmd = "";
                // Convert CAN data bytes back into a String
                for(int i=0; i<rxFrame.data_length_code; i++) cmd += (char)rxFrame.data[i];
                cmd.trim();
                canStatus = cmd;         // Update UI string
                
                // Only follow CAN if Bluetooth (Human) isn't currently active
                if (!humanInControl) {
                    activeMove = cmd;
                    lastMsgTime = now;   // Update watchdog timer
                }
            }
        }

        // 3. SAFETY WATCHDOG
        // If no valid command (BT or CAN) is received for 2 seconds, stop movement
        if (now - lastMsgTime > TIMEOUT_MS) {
            activeMove = "Stop";
            humanInControl = false;      // Revert to "Brain" mode
            canStatus = "Idle";          // Clear UI text
            btStatus = "Idle";
        }

        // 4. TELEMETRY OUTPUT (Every 25ms)
        // Reports IR sensor states and heartbeat back to the PC Dashboard
        if (now - lastHBTime > 25) { 
            heartbeatCounter++;
            lastHBTime = now;
            
            // Pack IR sensor states into a single byte bitmask
            uint8_t irStatus = 0;
            if (digitalRead(IR_FL) == LOW) irStatus |= 0x01; // Bit 0
            if (digitalRead(IR_FR) == LOW) irStatus |= 0x02; // Bit 1
            if (digitalRead(IR_RL) == LOW) irStatus |= 0x04; // Bit 2
            if (digitalRead(IR_RR) == LOW) irStatus |= 0x08; // Bit 3

            // Send CAN frame with ID 0x123 (S3 Telemetry)
            CanFrame txFrame;
            txFrame.identifier = 0x123;
            txFrame.data_length_code = 4;
            txFrame.data[2] = heartbeatCounter; // Heartbeat in byte 2
            txFrame.data[3] = irStatus;         // IR mask in byte 3
            ESP32Can.writeFrame(txFrame);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS); // Yield to other tasks for 5ms
    }
}

/**
 * TASK 2: Execution & Display
 * Periodically updates the OLED screen and refreshes motor PWM signals.
 */
void TaskExecution(void * pvParameters) {
    Wire1.begin(38, 39); // Start I2C pins for OLED
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C
    display.setTextColor(WHITE);
    
    for(;;) {
        applyMotors(activeMove); // Constantly refresh motor states
        
        // Update OLED screen
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextSize(1);
        display.printf("MODE: %s\n", humanInControl ? "HUMAN" : "BRAIN"); // Control Source
        display.printf("ACT : %s\n", activeMove.c_str());                // Current Command
        display.printf("BT  : %s\n", btStatus.c_str());                  // Last BT Input
        display.printf("CAN : %s\n", canStatus.c_str());                 // Last CAN Input
        display.display();
        
        vTaskDelay(50 / portTICK_PERIOD_MS); // Update screen at 20Hz
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    
    // Set IR sensors as inputs with internal Pullup resistors to stabilize signal
    pinMode(IR_FL, INPUT_PULLUP); pinMode(IR_FR, INPUT_PULLUP);
    pinMode(IR_RL, INPUT_PULLUP); pinMode(IR_RR, INPUT_PULLUP);
    
    // Set Motor Control pins as outputs
    pinMode(4, OUTPUT); pinMode(5, OUTPUT); pinMode(6, OUTPUT);
    pinMode(7, OUTPUT); pinMode(15, OUTPUT); pinMode(16, OUTPUT);
    
    // Create FreeRTOS tasks and assign them to specific ESP32 cores for maximum efficiency
    // Radio Task on Core 0
    xTaskCreatePinnedToCore(TaskRadio, "Radio", 4096, NULL, 1, NULL, 0);
    // Execution/UI Task on Core 1
    xTaskCreatePinnedToCore(TaskExecution, "Exec", 4096, NULL, 1, NULL, 1);
}

void loop() {
    // Empty: Functionality is handled by FreeRTOS Tasks above
}