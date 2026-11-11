import csv
import time
from datetime import datetime, timezone

import serial

ser = serial.Serial("COM3", 9600, timeout=3)
time.sleep(2)                    # l'Arduino redémarre à l'ouverture du port
ser.reset_input_buffer()
ser.write(b"D")

lignes = []
while True:
    ligne = ser.readline().decode().strip()
    if ligne in ("", "FIN"):
        break
    lignes.append(ligne)
ser.close()

with open("temperatures.csv", "w", newline="") as f:
    ecrivain = csv.writer(f, delimiter=";")
    ecrivain.writerow(["date_heure", "temperature_C"])
    for ligne in lignes[1:]:     # on saute l'en-tête envoyé par l'Arduino
        ts, temp = ligne.split(";")
        date = datetime.fromtimestamp(int(ts), timezone.utc)
        ecrivain.writerow([date.isoformat(sep=" "), temp])

print(f"{len(lignes) - 1} mesures enregistrées dans temperatures.csv")