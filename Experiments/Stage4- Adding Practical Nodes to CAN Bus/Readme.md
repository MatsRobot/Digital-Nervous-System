# 🛠️ Stage 4: Adding 4 Practical Nodes to the CAN Bus

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

**Stage 4** transitions from theoretical communication to functional robot "muscles." In this stage, we deploy multiple **Actuator Nodes** (Arduino Nano / Raspberry Pi Pico) to the bus to handle real-world physical tasks.

---

<img width="1361" height="768" alt="Practical Nodes to CAN Bus" src="https://github.com/user-attachments/assets/94442384-8459-434c-871d-8b73448b4709" />

---

## 🎯 Experimental Objectives
* **Node Scalability:** Verify the bus stability when more than two nodes are communicating simultaneously.
* **Command Routing:** Ensure the **N100/ESP32 Gateway** can target specific Node IDs (e.g., `0x201` for Leg 1) without interfering with others.
* **Practical Payload:** Move from "Blink" tests to sending actual PWM duty cycles or stepper motor coordinates over the 8-byte data field.

---

## 🔌 Circuit Design & Integration
<a name="circuit-design"></a>

| Component | Role | Connection |
| :--- | :--- | :--- |
| **MCP2515** | CAN Controller | SPI Interface to Nano/Pico |
| **TJA1050** | CAN Transceiver | Physical Bus Interface (H/L) |
| **Actuator** | "The Muscle" | PWM / H-Bridge / Stepper Driver |

> [!NOTE]
> **Termination Check:** As this stage adds physical length to the bus, ensure the 120Ω resistors remain only at the two furthest physical ends of the chain to avoid signal reflections.

---
## 🚌 Physical Network Topology: The Daisy-Chain

To maintain signal integrity at **500 KBPS**, the hardware is wired in a linear "Daisy-Chain" configuration. A single twisted-pair cable (CAN High & CAN Low) runs the length of the robot, connecting each node in sequence.


### 🛰️ Node Sequence
The bus starts at your workstation and travels through each specialized sensor node before ending at the drive system:

1.  **Node 0: The Brain (Laptop)** Connected via a **USB-to-CAN SLCAN Adapter**. This is the entry point for the Python Dashboard.
2.  **Node 1: Sonar Node (Arduino Nano)** Uses an **MCP2515 Shield** to interface with the bus.
3.  **Node 2: Calc Node (ESP32-C3)** Uses an **SN65HVD230 transceiver** to send adjusted Lidar data.
4.  **Node 3: Base Unit (ESP32-S3)** The final node in the chain, managing motors and IR safety.

### 🛠️ Wiring Specifications

| Feature | Requirement |
| :--- | :--- |
| **Cable Type** | 120Ω Shielded Twisted Pair (STP) preferred. |
| **Termination** | **120Ω Resistors** must be placed ONLY at Node 0 and Node 3. |
| **CAN_H / CAN_L** | Ensure polarity is consistent across all four terminal blocks. |

---

## 💻 Firmware & Source Code
<a name="firmware"></a>

### 📝 Node Logic (`src/main.cpp`)
The code for this stage implements **ID Filtering**. Instead of processing every message, the node only "wakes up" when it sees its specific assigned ID.

```cpp
// Example: Filter for Node 0x201 (Leg Actuator)
if (canId == 0x201) {
    int motorSpeed = data[0]; // Extract first byte of payload
    analogWrite(MOTOR_PIN, motorSpeed);
}

```
# Multi-Node CAN Robotic Control System

## 📌 Project Overview
This project involved the development of a distributed robotic control system using a **Controller Area Network (CAN)**. The architecture leverages an **ESP32-S3** as the primary Base Unit (Motor Controller), an **Arduino Nano** for sonar data, and an **ESP32-C3** for environmental calculations. The system is managed by a high-speed Python Dashboard ("The Brain") with a real-time manual override via Bluetooth (HC-06).

## 🛠️ Core Implementations

### 1. Dual-Priority Command Logic
To balance autonomous "Brain" control with manual "Human" safety, we implemented a strict priority hierarchy in the Base Unit firmware:
* **Bluetooth Supremacy**: Any data received via the HC-06 module (Bluetooth) immediately triggers the `humanInControl` flag.
* **Handover Watchdog**: The system uses a 2000ms safety timeout. If the Bluetooth source remains silent for >2 seconds, the system resets, clearing the "Human" lock and allowing the CAN-based Brain to resume control.
* **State Reset**: Upon a timeout, the system forces an "Idle" state, clearing the OLED display and halting motors to prevent "runaway" scenarios.

### 2. Bus Optimization: "Silence is Stop"
We moved away from a continuous "flood" of data to maintain a clean, efficient bus:
* **Event-Driven Transmission**: The Python dashboard only sends movement strings (e.g., "Forward") while a control button is actively pressed.
* **Heartbeat Logic**: To keep the hardware watchdog satisfied, the dashboard sends a single command "pulse" every 1 second during active hold.
* **Reduced Overhead**: By stopping all transmissions when no button is pressed, we utilize the hardware's natural timeout to stop the robot, reducing bus traffic by approximately 90% during idle periods.

### 3. High-Speed Telemetry & Bitmasking
To achieve a "real-time" feel in the dashboard, telemetry was optimized at both the hardware and software levels:
* **Fast Hardware Updates**: The ESP32-S3 transmits telemetry frames every 25ms.
* **Data Packing**: All four IR sensors (FL, FR, RL, RR) are packed into a single 8-bit integer using bitmasking (e.g., `irStatus |= 0x04` for Rear Left) to keep the CAN payload minimal.
* **High-Frequency Polling**: The Python Brain uses a dedicated thread with a 1ms `bus.recv()` timeout, ensuring incoming sensor data is processed and displayed instantly.

## 📊 Hardware & Network Configuration

| Node | ID (Hex) | Role | Key Hardware |
| :--- | :--- | :--- | :--- |
| **Base Unit** | `0x123` (TX) / `0x456` (RX) | Motor & Logic Controller | ESP32-S3, SSD1306 OLED, HC-06 |
| **Brain** | `0x456` (TX) / `0x123` (RX) | Dashboard & Autonomous Logic | Python (python-can), SLCAN Adapter |
| **Sonar Node** | `0x111` | Distance Sensing | Arduino Nano, HC-SR04 |
| **Calc Node** | `0x124` | Adjusted Distance/Angle | ESP32-C3 |

**Network Speed:** 500 KBPS (TWAI/CAN)

## ✅ Outcomes
* **Robust Safety**: Integrated local IR "Hard Stops" in the motor driver logic work independently of the CAN bus to prevent collisions even during signal loss.
* **Dynamic Handover**: Seamlessly switch between Python-based navigation and manual Bluetooth remote control without rebooting.
* **OLED Feedback**: The onboard display provides immediate diagnostic data, including current control mode (HUMAN/BRAIN) and active command status.

---
## ⚙️ Environment (platformio.ini)

[env:actuator_node]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps = 
    sandeepmistry/CAN @ ^0.3.1


---	
## 📈 Expected Results
Zero Collision Data: Confirm that high-priority messages (E-Stop) still override motor telemetry.

Thermal Stability: Monitor the transceivers for heat during prolonged bidirectional bursts.


---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
