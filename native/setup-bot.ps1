$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$venv = Join-Path $root ".venv"
if (!(Test-Path (Join-Path $venv "Scripts\python.exe"))) {
    & py -3.11 -m venv $venv
    if ($LASTEXITCODE -ne 0) { throw "Install Python 3.11 for Windows first." }
}
& (Join-Path $venv "Scripts\python.exe") -m pip install -r (Join-Path $root "requirements.txt")
if ($LASTEXITCODE -ne 0) { throw "Bot dependency installation failed." }
