# Digital-Nervous-System
## 🤖 Robotics Communications: The Digital Nervous System

<p align="left">
  <a href="./UART/">
    <img src="https://img.shields.io/badge/UART-Universal_Asynchronous-0366d6?style=for-the-badge&logo=cpu&logoColor=white" />
  </a>
  <a href="./I2C/">
    <img src="https://img.shields.io/badge/I2C-Inter--Integrated_Circuit-24292e?style=for-the-badge&logo=target&logoColor=white" />
  </a>
  <a href="./SPI/">
    <img src="https://img.shields.io/badge/SPI-Serial_Peripheral-0366d6?style=for-the-badge&logo=microchip&logoColor=white" />
  </a>
  <a href="./CAN-Bus/">
    <img src="https://img.shields.io/badge/CAN_Bus-Controller_Area_Network-24292e?style=for-the-badge&logo=connected-cars&logoColor=white" />
  </a>
</p>

**Robotics Communications** documents the protocols used to synchronize high-level intelligence with low-level hardware.
From **N100** computer-vision processing to **ESP32** motor control, this repo tracks how data flows through the machine.


---

<table width="100%">
  <tr>
    <td width="50%" align="left" valign="middle">
      <h2>🔌 The Mission: Seamless Connectivity</h2>
    </td>
    <td width="50%" align="center" valign="middle">
      <img src="https://github.com/user-attachments/assets/413bc2db-61d1-47e4-ab53-cd875a7cfaf1" alt="Circuitry Visualization" width="350" style="border-radius: 8px;" />
    </td>
  </tr>
  <tr>
    <td colspan="2">
      <p>
        In a complex robot, no single processor does everything. The <b>N100</b> handles heavy lifting like image recognition, while <b>ESP32s</b> and <b>Picos</b> manage real-time movement and sensor data. 
      </p>
      <p>
        Choosing the right protocol is critical: <b>UART</b> bridges the gap between the N100 and the robot, <b>I2C</b> gathers local sensor data, <b>SPI</b> handles high-speed display or storage, and <b>CAN Bus</b> provides a robust industrial backbone for daisy-chaining limbs and "eyes" across the chassis.
      </p>
    </td>
  </tr>
</table>

---

## 🏗️ Protocol Architecture

The project explores four primary communication modules:

### 1. 📟 [UART (Universal Asynchronous)](./UART/)

- **Primary Use:** The bridge between the **N100 (Brain)** and the **ESP32 (Main Controller)**.
- **Application:** Sending high-level movement instructions and receiving telemetry.
- **Hardware:** Utilized for Bluetooth-to-Serial modules and debugging logs on the **Arduino Nano**.

### 2. 🖇️ [I2C (Inter-Integrated Circuit)](./I2C/)

- **Primary Use:** Short-distance "Board-to-Sensor" communication.
- **Application:** Connecting the **ESP32-S3** to IMUs (Gyroscopes), OLED displays, and compasses.
- **Advantage:** Uses only two wires (SDA/SCL) to connect multiple devices on a single bus.

### 3. ⚡ [SPI (Serial Peripheral Interface)](./SPI/)

- **Primary Use:** High-speed data transfer for media and memory.
- **Application:** Interfacing the **Raspberry Pi Pico** with SD cards or fast-refresh LCD screens.
- **Hardware:** Essential for transferring image data before it is processed by the N100.

### 4. 🕸️ [CAN Bus (Controller Area Network)](./CAN-Bus/)

- **Primary Use:** The "Industrial Backbone" for modular robotics.
- **Application:** Daisy-chaining the **Robot Arms** and **Eyes** to the central system.
- **Advantage:** Exceptionally robust against electrical noise; allows multiple nodes to communicate without a central master.

---

## 🛠️ The Hardware Stack

| Category      | Component           | Communication Role                 |
| :------------ | :------------------ | :--------------------------------- |
| **Brain**     | Intel N100 Desktop  | Master UART / USB Gateway          |
| **Vision/BT** | ESP32-S3 / C3       | SPI (Cameras) & UART (Bluetooth)   |
| **Nodes**     | Arduino Nano / Pico | CAN Bus endpoints for Arms/Sensors |
| **Sensors**   | MPU6050 / Displays  | I2C Peripheral                     |

---

## 📐 Implementation Pipeline

1. **Topology Design:** Mapping the physical distance between the N100 and peripheral nodes.
2. **Circuit Drafting:** Implementing level-shifting (3.3V to 5V) for cross-platform stability.
3. **Firmware Dev:** Writing non-blocking drivers for the ESP32 and Pico.
4. **Integration:** Using the N100 to orchestrate the entire network via structured command packets.
5. **Testing:** Utilizing Logic Analyzers to verify signal integrity across the CAN Bus.

---

## 📺 Featured Hardware: Modular Robotics Rig

**Key Highlights:**

- **N100 Integration:** Running Python-based AI to send hex-commands via UART.
- **CAN-Node System:** Independent Nano/Pico modules controlling servos in the arms.
- **Wireless Bridge:** ESP32-C3 acting as a Bluetooth-to-CAN gateway.

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
