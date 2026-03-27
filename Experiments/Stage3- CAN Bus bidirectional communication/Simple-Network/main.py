import can      # Python-CAN library for bus communication
import time
import sys

# -----------------------------------------------------------------
# BUS CONFIGURATION
# -----------------------------------------------------------------
channel = 'COM11'  # Change this to your USB-to-CAN adapter port
bitrate = 500000   # Must match the ESP32 (500kbps)

def send_and_listen():
    try:
        # Initialize the 'slcan' (Serial Line CAN) interface
        bus = can.interface.Bus(interface='slcan', channel=channel, bitrate=bitrate)
        print(f"--- Connected to {channel} at {bitrate}bps ---")

        while True:
            # 1. TRANSMIT: Send a control packet to the ESP32
            # ID 0x456 is the 'Command ID' this system uses
            msg = can.Message(
                arbitration_id=0x456, 
                data=[0xAA, 0xBB, 0xCC, 0xDD], # Heartbeat/Control data
                is_extended_id=False
            )
            bus.send(msg)
            print("Sent Command: 0x456", flush=True)

            # 2. RECEIVE: Monitor for the ESP32's distance broadcast
            # We wait 0.5s for a packet; if none arrives, the loop continues
            rx_msg = bus.recv(0.5) 
            
            if rx_msg:
                # Filter for the specific Sensor Node ID
                if rx_msg.arbitration_id == 0x123:
                    # Reconstruct the distance value (High Byte << 8 | Low Byte)
                    distance = (rx_msg.data[0] << 8) | rx_msg.data[1]
                    print(f">>> ESP32 Distance: {distance} mm", flush=True)
                else:
                    # Log data from other potential nodes on the bus
                    print(f"Unknown Node: {hex(rx_msg.arbitration_id)}", flush=True)
            
            time.sleep(1) # Frequency of the Master-side monitoring

    except Exception as e:
        print(f"Critical Error: {e}")

if __name__ == "__main__":
    send_and_listen()