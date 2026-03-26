# 🛠️ Stage 4: Adding Practical Nodes to CAN Bus

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