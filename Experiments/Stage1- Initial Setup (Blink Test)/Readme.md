# 🧪 Experimental Lab: Digital Nervous System

## Stage 1: Initial Setup (Heartbeat LED Validation)

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#validation-strategy">
    <img src="https://img.shields.io/badge/Validation_Strategy-FFA500?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
  <a href="#circuit-diagram">
    <img src="https://img.shields.io/badge/Stage_1_Circuit-0366d6?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
  <a href="#workflow">
    <img src="https://img.shields.io/badge/Workflow-238636?style=for-the-badge&logo=visual-studio-code&logoColor=white" />
  </a>
</p>

This directory serves as the isolated initial validation phase for the MatsRobot complex multi-node architecture. It is designed to prove fundamental operation before introducing any sensors or communication protocols.

---

## 🎯 Stage 1 Objective: Proving Silicon Life
<a name="validation-strategy"></a>

The overall MatsRobot system (seen in the full schematic) features multiple concurrent peripherals: CAN-Bus, I2C0, and I2C1. This complexity introduces significant noise and debugging risks.

Stage 1 is the most critical phase: It bypasses all complex communication protocols. Its sole purpose is to establish a 'Known Good' point by answering two fundamental questions:
1.  **Can we compile code for the ESP32-S3?** (Verified by PlatformIO build)
2.  **Does the code physically execute on the Silicon?** (Verified by the 200ms blink of the Heartbeat LED on GPIO 13).

A successful blink test is required before adding any other wires.

---

## 🔌 Hardware Reference & Circuit Diagram
<a name="circuit-diagram"></a>

For this stage, **only the LED and its current-limiting resistor** are populated. You can ignore the CAN transceiver and I2C devices for now.

<p align="center">
  <img src="../path/to/full_circuit_diagram.png" alt="MatsRobot Full Circuit Diagram" width="800">
  <br>
  <em>Figure 1: Complete architecture. For Stage 1, only the components related to GP13 (Blue Wire, LED, and Power Rail) are tested and verified.</em>
</p>

### Connection Detail (GPIO 13):
This specific pin (often labeled '1' or 'GP13' on the DevKitC-1 header) is used to validate all stages. The firmware toggles the voltage on this pin.
* **Blue Wire:** Originates from GP13 -> Goes through a current-limiting resistor -> Plugs into the Anode (Long Leg) of the 'Heartbeat LED'.

---

## 🛠️ Stage-Specific Files
To maintain a "simple and effective" repository, we keep each stage autonomous. This folder contains:
* **`src/main.cpp`**: Source code focused entirely on the 200ms LED loop.
* **`platformio.ini`**: Environment configurations and required USB-CDC flags to connect to the N16P8 S3.

---

## 💻 Developer Workflow
<a name="workflow"></a>

### 🚀 Firmware Deployment (VS Code + PlatformIO)

1.  **Build:** Verify the code compiles without errors.
    ```bash
    pio run
    ```
2.  **Upload:** Flash the firmware to the ESP32-S3.
    ```bash
    pio run --target upload
    ```

**Observe:** If the S3 is functional, the LED attached to GPIO 13 will begin to blink at a 2.5Hz frequency (200ms ON, 200ms OFF). If it does not, all subsequent complex stages cannot be attempted.

---


<small>© 2026 MatsRobot | Experimental Logs for the Digital Nervous System Project</small>