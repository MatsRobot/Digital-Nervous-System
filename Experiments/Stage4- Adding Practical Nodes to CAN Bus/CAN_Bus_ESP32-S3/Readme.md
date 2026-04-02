# 🏎️ Stage 4.3: Base Unit / Motor Controller (0x123)

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

The **Base Unit** serves as the central "Muscular System" of the robot. Running on an **ESP32-S3**, it manages real-time motor actuation, collision avoidance via IR sensors, and a dual-priority control scheme that balances autonomous CAN commands with manual Bluetooth overrides.

---

## 🎯 Experimental Objectives
* **Priority Management**: Implement a "Human-in-Control" flag where local Bluetooth (HC-06) inputs override remote CAN bus commands for safety.
* **Hardware-Level Safety**: Integrate "Hard-Stop" logic that prevents movement if the 4-way IR sensor array detects an immediate obstacle, regardless of the incoming command.
* **Multicore Execution**: Utilize **FreeRTOS** to separate high-speed communication (Radio) from hardware execution and UI updates (OLED).

---

<a name="circuit-design"></a>![BaseUnit_ESP32-S3-N16R8-Ver2](https://github.com/user-attachments/assets/e64548a5-145b-4433-9d8b-b0b4f72e0ec2)

---

## 🔌 Circuit Design & Integration

| Component | Role | Connection |
| :--- | :--- | :--- |
| **ESP32-S3** | Main Controller | Central Logic (Dual Core) |
| **L298N / H-Bridge** | Motor Driver | Pins 4, 5, 6 (A) & 7, 15, 16 (B) |
| **HC-06** | Bluetooth Module | UART (RX: 43, TX: 44) |
| **SSD1306** | OLED Display | I2C (SDA: 38, SCL: 39) |
| **IR Sensors** | Collision Detection | Pins 1, 2, 41, 42 |
| **SN65HVD230** | CAN Transceiver | TX: 17, RX: 18 |

---

## 💻 Firmware & Source Code
<a name="firmware"></a>

### 📝 Node Logic (`src/main.cpp`)
The firmware is split into two independent tasks to ensure telemetry never lags during motor stalls:

**1. TaskRadio (Core 0)**: Handles Bluetooth parsing, CAN frame reading (ID `0x456`), and broadcasting telemetry (ID `0x123`) every 25ms.
**2. TaskExecution (Core 1)**: Refreshes PWM signals to the motors at high frequency and updates the OLED diagnostic screen.

**Key Implementation: Safety Watchdog**
To prevent "runaway" robots, the system automatically stops and reverts to "Brain" mode if no command is received for 2 seconds.

```cpp
// SAFETY WATCHDOG
if (now - lastMsgTime > 2000) {
    activeMove = "Stop";      // Halt Motors
    humanInControl = false;   // Revert to CAN mode
    canStatus = "Idle";       // Reset UI
}
```

## ⚙️ Environment (platformio.ini)
```cpp
[env:base_unit]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
lib_deps = 
    adafruit/Adafruit SSD1306 @ ^2.5.7
    yveon/ESP32-TWAI-CAN @ ^1.0.1
```

---

## 📊 Message Protocol
| ID (Hex) | Direction | Payload Description |
| :--- | :--- | :--- |
| **0x456** | **RX** | **Command**: Raw string (e.g., "Forward", "Left") from the Brain. |
| **0x123** | **TX** | **Telemetry**: Byte 2 = Heartbeat; Byte 3 = IR Bitmask. |


---

**IR Bitmask Detail (Byte 3)**:

* **0x01**: Front-Left Blocked 
* **0x02**: Front-Right Blocked 
* **0x04**: Rear-Left Blocked 
* **0x08**: Rear-Right Blocked 

---


## 📈 Expected Results

* **Deterministic Stop**: The robot must physically refuse to move "Forward" if the front IR sensors are triggered, even if the Dashboard is sending a "Forward" command.
* **Priority Swap**: If a Bluetooth controller connects and sends a command, the OLED should instantly switch from MODE: BRAIN to MODE: HUMAN.
* **High-Speed U**I: Telemetry updates on the Python Dashboard should appear nearly instantaneous (25ms latency).

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
