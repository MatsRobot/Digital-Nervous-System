#include <Arduino.h>
#include <Wire.h>           // Essential for I2C communication
#include <Adafruit_GFX.h>    // Base graphics functions
#include <Adafruit_SSD1306.h>// Driver for the 128x64 OLED
#include <VL53L0X.h>        // Driver for the Distance Sensor

// -----------------------------------------------------------------
// HARDWARE DEFINITIONS & PIN MAPPING
// -----------------------------------------------------------------
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C // Common I2C address for SSD1306 displays
#define OLED_RESET -1       // Reset pin (set to -1 if sharing Arduino reset)

const int LED_HEARTBEAT = 13; // Heartbeat LED pin (from Stage 1)

// Define Pins for I2C Bus 0 (Distance Sensor)
#define SDA0 8
#define SCL0 9

// Define Pins for I2C Bus 1 (OLED Display)
#define SDA1 38
#define SCL1 39

// -----------------------------------------------------------------
// OBJECT INITIALIZATION
// -----------------------------------------------------------------
// Initialize OLED using 'Wire1' (the second I2C bus on the ESP32)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, OLED_RESET);

// Initialize the Laser Distance Sensor object
VL53L0X sensor;

void setup() {
    Serial.begin(115200);           // Initialize Serial for debugging
    pinMode(LED_HEARTBEAT, OUTPUT); // Set LED pin as output

    // -------------------------------------------------------------
    // DUAL I2C BUS INITIALIZATION
    // -------------------------------------------------------------
    // Start the first I2C bus (I2C0) on pins 8 and 9 for the sensor
    Wire.begin(SDA0, SCL0);   
    
    // Start the second I2C bus (I2C1) on pins 38 and 39 for the OLED
    Wire1.begin(SDA1, SCL1); 

    // Initialize the OLED display on the Wire1 bus
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("OLED fail")); // Send error to Serial monitor
        for(;;); // Don't proceed if display fails
    }

    // Prepare initial display message
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0,0);
    display.println(F("Initializing Sensor..."));
    display.display();

    // -------------------------------------------------------------
    // SENSOR CALIBRATION
    // -------------------------------------------------------------
    sensor.setBus(&Wire); // Explicitly tell sensor to use the I2C0 bus
    if (!sensor.init()) {
        Serial.println(F("Failed to detect VL53L0X!"));
        display.println(F("Sensor Error!"));
        display.display();
        while (1); // Stop if sensor isn't found
    }
    
    sensor.setTimeout(500);      // Set a 500ms safety timeout
    sensor.startContinuous();    // Start taking readings constantly
}

void loop() {
    // 1. DATA ACQUISITION
    // Ask the sensor for the current distance in millimeters
    uint16_t distance = sensor.readRangeContinuousMillimeters();
    
    // Check if the sensor failed to respond in time
    bool timeout = sensor.timeoutOccurred();

    // 2. USER INTERFACE UPDATE
    display.clearDisplay();      // Wipe previous frame
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println(F("ESP32-S3 CAN NODE"));
    display.println(F("-----------------"));
    
    display.setTextSize(2);      // Use larger font for the reading
    display.setCursor(0, 25);
    
    if (timeout) {
        display.print(F("TIMEOUT")); // Show error on screen if sensor hangs
    } else {
        display.print(distance);     // Display the numeric distance
        display.print(F(" mm"));
    }
    
    display.display();           // Push the memory buffer to the physical screen

    // 3. VISUAL HEARTBEAT
    // Faster blink (100ms cycle) indicates the loop is successfully sampling
    digitalWrite(LED_HEARTBEAT, HIGH);
    delay(50);
    digitalWrite(LED_HEARTBEAT, LOW);
    delay(50);
}