# Aquarium OS Touch

Firmware per la gestione del tuo acquario tramite ESP32 e display touch.

## Animazione di Avvio (Boot GIF)
Il sistema supporta la riproduzione di un'animazione personalizzata durante l'accensione. 

Per utilizzarla:
1. Prepara una GIF animata e nominala **esattamente** `boot.gif` (tutto minuscolo).
2. Salva il file `boot.gif` nella **directory principale (root)** della scheda SD.
3. Inserisci la scheda SD e accendi il sistema. L'animazione verrà riprodotta per i primi 5 secondi di avvio.

### Specifiche e Limiti Consigliati per la GIF:
- **Risoluzione massima:** `320x240` pixel (pari allo schermo). Se la GIF è più piccola, il sistema la centrerà automaticamente su uno sfondo nero.
- **Dimensione file:** Lo spazio su SD non è un problema, ma per garantire una lettura fluida in tempo reale da parte dell'ESP32, è fortemente raccomandato mantenere il file **sotto i 2-3 MB**.
- **Framerate (FPS):** Si consiglia di esportare la GIF a **10-15 fps**. Framerate più alti (es. 60 fps) potrebbero non essere elaborati abbastanza velocemente dall'ESP32, causando rallentamenti o riproduzioni a scatti.
- **Trasparenza:** Pienamente supportata. Le aree trasparenti della GIF mostreranno il colore di sfondo nero nativo.

*Nota: Se il file `boot.gif` non è presente o la scheda SD non è inserita, il sistema ripiegherà automaticamente sulla classica schermata testuale ("Avvio sistema acquario...").*
