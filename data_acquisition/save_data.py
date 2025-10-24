#!/usr/bin/env python3
import serial
import json
import re
import os

# ============================
# CONFIGURAÇÕES
# ============================
PORT = "COM5"           # Substitua pela sua porta serial
BAUDRATE = 115200
JSON_FILE = "gps_data.json"

# Expressão regular para extrair dados do print do ESP32
REGEX = re.compile(
    r"Lat:\s*([-\d.]+)\s*Lon:\s*([-\d.]+)\s*Sats:\s*(\d+).*?X:\s*([-\d.]+)\s*Y:\s*([-\d.]+)",
    re.DOTALL
)

# ============================
# FUNÇÕES
# ============================

def load_json():
    """Carrega o arquivo JSON existente (ou cria novo)."""
    if os.path.exists(JSON_FILE):
        with open(JSON_FILE, "r") as f:
            try:
                return json.load(f)
            except json.JSONDecodeError:
                return {}
    return {}


def save_json(data_dict):
    """Salva o conteúdo completo no arquivo JSON."""
    with open(JSON_FILE, "w") as f:
        json.dump(data_dict, f, indent=4)


def read_latest_valid_data(ser):
    """Lê a serial até encontrar uma linha válida com dados de GPS."""
    while True:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue

        match = REGEX.search(line)
        if match:
            lat, lon, sats, x, y = match.groups()
            lat, lon, x, y = map(float, [lat, lon, x, y])
            sats = int(sats)
            return lat, lon, x, y, sats


def main():
    print(f"[INFO] Conectando à serial {PORT} @ {BAUDRATE}...")
    ser = serial.Serial(PORT, BAUDRATE, timeout=1)

    data_dict = load_json()
    point_counter = len(data_dict) + 1

    print("[INFO] Sistema iniciado.")
    print("Pressione ENTER para registrar uma nova leitura, ou 'q' para sair.\n")

    while True:
        user_input = input("Aguradando comando...").strip().lower() # Aguarda o ENTER
        if user_input == 'q':
            break

        print("[INFO] Lendo dados do GPS...")
        lat, lon, x, y, sats = read_latest_valid_data(ser)
        print(f"[INFO] Dado capturado:")
        print(f"Lat: {lat:.6f}, Lon: {lon:.6f}, X: {x:.3f}, Y: {y:.3f}, Sats: {sats}")

        try:
            true_x = float(input("Digite true_x (m): "))
            true_y = float(input("Digite true_y (m): "))
        except ValueError:
            print("[ERRO] Valores inválidos. Ponto ignorado.\n")
            continue

        point_name = f"point{point_counter}"
        point_counter += 1

        data_dict[point_name] = {
            "lat": lat,
            "lon": lon,
            "x": x,
            "y": y,
            "true_x": true_x,
            "true_y": true_y,
            "sats": sats
        }

        save_json(data_dict)
        print(f"[OK] {point_name} salvo no JSON!\n")

    ser.close()
    print("[INFO] Finalizado.")


if __name__ == "__main__":
    main()
