# Multi-Node CAN Bus Sensor & Control System

This project demonstrates a real-time, distributed control network using the **CAN (Controller Area Network)** protocol. It features a central Python-based dashboard ("The Brain") and two specialized hardware nodes (ESP32-S3 and Arduino Nano) that communicate to share sensor data and receive remote commands.

---

## 🛰️ System Architecture

The network operates at a bitrate of **500kbps**. It consists of three primary components:

1.  **The Dashboard (Python):** Acts as the master controller, monitoring sensor data and broadcasting motor/display commands.
2.  **Node 0x123 (ESP32-S3):** A high-speed node using a VL53L0X Laser sensor to measure distance in millimeters.
3.  **Node 0x111 (Arduino Nano):** A utility node using a MaxSonar Ultrasonic sensor and an I2C LCD for local status updates.

---

## 📡 Message Protocol (ID Strategy)

The system organizes traffic using standard 11-bit CAN identifiers to ensure data priority and organization:

| CAN ID | Direction | Data Description | Update Rate |
| :--- | :--- | :--- | :--- |
| **`0x123`** | ESP32 → PC | Distance (mm) [2 bytes] + Sequence [1 byte] | 20Hz (50ms) |
| **`0x111`** | Nano → PC | Distance (cm) [1 byte] + Sequence [1 byte] | 10Hz (100ms) |
| **`0x456`** | PC → ESP32 | Motor Command Strings (e.g., "Forward") | 10Hz (100ms) |
| **`0x500`** | PC → Nano | LCD Toggle Command (1 = On, 0 = Off) | Event-based |

---

## 💻 Code Explanations

### 1. Python Dashboard (`main - Dashboard.py`)
* **Threading:** Uses a background `can_worker` thread to handle traffic without freezing the GUI.
* **SLCAN Interface:** Connects to hardware via a serial-over-CAN (SLCAN) protocol on a specified COM port.
* **Safety Heartbeat:** Constantly broadcasts motor commands at 10Hz; if the connection is lost, the robot defaults to "Stop" logic.
* **Momentary Logic:** Movement buttons are bound to `<ButtonPress-1>` and `<ButtonRelease-1>` to ensure commands only run while the button is held.

### 2. ESP32-S3 Sensor Node (`main - ESP32S3 Sensor.cpp`)
* **Dual I2C Configuration:** Separates the VL53L0X sensor (Bus 0) and the SSD1306 OLED (Bus 1) for better stability.
* **Bit Shifting:** Splitting the 16-bit distance value into two 8-bit bytes (`High Byte` and `Low Byte`) for CAN transmission.
* **Command Parsing:** Receives raw bytes from ID `0x456`, converts them to a string, and updates the `motorStatus` for visual feedback.

### 3. Arduino Nano Sensor Node (`main - Nano Sensor.cpp`)
* **Software Serial:** Reads the "R" header and numeric distance string from the MaxSonar sensor.
* **Debounced Display:** Tracks `lastLcdState` to only update the I2C LCD when the value changes, preventing screen flickering.
* **Heartbeat LED:** Toggles a physical LED every time a CAN packet is successfully transmitted.

---

## 💡 Key Technical Concepts

### 🔄 The Heartbeat (Sequence Counters)
Every node increments a sequence byte (`nano_seq` or `tx_seq`) with every message. This allows the Dashboard to confirm the node is actively processing data and has not crashed.

### 🔢 Bitwise Reconstruction
To send a 16-bit distance value (up to 65,535) over an 8-bit bus:
* **Encoding (ESP32):** `(currentDistance >> 8)` for the high byte, and `currentDistance & 0xFF` for the low byte.
* **Decoding (Python):** `(msg.data[0] << 8) | msg.data[1]`.

### 🛑 Non-Blocking Loops
All nodes use `millis()` or specific timeouts rather than `delay()`. This ensures the system can process incoming CAN commands immediately even while reading sensors or updating displays.

---

## 🛠️ Hardware Setup
1.  Connect the CAN High (H) and CAN Low (L) lines across all nodes.
2.  Install **120Ω termination resistors** at the two physical ends of the bus.
3.  Ensure all devices share a common Ground (GND).
  
---
<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>

