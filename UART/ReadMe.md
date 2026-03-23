# 📟 UART: Universal Asynchronous Receiver-Transmitter

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_System-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#experimental-design">
    <img src="https://img.shields.io/badge/Experimental_Design-0366d6?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
  <a href="#source-code">
    <img src="https://img.shields.io/badge/Source_Code-238636?style=for-the-badge&logo=visual-studio-code&logoColor=white" />
  </a>
</p>

**UART** serves as the primary point-to-point communication bridge in this project, linking the high-level intelligence of the **Intel N100** to the real-time execution of the **ESP32**.

---

<table width="100%">
  <tr>
    <td width="60%" align="left" valign="middle">
      <h2>🔌 Protocol Overview</h2>
    </td>
    <td width="40%" align="center" valign="middle">
      <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/6/67/UART_schematic.svg/640px-UART_schematic.svg.png" alt="UART Connection Schematic" width="300" style="border-radius: 8px; background: white;" />
    </td>
  </tr>
  <tr>
    <td colspan="2">
      <p>
        Unlike I2C or SPI, UART is not a "bus" but a physical circuit. It uses two dedicated wires—<b>TX (Transmit)</b> and <b>RX (Receive)</b>—to send data serially. Because it is asynchronous, no clock signal is shared; instead, both devices must agree on a specific <b>Baud Rate</b> (e.g., 115200) beforehand.
      </p>
    </td>
  </tr>
</table>

---

## ⚖️ Strategic Analysis

| Strength | Weakness |
| :--- | :--- |
| **Simplicity:** Only two wires required (plus a common Ground). | **Point-to-Point:** Restricted to only two devices; cannot be "daisy-chained" easily. |
| **No Clock Needed:** Reduces wiring complexity over longer distances. | **Speed Limits:** Generally slower than SPI or I2C for large data packets. |
| **Ubiquity:** Every controller in this stack (N100, ESP32, Pico, Nano) supports it natively. | **Timing Sensitive:** Mismatched baud rates lead to "garbage" data. |

---

## 🛠️ Usage in this Project

In the **Digital Nervous System**, UART is the "Command Channel":
* **The Brain Link:** The **N100** (running Python/AI logic) sends structured Hex commands to the **ESP32** via a USB-to-Serial interface.
* **Wireless Bridge:** The **ESP32-C3** uses UART to communicate with external Bluetooth modules for manual remote control.
* **Debugging:** The **Arduino Nano** and **Pico** use UART (Serial Monitor) to output real-time sensor logs for developer analysis.

---

## 📐 Experimental Design
<a name="experimental-design"></a>

Our setup utilizes a **Logic Level Shifter** between the N100 (5V USB logic) and the ESP32 (3.3V logic) to ensure signal integrity. 
1. **N100 (Master):** Packages AI-processed target coordinates into a 10-byte packet.
2. **ESP32 (Slave):** Interrupt-driven RX buffer captures the packet and translates it into PWM signals for the motors.

---

## 💻 Source Code & Implementation
<a name="source-code"></a>

### Hardware Setup
* **N100 TX** → **Level Shifter** → **ESP32 RX (GPIO 16)**
* **N100 RX** ← **Level Shifter** ← **ESP32 TX (GPIO 17)**
* **Common GND** (Crucial for signal reference)

> [!TIP]
> Always use a non-blocking `Serial.available()` check in your ESP32 code to prevent the "brain" from freezing the motor control loop while waiting for data.

---

<small>© 2026 MatsRobot | Part of the [Digital Nervous System Project](../)</small>