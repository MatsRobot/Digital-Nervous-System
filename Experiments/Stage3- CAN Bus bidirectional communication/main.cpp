#include <Arduino.h> // Includes the core Arduino functions and ESP32 definitions

// -----------------------------------------------------------------
// HARDWARE DEFINITIONS
// Identifying the physical pin used for the onboard or external LED.
// -----------------------------------------------------------------
// Define a constant for GPIO 13, which corresponds to the '1' label on the board
const int LED_PIN = 13; 

// -----------------------------------------------------------------
// SETUP FUNCTION
// This runs once when the ESP32 starts up or is reset.
// -----------------------------------------------------------------
void setup() {
    // Configure the specified GPIO pin to act as an output (sending voltage out)
    pinMode(LED_PIN, OUTPUT);
}

// -----------------------------------------------------------------
// MAIN LOOP
// This runs repeatedly in an infinite loop as long as the board has power.
// -----------------------------------------------------------------
void loop() {
    // Set the pin to HIGH (3.3V) to turn the LED on
    digitalWrite(LED_PIN, HIGH);
    
    // Pause program execution for 200 milliseconds (0.2 seconds)
    delay(200);
    
    // Set the pin to LOW (0V/Ground) to turn the LED off
    digitalWrite(LED_PIN, LOW);
    
    // Pause program execution for another 200 milliseconds before repeating
    delay(200);
}