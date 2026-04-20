import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion
import time
import json

# --- Configuration ---
URL = "162dfd8d0eec4ab3866fe6a9eec7a2ac.s1.eu.hivemq.cloud"
USER = "Server"
PASS = "Server123"

# Dictionary to keep track of our online devices
live_devices = {}

# The callback for when a message is received from ESP32
def on_message(client, userdata, message, properties=None):
    payload = message.payload.decode("utf-8")
    topic_parts = message.topic.split('/')
    
    # We expect topics in the format: devices/[UUID]/[event]
    if len(topic_parts) >= 3 and topic_parts[0] == "devices":
        device_uuid = topic_parts[1]
        event_type = topic_parts[2]
        
        # 1. Handle Status (Online/Offline) Updates via LWT
        if event_type == "status":
            if payload == "online":
                live_devices[device_uuid] = True
                print(f"\n[🟢 STATUS] {device_uuid} is ONLINE!")
            elif payload == "offline":
                live_devices[device_uuid] = False
                print(f"\n[🔴 STATUS] {device_uuid} went OFFLINE!")
                
        # 2. Handle GPS Data
        elif event_type == "data":
            try:
                data = json.loads(payload)
                print(f"[{device_uuid}] {data}")
            except json.JSONDecodeError:
                print(f"[{device_uuid} | Data] {payload}")
                
             
        else:
            print(f"[{device_uuid} | {event_type}] {payload}")


# The callback for when we connect to the broker
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Successfully connected to HiveMQ Cloud!")
        # Subscribe using the wildcard '+' so we catch ALL devices dynamically
        client.subscribe("devices/+/status")
        client.subscribe("devices/+/data")
        client.subscribe("devices/+/temp")
        print("Subscribed to device topics.")
    else:
        print(f"Connection failed with code {rc}")


# 1. Initialize Client with VERSION2
client = mqtt.Client(CallbackAPIVersion.VERSION2, "Python_Backend_Server")

# 2. Set Callbacks
client.on_connect = on_connect
client.on_message = on_message

# 3. Security Settings
client.username_pw_set(USER, PASS)
client.tls_set() # Required for Port 8883

# 4. Connect
print(f"Connecting to {URL}...")
client.connect(URL, 8883)

# 5. Start Loop
client.loop_start()

try:
    while True:
        time.sleep(5)
            
except KeyboardInterrupt:
    print("Disconnecting...")
    client.loop_stop()
    client.disconnect()
