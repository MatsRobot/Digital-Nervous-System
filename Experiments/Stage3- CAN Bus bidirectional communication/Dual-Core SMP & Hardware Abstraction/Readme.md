# 🧪 Experimental Lab: Digital Nervous System

## Stage 3: Dual-Core SMP & Hardware Abstraction

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#dual-core-logic">
    <img src="https://img.shields.io/badge/Architecture_Specs-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
  </a>
</p>

This stage implements the **Asynchronous Execution Layer**. The ESP32-S3 is no longer bound by a single-loop bottleneck; it now utilizes **Symmetric Multiprocessing (SMP)** to split high-latency communication from time-critical motor safety.



---

## 🎯 Objectives
1.  **Core Segregation:** Assign Bluetooth/CAN communication to Core 0 and Motor/Safety logic to Core 1.
2.  **Hardware Abstraction:** Create a dedicated `applyMotors` function to act as a physical safety gate.
3.  **Human-in-the-Loop:** Implement a high-priority manual override for Bluetooth commands over autonomous CAN logic.
4.  **Constant Telemetry:** Ensure IR sensor data is broadcast to the "Brain" (Dashboard) at 10Hz regardless of motor state.

---

## 🏗️ Architecture: The Dual-Core Split
<a name="dual-core-logic"></a>

The ESP32-S3 contains two Xtensa® 32-bit LX7 microprocessors. We leverage **FreeRTOS** to run tasks without the need for a traditional `loop()`.

### Core 0: The Radio & Network Task (`TaskRadio`)
Core 0 handles the "Nervous System" and external inputs:
* **HC-06 Bluetooth:** Captures manual override commands from the human operator.
* [cite_start]**TWAI/CAN-Bus:** Receives high-level "Brain" instructions (ID 0x456) and transmits telemetry (ID 0x123)[cite: 2].
* **Arbitration:** If Bluetooth activity is detected, this core "latches" control, effectively muting CAN-Bus commands until a manual "Stop" is received.

### Core 1: The Execution Engine (`TaskExecution`)
Core 1 handles the "Reflexes" and hardware interaction:
* **Motor Control:** Direct PWM signaling to the L298N driver pins (GPIO 4, 5, 6, 7, 15, 16).
* **OLED Feedback:** Real-time visualization of the active command and sensor states via the SSD1306 display.
* **Safety Watchdog:** Checks the age of the last command to prevent "runaway" behavior if a connection is lost.

---

## 💻 Key Software Components

### 1. `applyMotors(String cmd)`
This function acts as the **Physical Safety Gate**. Instead of motors blindly following commands, this function reads the **4 IR Sensors** (Front Left/Right, Rear Left/Right) before every pulse.
* **Constraint:** If `cmd == "Forward"` but a front IR sensor is blocked (LOW), it forces `cmd = "Stop"`.

### 2. The Task Runners
We bypass the standard `void loop()` entirely by using the FreeRTOS scheduler in `setup()`:

```cpp
// Pinned to Core 0 (Radio/CAN)
xTaskCreatePinnedToCore(TaskRadio, "Radio", 4096, NULL, 1, NULL, 0);

// Pinned to Core 1 (Motors/Safety)
xTaskCreatePinnedToCore(TaskExecution, "Exec", 4096, NULL, 1, NULL, 1);
```

## 3. Non-Blocking Design

By using vTaskDelay(1) instead of delay(), the processors never truly "sleep"—they yield just enough time for the background WiFi/Bluetooth stacks to stay stable while maintaining sub-millisecond responsiveness to incoming CAN frames.

### 3. Launch the Monitor
Type the following command and press **Enter**:
```powershell
python main.py
```

![BaseUnit_ESP32-S3-N16R8-Ver2_bb](https://github.com/user-attachments/assets/ac244bf0-2433-49af-ad09-43d43df1d76e)

---

## 🔌 Hardware Wiring (Safety Nodes)
* **IR Sensors:**  Connected to GPIO 1, 2, 41, and 42.
* **Bluetoot:**  HC-06 module connected to GPIO 43 (RX) and 44 (TX).
* **CAN Transceiver:**  SN65HVD230 connected to GPIO 17 (TX) and 18 (RX).

---
<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>

