#!/usr/bin/env python3
"""
Simulador MQTT do ESP32 Calisto para Adafruit IO.

Uso:
  python scripts/simulate_esp32.py
"""

import argparse
import os
import random
import socket
import struct
import time


MQTT_HOST = "io.adafruit.com"
MQTT_PORT = 1883
IO_USERNAME = "SEU_USUARIO_ADAFRUIT"
IO_KEY = "SUA_CHAVE_AIO_KEY"
TELEMETRY_INTERVAL_SECONDS = 20
STATUS_INTERVAL_SECONDS = 60


def encode_remaining_length(length):
    encoded = bytearray()
    while True:
        byte = length % 128
        length //= 128
        if length > 0:
            byte |= 0x80
        encoded.append(byte)
        if length == 0:
            return bytes(encoded)


def mqtt_string(value):
    raw = value.encode("utf-8")
    return struct.pack("!H", len(raw)) + raw


def send_packet(sock, packet_type, payload):
    sock.sendall(bytes([packet_type]) + encode_remaining_length(len(payload)) + payload)


def connect_mqtt(sock, username, key):
    client_id = f"Calisto-Simulator-{random.randrange(0xffff):04x}"
    variable_header = (
        mqtt_string("MQTT")
        + bytes([4])
        + bytes([0xC2])
        + struct.pack("!H", 60)
    )
    payload = mqtt_string(client_id) + mqtt_string(username) + mqtt_string(key)
    send_packet(sock, 0x10, variable_header + payload)

    response = sock.recv(4)
    if len(response) < 4 or response[0] != 0x20 or response[3] != 0:
        raise RuntimeError(f"Falha ao conectar no MQTT. Resposta: {response!r}")


def publish(sock, topic, value):
    payload = mqtt_string(topic) + str(value).encode("utf-8")
    send_packet(sock, 0x30, payload)


def simulated_readings(mode):
    if mode == "alert":
        return {
            "calisto-temp": round(random.uniform(82.0, 88.0), 2),
            "calisto-vib-x": round(random.uniform(0.1200, 0.1800), 4),
            "calisto-vib-y": round(random.uniform(0.1100, 0.1700), 4),
            "calisto-vib-z": round(random.uniform(0.1300, 0.1900), 4),
        }

    if mode == "off":
        return {
            "calisto-temp": round(random.uniform(24.0, 29.0), 2),
            "calisto-vib-x": round(random.uniform(0.0010, 0.0080), 4),
            "calisto-vib-y": round(random.uniform(0.0010, 0.0080), 4),
            "calisto-vib-z": round(random.uniform(0.0010, 0.0080), 4),
        }

    return {
        "calisto-temp": round(random.uniform(32.0, 42.0), 2),
        "calisto-vib-x": round(random.uniform(0.0200, 0.0700), 4),
        "calisto-vib-y": round(random.uniform(0.0200, 0.0700), 4),
        "calisto-vib-z": round(random.uniform(0.0200, 0.0700), 4),
    }


def main():
    parser = argparse.ArgumentParser(description="Simula o ESP32 Calisto publicando no Adafruit IO.")
    parser.add_argument(
        "--mode",
        choices=["normal", "off", "alert"],
        default="normal",
        help="Perfil dos dados simulados.",
    )
    parser.add_argument(
        "--once",
        action="store_true",
        help="Publica uma rodada de telemetria e encerra.",
    )
    args = parser.parse_args()

    username = IO_USERNAME
    key = IO_KEY

    if username == "SEU_USUARIO_ADAFRUIT" or key == "SUA_CHAVE_AIO_KEY":
        raise SystemExit("Edite IO_USERNAME e IO_KEY no topo do script antes de executar.")

    with socket.create_connection((MQTT_HOST, MQTT_PORT), timeout=15) as sock:
        connect_mqtt(sock, username, key)
        print(f"[MQTT] Conectado em {MQTT_HOST}:{MQTT_PORT} como {username}")

        last_status = 0.0
        while True:
            readings = simulated_readings(args.mode)
            for feed, value in readings.items():
                topic = f"{username}/feeds/{feed}"
                publish(sock, topic, value)
                print(f"[PUB] {topic} = {value}")

            now = time.monotonic()
            if last_status == 0.0 or now - last_status >= STATUS_INTERVAL_SECONDS:
                publish(sock, f"{username}/feeds/calisto-status", 1)
                print(f"[PUB] {username}/feeds/calisto-status = 1")
                last_status = now

            if args.once:
                break

            time.sleep(TELEMETRY_INTERVAL_SECONDS)


if __name__ == "__main__":
    main()
