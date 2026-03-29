import can          # Library to interface with CAN hardware (USB-CAN adapters)
import time         # Used for delays and timing (e.g., the 10Hz heartbeat)
import threading    # Allows CAN communication to run in the background without freezing the UI
import tkinter as tk # Standard Python library for creating Windows/GUI
from tkinter import ttk # Improved themed widgets for a modern look

# --- CONFIGURATION ---
# These must match your specific hardware setup
CHANNEL = 'COM11'  # The Serial/COM port where your USB-CAN adapter is plugged in
BITRATE = 500000   # The speed of the CAN bus (500kbps) - all nodes must match this

class CanDashboard:
    def __init__(self, root):
        """Initializes the GUI and the background communication thread"""
        self.root = root
        self.root.title("CAN Multi-Node System Control")
        self.root.geometry("400x750") # Set the size of the dashboard window
        
        # --- Variables (Reactive Data) ---
        # Data from the ESP32 (ID 0x123)
        self.esp_dist = tk.StringVar(value="--- mm")    # Holds the distance string for the UI
        self.esp_heartbeat = tk.StringVar(value="0")    # Tracks the sequence number (proves it's alive)
        
        # Data from the Nano (ID 0x111)
        self.nano_dist = tk.StringVar(value="--- cm")   # Holds distance in cm
        self.nano_heartbeat = tk.StringVar(value="0")   # Tracks Nano sequence number
        
        # System State Variables
        self.last_status = tk.StringVar(value="Disconnected") # Displays "Connected" or "Error"
        self.port_status = tk.StringVar(value=f"Target: {CHANNEL}") # Shows which COM port is used
        self.pending_cmd_bytes = list("Stop".encode('ascii')) # Stores the current motor command as bytes
        self.bus = None # Placeholder for the CAN bus object
        
        self.setup_ui() # Calls the function to build the visual buttons and labels
        
        # Start background thread for CAN communication so the UI stays responsive
        self.running = True
        self.can_thread = threading.Thread(target=self.can_worker, daemon=True)
        self.can_thread.start()

    def setup_ui(self):
        """Creates all the labels, buttons, and frames on the screen"""
        # 1. Connection Status Section: Shows if the USB adapter is plugged in
        status_frame = ttk.LabelFrame(self.root, text=" Network Status ")
        status_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(status_frame, textvariable=self.port_status, font=('Arial', 9, 'bold')).pack()
        self.status_label = ttk.Label(status_frame, textvariable=self.last_status, foreground="red")
        self.status_label.pack()

        # 2. ESP32 Node Display: Shows data received from ID 0x123
        esp_frame = ttk.LabelFrame(self.root, text=" ESP32 Node (0x123) ")
        esp_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(esp_frame, text="VL53L0X Distance:", font=('Arial', 9)).pack()
        ttk.Label(esp_frame, textvariable=self.esp_dist, font=('Arial', 25, 'bold'), foreground="blue").pack()
        ttk.Label(esp_frame, textvariable=self.esp_heartbeat, font=('Arial', 9)).pack()

        # 3. Nano Node Display: Shows data received from ID 0x111
        nano_frame = ttk.LabelFrame(self.root, text=" Nano Node (0x111) ")
        nano_frame.pack(pady=10, padx=10, fill="x")
        ttk.Label(nano_frame, text="MaxSonar Distance:", font=('Arial', 9)).pack()
        ttk.Label(nano_frame, textvariable=self.nano_dist, font=('Arial', 25, 'bold'), foreground="purple").pack()
        ttk.Label(nano_frame, textvariable=self.nano_heartbeat, font=('Arial', 9)).pack()

        # 4. Robot Control Pad: Buttons to send motor commands (ID 0x456)
        btn_frame = ttk.LabelFrame(self.root, text=" Control Interface ")
        btn_frame.pack(pady=10, padx=10, fill="x")
        
        pad_frame = ttk.Frame(btn_frame)
        pad_frame.pack(pady=5)

        # List of (Display Text, Internal Command, Row, Column)
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
            # Momentary Logic: When button is pressed, set command. When released, set "Stop".
            b.bind('<ButtonPress-1>', lambda e, x=cmd: self.set_cmd(x))
            if cmd != "Stop":
                b.bind('<ButtonRelease-1>', lambda e: self.set_cmd("Stop"))

        # 5. Remote Nano LCD Command: A button that triggers a message on the Nano's screen
        trigger_btn = ttk.Button(btn_frame, text="LCD DISPLAY")
        trigger_btn.pack(pady=15, padx=20, fill="x")
        # Pressing sends value 1 (True), Releasing sends 0 (False) to ID 0x500
        trigger_btn.bind('<ButtonPress-1>', lambda e: self.send_lcd_cmd(1))
        trigger_btn.bind('<ButtonRelease-1>', lambda e: self.send_lcd_cmd(0))

    def set_cmd(self, word):
        """Converts the movement word (like 'Forward') into bytes for the ESP32"""
        self.pending_cmd_bytes = list(word.encode('ascii')) # Turn string into a list of numbers
        if self.bus:
            self.last_status.set(f"TX: {word}") # Show the last sent command on the UI

    def send_lcd_cmd(self, state):
        """Sends a high-priority single-byte message to the Nano (0x500)"""
        if self.bus:
            try:
                msg = can.Message(
                    arbitration_id=0x500, # The destination address
                    data=[state],         # The data (1 or 0)
                    is_extended_id=False  # Using Standard 11-bit CAN ID
                )
                self.bus.send(msg)        # Push the message onto the physical wire
                status_text = "LCD Message ON" if state == 1 else "Connected"
                self.last_status.set(status_text)
            except Exception as e:
                print(f"LCD Command failed: {e}")

    def can_worker(self):
        """The Background Engine: Handles all physical CAN traffic"""
        while self.running:
            # Step A: Connection Logic
            if self.bus is None:
                try:
                    # Attempt to open the serial port using the SLCAN protocol
                    self.bus = can.interface.Bus(interface='slcan', channel=CHANNEL, bitrate=BITRATE)
                    self.last_status.set("Connected")
                    self.status_label.configure(foreground="green")
                except Exception:
                    # If hardware isn't plugged in, wait 2 seconds and try again
                    self.last_status.set("Hardware Missing...")
                    time.sleep(2)
                    continue

            try:
                # 1. RECEIVE PHASE: Read all messages waiting in the buffer
                while True:
                    msg = self.bus.recv(0.001) # Check for messages (non-blocking)
                    if msg is None: break # If no more messages, move to sending
                    
                    # If message comes from ESP32 Sensor (ID 0x123)
                    if msg.arbitration_id == 0x123:
                        # Reconstruct 16-bit distance from two 8-bit bytes
                        dist_mm = (msg.data[0] << 8) | msg.data[1]
                        self.esp_dist.set(f"{dist_mm} mm")
                        self.esp_heartbeat.set(f"ESP Seq: {msg.data[2]}")
                    
                    # If message comes from Nano Sensor (ID 0x111)
                    elif msg.arbitration_id == 0x111:
                        dist_cm = msg.data[0] # Nano only sends 1 byte for distance
                        seq_num = msg.data[1] # Second byte is the sequence
                        self.nano_dist.set(f"{dist_cm} cm")
                        self.nano_heartbeat.set(f"Nano Seq: {seq_num}")

                # 2. SEND PHASE: Constantly broadcast the motor command at 10Hz (every 100ms)
                # This ensures the robot stops if it loses connection (Safety Heartbeat)
                tx_msg = can.Message(
                    arbitration_id=0x456,         # Motor Controller ID
                    data=self.pending_cmd_bytes,  # The "Forward", "Left", etc. bytes
                    is_extended_id=False
                )
                self.bus.send(tx_msg)
                time.sleep(0.1) # Frequency control: 10 times per second

            except Exception:
                # If the wire is unplugged or there is a short, reset the connection
                self.bus = None
                self.last_status.set("Bus Error - Retrying")
                time.sleep(1)

if __name__ == "__main__":
    root = tk.Tk()           # Create the main window
    app = CanDashboard(root) # Initialize the logic
    root.mainloop()          # Start the GUI event loop