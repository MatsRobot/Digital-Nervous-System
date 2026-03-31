# 🧪 Experimental Lab: Digital Nervous System

## Stage 3: Multi-Node Heterogeneous CAN Network

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#network-map">
    <img src="https://img.shields.io/badge/Network_Data_Map-blue?style=for-the-badge&logo=arduino&logoColor=white" />
  </a>
</p>

This stage implements a **Heterogeneous Network Layer**. The system has evolved from an isolated ESP32 connection to a multi-node bus architecture, integrating different microcontrollers (ESP32-S3 and Arduino Nano) and distinct sensor technologies (Time-of-Flight and Ultrasound) into a unified real-time Python dashboard.

---

## 🎯 Objectives
1.  **Multi-Architecture Integration:** Orchestrate communication between an ESP32-S3 (3.3V) and an Arduino Nano (5V) on a single 500kbps bus.
2.  **Bidirectional Control:** Implement a "Brain" (Laptop) that can simultaneously receive telemetry from two sensors and send targeted commands to specific nodes.
3.  **Bus Stability:** Overcome timing conflicts between SoftwareSerial, SPI, and I2C protocols.

---

![CAN_Bus_Nano_Display_bb](https://github.com/user-attachments/assets/625fba50-5650-477d-a219-07305f061c11)

---

## 📂 File Architecture & Node Roles

| File | Platform | Role |
| :--- | :--- | :--- |
| **`main.py`** | Laptop (PC) | **The N100 Brain:** Creates the central dashboard to communicate with all sensors and coordinate network traffic. |
| **`main.cpp` (Nano)** | Arduino Nano | **Ultrasound Node:** Manages the MaxSonar LV-EZ1 sensor and the LCD2004 status display. |
| **`main.cpp` (ESP32)** | ESP32-S3 | **ToF Node:** Running the baseline code from Stage 2 to manage the VL53L0X sensor and OLED. |

---

## 🧠 1. Network Data Map
This table defines the traffic priority and data structure across the CAN-High and CAN-Low lines.

| CAN ID | Direction | Content | Data Format |
| :--- | :--- | :--- | :--- |
| **0x111** | Nano → Laptop | Ultrasound Distance (cm) + Seq | `[Dist_Byte, Seq_Byte]` |
| **0x123** | ESP32 → Laptop | ToF Distance (mm) + Seq | `[Dist_High, Dist_Low, Seq_Byte]` |
| **0x456** | Laptop → ESP32 | Motor Commands | `ASCII ("Forward", "Stop", etc.)` |
| **0x500** | Laptop → Nano | LCD Display Toggle | `[0x01 = Show, 0x00 = Clear]` |

---

## 🔌 2. Physical Pinout & Hardware Baseline
Detailed wiring for the two unique sensor nodes in this stage.

### **Node A: Arduino Nano (Ultrasound & LCD2004)**
* **CAN Controller (MCP2515):** SCK(D13), MISO(D12), MOSI(D11), CS(D10), INT(D2).
* **Sensor:** MaxSonar LV-EZ1 (TX) → Nano **D3** (SoftwareSerial RX).
* **Display:** LCD2004 I2C → **A4 (SDA), A5 (SCL)**.
* **Oscillator:** 8MHz Crystal (Defined in firmware as `8E6`).

### **Node B: ESP32-S3 (ToF & OLED)**
* **CAN Transceiver:** SN65HVD230.
* **Interface:** TX(GPIO 17), RX(GPIO 18).
* **Sensor:** VL53L0X I2C0 → **GPIO 8 (SDA), GPIO 9 (SCL)**.
* **Display:** OLED I2C1 → **GPIO 38 (SDA), GPIO 39 (SCL)**.

---

## 🛠️ 3. Key Stability Measures
To resolve initial communication "hit and miss" issues, the following technical baselines were implemented:

1.  **Non-Blocking Logic:** Nano firmware was refactored to eliminate `delay()` calls, allowing the CPU to poll the CAN controller thousands of times per second.
2.  **State Tracking (Delta-Updates):** The LCD/OLED displays only refresh when the command state *changes*. This prevents the slow I2C bus from "choking" the CPU and creating SPI timing jitter.
3.  **Serial Buffer Management:** The Nano's `SoftwareSerial` buffer is manually flushed if it exceeds 15 bytes to ensure the ultrasound "R" header remains synchronized.

---

![Network_Laptop_Nano_ESP32S3](https://github.com/user-attachments/assets/c852ec64-d433-427d-8404-984e2b9cd33c)

---

## 💻 4. Final Python Dashboard Baseline
The laptop acts as the network coordinator using a USB-to-CAN adapter.

* **Libraries:** `python-can`, `pyserial`.
* **Interface:** `slcan` protocol via COM port.
* **Bitrate:** 500,000 (500kbps).
* **Cycle Time:** 100ms (10Hz) transmission for motor and LCD commands.

---

### **Execution**

#### **Option A: Virtual Environment (Recommended)**
1.  **Activate Environment:** `.venv\Scripts\activate`
2.  **Run Monitor:** `python main.py`

#### **Option B: Direct PowerShell Execution**
If libraries (`python-can`, `pyserial`) are installed globally, you can run the monitor directly from any PowerShell terminal:
1.  **Open PowerShell** in the project directory.
2.  **Run Monitor:**
    ```powershell
    python main.py
    ```

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
