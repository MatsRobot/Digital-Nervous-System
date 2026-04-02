# 🗼 Stage 4.2: Lidar Scanning Node (0x124)

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

This node functions as the "Advanced Spatial Awareness" unit. Utilizing an **ESP32-C3**, a **VL53L0X Laser ToF sensor**, and a **Servo motor**, it performs a 180° horizontal sweep to map the environment. It calculates real-time trigonometric adjustments before broadcasting spatial data to the CAN bus.

---

## 🎯 Experimental Objectives
* **Active Scanning**: Implement a non-blocking 180° servo sweep to provide a wide field of view.
* **Trigonometric Correction**: Use onboard math to calculate "straight-ahead" adjusted distance ($adjDist = rawDist \times \cos(rads)$) to account for the scanning angle.
* **High-Speed Telemetry**: Broadcast multi-byte spatial data (Raw Dist, Adjusted Dist, Angle) at a consistent 10Hz frequency.

---

![CAN Bus_Node_VL50L0x-ESP32-C3](https://github.com/user-attachments/assets/80779064-36e1-437b-b4f6-1a7fe14faa4f)

---

## 🔌 Circuit Design & Integration
<a name="circuit-design"></a>

| Component | Role | Connection |
| :--- | :--- | :--- |
| **ESP32-C3** | Microcontroller | Central Logic & TWAI Controller |
| **VL53L0X** | Lidar Sensor | I2C (SDA: 5, SCL: 6) |
| **SG90 Servo** | Sweep Actuator | PWM Pin 10 |
| **SSD1306** | OLED Display | I2C (Address 0x3C) |
| **SN65HVD230** | CAN Transceiver | TX: 21, RX: 20 |

> [!IMPORTANT]
> **Voltage Warning**: Ensure the Servo is powered by an external 5V rail, as the ESP32-C3 GPIO pins and internal regulator may not provide sufficient current for motor spikes.

---

## 💻 Firmware & Source Code
<a name="firmware"></a>

### 📝 Node Logic (`src/main.cpp`)
The firmware utilizes the **TWAI (Two-Way Automotive Interface)** driver to handle CAN communication at 500kbps.

**Key Implementation: Spatial Calculation**
Because the sensor rotates, the "raw" distance increases as the angle becomes more acute. The node normalizes this data into a forward-facing distance vector.

```cpp
// --- TRIGONOMETRY CALCULATION ---
float relativeAngle = servoPos - 90;           // Shift 0-180 to -90/+90 range
float rads = relativeAngle * (PI / 180.0);     // Convert degrees to radians
uint16_t adjDist = (uint16_t)(rawDist * cos(rads)); // Adjusted forward dist
```

## ⚙️ Environment (platformio.ini)
```cpp
[env:lidar_node]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
lib_deps = 
    pololu/VL53L0X @ ^1.3.1
    madhephaestus/ESP32Servo @ ^1.1.1
    greiman/SSD1306Ascii @ ^1.3.5
```

---

## 📊 Message Protocol
| ID (Hex) | DLC | Byte 0-1 | Byte 2-3 | Byte 4 | Byte 5 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **0x124** | 6 | Raw Dist (mm) | Adjusted Dist (mm) | Sequence # | Servo Angle |


* **Bytes 0-3**: Sent as 16-bit unsigned integers (Big Endian).
* **Byte 4**: Rolling heartbeat counter (0-255).
* **Byte 5**: Current servo position (0-180°).

---


## 📈 Expected Results
* **Dynamic Feedback**: The onboard OLED displays real-time Raw and Adjusted values alongside the current sequence number.
* **Smooth Sweep**: The servo moves in 1° increments every 20ms without interrupting CAN transmissions.
* **Bus Liveness**: The HEARTBEAT_LED (Pin 3) toggles every 100ms to confirm the 10Hz transmission loop is active.

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
