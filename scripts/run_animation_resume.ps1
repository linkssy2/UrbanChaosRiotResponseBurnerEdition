param([Parameter(Mandatory=$true)][string]$Script)
$ErrorActionPreference = 'Stop'
$animationMutex = [Threading.Mutex]::new($false, 'UrbanChaosMigrationHeavy20261009')
$animationOwned = $false
try {
    try { $animationOwned = $animationMutex.WaitOne(10000) }
    catch [Threading.AbandonedMutexException] {
        $animationOwned = $true
        if (@(Get-Process | Where-Object { $_.ProcessName -match '^(cl|link|MSBuild|cmake|ninja|checks|migration_.*checks|native_.*checks)$|Urban.*Chaos|^ucrr$' }).Count) { throw 'Abandoned mutex with active work; defer.' }
    }
    if (!$animationOwned) { Write-Output 'HEAVY_SLOT_OCCUPIED'; exit 75 }
    if (@(Get-Process | Where-Object { $_.ProcessName -match 'Urban.*Chaos|^ucrr$' }).Count) { throw 'Game active; defer.' }
    $env:UCRR_SERVICES_HEAVY_MUTEX = 'owned'
    $env:UCRR_SERVICES_HEAVY_MUTEX_PID = [string]$PID
    # MSBuild rejects inherited Path/PATH duplicates; pass one canonical key.
    & "$PSScriptRoot/../.tools/python/python.exe" -c "import os,subprocess,sys;sys.exit(subprocess.call([sys.executable,*sys.argv[1:]],env={k.upper():v for k,v in os.environ.items()}))" $Script @args
    if ($LASTEXITCODE) { throw "Animation qualification failed: $LASTEXITCODE" }
}
finally {
    Remove-Item Env:UCRR_SERVICES_HEAVY_MUTEX -ErrorAction SilentlyContinue
    Remove-Item Env:UCRR_SERVICES_HEAVY_MUTEX_PID -ErrorAction SilentlyContinue
    if ($animationOwned) { $animationMutex.ReleaseMutex() }
    $animationMutex.Dispose()
}
