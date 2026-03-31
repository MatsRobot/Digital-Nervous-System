import can
import time
import threading
import tkinter as tk
from tkinter import ttk

# --- CONFIGURATION ---
CHANNEL = 'COM11' 
BITRATE = 500000

class CanDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("CAN Momentary Control")
        self.root.geometry("350x550")
        
        # Variables
        self.current_dist = tk.StringVar(value="--- mm")
        self.esp_heartbeat = tk.StringVar(value="0")
        self.last_status = tk.StringVar(value="Disconnected")
        self.port_status = tk.StringVar(value=f"Target: {CHANNEL}")
        
        self.pending_cmd_bytes = list("Stop".encode('ascii')) 
        self.bus = None # Placeholder for the CAN bus object
        
        self.setup_ui()
        
        # Start background thread
        self.running = True
        self.can_thread = threading.Thread(target=self.can_worker, daemon=True)
        self.can_thread.start()

    def setup_ui(self):
        # 1. Connection Status Area (NEW)
        status_frame = ttk.LabelFrame(self.root, text=" Network Status ")
        status_frame.pack(pady=10, padx=10, fill="x")
        
        ttk.Label(status_frame, textvariable=self.port_status, font=('Arial', 9, 'bold')).pack()
        self.status_label = ttk.Label(status_frame, textvariable=self.last_status, foreground="red")
        self.status_label.pack()

        # 2. Distance Display
        ttk.Label(self.root, text="Distance (mm)", font=('Arial', 10)).pack(pady=(10,0))
        ttk.Label(self.root, textvariable=self.current_dist, font=('Arial', 30, 'bold'), foreground="blue").pack()

        # 3. Heartbeat Display
        hb_frame = ttk.Frame(self.root)
        hb_frame.pack(pady=5)
        ttk.Label(hb_frame, text="ESP32 Seq: ", font=('Arial', 10)).pack(side=tk.LEFT)
        ttk.Label(hb_frame, textvariable=self.esp_heartbeat, font=('Arial', 10, 'bold'), foreground="green").pack(side=tk.LEFT)

        # 4. Control Pad
        btn_frame = ttk.Frame(self.root)
        btn_frame.pack(pady=20)
        
        btn_fwd = ttk.Button(btn_frame, text="FORWARD")
        btn_left = ttk.Button(btn_frame, text="LEFT")
        btn_stop = ttk.Button(btn_frame, text="STOP")
        btn_right = ttk.Button(btn_frame, text="RIGHT")
        btn_bwd = ttk.Button(btn_frame, text="BACKWARD")

        btn_fwd.grid(row=0, column=1, pady=5)
        btn_left.grid(row=1, column=0, padx=5)
        btn_stop.grid(row=1, column=1, padx=5)
        btn_right.grid(row=1, column=2, padx=5)
        btn_bwd.grid(row=2, column=1, pady=5)

        # Momentary Bindings
        btn_fwd.bind('<ButtonPress-1>', lambda e: self.set_cmd("Forward"))
        btn_fwd.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Forward"))
        btn_bwd.bind('<ButtonPress-1>', lambda e: self.set_cmd("Backward"))
        btn_bwd.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))
        btn_left.bind('<ButtonPress-1>', lambda e: self.set_cmd("Left"))
        btn_left.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))
        btn_right.bind('<ButtonPress-1>', lambda e: self.set_cmd("Right"))
        btn_right.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))
        btn_stop.bind('<ButtonPress-1>', lambda e: self.set_cmd("Stop"))

    def set_cmd(self, word):
        self.pending_cmd_bytes = list(word.encode('ascii'))
        if self.bus: # Only update status text if connected
            self.last_status.set(f"Moving: {word}")

    def can_worker(self):
        """Threaded worker to handle CAN interface"""
        while self.running:
            if self.bus is None:
                try:
                    # Attempting to open the hardware
                    self.bus = can.interface.Bus(interface='slcan', channel=CHANNEL, bitrate=BITRATE)
                    self.last_status.set("Connected & Active")
                    self.status_label.configure(foreground="green")
                except Exception as e:
                    # Capture specific error (e.g., Port Busy, Access Denied)
                    err_msg = str(e).split(':')[-1] # Shorten the error
                    self.last_status.set(f"Error: {err_msg}")
                    self.status_label.configure(foreground="red")
                    time.sleep(2) # Wait before retrying
                    continue

            try:
                # 1. RECEIVE
                while True:
                    msg = self.bus.recv(0.001)
                    if msg is None: break
                    if msg.arbitration_id == 0x123:
                        dist = (msg.data[0] << 8) | msg.data[1]
                        self.current_dist.set(f"{dist} mm")
                        self.esp_heartbeat.set(str(msg.data[2]))

                # 2. SEND (10Hz)
                tx_msg = can.Message(
                    arbitration_id=0x456,
                    data=self.pending_cmd_bytes,
                    is_extended_id=False
                )
                self.bus.send(tx_msg)
                time.sleep(0.1)

            except Exception as e:
                self.bus = None # Trigger a reconnect
                self.last_status.set("Lost Connection")
                time.sleep(1)

if __name__ == "__main__":
    root = tk.Tk()
    app = CanDashboard(root)
    root.mainloop()