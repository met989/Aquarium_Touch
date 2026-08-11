Import("env")
import os
import shutil
import re

def get_version(env):
    config_path = os.path.join(env.get("PROJECT_DIR"), "include", "config.h")
    try:
        with open(config_path, "r") as f:
            content = f.read()
            match = re.search(r'#define\s+AQUARIUM_OS_VERSION\s+"([^"]+)"', content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Errore nella lettura di config.h: {e}")
    return "unknown"

def copy_bin(source, target, env):
    # Percorso della cartella release
    release_dir = os.path.join(env.get("PROJECT_DIR"), "release")
    
    # Crea la cartella se non esiste
    if not os.path.exists(release_dir):
        os.makedirs(release_dir)
    
    # Il percorso del file .bin compilato
    bin_file = str(target[0])
    
    # Ottieni la versione da config.h
    version = get_version(env)
    
    # Costruisci il nome del file di destinazione come richiesto
    dest_filename = f"esp32_2432S028_v3_{version}.bin"
    dest_file = os.path.join(release_dir, dest_filename)
    
    # Copia il file
    shutil.copy(bin_file, dest_file)
    print(f"\n[Release] Firmware copiato in: {dest_file}\n")

# Aggiungi l'azione dopo la creazione del file .bin
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_bin)
