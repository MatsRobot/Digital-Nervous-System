import can                  # Library to talk to the CAN-Bus hardware
import time                 # Library for handling delays and timing
import threading            # Allows us to run the CAN communication in the background
import tkinter as tk        # The standard Python library for creating Windows/GUIs
from tkinter import ttk     # "Themed" widgets for a more modern look

# --- CONFIGURATION ---
CHANNEL = 'COM11'           # The USB port where your CAN adapter is plugged in
BITRATE = 500000            # Speed of the CAN network (500kbps)

class CanDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("CAN Multi-Node System Control")
        self.root.geometry("400x700")
        
        # --- Variables (Hold the data shown on screen) ---
        self.esp_dist = tk.StringVar(value="--- mm")        # Storage for distance text
        self.esp_heartbeat = tk.StringVar(value="0")       # Storage for heartbeat number
        self.nano_dist = tk.StringVar(value="--- cm")       # Storage for 2nd node data
        self.last_status = tk.StringVar(value="Disconnected") 
        self.port_status = tk.StringVar(value=f"Target: {CHANNEL}")
        
        # Convert "Stop" to ASCII numbers for the CAN message
        self.pending_cmd_bytes = list("Stop".encode('ascii')) 
        self.bus = None     # This will hold the CAN connection
        
        self.setup_ui()     # Build the buttons and labels
        
        # Start a "Worker" thread so the GUI doesn't freeze while waiting for data
        self.running = True
        self.can_thread = threading.Thread(target=self.can_worker, daemon=True)
        self.can_thread.start()

    def setup_ui(self):
        """Creates all the buttons and labels you see on the screen"""
        # Section 1: Network Status
        status_frame = ttk.LabelFrame(self.root, text=" Network Status ")
        status_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(status_frame, textvariable=self.port_status).pack()
        self.status_label = ttk.Label(status_frame, textvariable=self.last_status, foreground="red")
        self.status_label.pack()

        # Section 2: ESP32 Data (Displays IR and Heartbeat)
        esp_frame = ttk.LabelFrame(self.root, text=" ESP32 Node (0x123) ")
        esp_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(esp_frame, textvariable=self.esp_dist, font=('Arial', 25, 'bold'), foreground="blue").pack()
        ttk.Label(esp_frame, textvariable=self.esp_heartbeat).pack()

        # Section 4: Movement Control Pad
        btn_frame = ttk.LabelFrame(self.root, text=" Control Interface ")
        btn_frame.pack(pady=10, padx=10, fill="x")
        pad_frame = ttk.Frame(btn_frame)
        pad_frame.pack(pady=5)

        # Button Layout: (Label, Command, Row, Column)
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
            # When button is PRESSED, send the command
            b.bind('<ButtonPress-1>', lambda e, x=cmd: self.set_cmd(x))
            # When button is RELEASED, immediately send "Stop"
            b.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))

    def set_cmd(self, word):
        """Converts text like 'Forward' into raw bytes for the CAN bus"""
        self.pending_cmd_bytes = list(word.encode('ascii'))
        self.last_status.set(f"Bus Active - CMD: {word}")

    def can_worker(self):
        """This function runs in the background. It sends and receives all data."""
        while self.running:
            # 1. Connection Logic: Try to open the USB-CAN port
            if self.bus is None:
                try:
                    self.bus = can.interface.Bus(interface='slcan', channel=CHANNEL, bitrate=BITRATE)
                    self.last_status.set("Connected")
                    self.status_label.configure(foreground="green")
                except Exception:
                    self.last_status.set("Waiting for hardware...")
                    time.sleep(2)
                    continue

            try:
                # 2. RECEIVE LOOP: Check for data coming from the Robot
                while True:
                    msg = self.bus.recv(0.001) # Check for a message for 1 millisecond
                    if msg is None: break     # If no message, move to sending phase
                    
                    if msg.arbitration_id == 0x123: # If it's the ESP32
                        self.esp_heartbeat.set(f"ESP Seq: {msg.data[2]}")
                        
                        # DECODE IR STATUS: Check the bits in data[3]
                        ir_byte = msg.data[3]
                        fl = "🔴" if (ir_byte & 0x01) else "⚪" # Front Left
                        fr = "🔴" if (ir_byte & 0x02) else "⚪" # Front Right
                        rl = "🔴" if (ir_byte & 0x04) else "⚪" # Rear Left
                        rr = "🔴" if (ir_byte & 0x08) else "⚪" # Rear Right
                        self.last_status.set(f"IR: FL{fl} FR{fr} RL{rl} RR{rr}")

                # 3. SEND PHASE: Send the current command to the Robot
                current_cmd_str = "".join(map(chr, self.pending_cmd_bytes))
                
                if current_cmd_str != "Stop":
                    # For movement, send the command repeatedly at 10Hz
                    tx_msg = can.Message(arbitration_id=0x456, data=self.pending_cmd_bytes)
                    self.bus.send(tx_msg)
                    self.already_stopped = False
                    time.sleep(0.1) 
                else:
                    # For Stop, send it once then be quiet until the next button press
                    if not hasattr(self, 'already_stopped') or not self.already_stopped:
                        tx_msg = can.Message(arbitration_id=0x456, data=self.pending_cmd_bytes)
                        self.bus.send(tx_msg)
                        self.already_stopped = True
                    time.sleep(0.1)

            except Exception as e:
                print(f"CAN Error: {e}")
                self.bus = None # Trigger reconnection
                time.sleep(1)

if __name__ == "__main__":
    root = tk.Tk()
    app = CanDashboard(root)
    root.mainloop() # Start the window