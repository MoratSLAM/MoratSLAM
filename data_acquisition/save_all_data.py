#!/usr/bin/env python3
import serial
import json
import re
import os
import time

# ============================
# CONFIGURAÇÕES
# ============================
PORT = "COM5"           # Porta serial do ESP32
BAUDRATE = 115200
JSON_FILE = "gps_data.json"

# Expressão regular para extrair dados
REGEX = re.compile(
    r"Lat:\s*([-\d.]+)\s*Lon:\s*([-\d.]+)\s*Sats:\s*(\d+)\s*X\s*:\s*([-\d.]+)\s*Y\s*:\s*([-\d.]+)"
)

# ============================
# FUNÇÕES
# ============================

def load_json():
    """Carrega o JSON existente ou cria novo."""
    if os.path.exists(JSON_FILE):
        with open(JSON_FILE, "r") as f:
            try:
                return json.load(f)
            except json.JSONDecodeError:
                return {}
    return {}

def save_json(data_dict):
    """Salva no arquivo JSON."""
    with open(JSON_FILE, "w") as f:
        json.dump(data_dict, f, indent=4)

def main():
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)
    data_dict = load_json()
    point_counter = len(data_dict) + 1

    print(f"[INFO] Lendo dados da porta {PORT}... (Ctrl+C para parar)")

    try:
        while True:
            line = ser.readline().decode(errors="ignore").strip()
            if not line:
                continue

            match = REGEX.search(line)
            if not match:
                continue

            lat, lon, sats, x, y = match.groups()
            lat, lon, x, y = map(float, [lat, lon, x, y])
            sats = int(sats)

            point_name = f"point{point_counter}"
            data_dict[point_name] = {
                "lat": lat,
                "lon": lon,
                "x": x,
                "y": y,
                "sats": sats
            }

            save_json(data_dict)
            point_counter += 1

            # Pequeno intervalo para evitar gravações muito rápidas
            time.sleep(0.5)

    except KeyboardInterrupt:
        print("\n[INFO] Finalizando leitura...")

    finally:
        ser.close()
        save_json(data_dict)
        print("[INFO] Arquivo JSON salvo e porta serial fechada.")

# ============================
# EXECUÇÃO
# ============================
if __name__ == "__main__":
    main()
