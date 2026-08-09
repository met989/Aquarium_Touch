#include "aquarium_server.h"
#include "aquarium_logic.h"
#include "language_manager.h"
#include "config.h"
#include <WiFi.h>
#include <SD.h>

AquariumServer aquariumServer;

extern bool writeWholeConfigFileSafe();

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="it">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Aquarium Master Control Panel</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg-ocean: #0a192f;
      --card-bg: rgba(23, 42, 69, 0.75);
      --card-border: rgba(46, 76, 109, 0.8);
      --cyan-glow: #00f0ff;
      --emerald: #00e676;
      --coral: #ff5252;
      --gold: #ffd700;
      --text-muted: #8892b0;
      --text-light: #e6f1ff;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Outfit', sans-serif; }
    body { background: radial-gradient(circle at top, #0e2447 0%, var(--bg-ocean) 100%); color: var(--text-light); min-height: 100vh; padding: 20px; }
    .container { max-width: 1200px; margin: 0 auto; }
    header { display: flex; justify-content: space-between; align-items: center; padding: 18px 24px; background: var(--card-bg); backdrop-filter: blur(12px); border-radius: 16px; border: 1px solid var(--card-border); margin-bottom: 24px; box-shadow: 0 8px 32px rgba(0,0,0,0.3); flex-wrap: wrap; gap: 12px; }
    .title-group { display: flex; align-items: center; gap: 14px; }
    .title-group h1 { font-size: 1.5rem; font-weight: 700; color: var(--cyan-glow); }
    .header-badges { display: flex; gap: 10px; flex-wrap: wrap; align-items: center; }
    .badge { padding: 6px 14px; border-radius: 20px; font-size: 0.85rem; font-weight: 600; background: rgba(0,230,118,0.15); color: var(--emerald); border: 1px solid var(--emerald); }
    .badge-gold { background: rgba(255,215,0,0.15); color: var(--gold); border-color: var(--gold); }
    .badge-cyan { background: rgba(0,240,255,0.1); color: var(--cyan-glow); border-color: var(--cyan-glow); }
    .section-title { font-size: 1.1rem; font-weight: 700; color: var(--gold); margin: 28px 0 14px; padding-left: 8px; border-left: 3px solid var(--gold); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(340px, 1fr)); gap: 20px; margin-bottom: 10px; }
    .card { background: var(--card-bg); backdrop-filter: blur(12px); border-radius: 16px; border: 1px solid var(--card-border); padding: 22px; transition: border-color 0.2s; }
    .card:hover { border-color: var(--cyan-glow); }
    .card-title { font-size: 1.05rem; font-weight: 600; color: var(--cyan-glow); margin-bottom: 16px; display: flex; align-items: center; gap: 8px; }
    .temp-display { font-size: 2.8rem; font-weight: 700; color: #fff; text-align: center; margin: 6px 0; }
    .btn { width: 100%; padding: 11px; border-radius: 10px; border: none; font-weight: 600; font-size: 0.92rem; cursor: pointer; transition: all 0.2s; display: flex; align-items: center; justify-content: center; gap: 8px; margin-top: 10px; }
    .btn-cyan { background: var(--cyan-glow); color: #0a192f; }
    .btn-cyan:hover { background: #56f5ff; box-shadow: 0 0 15px rgba(0,240,255,0.4); }
    .btn-gold { background: var(--gold); color: #0a192f; }
    .btn-gold:hover { background: #ffe44d; }
    .btn-emerald { background: var(--emerald); color: #0a192f; }
    .btn-emerald:hover { background: #69f0ae; }
    .btn-outline { background: transparent; border: 1px solid var(--card-border); color: var(--text-light); }
    .btn-outline:hover { border-color: var(--cyan-glow); color: var(--cyan-glow); }
    .form-group { margin-bottom: 12px; }
    .form-group label { display: block; font-size: 0.85rem; color: var(--text-muted); margin-bottom: 5px; }
    .form-group input, .form-group select { width: 100%; padding: 9px 12px; border-radius: 8px; background: rgba(10,25,47,0.8); border: 1px solid var(--card-border); color: #fff; font-size: 0.95rem; outline: none; }
    .form-group input:focus, .form-group select:focus { border-color: var(--cyan-glow); }
    textarea { width: 100%; height: 220px; background: rgba(10,25,47,0.9); border: 1px solid var(--card-border); border-radius: 10px; color: var(--cyan-glow); font-family: monospace; padding: 12px; font-size: 0.88rem; resize: vertical; outline: none; }
    .full-width { grid-column: 1 / -1; }
    .flex-row { display: flex; gap: 10px; }
    .flex-row > * { flex: 1; }
    .divider { border: 0; border-top: 1px solid var(--card-border); margin: 14px 0; }
    .stat-row { display: flex; justify-content: space-between; align-items: center; padding: 6px 0; border-bottom: 1px solid rgba(46,76,109,0.4); }
    .stat-row:last-child { border-bottom: none; }
    .stat-label { color: var(--text-muted); font-size: 0.88rem; }
    .stat-value { color: #fff; font-weight: 600; font-size: 0.95rem; }
    .toast { position: fixed; bottom: 20px; right: 20px; padding: 12px 20px; border-radius: 10px; background: var(--emerald); color: #0a192f; font-weight: 600; display: none; box-shadow: 0 5px 20px rgba(0,0,0,0.4); z-index: 1000; }
  </style>
</head>
<body>
<div class="container">
  <header>
    <div class="title-group">
      <span style="font-size:1.8rem">🐠</span>
      <div>
        <h1>Aquarium Master Control Panel</h1>
        <div style="font-size:0.8rem;color:var(--text-muted)">ESP32 Aquarium OS v<span id="fwVersion">--</span> &nbsp;|&nbsp; Lingua: <span id="langNameHeader">--</span></div>
      </div>
    </div>
    <div class="header-badges">
      <div class="badge badge-cyan">IP: <span id="ipAddr">...</span></div>
      <div class="badge badge-gold" id="wifiStatusBadge">Wi-Fi: --</div>
    </div>
  </header>

  <!-- STATO LIVE -->
  <div class="section-title">📊 Stato Live</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">🌡️ Temperatura Acqua</div>
      <div id="tempVal" class="temp-display">--.- °C</div>
      <div style="text-align:center;color:var(--emerald);font-weight:600;margin-bottom:12px" id="tempStatus">--</div>
      <div class="stat-row"><span class="stat-label">Min Ottimale</span><span class="stat-value" id="statTempMin">-- °C</span></div>
      <div class="stat-row"><span class="stat-label">Max Ottimale</span><span class="stat-value" id="statTempMax">-- °C</span></div>
      <div class="stat-row"><span class="stat-label">Offset Sensore</span><span class="stat-value" id="statTempOff">-- °C</span></div>
    </div>
    <div class="card">
      <div class="card-title">💡 Luce & Timer</div>
      <div style="font-size:1.3rem;font-weight:600;text-align:center;margin:8px 0" id="lightStatus">STATO: --</div>
      <div class="stat-row"><span class="stat-label">Modalità</span><span class="stat-value" id="statMode">--</span></div>
      <div class="stat-row"><span class="stat-label">Accensione</span><span class="stat-value" id="statOnTime">--:--</span></div>
      <div class="stat-row"><span class="stat-label">Spegnimento</span><span class="stat-value" id="statOffTime">--:--</span></div>
      <div class="stat-row"><span class="stat-label">Polarità Relè</span><span class="stat-value" id="statRelay">--</span></div>
      <div class="flex-row" style="margin-top:14px">
        <button class="btn btn-gold" onclick="toggleLight()">⚡ Commuta Luce</button>
        <button class="btn btn-outline" onclick="toggleAuto()">🔄 Auto/Manuale</button>
      </div>
    </div>
    <div class="card">
      <div class="card-title">⏰ Orologio & Data</div>
      <div style="font-size:2rem;font-weight:700;text-align:center;color:var(--cyan-glow);margin:10px 0" id="liveTime">00:00:00</div>
      <div style="text-align:center;color:var(--text-muted);margin-bottom:12px" id="liveDate">--/--/----</div>
      <div class="stat-row"><span class="stat-label">Timezone</span><span class="stat-value" id="statTz">--</span></div>
      <div class="stat-row"><span class="stat-label">Formato ora</span><span class="stat-value" id="statFmtHour">--</span></div>
      <div class="stat-row"><span class="stat-label">Formato data</span><span class="stat-value" id="statFmtDate">--</span></div>
      <div class="stat-row"><span class="stat-label">NTP</span><span class="stat-value" id="statNtp1">--</span></div>
      <button class="btn btn-outline" onclick="triggerNtpSync()" style="margin-top:12px">🌐 Sincronizza NTP Ora</button>
    </div>
  </div>

  <!-- TEMPERATURA -->
  <div class="section-title">🌡️ Impostazioni Temperatura</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">🌡️ Soglie Temperatura</div>
      <div class="form-group"><label>Minima Ottimale (°C):</label><input type="number" step="0.1" id="cfgTempMin"></div>
      <div class="form-group"><label>Massima Ottimale (°C):</label><input type="number" step="0.1" id="cfgTempMax"></div>
      <div class="form-group"><label>Offset Calibrazione Sensore (°C):</label><input type="number" step="0.1" id="cfgTempOffset"></div>
      <button class="btn btn-cyan" onclick="saveTempSettings()">💾 Salva Soglie Temperatura</button>
    </div>
  </div>

  <!-- RELÈ -->
  <div class="section-title">💡 Luce & Programmazione Relè</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">💡 Timer & Polarità Relè</div>
      <div class="form-group"><label>Polarità Relè (GPIO 17):</label><select id="cfgRelayInv"><option value="0">Active High (Normale)</option><option value="1">Active Low (Invertito)</option></select></div>
      <div class="flex-row">
        <div class="form-group"><label>Ora Accensione (ON):</label><input type="time" id="timeOn"></div>
        <div class="form-group"><label>Ora Spegnimento (OFF):</label><input type="time" id="timeOff"></div>
      </div>
      <button class="btn btn-cyan" onclick="saveRelaySettings()">💾 Salva Orario & Relè</button>
    </div>
  </div>

  <!-- DATA & NTP -->
  <div class="section-title">⏰ Data, Ora & NTP</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">⏰ Configurazione Orario</div>
      <div class="form-group"><label>Timezone Offset (es. 0, +1, -1, +2):</label><input type="text" id="cfgTimezone"></div>
      <div class="form-group"><label>Formato Ora:</label><select id="cfgFormatHour"><option value="24">24 ore (es. 14:30)</option><option value="12">12 ore (es. 2:30 PM)</option></select></div>
      <div class="form-group"><label>Formato Data:</label><select id="cfgDateFormat"><option value="0">GG/MM/AAAA</option><option value="1">MM/GG/AAAA</option><option value="2">AAAA/MM/GG</option></select></div>
      <div class="form-group"><label>Server NTP 1:</label><input type="text" id="cfgNtp1"></div>
      <div class="form-group"><label>Server NTP 2:</label><input type="text" id="cfgNtp2"></div>
      <div class="flex-row">
        <button class="btn btn-outline" onclick="triggerNtpSync()">🌐 Sincronizza</button>
        <button class="btn btn-cyan" onclick="saveTimeSettings()">💾 Salva Data & NTP</button>
      </div>
    </div>
  </div>

  <!-- WI-FI -->
  <div class="section-title">📶 Rete Wi-Fi</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">📶 Credenziali Wi-Fi</div>
      <div class="form-group"><label>SSID Wi-Fi:</label><input type="text" id="cfgWifiSsid"></div>
      <div class="form-group"><label>Password Wi-Fi:</label><input type="password" id="cfgWifiPass" placeholder="(invariata se vuoto)"></div>
      <button class="btn btn-emerald" onclick="saveWifiSettings()">📶 Salva & Connetti Wi-Fi</button>
    </div>
    <div class="card">
      <div class="card-title">🔧 Rete Avanzata (IP Statico)</div>
      <div style="font-size:0.82rem;color:var(--text-muted);margin-bottom:12px">Lascia 0.0.0.0 per DHCP automatico</div>
      <div class="form-group"><label>IP Statico:</label><input type="text" id="cfgWifiIp"></div>
      <div class="form-group"><label>Subnet Mask:</label><input type="text" id="cfgWifiSubnet"></div>
      <div class="form-group"><label>Gateway:</label><input type="text" id="cfgWifiGateway"></div>
      <div class="flex-row">
        <div class="form-group"><label>DNS 1:</label><input type="text" id="cfgWifiDns1"></div>
        <div class="form-group"><label>DNS 2:</label><input type="text" id="cfgWifiDns2"></div>
      </div>
      <button class="btn btn-cyan" onclick="saveNetworkSettings()">💾 Salva Rete Avanzata</button>
    </div>
  </div>

  <!-- MQTT -->
  <div class="section-title">📡 MQTT</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">📡 Configurazione MQTT</div>
      <div class="form-group"><label>Server MQTT:</label><input type="text" id="cfgMqttServer"></div>
      <div class="form-group"><label>Porta MQTT:</label><input type="number" id="cfgMqttPort" min="1" max="65535"></div>
      <div class="form-group"><label>Utente MQTT:</label><input type="text" id="cfgMqttUser"></div>
      <div class="form-group"><label>Password MQTT:</label><input type="password" id="cfgMqttPass" placeholder="(invariata se vuoto)"></div>
      <button class="btn btn-cyan" onclick="saveMqttSettings()">💾 Salva Impostazioni MQTT</button>
    </div>
  </div>

  <!-- SISTEMA -->
  <div class="section-title">⚙️ Sistema & Risparmio Energia</div>
  <div class="grid">
    <div class="card">
      <div class="card-title">⚙️ Impostazioni di Sistema</div>
      <div class="form-group"><label>Lingua Interfaccia:</label><select id="cfgLangFile"><option value="english.lng">English</option><option value="italian.lng">Italiano</option></select></div>
      <button class="btn btn-gold" onclick="saveLanguageSetting()">🌐 Cambia Lingua (Riavvia ESP32)</button>
      <hr class="divider">
      <div class="form-group"><label>Modalità Debug Seriale:</label><select id="cfgDebug"><option value="false">Disattivato</option><option value="true">Attivato</option></select></div>
      <div class="form-group"><label>Rotazione Schermo (screen_mode 0-3):</label><select id="cfgScreenMode"><option value="0">Modalità 0</option><option value="1">Modalità 1</option><option value="2">Modalità 2</option><option value="3">Modalità 3</option></select></div>
      <button class="btn btn-outline" onclick="saveSystemSettings()">💾 Salva Impostazioni Sistema</button>
    </div>
    <div class="card">
      <div class="card-title">🌙 Risparmio Energia (Screensaver)</div>
      <div style="font-size:0.85rem;color:var(--text-muted);margin-bottom:14px">Lo schermo LCD si spegne dopo il periodo di inattività. Tocca il display per riattivarlo.</div>
      <div class="form-group"><label>Spegni schermo dopo:</label>
        <select id="cfgScreensaver">
          <option value="0">MAI (Disattivato)</option>
          <option value="30">30 secondi</option>
          <option value="60">1 minuto</option>
          <option value="120">2 minuti</option>
          <option value="180">3 minuti</option>
          <option value="240">4 minuti</option>
          <option value="300">5 minuti</option>
          <option value="360">6 minuti</option>
          <option value="420">7 minuti</option>
          <option value="480">8 minuti</option>
          <option value="540">9 minuti</option>
          <option value="600">10 minuti</option>
          <option value="900">15 minuti</option>
          <option value="1200">20 minuti</option>
          <option value="1800">30 minuti</option>
          <option value="2700">45 minuti</option>
          <option value="3600">60 minuti</option>
        </select>
      </div>
      <button class="btn btn-cyan" onclick="saveScreensaverSettings()">💾 Salva Risparmio Energia</button>
    </div>
  </div>

  <!-- EDITOR RAW -->
  <div class="section-title">📁 Editor Diretto config.cfg</div>
  <div class="grid">
    <div class="card full-width">
      <div class="card-title">📁 File di Configurazione SD (/config.cfg)</div>
      <textarea id="rawConfigText" placeholder="Caricamento file config.cfg su SD in corso..."></textarea>
      <div class="flex-row" style="margin-top:10px">
        <button class="btn btn-outline" onclick="loadRawConfig()">🔄 Ricarica da SD</button>
        <button class="btn btn-cyan" onclick="saveRawConfig()">💾 Salva Modifiche Dirette su SD</button>
      </div>
    </div>
  </div>

</div>
<div id="toast" class="toast">Salvato!</div>
<script>
  function showToast(msg) {
    const t = document.getElementById('toast');
    t.innerText = msg || 'Salvato!';
    t.style.display = 'block';
    setTimeout(() => { t.style.display = 'none'; }, 3000);
  }
  function pad(n) { return (n < 10 ? '0' : '') + n; }
  const DATE_FMTS = ['GG/MM/AAAA','MM/GG/AAAA','AAAA/MM/GG'];

  async function updateStatus() {
    try {
      const res = await fetch('/api/status');
      const d = await res.json();
      document.getElementById('ipAddr').innerText = d.ip || '--';
      document.getElementById('langNameHeader').innerText = d.lang_name || '--';
      document.getElementById('fwVersion').innerText = d.version || '--';
      document.getElementById('wifiStatusBadge').innerText = 'Wi-Fi: ' + (d.wifi_ssid || '--');
      document.getElementById('tempVal').innerText = parseFloat(d.temp).toFixed(1) + ' C';
      document.getElementById('tempStatus').innerText = d.temp_status || '--';
      document.getElementById('statTempMin').innerText = parseFloat(d.temp_min).toFixed(1) + ' C';
      document.getElementById('statTempMax').innerText = parseFloat(d.temp_max).toFixed(1) + ' C';
      document.getElementById('statTempOff').innerText = parseFloat(d.temp_offset).toFixed(1) + ' C';
      const lightEl = document.getElementById('lightStatus');
      lightEl.innerText = d.light_status || '--';
      lightEl.style.color = d.light_on ? 'var(--gold)' : 'var(--text-muted)';
      document.getElementById('statMode').innerText = d.auto_sched ? 'AUTO (Timer)' : 'MANUALE';
      document.getElementById('statOnTime').innerText = pad(d.light_on_h) + ':' + pad(d.light_on_m);
      document.getElementById('statOffTime').innerText = pad(d.light_off_h) + ':' + pad(d.light_off_m);
      document.getElementById('statRelay').innerText = d.relay_inv ? 'Active Low' : 'Active High';
      document.getElementById('liveTime').innerText = d.time || '00:00:00';
      document.getElementById('liveDate').innerText = d.date || '--';
      document.getElementById('statTz').innerText = 'UTC' + (d.timezone >= 0 ? '+' : '') + d.timezone;
      document.getElementById('statFmtHour').innerText = d.format_hour + 'h';
      document.getElementById('statFmtDate').innerText = DATE_FMTS[d.date_format] || '--';
      document.getElementById('statNtp1').innerText = d.ntp_server1 || '--';
      const active = document.activeElement;
      if (!active || active.tagName !== 'INPUT') {
        document.getElementById('cfgTempMin').value = parseFloat(d.temp_min).toFixed(1);
        document.getElementById('cfgTempMax').value = parseFloat(d.temp_max).toFixed(1);
        document.getElementById('cfgTempOffset').value = parseFloat(d.temp_offset).toFixed(1);
        document.getElementById('cfgRelayInv').value = d.relay_inv ? '1' : '0';
        document.getElementById('timeOn').value = pad(d.light_on_h) + ':' + pad(d.light_on_m);
        document.getElementById('timeOff').value = pad(d.light_off_h) + ':' + pad(d.light_off_m);
        document.getElementById('cfgTimezone').value = d.timezone;
        document.getElementById('cfgFormatHour').value = d.format_hour;
        document.getElementById('cfgDateFormat').value = d.date_format;
        document.getElementById('cfgNtp1').value = d.ntp_server1;
        document.getElementById('cfgNtp2').value = d.ntp_server2;
        document.getElementById('cfgWifiSsid').value = d.wifi_ssid;
        document.getElementById('cfgWifiIp').value = d.wifi_ip_static || '0.0.0.0';
        document.getElementById('cfgWifiSubnet').value = d.wifi_subnet || '0.0.0.0';
        document.getElementById('cfgWifiGateway').value = d.wifi_gateway || '0.0.0.0';
        document.getElementById('cfgWifiDns1').value = d.wifi_dns1 || '0.0.0.0';
        document.getElementById('cfgWifiDns2').value = d.wifi_dns2 || '0.0.0.0';
        document.getElementById('cfgMqttServer').value = d.mqtt_server || '';
        document.getElementById('cfgMqttPort').value = d.mqtt_port || 1883;
        document.getElementById('cfgMqttUser').value = d.mqtt_user || '';
        document.getElementById('cfgLangFile').value = d.lang_file || 'english.lng';
        document.getElementById('cfgDebug').value = d.debug ? 'true' : 'false';
        document.getElementById('cfgScreenMode').value = d.screen_mode || 1;
        document.getElementById('cfgScreensaver').value = d.screensaver_t || 0;
      }
    } catch(e) { console.error('API Error:', e); }
  }

  async function toggleLight() { await fetch('/api/light/toggle',{method:'POST'}); updateStatus(); }
  async function toggleAuto()  { await fetch('/api/auto/toggle', {method:'POST'}); updateStatus(); }

  async function saveTempSettings() {
    const b = 'min_t='+document.getElementById('cfgTempMin').value+'&max_t='+document.getElementById('cfgTempMax').value+'&offset='+document.getElementById('cfgTempOffset').value;
    await fetch('/api/settings/temp',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Soglie Temperatura Salvate!'); updateStatus();
  }
  async function saveRelaySettings() {
    const inv=document.getElementById('cfgRelayInv').value;
    const on=document.getElementById('timeOn').value.split(':');
    const off=document.getElementById('timeOff').value.split(':');
    const b='relay_inv='+inv+'&on_h='+on[0]+'&on_m='+on[1]+'&off_h='+off[0]+'&off_m='+off[1];
    await fetch('/api/settings/relay',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Impostazioni Rele Salvate!'); updateStatus();
  }
  async function saveTimeSettings() {
    const b='timezone='+encodeURIComponent(document.getElementById('cfgTimezone').value)+'&format_hour='+document.getElementById('cfgFormatHour').value+'&date_format='+document.getElementById('cfgDateFormat').value+'&ntp1='+encodeURIComponent(document.getElementById('cfgNtp1').value)+'&ntp2='+encodeURIComponent(document.getElementById('cfgNtp2').value);
    await fetch('/api/settings/time',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Data, Ora & NTP Salvati!'); updateStatus();
  }
  async function triggerNtpSync() { await fetch('/api/ntp/sync',{method:'POST'}); showToast('Sincronizzazione NTP Avviata!'); updateStatus(); }
  async function saveWifiSettings() {
    const b='ssid='+encodeURIComponent(document.getElementById('cfgWifiSsid').value)+'&pass='+encodeURIComponent(document.getElementById('cfgWifiPass').value);
    await fetch('/api/settings/wifi',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Credenziali Wi-Fi Salvate!'); updateStatus();
  }
  async function saveNetworkSettings() {
    const b='ip='+encodeURIComponent(document.getElementById('cfgWifiIp').value)+'&subnet='+encodeURIComponent(document.getElementById('cfgWifiSubnet').value)+'&gateway='+encodeURIComponent(document.getElementById('cfgWifiGateway').value)+'&dns1='+encodeURIComponent(document.getElementById('cfgWifiDns1').value)+'&dns2='+encodeURIComponent(document.getElementById('cfgWifiDns2').value);
    await fetch('/api/settings/network',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Rete Avanzata Salvata!'); updateStatus();
  }
  async function saveMqttSettings() {
    const b='server='+encodeURIComponent(document.getElementById('cfgMqttServer').value)+'&port='+document.getElementById('cfgMqttPort').value+'&user='+encodeURIComponent(document.getElementById('cfgMqttUser').value)+'&pass='+encodeURIComponent(document.getElementById('cfgMqttPass').value);
    await fetch('/api/settings/mqtt',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Impostazioni MQTT Salvate!'); updateStatus();
  }
  async function saveLanguageSetting() {
    showToast('Cambio Lingua! Riavvio ESP32...');
    await fetch('/api/settings/language',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'lang_file='+encodeURIComponent(document.getElementById('cfgLangFile').value)});
    setTimeout(()=>{location.reload();},3500);
  }
  async function saveSystemSettings() {
    const b='debug='+document.getElementById('cfgDebug').value+'&screen_mode='+document.getElementById('cfgScreenMode').value;
    await fetch('/api/settings/system',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
    showToast('Impostazioni Sistema Salvate!'); updateStatus();
  }
  async function saveScreensaverSettings() {
    await fetch('/api/settings/screensaver',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'screensaver_t='+document.getElementById('cfgScreensaver').value});
    showToast('Risparmio Energia Salvato!'); updateStatus();
  }
  async function loadRawConfig() {
    try { document.getElementById('rawConfigText').value = await (await fetch('/api/config/raw')).text(); }
    catch(e) { console.error('Config load error'); }
  }
  async function saveRawConfig() {
    const res = await fetch('/api/config/raw',{method:'POST',headers:{'Content-Type':'text/plain'},body:document.getElementById('rawConfigText').value});
    if(res.ok){showToast('Config.cfg Sovrascritto!'); updateStatus();}
  }
  updateStatus();
  loadRawConfig();
  setInterval(updateStatus,3000);
</script>
</body>
</html>
)rawliteral";

AquariumServer::AquariumServer() : m_server(80), m_started(false) {}

void AquariumServer::init() {
  if (m_started) return;
  setupRoutes();
  m_server.begin();
  m_started = true;
  Serial.println("[WEB] Web Server avviato su porta 80");
}

void AquariumServer::update() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!m_started) init();
    m_server.handleClient();
  }
}

void AquariumServer::setupRoutes() {
  m_server.on("/",                         HTTP_GET,  [this]() { handleRoot(); });
  m_server.on("/api/status",               HTTP_GET,  [this]() { handleApiStatus(); });
  m_server.on("/api/light/toggle",         HTTP_POST, [this]() { handleApiToggleLight(); });
  m_server.on("/api/auto/toggle",          HTTP_POST, [this]() { handleApiToggleAuto(); });
  m_server.on("/api/schedule",             HTTP_POST, [this]() { handleApiSetSchedule(); });
  m_server.on("/api/settings/temp",        HTTP_POST, [this]() { handleApiSetTempSettings(); });
  m_server.on("/api/settings/time",        HTTP_POST, [this]() { handleApiSetTimeSettings(); });
  m_server.on("/api/ntp/sync",             HTTP_POST, [this]() { handleApiNtpSync(); });
  m_server.on("/api/settings/relay",       HTTP_POST, [this]() { handleApiSetRelaySettings(); });
  m_server.on("/api/settings/wifi",        HTTP_POST, [this]() { handleApiSetWifiSettings(); });
  m_server.on("/api/settings/network",     HTTP_POST, [this]() { handleApiSetNetworkSettings(); });
  m_server.on("/api/settings/mqtt",        HTTP_POST, [this]() { handleApiSetMqttSettings(); });
  m_server.on("/api/settings/system",      HTTP_POST, [this]() { handleApiSetSystemSettings(); });
  m_server.on("/api/settings/screensaver", HTTP_POST, [this]() { handleApiSetScreensaver(); });
  m_server.on("/api/settings/language",    HTTP_POST, [this]() { handleApiSetLanguage(); });
  m_server.on("/api/config/raw",           HTTP_GET,  [this]() { handleApiConfigRawGet(); });
  m_server.on("/api/config/raw",           HTTP_POST, [this]() { handleApiConfigRawPost(); });
}

// AppConfig is defined in config.h (included via aquarium_logic.h)
extern AppConfig cfg;

void AquariumServer::handleRoot() {
  m_server.send_P(200, PSTR("text/html; charset=utf-8"), INDEX_HTML);
}

// Sanitize a String for JSON: escape backslash and double-quote
static String jStr(const String& s) {
  String out = s;
  out.replace("\\", "\\\\");
  out.replace("\"", "\\\"");
  out.replace("\n", "");
  out.replace("\r", "");
  return out;
}

// Convert float to JSON — emits null for NaN/Inf to keep JSON valid
static String jFloat(float v, int decimals = 1) {
  if (isnan(v) || isinf(v)) return "null";
  return String(v, decimals);
}

void AquariumServer::handleApiStatus() {
  const AquariumConfig& aq = aquarium.getConfig();

  int h, m, s;
  aquarium.getTime(h, m, s);
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", h, m, s);

  char dateBuf[16];
  int day, mon, year;
  aquarium.getDate(day, mon, year);
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", day, mon, year);

  String tempStatusStr = (aquarium.getTempStatus() == TEMP_OPTIMAL)
    ? langManager.getText("MSG_OPTIMAL", "OPTIMAL")
    : (aquarium.getTempStatus() == TEMP_TOO_COLD
       ? langManager.getText("MSG_COLD", "TOO COLD")
       : langManager.getText("MSG_HOT",  "TOO HOT"));

  String lightStatusStr = aquarium.isLightOn()
    ? langManager.getText("MSG_LIGHT_ON",  "LIGHT ON")
    : langManager.getText("MSG_LIGHT_OFF", "LIGHT OFF");

  String json = "{";
  json += "\"temp\":"             + jFloat(aquarium.getTemperature()) + ",";
  json += "\"temp_min\":"         + jFloat(aq.targetTempMin) + ",";
  json += "\"temp_max\":"         + jFloat(aq.targetTempMax) + ",";
  json += "\"temp_offset\":"      + jFloat(aq.tempOffset) + ",";
  json += "\"temp_status\":\""    + jStr(tempStatusStr)  + "\",";
  json += "\"light_on\":"         + String(aquarium.isLightOn() ? "true" : "false") + ",";
  json += "\"light_status\":\""   + jStr(lightStatusStr) + "\",";
  json += "\"auto_sched\":"       + String(aquarium.isAutoSchedule() ? "true" : "false") + ",";
  json += "\"light_on_h\":"       + String(aq.lightOnHour) + ",";
  json += "\"light_on_m\":"       + String(aq.lightOnMin)  + ",";
  json += "\"light_off_h\":"      + String(aq.lightOffHour) + ",";
  json += "\"light_off_m\":"      + String(aq.lightOffMin)  + ",";
  json += "\"relay_inv\":"        + String(aquarium.isRelayInverted() ? "true" : "false") + ",";
  json += "\"date_format\":"      + String(aq.dateFormat) + ",";
  json += "\"screensaver_t\":"    + String(aq.screensaverTime) + ",";
  json += "\"timezone\":\""       + jStr(cfg.timezone)    + "\",";
  json += "\"format_hour\":"      + String(cfg.formatHour) + ",";
  json += "\"ntp_server1\":\""    + jStr(cfg.ntpServer1)  + "\",";
  json += "\"ntp_server2\":\""    + jStr(cfg.ntpServer2)  + "\",";
  json += "\"wifi_ssid\":\""      + jStr(cfg.wifiSsid)    + "\",";
  json += "\"wifi_ip_static\":\"" + jStr(cfg.wifiIpStatic) + "\",";
  json += "\"wifi_subnet\":\""    + jStr(cfg.wifiSubnet)   + "\",";
  json += "\"wifi_gateway\":\""   + jStr(cfg.wifiGateway)  + "\",";
  json += "\"wifi_dns1\":\""      + jStr(cfg.wifiDns1)     + "\",";
  json += "\"wifi_dns2\":\""      + jStr(cfg.wifiDns2)     + "\",";
  json += "\"mqtt_server\":\""    + jStr(cfg.mqttServer)   + "\",";
  json += "\"mqtt_port\":"        + String(cfg.mqttPort) + ",";
  json += "\"mqtt_user\":\""      + jStr(cfg.mqttUser)     + "\",";
  json += "\"debug\":"            + String(cfg.debug ? "true" : "false") + ",";
  json += "\"screen_mode\":"      + String(cfg.screenMode) + ",";
  json += "\"lang_file\":\""      + jStr(langManager.getActiveLanguageFile()) + "\",";
  json += "\"lang_name\":\""      + jStr(langManager.getActiveLanguageName()) + "\",";
  json += "\"time\":\""           + String(timeBuf) + "\",";
  json += "\"date\":\""           + String(dateBuf) + "\",";
  json += "\"ip\":\""             + WiFi.localIP().toString() + "\",";
  json += "\"version\":\""        + jStr(String(AQUARIUM_OS_VERSION)) + "\"";
  json += "}";

  Serial.printf("[API] /api/status JSON len=%d\n", json.length());
  m_server.send(200, "application/json", json);
}


void AquariumServer::handleApiToggleLight() {
  aquarium.toggleLight();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiToggleAuto() {
  aquarium.setAutoSchedule(!aquarium.isAutoSchedule());
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetSchedule() {
  if (m_server.hasArg("on_h") && m_server.hasArg("on_m") && m_server.hasArg("off_h") && m_server.hasArg("off_m")) {
    aquarium.setScheduleOn(m_server.arg("on_h").toInt(), m_server.arg("on_m").toInt());
    aquarium.setScheduleOff(m_server.arg("off_h").toInt(), m_server.arg("off_m").toInt());
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetTempSettings() {
  if (m_server.hasArg("min_t") && m_server.hasArg("max_t") && m_server.hasArg("offset")) {
    aquarium.setTargetTemp(m_server.arg("min_t").toFloat(), m_server.arg("max_t").toFloat());
    aquarium.setTempOffset(m_server.arg("offset").toFloat());
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetTimeSettings() {
  if (m_server.hasArg("timezone"))    { String tz = m_server.arg("timezone"); tz.trim(); if (tz.length() > 0) cfg.timezone = tz; }
  if (m_server.hasArg("format_hour")) { cfg.formatHour = m_server.arg("format_hour").toInt(); }
  if (m_server.hasArg("date_format")) { aquarium.setDateFormat(m_server.arg("date_format").toInt()); }
  if (m_server.hasArg("ntp1"))        { cfg.ntpServer1 = m_server.arg("ntp1"); }
  if (m_server.hasArg("ntp2"))        { cfg.ntpServer2 = m_server.arg("ntp2"); }
  aquarium.syncNTP();
  writeWholeConfigFileSafe();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiNtpSync() {
  aquarium.syncNTP();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetRelaySettings() {
  if (m_server.hasArg("relay_inv")) {
    bool inv = (m_server.arg("relay_inv") == "1" || m_server.arg("relay_inv").equalsIgnoreCase("true"));
    if (inv != aquarium.isRelayInverted()) aquarium.toggleRelayInverted();
  }
  if (m_server.hasArg("on_h") && m_server.hasArg("on_m") && m_server.hasArg("off_h") && m_server.hasArg("off_m")) {
    aquarium.setScheduleOn(m_server.arg("on_h").toInt(), m_server.arg("on_m").toInt());
    aquarium.setScheduleOff(m_server.arg("off_h").toInt(), m_server.arg("off_m").toInt());
  }
  writeWholeConfigFileSafe();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetWifiSettings() {
  if (m_server.hasArg("ssid")) {
    aquarium.connectWifiSSID(m_server.arg("ssid"), m_server.hasArg("pass") ? m_server.arg("pass") : "");
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetNetworkSettings() {
  if (m_server.hasArg("ip"))      cfg.wifiIpStatic = m_server.arg("ip");
  if (m_server.hasArg("subnet"))  cfg.wifiSubnet   = m_server.arg("subnet");
  if (m_server.hasArg("gateway")) cfg.wifiGateway  = m_server.arg("gateway");
  if (m_server.hasArg("dns1"))    cfg.wifiDns1     = m_server.arg("dns1");
  if (m_server.hasArg("dns2"))    cfg.wifiDns2     = m_server.arg("dns2");
  writeWholeConfigFileSafe();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetMqttSettings() {
  if (m_server.hasArg("server")) cfg.mqttServer = m_server.arg("server");
  if (m_server.hasArg("port"))   cfg.mqttPort   = m_server.arg("port").toInt();
  if (m_server.hasArg("user"))   cfg.mqttUser   = m_server.arg("user");
  if (m_server.hasArg("pass") && m_server.arg("pass").length() > 0)
    cfg.mqttPassword = m_server.arg("pass");
  writeWholeConfigFileSafe();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetSystemSettings() {
  if (m_server.hasArg("debug"))       cfg.debug      = (m_server.arg("debug") == "true");
  if (m_server.hasArg("screen_mode")) cfg.screenMode = (uint8_t)m_server.arg("screen_mode").toInt();
  writeWholeConfigFileSafe();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetScreensaver() {
  if (m_server.hasArg("screensaver_t")) {
    uint16_t val = (uint16_t)constrain(m_server.arg("screensaver_t").toInt(), 0, 3600);
    aquarium.setScreensaverTime(val);
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetLanguage() {
  if (m_server.hasArg("lang_file")) {
    aquarium.setLanguageFile(m_server.arg("lang_file"));
    m_server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(300);
    ESP.restart();
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiConfigRawGet() {
  if (!SD.exists(CONFIG_PATH)) { m_server.send(404, "text/plain", "# Errore: file config.cfg non trovato"); return; }
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) { m_server.send(500, "text/plain", "# Errore lettura SD"); return; }
  m_server.send(200, "text/plain", f.readString());
  f.close();
}

void AquariumServer::handleApiConfigRawPost() {
  if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Corpo vuoto"); return; }
  String newContent = m_server.arg("plain");
  if (SD.exists(CONFIG_TMP_PATH)) SD.remove(CONFIG_TMP_PATH);
  File tmp = SD.open(CONFIG_TMP_PATH, FILE_WRITE);
  if (!tmp) { m_server.send(500, "text/plain", "Errore apertura tmp"); return; }
  tmp.print(newContent); tmp.flush(); tmp.close();
  if (SD.exists(CONFIG_BAK_PATH)) SD.remove(CONFIG_BAK_PATH);
  if (SD.exists(CONFIG_PATH)) SD.rename(CONFIG_PATH, CONFIG_BAK_PATH);
  if (SD.rename(CONFIG_TMP_PATH, CONFIG_PATH)) {
    if (SD.exists(CONFIG_BAK_PATH)) SD.remove(CONFIG_BAK_PATH);
    bool created = false, updated = false;
    loadConfigFromSD(created, updated);
    m_server.send(200, "text/plain", "OK");
  } else {
    m_server.send(500, "text/plain", "Errore sovrascrittura config.cfg");
  }
}
