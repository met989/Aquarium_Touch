$ErrorActionPreference = "Stop"
$rootDir = Split-Path $PSScriptRoot -Parent; $toolsDir = Join-Path $rootDir "tools"
$esptoolPath = Join-Path $toolsDir "esptool.exe"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "      Aquarium OS Touch - Flasher       " -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

if (-not (Test-Path $esptoolPath)) {
    Write-Host "Errore: esptool.exe non trovato nella cartella tools!" -ForegroundColor Red
    Write-Host "Assicurati di aver scaricato il progetto completo." -ForegroundColor Red
    exit
}

# 2. Select firmware
$releaseDir = Join-Path $rootDir "release"
if (-not (Test-Path $releaseDir)) {
    Write-Host "Cartella 'release' non trovata. Nessun firmware da flashare." -ForegroundColor Red
    exit
}

$bins = Get-ChildItem -Path $releaseDir -Filter "*.bin" | Sort-Object LastWriteTime -Descending
if ($bins.Count -eq 0) {
    Write-Host "Nessun file .bin trovato nella cartella release." -ForegroundColor Red
    exit
}

Write-Host "File Firmware disponibili (dal piu' recente):" -ForegroundColor Yellow
for ($i = 0; $i -lt $bins.Count; $i++) {
    Write-Host "[$($i + 1)] $($bins[$i].Name)"
}

$binChoice = Read-Host "`nSeleziona il numero del firmware da flashare [predefinito: 1]"
if ([string]::IsNullOrWhiteSpace($binChoice)) { $binChoice = 1 }
$binIndex = [int]$binChoice - 1
if ($binIndex -lt 0 -or $binIndex -ge $bins.Count) {
    Write-Host "Selezione non valida." -ForegroundColor Red
    exit
}

$selectedBin = $bins[$binIndex].FullName
Write-Host "-> Selezionato: $($bins[$binIndex].Name)`n" -ForegroundColor Green


# 3. Select COM Port using WMI
$ports = Get-WmiObject -Class Win32_PnPEntity | Where-Object { $_.Name -match "\(COM\d+\)" }

if (-not $ports) {
    Write-Host "Nessuna porta COM rilevata automaticamente." -ForegroundColor Yellow
    $selectedPort = Read-Host "Scrivi la porta COM manualmente (es. COM3)"
} else {
    $portsArray = @($ports)
    Write-Host "Porte COM disponibili:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $portsArray.Count; $i++) {
        Write-Host "[$($i + 1)] $($portsArray[$i].Name)"
    }
    
    $portChoice = Read-Host "`nSeleziona la porta COM della tua scheda [predefinito: 1]"
    if ([string]::IsNullOrWhiteSpace($portChoice)) { $portChoice = 1 }
    $portIndex = [int]$portChoice - 1
    if ($portIndex -lt 0 -or $portIndex -ge $portsArray.Count) {
        Write-Host "Selezione non valida." -ForegroundColor Red
        exit
    }
    
    if ($portsArray[$portIndex].Name -match "(COM\d+)") {
        $selectedPort = $matches[1]
    } else {
        $selectedPort = Read-Host "Non sono riuscito a estrarre la porta. Scrivila manualmente (es. COM3)"
    }
}

Write-Host "-> Porta selezionata: $selectedPort`n" -ForegroundColor Green

# 4. Flash!
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Avvio il Flash su $selectedPort..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

& $esptoolPath --chip esp32 --port $selectedPort --baud 460800 --before default_reset --after hard_reset write_flash -z 0x10000 $selectedBin

if ($LASTEXITCODE -eq 0) {
    Write-Host "`n========================================" -ForegroundColor Green
    Write-Host "SUCCESSO! Firmware caricato correttamente." -ForegroundColor Green
    Write-Host "Il dispositivo si riavviera' da solo." -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
} else {
    Write-Host "`n[ERRORE] Il flash e' fallito. Controlla che la porta COM sia giusta, o premi il tasto BOOT sulla scheda." -ForegroundColor Red
}
