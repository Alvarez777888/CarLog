param(
    [string]$ServerExe,
    [switch]$NoBot
)
$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
if (!$ServerExe) { $ServerExe = Join-Path $root "build\native-server\carlog-server.exe" }
$ServerExe = (Resolve-Path -LiteralPath $ServerExe).Path
$config = Join-Path $root ".env"
if (!(Test-Path -LiteralPath $config)) { throw "Copy .env.example to .env and fill in the settings." }
foreach ($line in Get-Content -Encoding UTF8 -LiteralPath $config) {
    if ($line -match '^\s*(#.*)?$') { continue }
    if ($line -notmatch '^\s*((?:CARLOG_|PG)[A-Z0-9_]*|POSTGRES_PASSWORD)\s*=\s*(.*?)\s*$') {
        throw "Invalid entry in .env. Use NAME=value."
    }
    $name = $Matches[1]
    $value = $Matches[2]
    if ($value -match '^([''"])(.*)\1$') { $value = $Matches[2] }
    [Environment]::SetEnvironmentVariable($name, $value, "Process")
}
if (!$env:CARLOG_DATABASE_URL) {
    if (!$env:POSTGRES_PASSWORD -and !$env:PGPASSWORD) { throw "Set POSTGRES_PASSWORD for the local carlog database." }
    if (!$env:PGPASSWORD) { $env:PGPASSWORD = $env:POSTGRES_PASSWORD }
    $env:CARLOG_DATABASE_URL = "host=127.0.0.1 port=5432 dbname=carlog user=carlog connect_timeout=3"
}
if (!$env:CARLOG_PORT) { $env:CARLOG_PORT = "8000" }
$env:CARLOG_HOST = "127.0.0.1"
$env:CARLOG_SERVER_URL = "http://127.0.0.1:$env:CARLOG_PORT"
$env:CARLOG_SCHEMA = "cpp/schema.sql"
if ($env:CARLOG_TIMEZONE) { $env:PGTZ = $env:CARLOG_TIMEZONE }
$python = Join-Path $root ".venv\Scripts\python.exe"
if (!$NoBot) {
    if (!$env:CARLOG_BOT_TOKEN -or !$env:CARLOG_BOT_SECRET) { throw "Set CARLOG_BOT_TOKEN and CARLOG_BOT_SECRET in .env." }
    if (!(Test-Path -LiteralPath $python)) { throw "Run native\setup-bot.ps1 first." }
    & $python -c "import requests, PIL, pytesseract"
    if ($LASTEXITCODE -ne 0) { throw "Run native\setup-bot.ps1 to install the bot dependencies." }
    if ($env:CARLOG_TESSERACT_DIR) { $env:Path = "$env:CARLOG_TESSERACT_DIR;$env:Path" }
}
$logs = Join-Path $root "build\native-logs"
New-Item -ItemType Directory -Force $logs | Out-Null
# Only one supervisor per Windows session and port.
$lock = New-Object System.Threading.Mutex($false, "Local\CarLog-$env:CARLOG_PORT")
try { $locked = $lock.WaitOne(0) } catch [System.Threading.AbandonedMutexException] { $locked = $true }
if (!$locked) { $lock.Dispose(); throw "CarLog is already running on this port." }
function Start-Child($program, $arguments, $name) {
    $options = @{
        FilePath = $program
        WorkingDirectory = $root
        RedirectStandardOutput = (Join-Path $logs "$name.out.log")
        RedirectStandardError = (Join-Path $logs "$name.err.log")
        NoNewWindow = $true
        PassThru = $true
    }
    if ($arguments) { $options.ArgumentList = $arguments }
    Start-Process @options
}
Push-Location $root
$server = $null
$bot = $null
try {
    & $ServerExe --init
    if ($LASTEXITCODE -ne 0) { throw "Database initialization failed. Check PostgreSQL and .env." }
    while ($true) {
        if (!$server -or $server.HasExited) {
            $server = Start-Child $ServerExe $null "server"
            $ready = $false
            for ($i = 0; $i -lt 20 -and !$server.HasExited; $i++) {
                try {
                    $health = Invoke-RestMethod "$env:CARLOG_SERVER_URL/api/health" -TimeoutSec 2
                    if ($health.status -in @("ok", "degraded")) { $ready = $true; break }
                } catch { Start-Sleep -Milliseconds 500 }
            }
            if (!$ready -or $server.HasExited) { throw "API startup failed. See $logs\server.err.log and check if the port is occupied." }
            Write-Host "CarLog API: $env:CARLOG_SERVER_URL"
        }
        if (!$NoBot -and (!$bot -or $bot.HasExited)) {
            $bot = Start-Child $python "run_bot.py" "bot"
            Write-Host "Telegram bot started. Logs: $logs"
        }
        Start-Sleep -Seconds 5
    }
}
finally {
    foreach ($child in @($bot, $server)) {
        if ($child -and !$child.HasExited) { Stop-Process -Id $child.Id -ErrorAction SilentlyContinue }
    }
    Pop-Location
    $lock.ReleaseMutex()
    $lock.Dispose()
}
