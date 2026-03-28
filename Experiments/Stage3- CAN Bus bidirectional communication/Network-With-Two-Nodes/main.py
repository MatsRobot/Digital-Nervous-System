import can
import time
import threading
import tkinter as tk
from tkinter import ttk

# --- CONFIGURATION ---
CHANNEL = 'COM11'  # Change this to match your USB-CAN adapter port
BITRATE = 500000

class CanDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("CAN Multi-Node System Control")
        self.root.geometry("400x750")
        
        # --- Variables ---
        # ESP32 Node (ID 0x123)
        self.esp_dist = tk.StringVar(value="--- mm")
        self.esp_heartbeat = tk.StringVar(value="0")
        
        # Nano Node (ID 0x111)
        self.nano_dist = tk.StringVar(value="--- cm")
        self.nano_heartbeat = tk.StringVar(value="0")
        
        # System State
        self.last_status = tk.StringVar(value="Disconnected")
        self.port_status = tk.StringVar(value=f"Target: {CHANNEL}")
        self.pending_cmd_bytes = list("Stop".encode('ascii')) 
        self.bus = None 
        
        self.setup_ui()
        
        # Start background thread for CAN communication
        self.running = True
        self.can_thread = threading.Thread(target=self.can_worker, daemon=True)
        self.can_thread.start()

    def setup_ui(self):
        # 1. Connection Status
        status_frame = ttk.LabelFrame(self.root, text=" Network Status ")
        status_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(status_frame, textvariable=self.port_status, font=('Arial', 9, 'bold')).pack()
        self.status_label = ttk.Label(status_frame, textvariable=self.last_status, foreground="red")
        self.status_label.pack()

        # 2. ESP32 Node Display (ID 0x123)
        esp_frame = ttk.LabelFrame(self.root, text=" ESP32 Node (0x123) ")
        esp_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(esp_frame, text="VL53L0X Distance:", font=('Arial', 9)).pack()
        ttk.Label(esp_frame, textvariable=self.esp_dist, font=('Arial', 25, 'bold'), foreground="blue").pack()
        ttk.Label(esp_frame, textvariable=self.esp_heartbeat, font=('Arial', 9)).pack()

        # 3. Nano Node Display (ID 0x111)
        nano_frame = ttk.LabelFrame(self.root, text=" Nano Node (0x111) ")
        nano_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(nano_frame, text="MaxSonar Distance:", font=('Arial', 9)).pack()
        ttk.Label(nano_frame, textvariable=self.nano_dist, font=('Arial', 25, 'bold'), foreground="purple").pack()
        ttk.Label(nano_frame, textvariable=self.nano_heartbeat, font=('Arial', 9)).pack()

        # 4. Robot Control Pad (TX to ID 0x456)
        btn_frame = ttk.LabelFrame(self.root, text=" Control Interface ")
        btn_frame.pack(pady=10, padx=10, fill="x")
        
        pad_frame = ttk.Frame(btn_frame)
        pad_frame.pack(pady=5)

        btns = [
            ("FORWARD", "Forward", 0, 1),
            ("LEFT", "Left", 1, 0),
            ("STOP", "Stop", 1, 1),
            ("RIGHT", "Right", 1, 2),
            ("BACKWARD", "Backward", 2, 1)
        ]

        for text, cmd, r, c in btns:
            b = ttk.Button(pad_frame, text=text)
            b.grid(row=r, column=c, padx=3, pady=3)
            # Movement Momentary Logic
            b.bind('<ButtonPress-1>', lambda e, x=cmd: self.set_cmd(x))
            if cmd != "Stop":
                b.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))

        # 5. Remote Nano LCD Command (TX to ID 0x500)
        # Momentary: Press sends 1 (Show), Release sends 0 (Clear)
        trigger_btn = ttk.Button(btn_frame, text="LCD DISPLAY")
        trigger_btn.pack(pady=15, padx=20, fill="x")
        trigger_btn.bind('<ButtonPress-1>', lambda e: self.send_lcd_cmd(1))
        trigger_btn.bind('<ButtonRelease-1>', lambda e: self.send_lcd_cmd(0))

    def set_cmd(self, word):
        """Sets the motor command string for the ESP32 (0x456)"""
        self.pending_cmd_bytes = list(word.encode('ascii'))
        if self.bus:
            self.last_status.set(f"TX: {word}")

    def send_lcd_cmd(self, state):
        """Sends momentary 1 or 0 to Nano LCD (0x500)"""
        if self.bus:
            try:
                msg = can.Message(
                    arbitration_id=0x500,
                    data=[state], 
                    is_extended_id=False
                )
                self.bus.send(msg)
                status_text = "LCD Message ON" if state == 1 else "Connected"
                self.last_status.set(status_text)
            except Exception as e:
                print(f"LCD Command failed: {e}")

    def can_worker(self):
        """Threaded worker for full-duplex CAN traffic"""
        while self.running:
            if self.bus is None:
                try:
                    self.bus = can.interface.Bus(interface='slcan', channel=CHANNEL, bitrate=BITRATE)
                    self.last_status.set("Connected")
                    self.status_label.configure(foreground="green")
                except Exception:
                    self.last_status.set("Hardware Missing...")
                    time.sleep(2)
                    continue

            try:
                # 1. RECEIVE PHASE (Process all incoming messages)
                while True:
                    msg = self.bus.recv(0.001)
                    if msg is None: break
                    
                    # Handle ESP32 (0x123) - 3 bytes: DistH, DistL, Seq
                    if msg.arbitration_id == 0x123:
                        dist_mm = (msg.data[0] << 8) | msg.data[1]
                        self.esp_dist.set(f"{dist_mm} mm")
                        self.esp_heartbeat.set(f"ESP Seq: {msg.data[2]}")
                    
                    # Handle Nano (0x111) - 2 bytes: Dist, Seq
                    elif msg.arbitration_id == 0x111:
                        dist_cm = msg.data[0]
                        seq_num = msg.data[1]
                        self.nano_dist.set(f"{dist_cm} cm")
                        self.nano_heartbeat.set(f"Nano Seq: {seq_num}")

                # 2. SEND PHASE (Fixed 10Hz Heartbeat for motor commands)
                tx_msg = can.Message(
                    arbitration_id=0x456,
                    data=self.pending_cmd_bytes,
                    is_extended_id=False
                )
                self.bus.send(tx_msg)
                time.sleep(0.1)

            except Exception:
                self.bus = None
                self.last_status.set("Bus Error - Retrying")
                time.sleep(1)

if __name__ == "__main__":
    root = tk.Tk()
    app = CanDashboard(root)
    root.mainloop()