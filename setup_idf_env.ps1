$idfPath = "L:\Program\Espressif\frameworks\esp-idf-v5.3.1"

if (Test-Path "$idfPath\export.ps1") {
    Write-Host "Activating ESP-IDF environment..." -ForegroundColor Green
    . "$idfPath\export.ps1"
    Write-Host ""
    Write-Host "ESP-IDF environment is ready!" -ForegroundColor Cyan
    Write-Host "You can now run: idf.py build" -ForegroundColor Yellow
} else {
    Write-Host "ERROR: ESP-IDF not found at $idfPath" -ForegroundColor Red
    Write-Host "Please check your ESP-IDF installation." -ForegroundColor Red
}
