import serial
import csv
import re
from datetime import datetime

# Configurações da porta serial
PORTA = '/dev/ttyACM0'   # altere conforme sua porta (ex: 'COM3' no Windows)
BAUDRATE = 115200

# Nome do arquivo CSV com timestamp
arquivo_csv = f"serial_data.csv"

# Expressão regular para capturar os dados
padrao = re.compile(
    r"Lat:\s*([-0-9.]+)\s+Lon:\s*([-0-9.]+)\s+Sats:\s*(\d+)\s+X\s*:\s*([-0-9.]+)\s+Y\s*:\s*([-0-9.]+)"
)

# Abre a porta serial
ser = serial.Serial(PORTA, BAUDRATE, timeout=1)

# Cria o arquivo CSV e escreve o cabeçalho
with open(arquivo_csv, mode='w', newline='') as csvfile:
    writer = csv.writer(csvfile, delimiter=';')
    writer.writerow(['Lat', 'Lon', 'Sats', 'X', 'Y'])

    print(f"Lendo dados da serial ({PORTA}) e salvando em {arquivo_csv}...\nPressione Ctrl+C para parar.\n")

    try:
        while True:
            linha = ser.readline().decode(errors='ignore').strip()

            # Tenta extrair os dados com regex
            match = padrao.search(linha)
            if match:
                lat, lon, sats, x, y = match.groups()
                writer.writerow([lat, lon, sats, x, y])
                csvfile.flush()  # garante que os dados sejam gravados imediatamente
                print(f"Lat={lat}, Lon={lon}, Sats={sats}, X={x}, Y={y}")

    except KeyboardInterrupt:
        print("\nLeitura interrompida pelo usuário.")
    finally:
        ser.close()
        print("Conexão serial encerrada.")
