import can
import time

def send_and_listen():
    try:
        # Initialize the CAN bus via the USB adapter (Serial Line CAN - slcan)
        # channel='COM11' should match the port your adapter is plugged into
        bus = can.interface.Bus(interface='slcan', channel='COM11', bitrate=500000)
        print("--- High-Speed Mode Active ---")
        
        pc_seq = 0         # Sequence counter for messages sent by the Laptop
        last_send = time.time()

        while True:
            # SECTION 1: FLUSH RECEIVE BUFFER
            # We use an inner 'while' loop to read every message currently waiting
            # in the USB adapter's memory. This prevents "lag" or "buildup".
            while True:
                msg = bus.recv(0.001) # Look for a message for 1 millisecond
                if msg is None:       # If no more messages are waiting, exit inner loop
                    break
                
                # Check if the message is from our ESP32 (ID 0x123)
                if msg.arbitration_id == 0x123:
                    # Reconstruct the 16-bit distance from two 8-bit bytes
                    dist = (msg.data[0] << 8) | msg.data[1]
                    # The 3rd byte is the ESP32's sequence counter
                    esp_seq = msg.data[2]
                    
                    # Print formatted output to the terminal
                    print(f"[RX] Dist: {dist}mm | ESP Seq: {esp_seq} | PC Sent: {pc_seq}")

            # SECTION 2: PRECISE TRANSMIT
            # We use 'time.time()' to ensure we send a heartbeat exactly every 100ms (10Hz)
            if time.time() - last_send >= 0.1:
                pc_seq = (pc_seq + 1) % 256  # Increment counter (rolls over at 255)
                
                # Construct the outgoing message
                tx_msg = can.Message(
                    arbitration_id=0x456,    # ID for the ESP32 to look for
                    data=[0xAA, 0xBB, pc_seq], # Dummy data + our sequence number
                    is_extended_id=False     # Standard 11-bit CAN ID
                )
                bus.send(tx_msg)             # Send via USB
                last_send = time.time()      # Reset the timer

    except Exception as e:
        # Catch errors (like unplugging the USB) gracefully
        print(f"Error: {e}")

if __name__ == "__main__":
    send_and_listen()