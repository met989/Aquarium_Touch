# Aquarium OS Touch 🐠

Firmware avanzato per microcontrollori **ESP32** dedicato al controllo, monitoraggio ed automazione completa di un acquario tramite display touch-screen a colori e server web integrato.

---

## 📋 Indice
1. [Presentazione del Progetto](#-presentazione-del-progetto)
2. [Caratteristiche Principali](#-caratteristiche-principali)
3. [Specifiche Hardware & Pinout](#-specifiche-hardware--pinout)
4. [Interfaccia Utente & Touch](#-interfaccia-utente--touch)
5. [Server Web & API REST](#-server-web--api-rest)
6. [Animazione di Avvio (Boot GIF)](#-animazione-di-avvio-boot-gif)
7. [Localizzazione & Lingue](#-localizzazione--lingue)
8. [Architettura Software & File di Progetto](#-architettura-software--file-di-progetto)
9. [Configurazione & Persistenza](#-configurazione--persistenza)
10. [Aggiornamento del Firmware](#-aggiornamento-del-firmware-update)
11. [Compilazione & Sviluppo](#-compilazione--sviluppo)
12. [Sviluppi Futuri (Roadmap)](#-sviluppi-futuri-roadmap)
13. [Manutenzione del README](#-manutenzione-del-readme)

---

## 🚀 Presentazione del Progetto

**Aquarium OS Touch** è un sistema di gestione per acquari basato sul framework Arduino per ESP32. Unisce un'interfaccia grafica moderna (stile *Glassmorphism* con animazioni in tempo reale) su un display **TFT ILI9341 da 2.8" (320x240)**. Il firmware è stato progettato e ottimizzato nativamente per la scheda all-in-one **ESP32-2432S028** (nota comunemente come *Cheap Yellow Display* o *CYD*), offrendo un potente backend con sincronizzazione NTP, gestione relè per illuminazione, monitoraggio della temperatura tramite sensore impermeabile **DS18B20** e un **Server Web HTTP** per il controllo remoto da browser o sistemi domotici.

### 🖼️ Anteprime dell'Interfaccia

<div align="center">

**Display Touch:**<br>
<img src="scheme/1.jpg?v=v0.6" alt="Dashboard" />
<img src="scheme/2.jpg?v=v0.6" alt="Luce" />
<img src="scheme/3.jpg?v=v0.6" alt="Programmazione" />
<img src="scheme/4.jpg?v=v0.6" alt="Impostazioni" />
<img src="scheme/5.jpg?v=v0.6" alt="Impostazioni2" />

<br><br>

**Interfaccia Web:**<br>
<img src="scheme/1.png?v=v0.6" alt="Web Dashboard" />
<img src="scheme/2.png?v=v0.6" alt="Web Settings" />

<br><br>

**Video Dimostrativo:**<br>
<img src="scheme/video.gif?v=v0.6" alt="Video Demo" />

</div>

---

## ✨ Caratteristiche Principali

- **Display Touchscreen & Grafica Avanzata:**
  - Risoluzione 320x240 pixel (RGB565) su driver ILI9341.
  - Touchscreen resistivo XPT2046 con supporto alla calibrazione e tastiera QWERTY touch a tre layout (lettere minuscole, maiuscole, numeri e simboli).
  - Interfaccia reattiva a 4 tab principali: *Dashboard*, *Luce*, *Programmazione (Schedule)*, *Impostazioni*.
  - Sfondo animato a tema marino con onde dinamiche, bolle fluttuanti e pesciolini traslanti.
  - Indicatore di temperatura con lancetta circolare animata.
  - Screensaver e modalità risparmio energetico a spegnimento retroilluminazione temporizzato.
  - **Sensore di Illuminamento e Auto-Dimming:** Regolazione automatica della luminosità del display (TFT_BL) in base alla luce ambientale utilizzando il fotoresistore GT36516 integrato sulla scheda. Gestibile sia dal display touch che dall'interfaccia web, con lettura in tempo reale del valore (mV).
- **Controllo Temperatura:**
  - Lettura continua da sensore DS18B20 su bus 1-Wire.
  - Soglie di temperatura ottimale configurabili (min/max predefiniti: 24.0°C – 27.0°C) con isteresi di 0.3°C e calibrazione tramite offset.
  - Segnalazione visiva degli stati: *Ottimale* (Verde Smeraldo), *Troppo Freddo* (Blu), *Troppo Caldo* (Rosso Corallo).
- **Gestione Illuminazione & Automatizzazione:**
  - Controllo output relè per la lampada o apparecchiatura dell'acquario (PIN 27).
  - Inizializzazione hardware sicura (Anti-Glitch) per evitare accensioni involontarie del relè all'avvio o al riavvio della scheda.
  - Logica relè configurabile: *Attivo HIGH* o *Attivo LOW* (Relè invertito).
  - Modalità automatica a orario (es. Accensione ore 10:00, Spegnimento ore 18:00).
  - Override manuale rapido da touch screen o da interfaccia web.
- **Connettività Wi-Fi & Servizi di Rete:**
  - Scansione asincrona delle reti Wi-Fi direttamente dall'interfaccia touch con selezione ed inserimento password a schermo.
  - Sincronizzazione dell'ora solare via server NTP (`pool.ntp.org`, `time.nist.gov`) con fuso orario regolabile (tramite comodo menu a tendina nella Web UI).
  - Supporto per IP Statico o DHCP.
  - **Safety & Cold Boot Anti-Crash:** Inizializzazione ottimizzata della radio Wi-Fi per evitare blocchi o "kernel panic" all'accensione. Se la connessione fallisce per 3 volte, il sistema interrompe i tentativi automatici in attesa di un riavvio della scansione manuale.
  - **Integrazione MQTT Avanzata e Dinamica:** Configurazione del broker aggiornata in tempo reale "al volo" (ogni 30s) dalla Web UI, senza necessità di riavviare l'ESP32.
  - **Indicatori di Stato Minimalisti:** La Dashboard integra comodi "semafori" (Verde = Connesso, Giallo = In connessione/Attesa, Rosso = Errore o IP errato) che mostrano a colpo d'occhio lo stato di Wi-Fi e MQTT mantenendo il design super pulito.
- **Server Web Integrato & API REST:**
  - Dashboard Web responsive per controllare luci, orari, temperature min/max e configurazioni.
  - API HTTP in formato JSON per l'integrazione con Home Assistant, Node-RED o script esterni.
  - Gestione remota del file di configurazione grezzo (`config.cfg`).
- **Sistema Multilingua:**
  - Caricamento dinamico dei file lingua `.lng` da scheda SD (`/languages/`) o dalla memoria Flash interna.
  - Generazione automatica dello header `embedded_languages.h` in fase di build tramite script Python/SCons (`embed_languages.py`).
  - Lingue incluse: Italiano (`italian.lng`), Inglese (`english.lng`).
- **Persistenza su Scheda SD:**
  - Tutte le preferenze utente vengono salvate in `/config.cfg` con gestione di file temporanei (`.tmp`) e backup di sicurezza (`.bak`).

---

## 🛠 Specifiche Hardware & Pinout

Il progetto è ottimizzato per l'utilizzo della scheda **ESP32-2432S028 (2.8" All-in-One)** ([acquistabile qui su AliExpress](https://it.aliexpress.com/item/1005005262421075.html)). 

<img src="scheme/esp32_2432s028_pcb-3.png?v=v0.6" alt="board" />

> [!NOTE]
> La [pagina di macsbug (ESP32-2432S028)](https://macsbug.wordpress.com/2022/08/17/esp32-2432s028/) illustra nel dettaglio tutto l'hardware. Esistono 3 varianti fisiche di questa scheda: **l'ultima versione** (la più aggiornata e consigliata) è riconoscibile dalla presenza simultanea di entrambe le porte **USB-C e Micro USB**.

Tutte le definizioni dei pin e dei parametri hardware sono accentrate in `include/config.h` (**Single Source of Truth**).

| Componente | Periferica | Pin ESP32 | Note / Descrizione |
| :--- | :--- | :--- | :--- |
| **Display TFT (ILI9341)** |
| | `TFT_MOSI` | `GPIO 13` | SPI MOSI Display |
| | `TFT_MISO` | `GPIO 12` | SPI MISO Display |
| | `TFT_SCLK` | `GPIO 14` | SPI Clock Display |
| | `TFT_CS` | `GPIO 15` | Chip Select Display |
| | `TFT_DC` | `GPIO 2` | Data / Command Select |
| | `TFT_RST` | `-1` | Hardware Reset (collegato a EN/VCC) |
| | `TFT_BL` | `GPIO 21` | Controllo Retroilluminazione (PWM / High-Low) |
| **Touchscreen (XPT2046)** |
| | `TOUCH_CS` | `GPIO 33` | Chip Select Touch |
| | `TOUCH_IRQ` | `GPIO 36` | Interrupt Touch (Input) |
| | `TOUCH_MOSI` | `GPIO 32` | SPI MOSI Touch |
| | `TOUCH_MISO` | `GPIO 39` | SPI MISO Touch |
| | `TOUCH_SCK` | `GPIO 25` | SPI Clock Touch |
| **Scheda SD** | `SD_CS` | `GPIO 5` | Chip Select SD Card |
| **Sensore Temperatura** | `TEMP_SENSOR_PIN` | `GPIO 22` | Bus 1-Wire DS18B20 (tramite connettore CN1, pull-up integrato o richiesta 4.7kΩ) |
| **Relè Luci** | `LIGHT_RELAY_PIN` | `GPIO 27` | Output per il controllo della plafoniera (su CN1) |
| **Sensore pH** | `PH_SENSOR_PIN` | `GPIO 35` | Input analogico per la lettura del sensore pH (su P3) |
| **Bus I2C (SDA)** | `I2C_SDA_PIN` | `GPIO 1` | Pin SDA per dispositivi I2C aggiuntivi (P1 TX) |
| **Bus I2C (SCL)** | `I2C_SCL_PIN` | `GPIO 3` | Pin SCL per dispositivi I2C aggiuntivi (P1 RX) |
| **LED Stato (Rosso)** | `LED_RED_PIN` | `GPIO 4` | Indica un problema o errore di sistema |
| **LED Stato (Verde)** | `LED_GREEN_PIN` | `GPIO 16` | Indica sistema completamente funzionante e pronto |
| **LED Stato (Blu)** | `LED_BLUE_PIN` | `GPIO 17` | Indica caricamento all'avvio in corso |
| **Sensore Luce (LDR)** | `LDR_PIN` | `GPIO 34` | Fotoresistore GT36516 integrato (Input Analogico) |
| **Pulsante di Avvio** | `BOOT_BTN` | `GPIO 0` | Pulsante BOOT integrato |

### Specifiche Bus SPI
- **Frequenza SPI Display:** 27 MHz
- **Frequenza Lettura SPI:** 20 MHz
- **Frequenza SPI Touch:** 2.5 MHz
- **Porta SPI:** HSPI

---

## 🎨 Interfaccia Utente & Touch

L'interfaccia utente (UI) è sviluppata su libreria `TFT_eSPI` sfruttando un buffer **TFT_eSprite** a tutta larghezza (320x172 px) per eliminare qualsiasi sfarfallamento.

> [!TIP]
> **Modalità Recovery (Ricalibrazione Touch):** Se il touch screen risulta impreciso o sfasato, puoi ricalibrarlo facilmente. Premi una volta il tasto **Reset** della scheda e mantieni premuto il tasto **Boot** per **3 secondi**. Il sistema entrerà in modalità Recovery e ti guiderà nella ricalibrazione guidata a schermo.

### Tab di Navigazione
1. **Dashboard:** Mostra l'indicatore analogico a lancetta della temperatura, lo stato della sonda, le temperature min/max registrate, la data/ora attuale e lo stato dell'illuminazione con animazione marina attiva.
2. **Luce:** Consente di accendere/spegnere manualmente il relè delle luci, attivare o disattivare la programmazione automatica a orario e invertire la polarità del relè (Active HIGH / LOW).
3. **Programmazione (Schedule):** Impostazione degli orari di accensione (ON) e spegnimento (OFF) dell'illuminazione acquario.
4. **Impostazioni (Settings):** Menu multi-pagina (4 voci per pagina) per configurare:
   - **1. Data & Ora:** Configurazione data, ora e sincronizzazione NTP.
   - **2. Temp. Target:** Target min/max temperatura e offset di calibrazione.
   - **3. Target pH:** Impostazioni per i limiti di allarme pH e offset sensore.
   - **4. Stato Luce:** Impostazioni polarità relè (Active HIGH/LOW).
   - **5. Wi-Fi:** Connessione, scansione (tastiera integrata) e gestione rete.
   - **6. Info di sistema:** Versione firmware, memoria e framework.
   - **7. Lingua:** Selezione lingua di sistema (es. Italiano, Inglese).
   - **8. Risparmio energia:** Impostazioni Screensaver e spegnimento retroilluminazione.
   - **9. Ripristino Fabbrica:** Reset di tutti i parametri utente.
   - **10. Hardware & I2C:** Strumento di scansione hardware del bus I2C (attivazione manuale on-demand per non bloccare l'interfaccia).
   - **11. Schermo:** Controllo luminosità manuale o Auto-Dimming (con lettura in tempo reale del sensore LDR).
   - **12. Timer Luce:** Accesso rapido alle impostazioni del programmatore orario.

---

## 🌐 Server Web & API REST

Quando l'ESP32 è connesso alla rete Wi-Fi, il Web Server integrato risponde sulla porta **80** offrendo una pagina web di controllo ed endpoint JSON.

### Endpoint API Principali
- `GET /` : Pagina web principale di controllo (Dashboard responsive).
- `GET /api/status` : Restituisce lo stato attuale del sistema (temperatura, relè, data, ora, Wi-Fi, memoria).
- `POST /api/light/toggle` : Inverte lo stato manuale della luce.
- `POST /api/auto/toggle` : Attiva/disattiva la programmazione automatica.
- `POST /api/schedule` : Imposta gli orari di accensione e spegnimento della luce.
- `POST /api/temp/settings` : Configura le temperature target min/max e l'offset.
- `POST /api/relay/invert` : Inverte la logica del relè.
- `POST /api/wifi/settings` : Imposta credenziali Wi-Fi o configurazione IP.
- `POST /api/mqtt/settings` : Configura il broker MQTT.
- `POST /api/settings/screen` : Configura l'Auto-Dimming e le impostazioni dello schermo.
- `POST /api/screensaver` : Imposta il tempo del salvaschermo.
- `POST /api/language` : Cambia la lingua attiva del sistema.
- `GET /api/config/raw` : Scarica il file `config.cfg` grezzo.
- `POST /api/config/raw` : Sovrascrive il file `config.cfg` con nuovo contenuto.

---

## 🎬 Animazione di Avvio (Boot GIF)

Il sistema supporta la riproduzione di un'animazione personalizzata in formato GIF durante l'accensione del dispositivo.

### Come utilizzarla:
1. Prepara un file GIF animato e rinominalo **esattamente** `boot.gif` (tutto minuscolo).
2. Salva `boot.gif` nella **root principale della scheda SD**.
3. Inserisci la scheda SD nell'ESP32 ed accendi il sistema. L'animazione verrà riprodotta per i primi 5 secondi di boot.

### Specifiche Raccomandate per la GIF:
- **Risoluzione massima:** `320x240` pixel (se più piccola verrà centrata su sfondo nero).
- **Dimensione file:** Mantenere preferibilmente sotto i **1 MB** per garantire la riproduzione fluida in streaming da scheda SD.
- **Framerate consigliato:** **10–15 FPS**.
- **Trasparenza:** Supportata.

*Nota: Se `boot.gif` non è presente o la scheda SD non è inserita, il sistema avvierà automaticamente la schermata di caricamento testuale standard.*

---

## 🌐 Localizzazione & Lingue

Il sistema gestisce la localizzazione dinamica tramite file di testo con estensione `.lng` memorizzati nella cartella `/languages/` della scheda SD oppure incorporati nella memoria Flash dell'ESP32.

### Generazione automatica delle lingue incorporate
All'avvio della compilazione con PlatformIO, lo script `embed_languages.py` legge tutti i file `.lng` presenti nella directory `languages/` e genera automaticamente l'header `include/embedded_languages.h`. Ciò permette al sistema di funzionare nella lingua corretta anche senza scheda SD.

---

## 📁 Architettura Software & File di Progetto

```
Aquarium_Touch/
├── .agents/
│   └── AGENTS.md               # Regole di progetto e workflow per agenti AI
├── include/
│   ├── config.h                # Single Point of Truth (Pinout, Colori RGB565, Default)
│   ├── aquarium_logic.h        # Definizione classe AquariumLogic (Temperatura, Relè, Wi-Fi, SD)
│   ├── aquarium_ui.h           # Definizione classe AquariumUI (Render TFT_eSPI, Animazioni, Touch)
│   ├── aquarium_server.h       # Definizione Web Server HTTP & API REST
│   ├── language_manager.h      # Gestore del dizionario di localizzazione i18n
│   ├── embedded_languages.h    # Header autogenerato con lingue integrate nella Flash
│   ├── embedded_web.h          # Header autogenerato con il file web compresso in GZIP

├── src/
│   ├── main.cpp                # Setup, Loop principale, gestione Boot GIF e calibrazione Touch
│   ├── aquarium_logic.cpp      # Logica di business, schedulazione, sensore DS18B20, gestione SD
│   ├── aquarium_ui.cpp         # Implementazione grafica TFT, menu, tastiera touch, animazioni
│   ├── aquarium_server.cpp     # Implementazione Server Web ed endpoint API JSON
│   ├── aquarium_mqtt.cpp       # Implementazione client MQTT per integrazione Home Assistant
│   └── language_manager.cpp    # Implementazione caricamento e parsing dei file .lng
├── languages/
│   ├── italian.lng             # File di localizzazione in lingua Italiana
│   └── english.lng             # File di localizzazione in lingua Inglese
├── scripts/
│   ├── embed_languages.py      # Script SCons pre-build per compilazione automatica lingue
│   ├── extract_html.py         # Script SCons pre-build per minificazione HTML e GZIP
│   └── copy_release.py         # Script post-build per spostare il firmware nella cartella release
├── web/
│   └── index.html              # Interfaccia web (Dashboard HTML/CSS/JS)
├── boot.gif                    # Esempio di animazione di boot per scheda SD
├── build.bat                   # Script per avviare la compilazione offline locale
├── flash.bat                   # Flasher intelligente offline per caricare il firmware compilato
├── publish_release.bat         # Tool automatico per creare una release tramite GitHub Actions
└── platformio.ini              # Configurazione ambiente PlatformIO e dipendenze
```

---

## ⚙️ Configurazione & Persistenza

Le configurazioni del sistema seguono una rigorosa gerarchia:
1. **Configurazioni predefinite di sistema (`include/config.h`):** Contiene i valori di default precompilati per hardware, pin, soglie di temperatura e colori.
2. **File di configurazione runtime (`/config.cfg` su scheda SD):** Salva ed aggiorna in tempo reale le impostazioni modificate dall'utente (Wi-Fi, orari illuminazione, temperature target, lingua, fuso orario).
3. **Meccanismo di Safety Fail-Safe:** Ogni salvataggio scrive prima su `/config.tmp` e mantiene una copia `/config.bak` per prevenire la corruzione dei dati in caso di spegnimento improvviso.

---

## 🔄 Aggiornamento del Firmware (Update)

Mantenere il tuo Aquarium OS Touch aggiornato è semplicissimo e non richiede competenze di programmazione. Puoi eseguire l'aggiornamento in due modi:

### Metodo 1: Aggiornamento OTA (Over-The-Air) via Web
Se il dispositivo è connesso al Wi-Fi, puoi aggiornarlo comodamente senza cavi direttamente dal tuo browser:
1. **Scarica l'Aggiornamento:** Vai nella sezione [Releases](https://github.com/met989/Aquarium_Touch/releases) e scarica l'ultimo file `.bin` disponibile (es. `esp32_2432S028_v3_0.6.bin`).
2. **Accedi alla Web UI:** Apri l'indirizzo IP del tuo acquario dal browser (es. `http://192.168.1.100`).
3. **Avvia OTA:** Clicca sul pulsante dorato **"🔄 OTA Update"** (in alto).
4. **Carica il Firmware:** Si aprirà la pagina sicura di ElegantOTA. Seleziona il file `.bin` appena scaricato e attendi il caricamento. Al termine, il dispositivo si riavvierà automaticamente con la nuova versione!

### Metodo 2: Aggiornamento Offline via USB (Flasher)
Se il Wi-Fi non è configurato o preferisci l'aggiornamento classico via cavo:
1. **Scarica l'Aggiornamento:** Scarica l'ultimo file `.bin` dalle [Releases](https://github.com/met989/Aquarium_Touch/releases).
2. **Posiziona il File:** Inseriscilo all'interno della cartella `release/` del tuo progetto locale.
3. **Collega lo Schermo:** Collega il display ESP32 al computer tramite cavo USB.
4. **Avvia l'Aggiornamento:** Fai doppio clic sul file **`flash.bat`** (Flasher Intelligente Offline). Lo script:
   - Rileverà automaticamente in modo nativo la porta USB (COM) a cui è collegato lo schermo (senza bisogno di driver aggiuntivi).
   - Ti farà selezionare il file firmware da installare.
   - Caricherà il nuovo sistema operativo in pochi secondi.

> [!TIP]
> **Log delle Operazioni (`operations.log`):** Tutti i file eseguibili `.bat` salvano automaticamente uno storico delle loro esecuzioni nel file `operations.log`, permettendoti di sapere sempre in un batter d'occhio quando hai effettuato l'ultimo aggiornamento!

---

## 🛠 Compilazione & Sviluppo (Per Sviluppatori)

### Requisiti
- **VS Code** con estensione **PlatformIO IDE** (o CLI PlatformIO).
- Scheda ESP32 Dev Module collegata via USB.

### Librerie Dipendenti (`platformio.ini`)
- `bodmer/TFT_eSPI @ ^2.5.43`
- `XPT2046_Touchscreen`
- `OneWire`
- `DallasTemperature` (Arduino-Temperature-Control-Library)
- `bitbank2/AnimatedGIF @ ^1.4.7`

> [!TIP]
> **Libreria Grafica Consigliata:** Sebbene il progetto utilizzi attualmente `TFT_eSPI`, la libreria grafica più consigliata e performante per l'hardware specifico **ESP32-2432S028** è **LovyanGFX**. Nelle future iterazioni del firmware, il motore di rendering potrebbe essere migrato a LovyanGFX per sfruttarne appieno le potenzialità hardware.

### Comandi PlatformIO
- **Compilazione firmware:**
  ```bash
  pio run
  ```
- **Caricamento firmware su ESP32 (Upload):**
  ```bash
  pio run --target upload
  ```
- **Monitor Seriale (115200 baud):**
  ```bash
  pio run --target monitor
  ```

---

## 🔮 Sviluppi Futuri (Roadmap)

Il progetto è in continua evoluzione. Le prossime funzionalità e integrazioni hardware in programma includono:

- **Sensore pH:** Monitoraggio in tempo reale dell'acidità dell'acqua.
- **Sensore Livello Acqua:** Rilevamento del livello di evaporazione o livello critico con avvisi a schermo/web.
- **Attivatore Mangiatoia Automatica:** Integrazione con la programmazione (Schedule) per erogazione cibo agli orari prestabiliti.
- **Gravity: Analog TDS Sensor Meter (Arduino):** Misurazione dei Solidi Totali Disciolti (TDS) in *ppm* (parti per milione), perfetto per monitorare in modo granulare la concentrazione di fertilizzanti in colonna d'acqua e la purezza generale.

---

## ☕ Supporta il Progetto

Se questo firmware ti è stato utile per il tuo acquario o semplicemente apprezzi il lavoro open-source, puoi sostenere lo sviluppo offrendomi un caffè virtuale! ❤️

[![Offrimi un caffè con PayPal](https://img.shields.io/badge/Offrimi_un_caffè-PayPal-00457C.svg?style=for-the-badge&logo=paypal)](https://paypal.me/MatteoSacanna)

---
