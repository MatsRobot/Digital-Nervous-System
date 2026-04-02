# 🧠 Stage 4.4: The Brain (Python CAN Dashboard)

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Experiments-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#dashboard-ui">
    <img src="https://img.shields.io/badge/Dashboard_UI-0366d6?style=for-the-badge&logo=python&logoColor=white" />
  </a>
  <a href="#network-logic">
    <img src="https://img.shields.io/badge/Network_Logic-238636?style=for-the-badge&logo=gitbook&logoColor=white" />
  </a>
</p>

The **CAN Dashboard** is the central "Brain" of the robotic system. Developed in Python using `tkinter` and `python-can`, it provides a high-level graphical interface to monitor real-time telemetry from all distributed nodes and issue movement commands via an SLCAN-compatible USB adapter.

---

## 🎯 System Role
* **Central Monitoring**: Aggregates data from the Sonar Node (`0x111`), Lidar Node (`0x124`), and Base Unit (`0x123`) into a single unified view.
* **Momentary Command Control**: Implements a "press-to-move" system, sending ASCII-encoded movement strings to the Base Unit over the CAN bus.
* **Safety Visualization**: Provides instant visual feedback of the robot's IR collision sensors using dynamic UI icons.

---

<img width="620" height="777" alt="PC Dashboard Tool" src="https://github.com/user-attachments/assets/b80f9f85-d1b6-47f5-81ad-1efd9e76b434" />

---

## 🖥️ Dashboard Interface
<a name="dashboard-ui"></a>

The UI is divided into three logical sections reflecting the physical hardware distribution:

| Section | Node ID | Displayed Data |
| :--- | :--- | :--- |
| **Network Status** | N/A | Connection state of the SLCAN USB hardware. |
| **Base Unit (S3)** | `0x123` | IR sensor status (FL, FR, RL, RR) and heartbeat. |
| **Sonar Node (Nano)**| `0x111` | Real-time ultrasonic distance in centimeters. |
| **Lidar Node (C3)** | `0x124` | Raw distance, Adjusted distance, and Servo angle. |

---

## ⚙️ Network Logic & Control
<a name="network-logic"></a>

### 📡 Multi-Threaded Architecture
To ensure the GUI remains responsive while processing high-speed CAN traffic, the application uses a **Background Worker Thread** (`can_worker`).

* **RX (Receive)**: The worker polls the bus every 1ms. It uses bitwise operations to decode IR sensor masks and byte-shifting to reconstruct 16-bit Lidar values.
* **TX (Transmit)**: When a direction button is held, the Brain broadcasts the command (e.g., "Forward") to ID `0x456`.

### 🛠️ Bus Optimization: "Silence is Stop"
To maintain bus efficiency and satisfy the hardware safety watchdogs:
1.  **Event-Driven**: Commands are only sent when a button is actively pressed.
2.  **Heartbeat Interval**: To avoid bus flooding, active commands are throttled to a **1.0-second interval** while being held.
3.  **Automatic Halt**: When a button is released, the Brain stops transmitting. The Base Unit’s 2-second watchdog then automatically halts the motors.

---

## 💻 Prerequisites & Setup

### 📦 Dependencies
Install the required libraries via pip:
```bash
pip install python-can
🔌 Hardware Configuration
Ensure your SLCAN adapter (e.g., USB-to-CAN) is set to the correct COM port in the script:
```

```Python
# --- CONFIGURATION ---
CHANNEL = 'COM11'  # Update this to match your Device Manager
BITRATE = 500000   # Must match all nodes (500 KBPS)
```
---

## 📈 Expected Results

* **Responsive Feedback**: IR icons should turn 🔴 (Red) within 25ms of a physical sensor trigger.
* **Thread Stability**: The UI should not stutter or freeze, even when receiving high-frequency telemetry from the ESP32-C3.
* **Clean Exit**: Closing the window automatically shuts down the CAN interface, freeing the COM port for other applications.

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
