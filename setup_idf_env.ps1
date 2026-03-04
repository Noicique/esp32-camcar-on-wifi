$idfPath = "L:\Program\Espressif\frameworks\esp-idf-v5.3.1"
$toolsPath = "L:\Program\Espressif\tools"
$pythonEnvPath = "L:\Program\Espressif\python_env\idf5.3_py3.11_env"
$pythonExe = "$pythonEnvPath\Scripts\python.exe"
$idfPyScript = "$idfPath\tools\idf.py"

$env:IDF_PATH = $idfPath
$env:IDF_PYTHON_ENV_PATH = $pythonEnvPath

$toolPaths = @(
    "$pythonEnvPath\Scripts",
    "$idfPath\tools",
    "$toolsPath\cmake\3.24.0\bin",
    "$toolsPath\ninja\1.11.1",
    "$toolsPath\idf-exe\1.0.3",
    "$toolsPath\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin",
    "$toolsPath\riscv32-esp-elf\esp-13.2.0_20240530\riscv32-esp-elf\bin",
    "$toolsPath\esp-clang\16.0.1-fe4f10a809\esp-clang\bin",
    "$toolsPath\xtensa-esp-elf-gdb\14.2_20240403\xtensa-esp-elf-gdb\bin",
    "$toolsPath\riscv32-esp-elf-gdb\14.2_20240403\riscv32-esp-elf-gdb\bin",
    "$toolsPath\openocd-esp32\v0.12.0-esp32-20240318\openocd-esp32\bin",
    "$toolsPath\dfu-util\0.11\dfu-util-0.11-win64"
)

$env:Path = ($toolPaths -join ";") + ";" + $env:Path

function idf {
    param([Parameter(ValueFromRemainingArguments)]$Args)
    & $pythonExe $idfPyScript @Args
}

Write-Host "Activating ESP-IDF environment (Python 3.11)..." -ForegroundColor Green
Write-Host "Python: $pythonExe" -ForegroundColor DarkGray
Write-Host ""
Write-Host "ESP-IDF environment is ready!" -ForegroundColor Cyan
Write-Host "Commands: idf build, idf flash, idf monitor" -ForegroundColor Yellow
