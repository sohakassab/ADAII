"""
buzzer_control.py
=================
Python backend example for controlling the ESP32 buzzer via MQTT.

MQTT Topic:  devices/{DEVICE_UUID}/cmd/buzzer
Payload:     JSON  {"duration_ms": <milliseconds>}
             {"duration_ms": 0}  →  stop immediately

Usage:
    python buzzer_control.py                     # interactive menu
    python buzzer_control.py --buzz 5000         # buzz once for 5 seconds then exit
    python buzzer_control.py --stop              # send stop command then exit
"""

import argparse
import json
import ssl
import time

import paho.mqtt.client as mqtt
from paho.mqtt.enums import CallbackAPIVersion

# ─── Configuration ────────────────────────────────────────────────────────────
BROKER_URL  = "162dfd8d0eec4ab3866fe6a9eec7a2ac.s1.eu.hivemq.cloud"
BROKER_PORT = 8883
MQTT_USER   = "Server"
MQTT_PASS   = "Server123"

DEVICE_UUID = "Player1_ESP32"   # Must match DEVICE_UUID in Config.h

# Maximum duration the backend will ever request (safety mirror of Config.h)
MAX_DURATION_MS = 30_000
# ──────────────────────────────────────────────────────────────────────────────


def buzzer_topic() -> str:
    return f"devices/{DEVICE_UUID}/cmd/buzzer"


def status_topic() -> str:
    return f"devices/{DEVICE_UUID}/status"


# ─── MQTT Callbacks ───────────────────────────────────────────────────────────

# Track whether target device is online
_device_online: bool = False


def on_connect(client: mqtt.Client, userdata, flags, rc, properties=None):
    if rc == 0:
        print(f"[MQTT] Connected to {BROKER_URL}")
        # Subscribe to device status so we know when it's safe to send commands
        client.subscribe(status_topic())
        print(f"[MQTT] Subscribed to {status_topic()}")
    else:
        print(f"[MQTT] Connection failed – rc={rc}")


def on_message(client: mqtt.Client, userdata, message, properties=None):
    global _device_online
    payload = message.payload.decode("utf-8")
    topic   = message.topic

    if topic == status_topic():
        _device_online = payload == "online"
        icon = "🟢" if _device_online else "🔴"
        print(f"[STATUS] {DEVICE_UUID} is {'ONLINE' if _device_online else 'OFFLINE'} {icon}")


# ─── Helper Functions ─────────────────────────────────────────────────────────

def build_client() -> mqtt.Client:
    """Create, configure, and return a connected MQTT client."""
    client = mqtt.Client(CallbackAPIVersion.VERSION2, client_id="Python_Buzzer_Controller")
    client.username_pw_set(MQTT_USER, MQTT_PASS)
    client.tls_set(cert_reqs=ssl.CERT_NONE)
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER_URL, BROKER_PORT)
    return client


def send_buzz(client: mqtt.Client, duration_ms: int) -> None:
    """
    Publish a buzzer command.

    Args:
        duration_ms: How long the buzzer should sound (milliseconds).
                     Pass 0 to stop an active buzz immediately.
    """
    duration_ms = min(max(duration_ms, 0), MAX_DURATION_MS)
    payload = json.dumps({"duration_ms": duration_ms})
    topic   = buzzer_topic()
    result  = client.publish(topic, payload)
    result.wait_for_publish(timeout=5)

    if duration_ms == 0:
        print(f"[BUZZER] ⏹  Stop command sent → {topic}")
    else:
        print(f"[BUZZER] 🔔 Buzzing for {duration_ms / 1000:.1f} s → {topic}  payload={payload}")


# ─── Modes ────────────────────────────────────────────────────────────────────

def mode_single(duration_ms: int, stop: bool) -> None:
    """Send one command and disconnect."""
    client = build_client()
    client.loop_start()

    # Wait briefly for on_connect to fire
    time.sleep(1.5)

    if stop:
        send_buzz(client, 0)
    else:
        send_buzz(client, duration_ms)

    time.sleep(0.5)
    client.loop_stop()
    client.disconnect()


def mode_interactive() -> None:
    """Interactive menu — keeps running until the user presses Ctrl+C."""
    client = build_client()
    client.loop_start()

    print("\n" + "=" * 55)
    print("  ESP32 Buzzer Control Panel")
    print("  Waiting for device to come online …")
    print("  Press Ctrl+C at any time to exit.")
    print("=" * 55 + "\n")

    MENU = (
        "\n--- Buzzer Commands ---\n"
        "  1  Buzz for 1 second\n"
        "  2  Buzz for 3 seconds\n"
        "  3  Buzz for 5 seconds\n"
        "  4  Buzz for 10 seconds\n"
        "  c  Custom duration (you type the seconds)\n"
        "  s  Stop (silence immediately)\n"
        "  q  Quit\n"
        "Choice: "
    )

    try:
        while True:
            if not _device_online:
                time.sleep(1)
                continue

            choice = input(MENU).strip().lower()

            if choice == "1":
                send_buzz(client, 1_000)
            elif choice == "2":
                send_buzz(client, 3_000)
            elif choice == "3":
                send_buzz(client, 5_000)
            elif choice == "4":
                send_buzz(client, 10_000)
            elif choice == "c":
                try:
                    secs = float(input("  Enter duration in seconds: "))
                    send_buzz(client, int(secs * 1000))
                except ValueError:
                    print("  ⚠  Invalid number, try again.")
            elif choice == "s":
                send_buzz(client, 0)
            elif choice == "q":
                break
            else:
                print("  ⚠  Unrecognised option.")

    except KeyboardInterrupt:
        print("\n[Exit] Keyboard interrupt received.")

    finally:
        # Always silence before disconnecting
        print("[Exit] Sending stop before disconnect …")
        send_buzz(client, 0)
        time.sleep(0.5)
        client.loop_stop()
        client.disconnect()
        print("[Exit] Disconnected.")


# ─── Entry Point ──────────────────────────────────────────────────────────────

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="ESP32 Buzzer MQTT Controller")
    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--buzz",
        metavar="MS",
        type=int,
        help="Send a single buzz for MS milliseconds then exit",
    )
    group.add_argument(
        "--stop",
        action="store_true",
        help="Send an immediate stop command then exit",
    )
    args = parser.parse_args()

    if args.buzz is not None:
        mode_single(duration_ms=args.buzz, stop=False)
    elif args.stop:
        mode_single(duration_ms=0, stop=True)
    else:
        mode_interactive()
