# 🏎️ CAN-Bus: Controller Area Network (The Backbone)

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

**CAN-Bus** is the most robust communication protocol in the MatsRobot architecture. Originally designed for automotive environments, we use it as a high-reliability backbone that connects the **N100/ESP32 Gateway** to the various **Motor Nodes (Arduino Nano / Raspberry Pi Pico)** distributed across the robot's frame.

---

<table width="100%">
  <tr>
    <td width="60%" align="left" valign="middle">
      <h2>🔌 Protocol Logic: Multi-Master & Differential Signaling</h2>
    </td>
    <td width="40%" align="center" valign="middle">
      <img src="canbus-schematic.png" alt="CAN-Bus Wiring Diagram" width="300" style="border-radius: 8px;" />
    </td>
  </tr>
  <tr>
    <td colspan="2">
      <p>
        Unlike UART (point-to-point) or I2C (Master/Slave), CAN-Bus is <b>Multi-Master</b>. Any node can talk at any time. It uses <b>Differential Signaling</b> (CAN High and CAN Low) to cancel out electrical noise from high-current motors, making it nearly bulletproof for robotics.
      </p>
    </td>
  </tr>
</table>

---

## ⚖️ Strategic Analysis

| Feature | Engineering Implication |
| :--- | :--- |
| **Differential Pair** | The system measures the *difference* between two wires. This ignores electromagnetic noise from motors. |
| **Arbitration** | If two nodes talk at once, the message with the lowest **ID** (highest priority) automatically wins without losing data. |
| **No Master** | If the "Brain" (N100) fails, the individual leg or arm nodes can still communicate or enter a synchronized safety state. |
| **Termination** | Requires a 120Ω resistor at each physical end of the bus to prevent signal "reflections." |

---

## 📦 The CAN Frame Structure
CAN-Bus doesn't send data to "addresses"; it broadcasts "Messages" with "IDs." Every node decides for itself if a message ID is relevant.



1.  **Identifier (ID):** Defines the priority and the "Type" of data (e.g., `0x101` = Left Arm Move).
2.  **Data Length Code (DLC):** Tells the nodes how many bytes of data are following (up to 8 bytes).
3.  **Data Field:** The actual payload (motor positions, sensor states, etc.).
4.  **CRC (Cyclic Redundancy Check):** Advanced error checking that ensures the data wasn't corrupted by a motor spark.

---

## 🛠️ System Implementation
In the **Digital Nervous System**, CAN-Bus handles the "Muscle Coordination":

* **Actuator Nodes:** **Arduino Nanos** and **Picos** act as CAN endpoints, receiving high-level vectors and translating them into local PWM motor signals.
* **Remote Gateway:** An **ESP32-C3** acts as a bridge, taking Bluetooth/WiFi commands and injecting them onto the CAN bus.
* **Global Telemetry:** High-priority sensor data (like emergency stop triggers) is broadcast across the bus for all nodes to see instantly.

---

## 🧪 Experimental Design: Termination & Noise
<a name="experimental-design"></a>

Our implementation focuses on physical layer stability in a high-vibration environment:
1.  **120Ω Termination:** We verified the bus impedance using a multimeter. Without the 120Ω resistors at the start and end of the chain, the "echo" of the signals causes a complete bus crash.
2.  **Twisted Pair Wiring:** CAN_H and CAN_L are twisted together. This ensures that any external interference hits both wires equally, allowing the differential receiver to cancel the noise out.
3.  **Transceiver Isolation:** We use **MCP2515 (SPI to CAN)** modules. These provide a layer of electrical protection between the logic pins of our microcontrollers and the high-voltage potential of the long-distance bus.

---

## 💻 Source Code & Setup
<a name="source-code"></a>

### Physical Wiring
* **CAN High (H):** Connected in parallel to all nodes.
* **CAN Low (L):** Connected in parallel to all nodes.
* **Ground:** Essential to keep the transceivers at the same reference.

> [!IMPORTANT]
> **Priority Mapping:** Always assign the Emergency Stop (E-Stop) to the lowest ID (e.g., `0x001`) to ensure it wins arbitration and halts the robot even if the bus is busy with telemetry.

---

<small>© 2026 MatsRobot | Part of the [Digital Nervous System Project](../)</small>