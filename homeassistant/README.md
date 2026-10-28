# 🐠 Integrazione & Dashboard Home Assistant per Aquarium Touch

Questa cartella contiene tutti i file pronti per integrare e visualizzare al meglio il tuo progetto **Aquarium Touch** in **Home Assistant**, con un'interfaccia moderna in stile *Ocean Dark & Glassmorphism* perfettamente coordinata con la Web UI e il display CYD.

---

## 📦 File Inclusi

1. **[`aquarium_dashboard.yaml`](file:///c:/Antigravity/Aquarium_Touch/homeassistant/aquarium_dashboard.yaml)**: Codice YAML completo della Dashboard Lovelace con tutte le card, metriche, telemetria in tempo reale, grafici e controlli.
2. **[`mqtt_entities.yaml`](file:///c:/Antigravity/Aquarium_Touch/homeassistant/mqtt_entities.yaml)**: Configurazione opzionale per creare nativamente in Home Assistant lo switch della plafoniera (`switch.aquarium_light`) e il pulsante di riavvio (`button.aquarium_reboot`).

---

## 🧩 Plugin HACS Raccomandati

Per la resa estetica ottimale della dashboard, installa tramite **HACS** (Home Assistant Community Store) i seguenti componenti frontend:

| Plugin | Descrizione | Perché è utile |
| :--- | :--- | :--- |
| **Mushroom Cards** (`custom:mushroom-*`) | Schede minimaliste e moderne | Card con icone dinamiche, chip interattivi e badge di stato |
| **ApexCharts Card** (`custom:apexcharts-card`) | Grafici avanzati | Andamento 24h di Temperatura e pH con gradiente ed assi personalizzati |
| **card-mod** (`card_mod`) | Styling CSS per Lovelace | Effetto Glassmorphism, bordi neon ciano (`#00f0ff`) e trasparenze oceaniche |
| **layout-card** (`custom:grid-layout`) | Gestione griglie reattive | Adattamento perfetto su smartphone, tablet e PC desktop |

*(Nota: anche se non installi tutti i plugin, le schede standard di Home Assistant mostreranno comunque i dati, ma l'aspetto grafico sarà quello predefinito).*

---

## 🚀 Come Installare la Dashboard

### Metodo 1: Creare una Nuova Dashboard
1. Vai su Home Assistant -> **Impostazioni** -> **Dashboard** -> **Aggiungi Dashboard**.
2. Scegli **Nuova Dashboard da zero**, dai un nome (es. `Acquario`) e icona `mdi:fish`.
3. Apri la nuova dashboard, clicca sui tre puntini in alto a destra e seleziona **Modifica Dashboard**.
4. Clicca nuovamente sui tre puntini -> **Editor di configurazione non elaborata** (Raw configuration editor).
5. Copia e incolla il contenuto di [`aquarium_dashboard.yaml`](file:///c:/Antigravity/Aquarium_Touch/homeassistant/aquarium_dashboard.yaml) e salva.

### Metodo 2: Aggiungere come Nuova Scheda/Tab in una Dashboard Esistente
1. Entra nella tua Dashboard preferita -> **Modifica Dashboard**.
2. Clicca sul tasto `+` per creare una nuova vista.
3. Clicca sui tre puntini -> **Modifica vista in YAML** o copia i blocchi cards desiderati.

---

## 💡 Integrazione Comandi MQTT (Switch & Reboot)

Nel tuo file `configuration.yaml` di Home Assistant, puoi incollare il contenuto di [`mqtt_entities.yaml`](file:///c:/Antigravity/Aquarium_Touch/homeassistant/mqtt_entities.yaml):

```yaml
mqtt:
  switch:
    - name: "Aquarium Light"
      unique_id: aquarium_light_switch
      state_topic: "aquarium/data"
      value_template: "{{ value_json.lightOn }}"
      command_topic: "aquarium/command/light"
      payload_on: "ON"
      payload_off: "OFF"
      state_on: "ON"
      state_off: "OFF"
      icon: mdi:fishbowl-outline

  button:
    - name: "Aquarium Reboot"
      unique_id: aquarium_reboot_button
      command_topic: "aquarium/command/reboot"
      payload_press: "REBOOT"
      icon: mdi:restart
      device_class: restart
```

Dopo aver salvato, riavvia Home Assistant o ricarica le entità MQTT.
