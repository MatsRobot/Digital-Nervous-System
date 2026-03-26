# 🕸️ CAN Bus: The Physical Layer (Cabling Guide)

In a **Digital Nervous System**, the quality of the "nerves" determines the reliability of the robot. This guide covers how to select, wire, and terminate CAN Bus cables to ensure noise-free communication between the **N100**, **ESP32s**, and **Picos**.

---

## 🔌 1. The Role of the Cable

CAN Bus is a **differential signal** protocol. It relies on the voltage difference between $CAN\_H$ and $CAN\_L$ to ignore electrical noise. For this to work, the cable must provide:

- **Twisted Pairs:** To ensure any interference hits both wires equally (Common Mode Rejection).
- **Controlled Impedance:** A characteristic impedance of $\approx 120\Omega$ to prevent signal reflections.
- **Common Ground:** A reference point so all nodes "speak the same language."

---

## 🏗️ 2. Cable Selection

### 💎 The "Pro" Choice: Shielded 22 AWG

If purchasing new, look for **Shielded Twisted Pair (STP)**.

- **Gauge:** 22 AWG is ideal for carrying both data and power to sensors.
- **Shielding:** Connect the outer foil/braid to **Chassis Ground** at **one point only** to drain EMI without creating ground loops.

### 🛠️ The "Hacker" Choice: CAT5e / CAT6

Standard Ethernet cable is an excellent substitute due to its high-quality internal twists.

- **Type:** Use **Stranded** (Patch) cable for robotics. Avoid "Solid Core" as it will snap under the vibration of moving limbs.
- **The CAT5e Layout:** Use the following pinout to maximize stability:

| Pair       | Color                 | Function                 |
| :--------- | :-------------------- | :----------------------- |
| **Pair 1** | Blue / Blue-White     | **CAN High / CAN Low**   |
| **Pair 2** | Orange / Orange-White | **Node Power (12V)**     |
| **Pair 3** | Green / Green-White   | **Common Ground (0V)**   |
| **Pair 4** | Brown / Brown-White   | **Spare / Extra Ground** |

---

## ⚡ 3. Handling Motor Interference

For this "Nervous System," we separate **Heavy Lifting** from **Sensory Data**:

- **High Current (2A+):** Do **not** run motor power through thin Ethernet strands. Use a separate, thicker gauge power line directly from the battery to the motor node.
- **Inductive Kick:** Ensure motor nodes have flyback diodes and local capacitors (100uF+) to prevent spikes from leaking into the CAN data lines.

---

## 🛑 4. Termination & Topology

To prevent signal "ghosts" (reflections), the bus must be terminated correctly.

1.  **The $120\Omega$ Rule:** Place a **$120\Omega$ resistor** across $CAN\_H$ and $CAN\_L$ at the two physical ends of the bus.
2.  **Multimeter Test:** With power OFF, measure resistance between $CAN\_H$ and $CAN\_L$.
    - **$60\Omega$:** Perfect (two $120\Omega$ resistors in parallel).
    - **$120\Omega$:** Missing a terminator.
    - **$40\Omega$:** Too many terminators.
3.  **Stub Lengths:** Keep the "branches" from the main trunk to your nodes under **30cm** to maintain signal integrity.

---

<small>© 2026 MatsRobot | Licensed under the [MIT License](https://github.com/MatsRobot/matsrobot.github.io/blob/main/LICENSE)</small>
