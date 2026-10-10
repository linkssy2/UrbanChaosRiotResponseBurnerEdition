[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolsRoot = Join-Path $projectRoot '.tools'
$downloadRoot = Join-Path $toolsRoot 'downloads'
$pythonRoot = Join-Path $toolsRoot 'python'
$pythonExe = Join-Path $pythonRoot 'python.exe'
$pythonArchive = Join-Path $downloadRoot 'python-3.12.10-embed-amd64.zip'
$getPip = Join-Path $downloadRoot 'get-pip.py'

$pythonUrl = 'https://www.python.org/ftp/python/3.12.10/python-3.12.10-embed-amd64.zip'
$pythonSha256 = '4ACBED6DD1C744B0376E3B1CF57CE906F9DC9E95E68824584C8099A63025A3C3'
$getPipUrl = 'https://bootstrap.pypa.io/get-pip.py'

New-Item -ItemType Directory -Force -Path $downloadRoot,$pythonRoot | Out-Null

if (-not (Test-Path -LiteralPath $pythonExe)) {
    Invoke-WebRequest -Uri $pythonUrl -OutFile $pythonArchive
    $actualHash = (Get-FileHash -LiteralPath $pythonArchive -Algorithm SHA256).Hash
    if ($actualHash -ne $pythonSha256) {
        throw "Portable Python hash mismatch: expected $pythonSha256, got $actualHash"
    }
    Expand-Archive -LiteralPath $pythonArchive -DestinationPath $pythonRoot -Force
}

$pthPath = Join-Path $pythonRoot 'python312._pth'
$pthLines = @(
    'python312.zip'
    '.'
    '..\..'
    '..\..\third_party\xboxrecomp'
    ''
    '# Load project-local site-packages'
    'import site'
)
Set-Content -LiteralPath $pthPath -Value $pthLines -Encoding Ascii

if (-not (Test-Path -LiteralPath $getPip)) {
    Invoke-WebRequest -Uri $getPipUrl -OutFile $getPip
}

& $pythonExe $getPip --no-warn-script-location
if ($LASTEXITCODE -ne 0) { throw "pip bootstrap failed with exit code $LASTEXITCODE" }

& $pythonExe -m pip install --disable-pip-version-check --no-warn-script-location 'capstone==5.0.6'
if ($LASTEXITCODE -ne 0) { throw "Capstone install failed with exit code $LASTEXITCODE" }

& $pythonExe -c "import capstone, sys; print(sys.version); print('capstone', capstone.__version__)"

