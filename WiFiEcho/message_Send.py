#!/usr/bin/env python3

import socket

# === CONFIGURATION ===
ESP_HOST = '10.186.148.213'  # <-- CHANGE THIS to your ESP8266 IP or 'echo23.local' if mDNS works
ESP_PORT = 23               # Must match the port used by ESP (default: 23)

# === CONNECT TO SERVER ===
try:
    print(f"[+] Connecting to {ESP_HOST}:{ESP_PORT} ...")
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client.connect((ESP_HOST, ESP_PORT))
    print("[+] Connected! Type messages. Press Ctrl+C to exit.\n")
except Exception as e:
    print("[-] Connection failed:", e)
    exit(1)

# === INTERACTIVE LOOP ===
try:
    while True:
        msg = input("You > ").strip()
        if not msg:
            continue

        client.sendall(msg.encode())
        response = client.recv(1024)
        print("ESP > " + response.decode(errors='ignore'))

except KeyboardInterrupt:
    print("\n[!] Disconnected by user.")
finally:
    client.close()
