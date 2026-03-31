#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32-TWAI-CAN.hpp>

// Hardware Pins
#define BT_RX 43 
#define BT_TX 44 
#define CAN_TX_PIN 17
#define CAN_RX_PIN 18
#define LED_PIN 13  

// IR Sensor Pins (Hard Stops)
#define IR_FL 1  
#define IR_FR 2  
#define IR_RL 41 
#define IR_RR 42 

// Shared Global Variables
volatile bool humanInControl = false;
volatile uint8_t heartbeatCounter = 0; 

String canStatus = "Idle";
String btStatus = "Idle";
String activeMove = "Stop";

Adafruit_SSD1306 display(128, 64, &Wire1, -1);

// Function to handle physical motor constraints
void applyMotors(String cmd) {
    // 1. Read Sensors (LOW = Blocked)
    bool blockFwd = (digitalRead(IR_FL) == LOW || digitalRead(IR_FR) == LOW);
    bool blockBwd = (digitalRead(IR_RL) == LOW || digitalRead(IR_RR) == LOW);

    // 2. Safety Override: If path is blocked, force "Stop" internally
    if (cmd == "Forward" && blockFwd) cmd = "Stop";
    if (cmd == "Backward" && blockBwd) cmd = "Stop";

    // 3. Motor Driver Execution (Pins 4, 5, 6 & 7, 15, 16)
    if (cmd == "Forward") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);
        digitalWrite(7, HIGH); digitalWrite(15, LOW);
        analogWrite(6, 200);   analogWrite(16, 200);
    } else if (cmd == "Backward") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);
        digitalWrite(7, LOW);  digitalWrite(15, HIGH);
        analogWrite(6, 200);   analogWrite(16, 200);
    } else if (cmd == "Left") {
        digitalWrite(4, LOW);  digitalWrite(5, HIGH);
        digitalWrite(7, HIGH); digitalWrite(15, LOW);
        analogWrite(6, 180);   analogWrite(16, 180);
    } else if (cmd == "Right") {
        digitalWrite(4, HIGH); digitalWrite(5, LOW);
        digitalWrite(7, LOW);  digitalWrite(15, HIGH);
        analogWrite(6, 180);   analogWrite(16, 180);
    } else {
        digitalWrite(4, LOW); digitalWrite(5, LOW);
        digitalWrite(7, LOW); digitalWrite(15, LOW);
        analogWrite(6, 0);    analogWrite(16, 0);
    }
}

void TaskRadio(void * pvParameters) {
    Serial1.begin(9600, SERIAL_8N1, BT_RX, BT_TX);
    Serial1.setTimeout(20); 
    ESP32Can.begin(TWAI_SPEED_500KBPS, CAN_TX_PIN, CAN_RX_PIN);
    
    CanFrame rxFrame;
    unsigned long lastHBTime = 0;

    for(;;) {
        // A. BLUETOOTH (Human Priority)
        if (Serial1.available() > 0) {
            String content = Serial1.readString();
            content.trim();
            if (content.length() > 0) {
                btStatus = content;
                if (content == "Stop") {
                    humanInControl = false;
                    activeMove = "Stop";
                } else {
                    humanInControl = true;
                    activeMove = content; // Latch movement
                }
            }
        }

        // B. CAN BUS (Brain Logic)
        if (ESP32Can.readFrame(rxFrame)) {
            if (rxFrame.identifier == 0x456) {
                String cmd = "";
                for(int i=0; i<rxFrame.data_length_code; i++) cmd += (char)rxFrame.data[i];
                cmd.trim();
                canStatus = cmd;
                if (!humanInControl) activeMove = cmd;
            }
        }

        // C. CONSTANT TELEMETRY BROADCAST (Every 200ms for responsiveness)
        // This runs regardless of motor state
        if (millis() - lastHBTime > 200) {
            heartbeatCounter++;
            lastHBTime = millis();
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));

            // Map all 4 IR sensors to bits for the Brain
            uint8_t irStatus = 0;
            if (digitalRead(IR_FL) == LOW) irStatus |= 0x01; // Front Left
            if (digitalRead(IR_FR) == LOW) irStatus |= 0x02; // Front Right
            if (digitalRead(IR_RL) == LOW) irStatus |= 0x04; // Rear Left
            if (digitalRead(IR_RR) == LOW) irStatus |= 0x08; // Rear Right

            CanFrame txFrame;
            txFrame.identifier = 0x123;
            txFrame.data_length_code = 4;
            txFrame.data[0] = 0; 
            txFrame.data[1] = 0; 
            txFrame.data[2] = heartbeatCounter; // Updates "ESP Seq" on Dashboard
            txFrame.data[3] = irStatus;         // Updates ⚪/🔴 indicators on Dashboard
            ESP32Can.writeFrame(txFrame);
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void TaskExecution(void * pvParameters) {
    Wire1.begin(38, 39);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    for(;;) {
        applyMotors(activeMove);
        display.clearDisplay();
        display.setCursor(0,0);
        display.setTextColor(WHITE);
        display.printf("HB: %u | MODE: %s\n", heartbeatCounter, humanInControl ? "HUMAN" : "BRAIN");
        display.printf("CAN: %s\nBT : %s\n", canStatus.c_str(), btStatus.c_str());
        display.setTextSize(2);
        display.print("ACT: "); display.println(activeMove);
        display.display();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    pinMode(IR_FL, INPUT); pinMode(IR_FR, INPUT);
    pinMode(IR_RL, INPUT); pinMode(IR_RR, INPUT);
    pinMode(4, OUTPUT); pinMode(5, OUTPUT); pinMode(6, OUTPUT);
    pinMode(7, OUTPUT); pinMode(15, OUTPUT); pinMode(16, OUTPUT);
    xTaskCreatePinnedToCore(TaskRadio, "Radio", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(TaskExecution, "Exec", 4096, NULL, 1, NULL, 1);
}

void loop() {}