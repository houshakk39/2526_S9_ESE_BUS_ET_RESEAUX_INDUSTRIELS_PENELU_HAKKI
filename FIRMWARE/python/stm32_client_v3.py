#!/usr/bin/env python3
"""
Client série Raspberry Pi <-> STM32
TP Bus & Réseaux

Protocole main.c :
  - Commandes envoyées (ASCII, sans \r\n) :
      "GET_T", "GET_P", "GET_A", "GET_K", "SET_K=1234", ...
  - Réponses STM32 (exemples) :
      "T=+12.34_C\r\n"
      "P=101325Pa\r\n"
      "A=125.7000\r\n"
      "K=12.34000\r\n"
      "ERR=CMD\r\n"
"""

import serial
import time

# === Paramètres série ===
SERIAL_PORT = "/dev/ttyAMA0"   # ⚠️ à adapter : /dev/serial0, /dev/ttyACM0, etc.
BAUDRATE    = 115200
TIMEOUT_S   = 1.0              # délai de lecture en secondes


# --- Fonction générique d’envoi / lecture ligne ---
def send_command(ser, cmd: str) -> str:
    """
    Envoie une commande (sans \r\n) au STM32 :
      - envoie:  cmd + "\r\n"
      - lit:    une ligne jusqu'à \r ou \n (ou timeout)
    Retourne la ligne décodée, sans \r\n.
    """
    # On vide le buffer de réception avant d'envoyer une nouvelle commande
    ser.reset_input_buffer()

    # On envoie "cmd\r\n" (comme Enter dans minicom)
    line = (cmd + "\r\n").encode("ascii")
    ser.write(line)
    ser.flush()

    # Lecture "manuelle" jusqu'à \r ou \n
    deadline = time.time() + TIMEOUT_S
    buf = bytearray()
    while time.time() < deadline:
        if ser.in_waiting:
            b = ser.read(1)
            if not b:
                break
            if b in (b"\r", b"\n"):
                break
            buf += b
        else:
            time.sleep(0.01)

    resp = buf.decode("ascii", errors="ignore").strip()

    # Debug
    print(f"[DEBUG] cmd={cmd!r}, raw={bytes(buf)!r}, resp={resp!r}")
    return resp


# --- Parser générique de valeur d’après le main.c ---
def _parse_value(resp: str, prefix: str):
    """
    Parse une réponse du type :
      - 'T=+12.34_C'
      - 'P=101325Pa'
      - 'A=125.7000'
      - 'K=12.34000'
      - ou juste '534225'
    Retourne un float ou int suivant le cas.
    """
    if not resp:
        raise RuntimeError(f"Réponse vide pour {prefix}")

    # Vérification du préfixe : 'T=', 'P=', 'A=', 'K='...
    if '=' in resp:
        head, tail = resp.split('=', 1)
        head = head.strip()
        tail = tail.strip()
        if head != prefix:
            raise RuntimeError(f"Préfixe inattendu: {head!r} dans {resp!r}")
    else:
        # Cas où on reçoit juste un nombre
        tail = resp.strip()

    # Si ça se termine par 'H' → interpréter comme hexa brut (20 bits)
    if tail.endswith('H'):
        hex_str = tail[:-1]
        return int(hex_str, 16)

    # Sinon, extraire juste le nombre au début (chiffres, signe, point)
    num = ""
    for ch in tail:
        if ch.isdigit() or ch in "+-.":
            num += ch
        else:
            break

    if not num:
        raise RuntimeError(f"Impossible d'extraire un nombre depuis {resp!r}")

    # Si il y a un point, on renvoie float, sinon int
    if "." in num:
        return float(num)
    else:
        return int(num)


# === Une fonction par commande ===

def get_temperature(ser):
    resp = send_command(ser, "GET_T")
    return _parse_value(resp, "T")


def get_pressure(ser):
    resp = send_command(ser, "GET_P")
    return _parse_value(resp, "P")


def get_acceleration(ser):
    resp = send_command(ser, "GET_A")
    return _parse_value(resp, "A")


def get_K(ser):
    resp = send_command(ser, "GET_K")
    return _parse_value(resp, "K")


def set_K(ser, k_centi: int):
    """
    k_centi = K * 100 (ex: 12.34 -> 1234)
    """
    cmd = f"SET_K={k_centi}"
    resp = send_command(ser, cmd)
    return resp  # typiquement "SET_K=OK" ou "ERR=CMD"


def get_help(ser) -> str:
    return send_command(ser, "HELP")


# === Petit mode sniff au démarrage ===

def sniff_boot(ser, duration=2.0):
    """
    Pendant `duration` secondes, lit tout ce qui arrive sur le port
    et l'affiche (permet de voir '=== Protocole UART1 pret (Raspberry Pi) ===').
    """
    print(f"Sniff boot ({duration}s)...")
    start = time.time()
    ser.timeout = 0.1
    while time.time() - start < duration:
        data = ser.read(128)
        if data:
            try:
                txt = data.decode("ascii", errors="ignore")
            except Exception:
                txt = repr(data)
            print("[BOOT]", txt, end="")
    print("\n[Sniff terminé]\n")
    # on remet le timeout normal
    ser.timeout = TIMEOUT_S


# === Programme principal ===

def main():
    # Ouverture du port série
    ser = serial.Serial(
        port=SERIAL_PORT,
        baudrate=BAUDRATE,
        timeout=TIMEOUT_S
    )

    print(f"Connecté sur {SERIAL_PORT} à {BAUDRATE} bauds")
    print("Ctrl+C pour quitter.\n")

    # 1) On sniffe au démarrage pour vérifier qu’on voit le boot STM32
    sniff_boot(ser, duration=3.0)

    try:
        while True:
            print("=== Menu STM32 ===")
            print("1) Lire température")
            print("2) Lire pression")
            print("3) Lire accélération")
            print("4) Lire K (GET_K)")
            print("5) SET_K (ex: 1234 -> 12.34)")
            print("h) HELP")
            print("q) Quitter")
            choice = input("> ").strip()

            try:
                if choice == "1":
                    temp = get_temperature(ser)
                    print(f"Température : {temp}")
                elif choice == "2":
                    p = get_pressure(ser)
                    print(f"Pression : {p}")
                elif choice == "3":
                    a = get_acceleration(ser)
                    print(f"Accélération : {a}")
                elif choice == "4":
                    k = get_K(ser)
                    print(f"K : {k}")
                elif choice == "5":
                    val = input("K en centi (ex: 1234 pour 12.34) > ").strip()
                    k_centi = int(val)
                    resp = set_K(ser, k_centi)
                    print("Réponse SET_K :", resp)
                elif choice.lower() == "h":
                    print("HELP :", get_help(ser))
                elif choice.lower() == "q":
                    break
                else:
                    print("Choix invalide")
            except Exception as e:
                print("Erreur protocole :", e)

            print()
            time.sleep(0.1)

    except KeyboardInterrupt:
        print("\nArrêt demandé par l’utilisateur.")
    finally:
        ser.close()
        print("Port série fermé.")


if __name__ == "__main__":
    main()
