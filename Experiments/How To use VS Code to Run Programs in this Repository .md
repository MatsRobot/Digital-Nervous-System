# 🛠️ Setup Guide to compile and run programms: VS Code Portable for Microcontrollers

This guide provides a comprehensive walkthrough for setting up a **Portable VS Code** environment. This setup is designed to be a "one-stop-shop" for all projects in this repository, supporting both **ESP32 (PlatformIO)** and **Raspberry Pi Pico (MicroPico)** development.

---

## 📥 1. Initial Portable Installation
Using a portable version ensures that all your tools, extensions, and settings remain on your local drive (or even a USB stick) without affecting your computer's system settings.

1.  **Download:** Visit the [VS Code Download Page](https://code.visualstudio.com/download) and download the **Windows 64-bit .zip** (under the "ZIP" or ".zip" category).
2.  **Extract:** Create a folder on your local drive (e.g., `C:\MyRobotProject_IDE`) and extract the contents of the `.zip` file there.
3.  **Create Data Folder:** * Open the extracted folder where `Code.exe` is located.
    * Create a new empty folder named **`data`**. 
    * *Why?* This tells VS Code to store all your extensions and settings right here instead of in your Windows User profile.
4.  **Launch:** Double-click `Code.exe` to start.

---

## 🧩 2. Extension Suite Setup
Open the **Extensions** view by clicking the icon (four squares) on the left sidebar. Search for and install the following:

### Core Development Tools
* **PlatformIO IDE:** The primary tool for ESP32 and Arduino-based projects.
* **MicroPico:** Used for Raspberry Pi Pico and MicroPython development.

### Productivity & Styling Tools
* **Prettier - Code formatter:** Automatically cleans up your code formatting (indentation, spacing).
* **Code Spell Checker:** Catches typos in your code comments and variable names.
* **Material Icon Theme:** Changes folder/file icons to make the project structure easier to navigate.
* **Code Runner:** Allows you to quickly run snippets of code (like Python or C++ tests) with a single click.

---

## 📂 3. Downloading Your Files
1.  **Repository Access:** Go to the [Project GitHub Repository](https://github.com/MatsRobot/Face-Tracking-using-ESP32-CAM).
2.  **Download:** Click the green **Code** button and select **Download ZIP**.
3.  **Extract:** Unpack the ZIP file to a folder on your drive (e.g., `C:\Projects\MyRobotFiles`).

---

## 🚀 4. Creating a New Project (Step-by-Step)
Follow these steps whenever you start a new program from the repository.

1.  **Open PlatformIO:** Click the **Ant Icon** on the left sidebar.
2.  **Start Home:** Click **PIO Home > Open**.
3.  **New Project:** Click the **+ New Project** button.
4.  **Board Selection:** * For ESP32-CAM: Type and select **AI Thinker ESP32-CAM**.
    * For other boards: Type your specific board name (e.g., ESP32 Dev Module).
5.  **Framework:** Set this to **Arduino**.
6.  **Location:** Uncheck "Use default location" and select a folder where you want this specific program to live.
7.  **Finish:** Click **Finish**. Wait for PlatformIO to download the necessary framework files.

---

## 📝 5. Manual File Migration
To ensure the program compiles correctly, you must replace the "blank" files created by VS Code with the ones you downloaded from GitHub.

1.  **The Config (`platformio.ini`):** * Find the `platformio.ini` file in the GitHub folder you downloaded.
    * Copy and paste it into your **New Project Folder**, overwriting the existing one.
2.  **The Code (`main.cpp`):**
    * Go to the `src` folder in the GitHub download.
    * Copy `main.cpp` and paste it into the `src` folder of your **New Project**, replacing the empty file.
3.  **The Libraries (`lib` folder):**
    * Go to the `lib` folder in the GitHub download.
    * Copy all sub-folders (e.g., `esp32cam`, `ADS1115_Driver`).
    * Paste these into the **`lib`** folder of your **New Project**.

---

## 🔌 6. Compile and Upload
1.  **Connect:** Plug your microcontroller into your computer.
    * *Note for ESP32-CAM:* Ensure **GPIO 0** is connected to **GND** before plugging it in to enter "Flash Mode".
2.  **Build (Compile):** Click the **Checkmark (✔)** icon in the blue bar at the very bottom of the window. This translates your code into machine language.
3.  **Upload:** Click the **Right Arrow (→)** icon in the blue bar. 
4.  **Launch:**
    * If using ESP32-CAM, disconnect GPIO 0 from GND and press the **Reset** button on the board.
5.  **Monitor:** Click the **Plug Icon** (Serial Monitor) at the bottom to see live data and debugging info from your robot.

---

## 🔍 7. General Troubleshooting Guide

| Symptom | Potential Cause | Solution |
| :--- | :--- | :--- |
| **Red squiggly lines under `#include`** | IntelliSense hasn't indexed the libraries yet. | Click the **Ant Icon** > **Project Tasks** > **Miscellaneous** > **Rebuild IntelliSense Index**. Also, ensure libraries are in the `lib` folder. |
| **"Could not open port / COM X"** | Port is busy or device is disconnected. | Close any other Serial Monitors (like Arduino IDE). Unplug and replug the USB cable. |
| **"Timed out waiting for packet header"** | Board not in Bootloader/Flash mode. | Hold the 'Boot' button while clicking Upload, or ensure **GPIO 0** is grounded for ESP32-CAM. |
| **Success message, but nothing happens** | Code is uploaded but board is still in Flash mode. | Disconnect **GPIO 0** from **GND** and press the physical **Reset** button on the board. |
| **Brownout Detector / Constant Reboots** | Insufficient power to the board. | Connect an external 5V power source. USB ports often can't provide enough current for ESP32-CAM with Servos. |
| **`main.cpp: No such file or directory`** | File was pasted in the wrong location. | Ensure your code is inside the **`src`** folder, not the root directory of the project. |
| **Extension won't load / Python error** | Portable path issues. | Restart VS Code. If it persists, delete the `.platformio` folder inside your `data` directory and let it reinstall. |

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
