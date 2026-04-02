#include <Arduino.h>         // Base framework for ESP32
#include <Wire.h>            // Required for I2C communication (SDA/SCL)
#include <VL53L0X.h>         // Driver for the laser distance sensor
#include <ESP32Servo.h>      // Specialized servo control for ESP32 chips
#include <math.h>            // Needed for Cosine and Radians calculations
#include "SSD1306Ascii.h"    // High-speed text-only OLED library
#include "SSD1306AsciiWire.h"// I2C interface for the OLED library
#include "driver/twai.h"     // The "Two-Way Automotive Interface" (CAN driver)

// --- PIN DEFINITIONS ---
#define SDA_PIN 5              // I2C Data pin for the OLED and Lidar sensor
#define SCL_PIN 6              // I2C Clock pin
#define CAN_TX_PIN GPIO_NUM_21 // CAN Transmit pin
#define CAN_RX_PIN GPIO_NUM_20 // CAN Receive pin
#define SERVO_PIN 10           // PWM pin connected to the scanning servo
#define HEARTBEAT_LED 3        // Onboard LED to indicate the code is running

// --- GLOBAL OBJECTS ---
SSD1306AsciiWire oled;       // Create the OLED display object
VL53L0X sensor;              // Create the Time-of-Flight sensor object
Servo scanningServo;         // Create the Servo motor object

// --- STATE VARIABLES ---
bool sensorActive = false;   // Track if Lidar initialized correctly
bool canActive = false;      // Track if CAN bus started correctly
uint8_t c3_seq = 0;          // Counter that increments with every message sent
int servoPos = 0;            // Current angle (0-180 degrees)
int servoDirection = 1;      // Toggles between +1 and -1 for back-and-forth sweep
unsigned long lastServoMove = 0; // Timer for movement speed control
unsigned long lastTx = 0;    // Timer for sending CAN messages (10Hz)
bool ledState = false;       // Toggles the LED on/off

void setup() {
    Serial.begin(115200);           // Start serial debug output
    pinMode(HEARTBEAT_LED, OUTPUT); // Set LED as an output pin
    Wire.begin(SDA_PIN, SCL_PIN);   // Start the I2C bus
    Wire.setClock(400000);          // Set I2C speed to 400kHz (Fast Mode)

    // 1. OLED DISPLAY INITIALIZATION
    oled.begin(&Adafruit128x64, 0x3C); // Address 0x3C is standard for OLEDs
    oled.setFont(Adafruit5x7);         // Set font size
    oled.clear();                      // Clear any old data
    oled.println("Node: 0x124");       // Print this node's CAN ID

    // 2. SERVO CONFIGURATION
    ESP32PWM::allocateTimer(0);         // Assign a hardware timer for the PWM signal
    scanningServo.setPeriodHertz(50);   // Standard 50Hz for servo control
    // Attach servo with specific pulse widths (0.5ms to 2.4ms)
    scanningServo.attach(SERVO_PIN, 500, 2400);

    // 3. CAN BUS (TWAI) CONFIGURATION
    // General config sets pins and operating mode (NORMAL)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    // Timing config sets the bitrate to 500k
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    // Filter config allows us to receive any message ID (or none in this case)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Attempt to install the driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        twai_start();                  // Enable the CAN controller
        canActive = true;              // Mark as ready
        oled.println("CAN: Connected");
    }

    // 4. LIDAR SENSOR INITIALIZATION
    if (sensor.init()) {               // Check if sensor responds over I2C
        sensor.startContinuous();      // Set to read range constantly
        sensorActive = true;           // Mark as ready
        oled.println("Lidar: Online");
    }
}

void loop() {
    unsigned long currentMillis = millis(); // Get the current uptime

    // --- STEP 1: SERVO SWEEP (NON-BLOCKING) ---
    // Every 20ms, move the servo by 1 degree
    if (currentMillis - lastServoMove >= 20) {
        lastServoMove = currentMillis;
        servoPos += servoDirection; // Add or subtract 1 degree
        
        // Reverse direction at the 0 or 180 degree limits
        if (servoPos <= 0 || servoPos >= 180) servoDirection *= -1;
        
        scanningServo.write(servoPos); // Send new position to servo
    }

    // --- STEP 2: DATA PROCESSING & CAN TX (10Hz) ---
    // Executes exactly every 100 milliseconds
    if (currentMillis - lastTx >= 100) {
        lastTx = currentMillis;
        ledState = !ledState;
        digitalWrite(HEARTBEAT_LED, ledState); // Blink the status LED

        // Get distance in mm from the sensor
        uint16_t rawDist = sensorActive ? sensor.readRangeContinuousMillimeters() : 0;
        
        // --- TRIGONOMETRY CALCULATION ---
        // If the servo is at an angle, the beam measures a longer "hypotenuse".
        // We calculate the "straight ahead" distance using: D_adj = D_raw * cos(angle)
        float relativeAngle = servoPos - 90;           // Shift 0-180 to -90/+90 range
        float rads = relativeAngle * (PI / 180.0);    // Convert degrees to radians
        uint16_t adjDist = (uint16_t)(rawDist * cos(rads)); // Calculate adjusted forward dist

        // --- UPDATE LOCAL OLED ---
        oled.setCursor(0, 3);
        oled.print("Raw:  "); oled.print(rawDist); oled.print(" mm    ");
        oled.setCursor(0, 4);
        oled.print("Adj:  "); oled.print(adjDist); oled.print(" mm    ");
        oled.setCursor(0, 5);
        oled.print("Deg:  "); oled.print(servoPos); oled.print(" ("); oled.print((int)relativeAngle); oled.print(") ");
        oled.setCursor(0, 6);
        oled.print("Seq:  "); oled.print(c3_seq);

        // --- CONSTRUCT AND SEND CAN MESSAGE ---
        if (canActive) {
            twai_message_t msg;
            msg.identifier = 0x124;    // The unique ID for this Node
            msg.data_length_code = 6;  // Sending 6 bytes of data
            
            // Split 16-bit distance values into two 8-bit bytes for CAN
            msg.data[0] = (uint8_t)(rawDist >> 8);   // Raw Distance (Upper 8 bits)
            msg.data[1] = (uint8_t)(rawDist & 0xFF); // Raw Distance (Lower 8 bits)
            msg.data[2] = (uint8_t)(adjDist >> 8);   // Adjusted Distance (Upper 8 bits)
            msg.data[3] = (uint8_t)(adjDist & 0xFF); // Adjusted Distance (Lower 8 bits)
            msg.data[4] = c3_seq++;                  // Heartbeat counter (0-255)
            msg.data[5] = (uint8_t)servoPos;         // Current angle of scan
            
            // Transmit onto the bus with a 10ms timeout
            twai_transmit(&msg, pdMS_TO_TICKS(10));
        }
    }
}