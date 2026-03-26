# 🧪 Experimental Lab: Digital Nervous System

## Stage 2: Dual-Bus I2C Integration (Sensor & UI)

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#dual-bus-architecture">
    <img src="https://img.shields.io/badge/Dual_Bus_Logic-FFA500?style=for-the-badge&logo=cpu&logoColor=white" />
  </a>
  <a href="#hardware-setup">
    <img src="https://img.shields.io/badge/Hardware_Setup-0366d6?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
</p>

This stage evolves from the simple blink to a multi-peripheral system. We are now integrating a **VL53L0X Time-of-Flight (ToF) sensor** and an **SSD1306 OLED display**.

---

## 🎯 Stage 2 Objective: Multi-Peripheral Orchestration
<a name="dual-bus-architecture"></a>

The ESP32-S3 has two independent hardware I2C peripherals. In this stage, we implement a **Dual-Bus Strategy** to maximize stability:
1.  **I2C Bus 0 (GP8/GP9):** Dedicated to the **VL53L0X Sensor**.
2.  **I2C Bus 1 (GP38/GP39):** Dedicated to the **OLED Display**.

**Why two buses?**
Laser sensors can occasionally "hang" or timing-out. By placing the display on a separate bus, we ensure that a sensor error doesn't freeze the user interface, allowing for better real-time debugging.

---

## 🔌 Hardware Setup
<a name="hardware-setup"></a>

Refer to the master circuit diagram for wire colors. 

| Component | Pin Function | ESP32-S3 Pin | Bus ID |
| :--- | :--- | :--- | :--- |
| **VL53L0X** | SDA | **GPIO 8** | Wire (I2C0) |
| **VL53L0X** | SCL | **GPIO 9** | Wire (I2C0) |
| **OLED** | SDA | **GPIO 38** | Wire1 (I2C1) |
| **OLED** | SCL | **GPIO 39** | Wire1 (I2C1) |
| **Heartbeat LED**| Signal | **GPIO 13** | Digital Out |

---

## 💻 Validation Workflow

1.  **Install Libraries:** Ensure `Adafruit SSD1306` and `Pololu VL53L0X` are listed in your `platformio.ini`.
2.  **Upload Firmware:** Flash the Stage 2 code.
3.  **Verification:**
    * **The LED:** Should blink faster than Stage 1 (100ms cycle).
    * **The OLED:** Should display "ESP32-S3 CAN NODE" and a live distance reading.
    * **The Laser:** Move your hand in front of the sensor to see the millimeters update in real-time.

> [!IMPORTANT]
> **I2C Pull-up Resistors:** Most VL53L0X and OLED modules include internal 10k pull-up resistors. If you experience "OLED fail" or "TIMEOUT" messages, check that your 3.3V power rail is stable.


---


<small>© 2026 MatsRobot | Experimental Logs for the Digital Nervous System Project</small>