import can          # Library for CAN-Bus communication (handling messages/interfaces)
import time         # Library for timing, delays, and timestamps
import threading    # Allows running the CAN communication and the UI simultaneously
import tkinter as tk # Standard Python library for creating Graphical User Interfaces (GUIs)
from tkinter import ttk # The "Themed" widget set for a more modern UI look
import sys          # Provides access to system-specific parameters and functions

# --- CONFIGURATION ---
# The Serial Port where your CAN-to-USB adapter is plugged in
CHANNEL = 'COM11'
# The speed of the CAN network (must match all other nodes on the bus)
BITRATE = 500000

class CanDashboard:
    """Main application class for the CAN Network Monitor GUI."""
    
    def __init__(self, root):
        """Initialize the UI, variables, and communication thread."""
        self.root = root # Reference to the main window
        self.root.title("Multi-Node CAN Network Monitor") # Set window title
        self.root.geometry("500x900") # Set initial window size (width x height)
        self.style = ttk.Style() # Create a style object for custom widget looks
        self.style.configure('TLabelframe.Label', font=('Arial', 12, 'bold')) # Set font for frame titles
        self.root.protocol("WM_DELETE_WINDOW", self.on_closing) # Ensure clean exit when 'X' is clicked
        
        # --- UI VARIABLES (Thread-safe storage for data shown on screen) ---
        self.last_status = tk.StringVar(value="Searching for Hardware...") # Connection status text
        self.port_status = tk.StringVar(value=f"Target: {CHANNEL}") # Shows which COM port is targeted
        self.s3_seq = tk.StringVar(value="0") # Heartbeat counter for the ESP32-S3
        self.ir_fl = tk.StringVar(value="⚪") # Front-Left IR sensor status icon
        self.ir_fr = tk.StringVar(value="⚪") # Front-Right IR sensor status icon
        self.ir_rl = tk.StringVar(value="⚪") # Rear-Left IR sensor status icon
        self.ir_rr = tk.StringVar(value="⚪") # Rear-Right IR sensor status icon
        self.nano_dist = tk.StringVar(value="--- cm") # Distance data from Arduino Nano
        self.nano_seq = tk.StringVar(value="0") # Heartbeat counter for the Nano
        self.c3_adj = tk.StringVar(value="--- mm") # Calculated distance from ESP32-C3
        self.c3_raw_angle = tk.StringVar(value="Raw: --- @ ---°") # Raw sensor data and angle from C3
        self.c3_seq = tk.StringVar(value="0") # Heartbeat counter for the C3

        # --- CONTROL LOGIC STATE ---
        self.active_command = None  # Stores the string (e.g., "Forward") while a button is held
        self.last_tx_time = 0       # Stores time of last transmission to manage the 1-second interval
        self.bus = None             # Placeholder for the CAN bus interface object
        self.running = True         # Flag to keep the background thread running
        
        self.setup_ui() # Call the method to draw the interface
        
        # --- THREADING ---
        # Start the CAN worker in a separate thread so the GUI doesn't "freeze" while waiting for data
        self.can_thread = threading.Thread(target=self.can_worker, daemon=True)
        self.can_thread.start()

    def setup_ui(self):
        """Creates and arranges all buttons, labels, and frames in the window."""
        
        # --- Section: Network Status ---
        net_frame = ttk.LabelFrame(self.root, text=" Network Status ") # Grouping box for connection info
        net_frame.pack(pady=5, padx=10, fill="x") # Add to window with padding
        ttk.Label(net_frame, textvariable=self.port_status, font=('Arial', 10, 'bold')).pack() # Display COM port
        self.status_label = ttk.Label(net_frame, textvariable=self.last_status, foreground="orange") # Status msg
        self.status_label.pack()

        # --- Section 1: ESP32-S3 Node (Movement & IR) ---
        s3_frame = ttk.LabelFrame(self.root, text=" Section 1: ESP32-S3 Node ")
        s3_frame.pack(pady=5, padx=10, fill="x")
        ir_sub = ttk.Frame(s3_frame) # Sub-container for IR icons
        ir_sub.pack(pady=5)
        # Loop through names and variables to create 4 labels for IR status (FL, FR, RL, RR)
        for label, var in [("FL", self.ir_fl), ("FR", self.ir_fr), ("RL", self.ir_rl), ("RR", self.ir_rr)]:
            f = ttk.Frame(ir_sub) # Individual container for one IR point
            f.pack(side="left", padx=10) # Arrange side-by-side
            ttk.Label(f, text=label).pack() # Descriptive label (e.g., FL)
            ttk.Label(f, textvariable=var, font=('Arial', 14)).pack() # Dynamic circle icon
            
        h_frame1 = ttk.Frame(s3_frame) # Container for ESP32-S3 heartbeat
        h_frame1.pack(pady=5, fill="x")
        ttk.Label(h_frame1, text="Heartbeat: ", font=('Arial', 10, 'bold')).pack(side="left", padx=(10, 0))
        ttk.Label(h_frame1, textvariable=self.s3_seq).pack(side="left") # Display S3 sequence number

        # --- Movement Controls (Grid arrangement) ---
        ctrl_frame = ttk.Frame(s3_frame)
        ctrl_frame.pack(pady=10)
        # Defining the layout of buttons (Label, Command, Grid Row, Grid Column)
        btns = [("FORWARD", "Forward", 0, 1), ("LEFT", "Left", 1, 0), 
                ("STOP", "Stop", 1, 1), ("RIGHT", "Right", 1, 2), ("BACKWARD", "Backward", 2, 1)]

        for text, cmd, r, c in btns:
            b = ttk.Button(ctrl_frame, text=text) # Create button
            b.grid(row=r, column=c, padx=2, pady=2) # Place on grid
            # Binding events to allow "Momentary" control (Only active while pressed)
            b.bind('<ButtonPress-1>', lambda e, x=cmd: self.press_cmd(x)) # Runs when mouse clicked
            b.bind('<ButtonRelease-1>', lambda e: self.release_cmd()) # Runs when mouse released

        # --- Section 2: Arduino Nano Node (Ultrasonic Sonar) ---
        nano_frame = ttk.LabelFrame(self.root, text=" Section 2: Arduino Nano Node ")
        nano_frame.pack(pady=5, padx=10, fill="x")
        ttk.Label(nano_frame, text="Sonar Distance:").pack() # Static text
        ttk.Label(nano_frame, textvariable=self.nano_dist, font=('Arial', 18, 'bold'), foreground="purple").pack() # Distance readout
        h_frame2 = ttk.Frame(nano_frame) # Nano heartbeat display
        h_frame2.pack(pady=5, fill="x")
        ttk.Label(h_frame2, text="Heartbeat: ", font=('Arial', 10, 'bold')).pack(side="left", padx=(10, 0))
        ttk.Label(h_frame2, textvariable=self.nano_seq).pack(side="left")

        # --- Section 3: ESP32-C3 Node (Processed Sensor Data) ---
        c3_frame = ttk.LabelFrame(self.root, text=" Section 3: ESP32-C3 Node ")
        c3_frame.pack(pady=5, padx=10, fill="x")
        ttk.Label(c3_frame, text="Adjusted Distance:").pack()
        ttk.Label(c3_frame, textvariable=self.c3_adj, font=('Arial', 18, 'bold'), foreground="dark green").pack() # Display refined distance
        ttk.Label(c3_frame, textvariable=self.c3_raw_angle, font=('Arial', 10), foreground="gray").pack() # Display raw sensor/angle info
        h_frame3 = ttk.Frame(c3_frame) # C3 heartbeat display
        h_frame3.pack(pady=5, fill="x")
        ttk.Label(h_frame3, text="Heartbeat: ", font=('Arial', 10, 'bold')).pack(side="left", padx=(10, 0))
        ttk.Label(h_frame3, textvariable=self.c3_seq).pack(side="left")

    def press_cmd(self, word):
        """Called when a direction button is clicked; starts sending commands."""
        self.active_command = word # Store the command string (e.g., "Left")
        self.last_tx_time = 0 # Reset timer to 0 to ensure the CAN worker sends it immediately

    def release_cmd(self):
        """Called when a direction button is released; stops command transmission."""
        self.active_command = None # Clear the active command string

    def cleanup(self):
        """Properly shuts down the application and disconnects from the CAN interface."""
        self.running = False # Tell the background thread to stop its loop
        if self.bus: # If the CAN bus object exists
            try: self.bus.shutdown() # Attempt to gracefully close the connection
            except: pass # Ignore errors if already closed
            self.bus = None # Clear the reference

    def on_closing(self):
        """Fires when the user clicks the window close (X) button."""
        self.cleanup() # Run cleanup method
        self.root.quit() # Stop the tkinter main loop
        self.root.destroy() # Destroy the window object
        sys.exit(0) # Fully exit the Python process

    def can_worker(self):
        """Continuous background loop for receiving/sending CAN messages."""
        while self.running:
            if self.bus is None: # Attempt connection if not connected
                try:
                    # Initialize using SLCAN protocol (Standard for most USB-CAN adapters)
                    self.bus = can.interface.Bus(interface='slcan', channel=CHANNEL, bitrate=BITRATE)
                    self.last_status.set("Hardware Connected") # Update UI status
                    self.status_label.configure(foreground="green") # Change color to green
                except:
                    # If failed, update UI with error and wait before trying again
                    self.last_status.set(f"Error: {CHANNEL} Busy")
                    time.sleep(1)
                    continue
            try:
                # --- STEP 1: READ INCOMING CAN MESSAGES ---
                msg = self.bus.recv(0.001) # Poll for message (tiny timeout of 1ms to keep loop fast)
                if msg:
                    # If ID matches ESP32-S3 node (Telemetry)
                    if msg.arbitration_id == 0x123: 
                        self.s3_seq.set(str(msg.data[2])) # 3rd byte is sequence/heartbeat
                        if len(msg.data) > 3: # Check if data includes IR sensor bits
                            ir = msg.data[3] # 4th byte is IR sensor bitmask
                            # Bitwise check to toggle UI icons (Red Circle if hit, White Circle if clear)
                            self.ir_fl.set("🔴" if ir & 0x01 else "⚪") # Bit 0: Front Left
                            self.ir_fr.set("🔴" if ir & 0x02 else "⚪") # Bit 1: Front Right
                            self.ir_rl.set("🔴" if ir & 0x04 else "⚪") # Bit 2: Rear Left
                            self.ir_rr.set("🔴" if ir & 0x08 else "⚪") # Bit 3: Rear Right
                    
                    # If ID matches Arduino Nano (Sonar Telemetry)
                    elif msg.arbitration_id == 0x111: 
                        self.nano_dist.set(f"{msg.data[0]} cm") # 1st byte is distance in cm
                        self.nano_seq.set(f"{msg.data[1]}") # 2nd byte is sequence/heartbeat
                    
                    # If ID matches ESP32-C3 (Refined distance and angle)
                    elif msg.arbitration_id == 0x124 and len(msg.data) >= 6: 
                        # Combining two bytes for 16-bit values (Raw data)
                        raw = (msg.data[0] << 8) | msg.data[1]
                        # Combining two bytes for 16-bit values (Adjusted data)
                        adj = (msg.data[2] << 8) | msg.data[3]
                        self.c3_adj.set(f"{adj} mm") # Show adjusted distance in mm
                        self.c3_raw_angle.set(f"Raw: {raw} @ {msg.data[5]}°") # Show raw bits and angle
                        self.c3_seq.set(f"{msg.data[4]}") # Show sequence/heartbeat

                # --- STEP 2: WRITE OUTGOING COMMANDS ---
                now = time.time() # Get current timestamp
                # If a button is being held (active_command is not None)
                if self.active_command is not None:
                    # To avoid flooding the bus, send once every 1.0 seconds
                    if (now - self.last_tx_time) >= 1.0:
                        tx_bytes = list(self.active_command.encode('ascii')) # Convert string to ASCII bytes
                        # Create CAN message with ID 0x456 to control the S3
                        tx = can.Message(arbitration_id=0x456, data=tx_bytes)
                        self.bus.send(tx) # Send onto the bus
                        self.last_tx_time = now # Update time of last transmission

                # --- STEP 3: PERFORMANCE MANAGEMENT ---
                # Tiny sleep to prevent the CPU from running at 100% while still being ultra-responsive
                time.sleep(0.001) 
            except Exception:
                # If communication fails, reset bus and try to reconnect next loop
                self.bus = None
                time.sleep(1)

# --- APPLICATION ENTRY POINT ---
if __name__ == "__main__":
    root = tk.Tk() # Create the base window
    app = CanDashboard(root) # Instantiate the class
    try:
        root.mainloop() # Run the UI loop forever until closed
    finally:
        app.cleanup() # Clean exit logic to ensure CAN port is freed