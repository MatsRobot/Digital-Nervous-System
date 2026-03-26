# 🧪 Experimental Lab: Digital Nervous System

## Stage 3: Bidirectional CAN-Bus Network

<p align="left">
  <a href="../">
    <img src="https://img.shields.io/badge/←_Back_to_Main-24292e?style=for-the-badge&logo=github&logoColor=white" />
  </a>
  <a href="#python-setup">
    <img src="https://img.shields.io/badge/Python_Venv_Setup-3776AB?style=for-the-badge&logo=python&logoColor=white" />
  </a>
</p>

This stage implements the **Network Layer**. The ESP32-S3 is no longer an isolated device; it is now a broadcasting node on a 500kbps CAN-Bus network.

---

## 🎯 Objectives
1.  **Bit-Packing:** Convert 16-bit sensor data into 8-bit CAN frame segments.
2.  **TWAI Integration:** Utilize the ESP32-S3 hardware CAN controller.
3.  **Cross-Platform Telemetry:** Monitor physical distance values on a laptop via Python.

---

## 💻 Python Monitor Setup (PC Side)
<a name="python-setup"></a>

Follow these exact steps to initialize the monitoring environment on your laptop:

1.  **Prepare Workspace:** Place `main.py` in your local folder.
2.  **Open Terminal:** In VS Code, go to `View > Terminal`.
3.  **Create Virtual Environment:**
    ```bash
    python -m venv .venv
    ```
4.  **Activate Environment:**
    ```bash
    .venv\Scripts\activate
    ```
    *Note: Ensure `(.venv)` appears in green in your terminal prompt.*
5.  **Install Requirements:**
    ```bash
    pip install python-can pyserial
    ```
6.  **Update Pip (Optional):**
    ```bash
    python -m pip install --upgrade pip
    ```
7.  **Run the Monitor:**
    ```bash
    python main.py
    ```

---

## 🔌 Hardware Wiring (CAN)
* **CAN Transceiver:** SN65HVD230 (connected to 3.3V).
* **CTX (Transmit):** GPIO 17.
* **CRX (Receive):** GPIO 18.
* **Termination:** Ensure a 120Ω resistor is present between CAN-H and CAN-L to prevent signal reflection.

---


<small>© 2026 MatsRobot | Experimental Logs for the Digital Nervous System Project</small>