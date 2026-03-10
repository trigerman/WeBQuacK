# P4wnP1 Clone - LittleFS Upload (Uses System Python)
# Run: .\upload_littlefs.ps1

$PORT = "COM13"
$BAUD = "460800"
$DATA_DIR = "$PSScriptRoot\d1_mini\data"
$IMG = "$env:TEMP\littlefs.bin"
$FS_ADDR = "0x200000"
$FS_SIZE = 14680064
$FS_PAGE = 256
$FS_BLOCK = 8192
$TOOLS = "$env:LOCALAPPDATA\Arduino15\packages\esp8266\tools"

Write-Host ""
Write-Host "=== P4wnP1 LittleFS Upload ===" -ForegroundColor Cyan
Write-Host "Port: $PORT | Address: $FS_ADDR" -ForegroundColor DarkCyan
Write-Host ""

# --- Find mklittlefs ---
Write-Host "[1/3] Finding mklittlefs..." -ForegroundColor Yellow
$MKFS = (Get-ChildItem -Path $TOOLS -Recurse -Filter "mklittlefs.exe" 2>$null | Select-Object -First 1).FullName
if (-not $MKFS) { Write-Host "FAIL: mklittlefs not found" -ForegroundColor Red; pause; exit 1 }
Write-Host "  -> $MKFS" -ForegroundColor Green

# --- Build LittleFS image ---
Write-Host "[2/3] Building LittleFS image..." -ForegroundColor Yellow
& $MKFS -c $DATA_DIR -p $FS_PAGE -b $FS_BLOCK -s $FS_SIZE $IMG
if ($LASTEXITCODE -ne 0) { Write-Host "FAIL: mklittlefs failed" -ForegroundColor Red; pause; exit 1 }
Write-Host "  -> Image built: $([math]::Round((Get-Item $IMG).Length/1024/1024, 1)) MB" -ForegroundColor Green

# --- Upload using system python esptool ---
Write-Host "[3/3] Uploading to D1 Mini on $PORT at $FS_ADDR ..." -ForegroundColor Yellow
Write-Host "      (This takes ~90 seconds for 14MB - please wait)" -ForegroundColor DarkGray
Write-Host ""

python -m esptool --chip esp8266 --port $PORT --baud $BAUD write_flash $FS_ADDR $IMG

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=== SUCCESS! Web UI uploaded ===" -ForegroundColor Green
    Write-Host ""
    Write-Host "  1. Unplug D1 Mini -> plug back in" -ForegroundColor White
    Write-Host "  2. Phone WiFi -> P4wnP1  (password: hacker123)" -ForegroundColor White
    Write-Host "  3. Browser -> http://192.168.4.1" -ForegroundColor White
}
else {
    Write-Host ""
    Write-Host "UPLOAD FAILED (exit $LASTEXITCODE)" -ForegroundColor Red
    Write-Host "Make sure D1 Mini is plugged in on $PORT" -ForegroundColor Yellow
}
Write-Host ""
pause
