#include <Arduino.h>            // The core library for ESP32/Arduino programming
#include <Wire.h>               // Library for I2C communication (used for the OLED screen)
#include <Adafruit_GFX.h>       // Graphics library to help draw text/shapes on the screen
#include <Adafruit_SSD1306.h>   // Specific driver for the SSD1306 OLED display model
#include <ESP32-TWAI-CAN.hpp>   // Library to handle CAN-Bus (Car-style) communication

// --- Hardware Pin Definitions ---
// These define which physical holes (pins) on the ESP32 chip connect to which parts
#define BT_RX 43                // Bluetooth Receive pin (Data coming IN from Phone)
#define BT_TX 44                // Bluetooth Transmit pin (Data going OUT to Phone)
#define CAN_TX_PIN 17           // CAN-Bus Transmit (Sending data to the Laptop)
#define CAN_RX_PIN 18           // CAN-Bus Receive (Getting data from the Laptop)
#define LED_PIN 13              // The small built-in light on the ESP32 board

// --- IR Sensor Pins (The "Eyes" for Hard Stops) ---
#define IR_FL 1                 // Front Left Sensor (GPIO 1)
#define IR_FR 2                 // Front Right Sensor (GPIO 2)
#define IR_RL 41                // Rear Left Sensor (GPIO 41)
#define IR_RR 42                // Rear Right Sensor (GPIO 42)

// --- Shared Global Variables ---
// 'volatile' means these can change at any time between the two processor cores
volatile bool humanInControl = false; // Tracks if a human is currently using Bluetooth
volatile uint8_t heartbeatCounter = 0; // A ticking number (0-255) to show the system is "alive"

String canStatus = "Idle";      // Text storage for the last command from the Laptop
String btStatus = "Idle";       // Text storage for the last command from the Phone
String activeMove = "Stop";     // The final decision on how the robot should move

// Initialize the display: 128x64 pixels, using Wire1 (pins 38/39), no reset pin (-1)
Adafruit_SSD1306 display(128, 64, &Wire1, -1);

// --- Function: applyMotors ---
// This function takes a word like "Forward" and turns it into physical wheel movement.
void applyMotors(String cmd) {
    // 1. Check for Obstacles: IR sensors go "LOW" (0) when they see something close.
    bool blockFwd = (digitalRead(IR_FL) == LOW || digitalRead(IR_FR) == LOW);
    bool blockBwd = (digitalRead(IR_RL) == LOW || digitalRead(IR_RR) == LOW);

    // 2. Safety Gate: If the command is "Forward" but something is in front, change it to "Stop".
    if (cmd == "Forward" && blockFwd) cmd = "Stop";
    if (cmd == "Backward" && blockBwd) cmd = "Stop";

    // 3. Physical Execution: Setting the Motor Driver pins
    if (cmd == "Forward") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);   // Left Motor: spin forward
        digitalWrite(7, HIGH); digitalWrite(15, LOW);  // Right Motor: spin forward
        analogWrite(6, 200);   analogWrite(16, 200);   // Speed: 200 out of 255
    } 
    else if (cmd == "Backward") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);  // Left Motor: spin backward
        digitalWrite(7, LOW);  digitalWrite(15, HIGH); // Right Motor: spin backward
        analogWrite(6, 200);   analogWrite(16, 200);
    } 
    else if (cmd == "Left") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);  // Left backward
        digitalWrite(7, HIGH); digitalWrite(15, LOW);  // Right forward (Turn in place)
        analogWrite(6, 180);   analogWrite(16, 180);
    } 
    else if (cmd == "Right") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);   // Left forward
        digitalWrite(7, LOW);  digitalWrite(15, HIGH); // Right backward
        analogWrite(6, 180);   analogWrite(16, 180);
    } 
    else { // "Stop" or anything else
        digitalWrite(4, LOW); digitalWrite(5, LOW);    // Turn off all motor pins
        digitalWrite(7, LOW); digitalWrite(15, LOW);
        analogWrite(6, 0);    analogWrite(16, 0);      // Set speed to 0
    }
}

