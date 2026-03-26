# 🧪 Experimental Lab: Digital Nervous System

## Stage 2:- Sensor Working on I2C 

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#setup-guide">
    <img src="https://img.shields.io/badge/Setup_Guide-0366d6?style=for-the-badge&logo=blueprint&logoColor=white" />
  </a>
  <a href="#workflow">
    <img src="https://img.shields.io/badge/Workflow-238636?style=for-the-badge&logo=visual-studio-code&logoColor=white" />
  </a>
</p>

This directory serves as the **Living Lab** for the MatsRobot project. It documents the step-by-step evolution of the communication backbone, from basic heartbeats to complex multi-node CAN configurations.

---

## 📅 Experimental Roadmap & Status
Each stage builds upon the last. Use these folders to find specific circuit diagrams, firmware, and test results.

| Stage | Focus | Status | Hardware Reference |
| :--- | :--- | :--- | :--- |
| **[Stage 1](./Stage1-Initial-Setup)** | Blink Test & Basic GPIO | ✅ Verified | ESP32 / Arduino Nano |
| **[Stage 2](./Stage2-Sensor-working-on-I2C-bus)** | I2C Sensor Integration | ✅ Verified | MPU6050 / BME280 |
| **[Stage 3](./Stage3-CAN-Bus-bidirectional-communication)** | Bidirectional CAN-Bus | ✅ Verified | MCP2515 Transceivers |
| **[Stage 4](./Stage4-Adding-Practical-Nodes-to-CAN-Bus)** | Practical Node Deployment | 🛠️ In Progress | Custom Actuator Nodes |

---

## 🛠️ Setup & Strategy
<a name="setup-guide"></a>

To maintain a "simple and effective" repository, we keep each stage autonomous. Each folder contains only the essential files required to reproduce the experiment:
* **`circuit-diagrams/`**: Visual wiring guides (PNG/PDF).
* **`src/main.cpp`**: The specific firmware logic for that stage.
* **`platformio.ini`**: Environment configurations and library dependencies.
* **`main.py`**: Laptop-side scripts for bus monitoring or data logging.

---

## 💻 Developer Workflow
<a name="workflow"></a>

### 🔌 Firmware (VS Code + PlatformIO)
When adding or testing a node, use the PlatformIO Core CLI or the VS Code interface:

1.  **Initialize/Build:** `pio run`
2.  **Upload to Hardware:** `pio run --target upload`
3.  **Live Debugging:** `pio device monitor`

> [!TIP]
> **Port Conflicts:** Ensure the Serial Monitor is closed before attempting a new upload, especially when working with high-speed CAN data.

### 🐍 Python Host Setup (Laptop)
To interact with the nodes from your computer, use a **Virtual Environment** to manage dependencies like `python-can` or `pyserial`.

```bash
# 1. Create and Activate Environment
python -m venv venv
source venv/bin/activate  # Windows: .\venv\Scripts\activate

# 2. Install Stage-Specific Requirements
pip install python-can pyserial

# 3. Run the Experiment Script
python main.py