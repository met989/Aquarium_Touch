import os
import sys
import glob
import subprocess

def print_color(text, color):
    colors = {
        "cyan": "\033[96m",
        "red": "\033[91m",
        "yellow": "\033[93m",
        "green": "\033[92m",
        "reset": "\033[0m"
    }
    # Per semplicità stampiamo senza colori per evitare problemi con la console cmd base di windows
    print(text)

def main():
    print("========================================")
    print("      Aquarium OS Touch - Flasher       ")
    print("========================================\n")

    root_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    tools_dir = os.path.join(root_dir, "tools")
    esptool_path = os.path.join(tools_dir, "esptool.exe")

    if not os.path.exists(esptool_path):
        print("[ERRORE] esptool.exe non trovato nella cartella tools!")
        print("Assicurati di aver scaricato il progetto completo.")
        sys.exit(1)

    release_dir = os.path.join(root_dir, "release")
    if not os.path.exists(release_dir):
        print("[ERRORE] Cartella 'release' non trovata. Nessun firmware da flashare.")
        sys.exit(1)

    bins = glob.glob(os.path.join(release_dir, "*.bin"))
    if not bins:
        print("[ERRORE] Nessun file .bin trovato nella cartella release.")
        sys.exit(1)

    # Ordina per data (piu recenti prima)
    bins.sort(key=os.path.getmtime, reverse=True)

    print("File Firmware disponibili (dal piu' recente):")
    for i, b in enumerate(bins):
        print(f"[{i + 1}] {os.path.basename(b)}")

    choice = input("\nSeleziona il numero del firmware da flashare [predefinito: 1]: ").strip()
    if not choice:
        choice = "1"
    
    try:
        bin_index = int(choice) - 1
        if bin_index < 0 or bin_index >= len(bins):
            raise ValueError
    except ValueError:
        print("Selezione non valida.")
        sys.exit(1)

    selected_bin = bins[bin_index]
    print(f"-> Selezionato: {os.path.basename(selected_bin)}\n")

    # Ricerca Porte COM
    ports = []
    try:
        import serial.tools.list_ports
        for p in serial.tools.list_ports.comports():
            ports.append({"device": p.device, "description": p.description})
    except ImportError:
        if os.name == 'nt':
            import winreg
            try:
                key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DEVICEMAP\SERIALCOMM")
                for i in range(256):
                    try:
                        val = winreg.EnumValue(key, i)
                        ports.append({"device": val[1], "description": "Porta Seriale"})
                    except OSError:
                        break
            except Exception:
                pass

    selected_port = ""
    if not ports:
        print("Nessuna porta COM rilevata automaticamente. (Pyserial non installato e winreg fallito).")
        selected_port = input("Scrivi la porta COM manualmente (es. COM3): ").strip()
    else:
        print("Porte COM disponibili:")
        for i, p in enumerate(ports):
            print(f"[{i + 1}] {p['device']} - {p['description']}")
        
        p_choice = input("\nSeleziona la porta COM della tua scheda [predefinito: 1]: ").strip()
        if not p_choice:
            p_choice = "1"
        try:
            p_index = int(p_choice) - 1
            if p_index < 0 or p_index >= len(ports):
                raise ValueError
            selected_port = ports[p_index]['device']
        except ValueError:
            print("Selezione non valida.")
            sys.exit(1)
            
    if not selected_port:
        print("Porta COM non valida.")
        sys.exit(1)

    print(f"-> Porta selezionata: {selected_port}\n")
    print("========================================")
    print(f"Avvio il Flash su {selected_port}...")
    print("========================================")

    cmd = [
        esptool_path,
        "--chip", "esp32",
        "--port", selected_port,
        "--baud", "460800",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash", "-z", "0x10000", selected_bin
    ]

    result = subprocess.run(cmd)

    if result.returncode == 0:
        print("\n========================================")
        print("SUCCESSO! Firmware caricato correttamente.")
        print("Il dispositivo si riavviera' da solo.")
        print("========================================")
    else:
        print("\n[ERRORE] Il flash e' fallito. Controlla che la porta COM sia giusta, o premi il tasto BOOT sulla scheda.")
        sys.exit(1)

if __name__ == "__main__":
    main()
