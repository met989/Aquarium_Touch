import os
import re
import sys

def get_version():
    config_path = os.path.join("include", "config.h")
    try:
        with open(config_path, "r") as f:
            content = f.read()
            match = re.search(r'#define\s+AQUARIUM_OS_VERSION\s+"([^"]+)"', content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Errore nella lettura di config.h: {e}")
    return None

def main():
    version = get_version()
    if not version:
        print("Errore: Impossibile trovare la versione in include/config.h")
        sys.exit(1)

    tag = f"v{version}"
    print(f"==================================================")
    print(f"Preparazione della Release per la versione: {tag}")
    print(f"==================================================")
    print("1. Creazione del tag Git locale...")
    
    # Elimina il tag se esiste già localmente, per evitare errori se si vuole forzare un ri-upload
    os.system(f"git tag -d {tag} >nul 2>&1")
    
    # Crea il nuovo tag
    res_tag = os.system(f"git tag {tag}")
    if res_tag != 0:
        print("Errore durante la creazione del tag. Assicurati che non ci siano problemi con Git.")
        sys.exit(1)
    
    print("2. Caricamento del tag su GitHub (push)...")
    # Usa il comando push origin con il force (--force) in caso il tag sia stato modificato
    res_push = os.system(f"git push origin {tag} --force")
    if res_push != 0:
        print("Errore durante l'invio del tag su GitHub.")
        sys.exit(1)
        
    print(f"==================================================")
    print(f"SUCCESSO! Tag {tag} inviato a GitHub.")
    print(f"GitHub sta ora compilando il tuo codice nei suoi server.")
    print(f"Tra un paio di minuti troverai la tua nuova Release pronta")
    print(f"con il Changelog automatico e il file .bin allegato!")
    print(f"==================================================")

if __name__ == "__main__":
    main()
