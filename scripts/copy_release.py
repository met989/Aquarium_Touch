if "Import" not in globals():
    def Import(*args, **kwargs): pass
    class MockEnv:
        def get(self, key): return ""
        def AddPostAction(self, target, action): pass
    env = MockEnv()

Import("env")
env = globals().get("env") or locals().get("env")

import os
import shutil
import re

def get_version(env):
    version_file = os.path.join(env.get("PROJECT_DIR"), "version.json")
    try:
        import json
        with open(version_file, "r") as f:
            data = json.load(f)
            return data.get("version", "unknown")
    except Exception as e:
        print(f"Errore nella lettura di version.json: {e}")
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
