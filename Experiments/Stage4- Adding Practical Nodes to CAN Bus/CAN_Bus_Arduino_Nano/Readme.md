# 📡 Stage 4.1: Sonar Sensing Node (0x111)

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Experiments-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#circuit-design">
    <img src="https://img.shields.io/badge/Circuit_Design-0366d6?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
  <a href="#firmware">
    <img src="https://img.shields.io/badge/Firmware-238636?style=for-the-badge&logo=visual-studio-code&logoColor=white" />
  </a>
</p>

This node acts as the "eyes" of the robot. Utilizing an **Arduino Nano** and a **MaxSonar** sensor, it captures real-time distance data and broadcasts it across the CAN bus. It also serves as a diagnostic interface via an I2C LCD to display system heartbeats and incoming commands from the PC.

---

## 🎯 Experimental Objectives
* **Distance Telemetry:** Capture ultrasonic range data via SoftwareSerial and convert imperial "Rxxx" strings into metric (cm) units.
* **Heartbeat Monitoring:** Implement a rolling 8-bit sequence counter (`nano_seq`) to allow the "Brain" to verify node liveness.
* **Command Feedback:** Demonstrate bi-directional communication by updating a local LCD based on status updates received from Node `0x500`.

---

## 🔌 Circuit Design & Integration
<a name="circuit-design"></a>

| Component | Role | Connection |
| :--- | :--- | :--- |
| **Arduino Nano** | Microcontroller | Central Logic |
| **MCP2515** | CAN Controller | SPI (D10: CS, D2: INT) |
| **MaxSonar** | Rangefinder | SoftwareSerial (D3: RX) |
| **LCD 2004** | I2C Display | SDA/SCL (Address 0x27) |
| **LED** | Heartbeat | Pin D5 (Toggles on TX) |

> [!TIP]
> **Crystal Frequency:** The firmware is configured for an **8MHz** crystal on the MCP2515. Ensure your shield matches this frequency, or the baud rate will be mismatched.

---

## 💻 Firmware & Source Code
<a name="firmware"></a>

### 📝 Node Logic (`src/main.cpp`)
The firmware utilizes a non-blocking loop to ensure the CAN bus is polled "as fast as possible" for incoming commands while simultaneously handling sensor parsing.

**Key Implementation: Data Packing**
To maintain a clean bus, distance telemetry is sent every 100ms. The distance and sequence are packed into the first two bytes of the CAN frame.

```cpp
// Transmit Status (Every 100ms)
if (millis() - lastCanTx >= 100) {
    lastCanTx = millis();
    CAN.beginPacket(0x111);         // Distance Node ID
    CAN.write(currentDistCm & 0xFF); // Byte 0: Distance (truncated to 8-bit)
    CAN.write(nano_seq++);           // Byte 1: Heartbeat counter
    CAN.endPacket();
}
```

##⚙️ Environment (platformio.ini)
```cpp
[env:sonar_node]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps = 
    sandeepmistry/CAN @ ^0.3.1
    marcoschwartz/LiquidCrystal_I2C @ ^1.1.2
```

##📊 Message Protocol

| ID (Hex) | Direction | Byte 0 | Byte 1 | Description |
| :--- | :--- | :--- | :--- | :--- |
| **0x111** | **TX** | Distance (cm) | Sequence # | Periodic telemetry to Brain |
| **0x500** | **RX** | State (0 or 1) | N/A | PC Command to update LCD |



---	
## 📈 Expected Results

* **Real-time Telemetry**: The LCD displays the distance in centimeters and the incrementing sequence number.
* **Visual Confirmation**: The `LED_HEARTBEAT` (D5) toggles on every successful CAN transmission at 10Hz.
* **Remote Trigger**: When Node `0x500` sends `0x01`, the LCD bottom row displays "Brain CMD Received!".

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
