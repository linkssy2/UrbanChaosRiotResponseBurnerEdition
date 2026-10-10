[CmdletBinding()]
param(
    [string]$IsoPath,
    [switch]$Full
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (-not $IsoPath) {
    $IsoPath = Join-Path $projectRoot 'Urban Chaos - Riot Response.iso'
}
$IsoPath = (Resolve-Path -LiteralPath $IsoPath).Path
$pythonExe = Join-Path $projectRoot '.tools\python\python.exe'
$gameRoot = Join-Path $projectRoot 'game_files'
$expectedIsoHash = 'A14B95145A075908B1969FA35F5E1AF6E8657A7E5E1B7A555E54DCD9877E3245'
$expectedXbeHash = 'B74AE76FDBF1EF5F95DCFABF4F94F3AA738E649ECAEF4068D8531E07071B5DAC'

if (-not (Test-Path -LiteralPath $pythonExe)) {
    throw 'Project Python is missing. Run scripts/bootstrap.ps1 first.'
}

$isoHash = (Get-FileHash -LiteralPath $IsoPath -Algorithm SHA256).Hash
if ($isoHash -ne $expectedIsoHash) {
    throw "Unsupported disc revision. Expected $expectedIsoHash, got $isoHash"
}

New-Item -ItemType Directory -Force -Path $gameRoot | Out-Null
Push-Location (Join-Path $projectRoot 'third_party\xboxrecomp')
try {
    if ($Full) {
        & $pythonExe -m tools.xiso unpack $IsoPath -o $gameRoot
    } else {
        & $pythonExe -m tools.xiso get $IsoPath default.xbe -o $gameRoot
    }
    if ($LASTEXITCODE -ne 0) { throw "XDVDFS extraction failed with exit code $LASTEXITCODE" }
} finally {
    Pop-Location
}

$xbePath = Join-Path $gameRoot 'Default.xbe'
$xbeHash = (Get-FileHash -LiteralPath $xbePath -Algorithm SHA256).Hash
if ($xbeHash -ne $expectedXbeHash) {
    throw "Extracted XBE hash mismatch. Expected $expectedXbeHash, got $xbeHash"
}
Write-Host "Verified Default.xbe: $xbeHash"