// --- Task: TaskRadio (Running on Core 0) ---
// This handles all the "Talking" and "Listening" (Bluetooth and CAN)
void TaskRadio(void * pvParameters) {
    Serial1.begin(9600, SERIAL_8N1, BT_RX, BT_TX); // Start Bluetooth communication
    Serial1.setTimeout(20);                       // Don't wait long for Bluetooth text
    ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN); // Start CAN-Bus
    
    CanFrame rxFrame;                             // Storage for an incoming CAN message
    unsigned long lastHBTime = 0;                 // Timer for the telemetry broadcast

    for(;;) { // Infinite loop for this core
        // A. BLUETOOTH HANDLING: Check if the phone sent a message
        if (Serial1.available() > 0) {
            String content = Serial1.readString();
            content.trim();                       // Remove extra spaces or hidden characters
            if (content.length() > 0) {
                btStatus = content;
                if (content == "Stop") {
                    humanInControl = false;       // Human let go of the button
                    activeMove = "Stop";
                } else {
                    humanInControl = true;        // Human is pushing a button!
                    activeMove = content;         // Override everything else
                }
            }
        }

        // B. CAN BUS HANDLING: Check if the Brain (Laptop) sent a command
        if (ESP32Can.readFrame(rxFrame)) {
            if (rxFrame.identifier == 0x456) {    // If it's a movement command ID
                String cmd = "";
                for(int i=0; i<rxFrame.data_length_code; i++) cmd += (char)rxFrame.data[i];
                cmd.trim();
                canStatus = cmd;
                // Only follow the Laptop if a Human isn't using Bluetooth right now
                if (!humanInControl) activeMove = cmd;
            }
        }

        // C. CONSTANT TELEMETRY: Send data back to the Laptop every 200ms
        if (millis() - lastHBTime > 200) {
            heartbeatCounter++;
            lastHBTime = millis();
            digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Blink LED to show life

            // Map the 4 IR sensors into 4 bits (on/off) of a single byte
            uint8_t irStatus = 0;
            if (digitalRead(IR_FL) == LOW) irStatus |= 0x01; // Bit 0
            if (digitalRead(IR_FR) == LOW) irStatus |= 0x02; // Bit 1
            if (digitalRead(IR_RL) == LOW) irStatus |= 0x04; // Bit 2
            if (digitalRead(IR_RR) == LOW) irStatus |= 0x08; // Bit 3

            CanFrame txFrame;
            txFrame.identifier = 0x123;           // Identity of this ESP32
            txFrame.data_length_code = 4;         // Sending 4 bytes of data
            txFrame.data[0] = 0; txFrame.data[1] = 0; 
            txFrame.data[2] = heartbeatCounter;  // Seq number for the Laptop
            txFrame.data[3] = irStatus;         // Obstacle data for the Laptop
            ESP32Can.writeFrame(txFrame);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);       // Brief rest to let Core 0 breathe
    }
}

// --- Task: TaskExecution (Running on Core 1) ---
// This handles the "Body" (Motors and Screen)
void TaskExecution(void * pvParameters) {
    Wire1.begin(38, 39);                          // Start the I2C screen wires
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);    // Initialize the OLED hardware
    for(;;) {
        applyMotors(activeMove);                  // Physically move based on latest choice

        display.clearDisplay();                   // Wipe the screen for fresh text
        display.setCursor(0,0);
        display.setTextColor(WHITE);
        display.printf("HB: %u | MODE: %s\n", heartbeatCounter, humanInControl ? "HUMAN" : "BRAIN");
        display.printf("CAN: %s\nBT : %s\n", canStatus.c_str(), btStatus.c_str());
        display.setTextSize(2);                   // Make the next line big
        display.print("ACT: "); display.println(activeMove);
        display.display();                        // Push the text to the actual glass
        
        vTaskDelay(50 / portTICK_PERIOD_MS);      // Update the screen 20 times per second
    }
}

void setup() {
    // Set up the built-in LED and IR sensor pins as inputs/outputs
    pinMode(LED_PIN, OUTPUT);
    pinMode(IR_FL, INPUT); pinMode(IR_FR, INPUT);
    pinMode(IR_RL, INPUT); pinMode(IR_RR, INPUT);
    
    // Set up the motor controller pins as outputs
    pinMode(4, OUTPUT); pinMode(5, OUTPUT); pinMode(6, OUTPUT);
    pinMode(7, OUTPUT); pinMode(15, OUTPUT); pinMode(16, OUTPUT);

    // DUAL CORE SETUP: Start the two tasks on different "brains" inside the chip
    xTaskCreatePinnedToCore(TaskRadio, "Radio", 4096, NULL, 1, NULL, 0); // Task on Core 0
    xTaskCreatePinnedToCore(TaskExecution, "Exec", 4096, NULL, 1, NULL, 1); // Task on Core 1
}

void loop() {
    // This is empty because our "Tasks" above handle everything!
}