import can
import time
import threading
import tkinter as tk
from tkinter import ttk

# --- CONFIGURATION ---
CHANNEL = 'COM11'  # Ensure this matches your slcan hardware port
BITRATE = 500000

class CanDashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("CAN Multi-Node System Control")
        self.root.geometry("400x700")
        
        # --- Variables ---
        # ESP32 Node (0x123)
        self.esp_dist = tk.StringVar(value="--- mm")
        self.esp_heartbeat = tk.StringVar(value="0")
        
        # Nano Node (0x111)
        self.nano_dist = tk.StringVar(value="--- cm")
        
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
        ttk.Label(esp_frame, text="VL53L0X Sensor:", font=('Arial', 9)).pack()
        ttk.Label(esp_frame, textvariable=self.esp_dist, font=('Arial', 25, 'bold'), foreground="blue").pack()
        ttk.Label(esp_frame, textvariable=self.esp_heartbeat, font=('Arial', 9)).pack()

        # 3. Nano Node Display (ID 0x111)
        nano_frame = ttk.LabelFrame(self.root, text=" Nano Node (0x111) ")
        nano_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(nano_frame, text="MaxSonar Sensor:", font=('Arial', 9)).pack()
        ttk.Label(nano_frame, textvariable=self.nano_dist, font=('Arial', 25, 'bold'), foreground="purple").pack()

        # 4. Robot Control Pad (TX to ID 0x456)
        btn_frame = ttk.LabelFrame(self.root, text=" Control Interface ")
        btn_frame.pack(pady=10, padx=10, fill="x")
        
        pad_frame = ttk.Frame(btn_frame)
        pad_frame.pack(pady=5)

        # Movement Buttons
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
                
            # Change 1: Set command on Press
            b.bind('<ButtonPress-1>', lambda e, x=cmd: self.set_cmd(x))
                
            # Change 2: ALWAYS reset to "Stop" on Release for ALL buttons
            b.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))
                

        # 5. Remote Nano LCD Command (TX to ID 0x500)
        # Momentary: Press sends 1 (Show), Release sends 0 (Clear)
        trigger_btn = ttk.Button(btn_frame, text="LCD DISPLAY")
        trigger_btn.pack(pady=15, padx=20, fill="x")
        
        # Action on Press: Send '1' and show "ON" status
        trigger_btn.bind('<ButtonPress-1>', lambda e: self.send_lcd_cmd(1))
        
        # Action on Release: Send '0' and reset UI to "Connected"
        trigger_btn.bind('<ButtonRelease-1>', lambda e: self.send_lcd_cmd(0))
        
    def send_lcd_cmd(self, state):
            """Sends momentary 1 or 0 to Nano LCD (0x500)"""
            if self.bus:
                try:
                    # Send the state (1 or 0) as a single byte
                    msg = can.Message(
                        arbitration_id=0x500,
                        data=[state], 
                        is_extended_id=False
                    )
                    self.bus.send(msg)
                    
                    # Update UI: If state is 0, we "blank" the message back to "Connected"
                    status_text = "LCD Message ON" if state == 1 else "Connected"
                    self.last_status.set(status_text)
                except Exception as e:
                    print(f"LCD Command failed: {e}")
                
                
    def set_cmd(self, word):
            """Prepares motor commands. Now resets to Stop on release."""
            self.pending_cmd_bytes = list(word.encode('ascii'))
            if self.bus:
                # This helps you see in the UI that the command is pulsing
                self.last_status.set(f"Bus Active - CMD: {word}")
            
    def send_lcd_trigger(self):
        """Sends a specific trigger pulse to the Nano Node"""
        if self.bus:
            try:
                trigger_msg = can.Message(
                    arbitration_id=0x500,
                    data=[0x01, 0x41, 0x43, 0x4B], # "ACK" payload
                    is_extended_id=False
                )
                self.bus.send(trigger_msg)
                self.last_status.set("LCD Trigger Sent")
            except Exception as e:
                print(f"Trigger failed: {e}")

    def can_worker(self):
        """Background thread for handling all CAN traffic"""
        while self.running:
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
                # 1. RECEIVE PHASE
                while True:
                    msg = self.bus.recv(0.001)
                    if msg is None: break
                    
                    # Process ESP32 Distance (0x123)
                    if msg.arbitration_id == 0x123:
                        dist_mm = (msg.data[0] << 8) | msg.data[1]
                        self.esp_dist.set(f"{dist_mm} mm")
                        self.esp_heartbeat.set(f"ESP Seq: {msg.data[2]}")
                        
                        # NEW: Decode IR Status Byte
                        ir_byte = msg.data[3]
                        fl = "🔴" if (ir_byte & 0x01) else "⚪"
                        fr = "🔴" if (ir_byte & 0x02) else "⚪"
                        rl = "🔴" if (ir_byte & 0x04) else "⚪"
                        rr = "🔴" if (ir_byte & 0x08) else "⚪"
                        self.last_status.set(f"IR: FL{fl} FR{fr} RL{rl} RR{rr}")
                    
                    # Process Nano Distance (0x111)
                    elif msg.arbitration_id == 0x111:
                        dist_cm = msg.data[0]
                        self.nano_dist.set(f"{dist_cm} cm")

    # 2. SEND PHASE inside the while running loop
                    # If command is a movement, send it continuously (Heartbeat)
                    # If command is Stop, send it a few times then stay quiet
                    current_cmd_str = "".join(map(chr, self.pending_cmd_bytes))
                    
                    if current_cmd_str != "Stop":
                        tx_msg = can.Message(arbitration_id=0x456, data=self.pending_cmd_bytes, is_extended_id=False)
                        self.bus.send(tx_msg)
                        time.sleep(0.1) # 10Hz Heartbeat for movement
                    else:
                        # If it's "Stop", just send once to ensure robot heard us, then wait
                        if not hasattr(self, 'already_stopped') or not self.already_stopped:
                            tx_msg = can.Message(arbitration_id=0x456, data=self.pending_cmd_bytes, is_extended_id=False)
                            self.bus.send(tx_msg)
                            self.already_stopped = True
                        time.sleep(0.1)
                    
                    if current_cmd_str != "Stop":
                        self.already_stopped = False
            except Exception:
                self.bus = None
                self.last_status.set("Bus Error - Reconnecting")
                time.sleep(1)

if __name__ == "__main__":
    root = tk.Tk()
    app = CanDashboard(root)
    root.mainloop()