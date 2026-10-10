[CmdletBinding()]
param(
    [ValidateRange(0, 5)]
    [int]$FeedbackPasses = 2,
    [switch]$SkipLift
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$pythonExe = Join-Path $projectRoot '.tools\python\python.exe'
$xbePath = Join-Path $projectRoot 'game_files\Default.xbe'
$analysisRoot = Join-Path $projectRoot 'analysis'
$analysisJson = Join-Path $analysisRoot 'xbe_analysis.json'
$disasmRoot = Join-Path $analysisRoot 'disasm'
$funcIdRoot = Join-Path $analysisRoot 'func_id'
$abiRoot = Join-Path $analysisRoot 'abi'
$liftRoot = Join-Path $analysisRoot 'lift_full'
$genRoot = Join-Path $projectRoot 'src\recomp\gen'
$ordinalRemapHeader = Join-Path $projectRoot 'src\kernel_ordinal_remap.h'
$manualSeedPath = Join-Path $projectRoot 'config\seed_functions.json'
$traceFunctionPath = Join-Path $projectRoot 'config\trace_functions.json'
$manualOverridePath = Join-Path $projectRoot 'src\recomp_manual.c'

foreach ($requiredPath in @($pythonExe,$xbePath)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Missing prerequisite: $requiredPath"
    }
}
New-Item -ItemType Directory -Force -Path $analysisRoot,$disasmRoot,$funcIdRoot,$abiRoot | Out-Null

function Invoke-Tool([string[]]$Arguments) {
    & $pythonExe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Tool failed with exit code ${LASTEXITCODE}: $($Arguments -join ' ')"
    }
}

Push-Location $projectRoot
try {
    Invoke-Tool @('-m','tools.xbe_parser',$xbePath,'--json',$analysisJson,'--quiet')
    # Generate the bridge mapping from the same freshly parsed import table
    # used by disassembly. A stale hand-labelled XDK table previously mapped
    # KeConnectInterrupt (ordinal 98, one stack argument) to KeBugCheckEx
    # (ordinal 96, five arguments), corrupting the guest stack during D3D init.
    Invoke-Tool @('-m','tools.kernel_audit.gen_ordinal_remap',$analysisJson,
        '--out',$ordinalRemapHeader)
    $initialDisasmArguments = @('-m','tools.disasm',$xbePath,
        '--analysis-json',$analysisJson,'--output',$disasmRoot,
        '--exact-seed-functions',$manualSeedPath)
    $previousIdentifiedPath = Join-Path $funcIdRoot 'identified_functions.json'
    if (Test-Path -LiteralPath $previousIdentifiedPath) {
        $initialDisasmArguments += @('--seed-functions',$previousIdentifiedPath)
    }
    $initialDisasmArguments += '--force'
    Invoke-Tool $initialDisasmArguments
    Invoke-Tool @('-m','tools.func_id',$xbePath,
        '--functions',(Join-Path $disasmRoot 'functions.json'),
        '--strings',(Join-Path $disasmRoot 'strings.json'),
        '--xrefs',(Join-Path $disasmRoot 'xrefs.json'),
        '--output',$funcIdRoot)

    for ($pass = 1; $pass -le $FeedbackPasses; $pass++) {
        Invoke-Tool @('-m','tools.disasm',$xbePath,'--analysis-json',$analysisJson,
            '--output',$disasmRoot,
            '--exact-seed-functions',$manualSeedPath,
            '--seed-functions',(Join-Path $funcIdRoot 'identified_functions.json'),'--force')
        Invoke-Tool @('-m','tools.func_id',$xbePath,
            '--functions',(Join-Path $disasmRoot 'functions.json'),
            '--strings',(Join-Path $disasmRoot 'strings.json'),
            '--xrefs',(Join-Path $disasmRoot 'xrefs.json'),
            '--output',$funcIdRoot)
    }

    Invoke-Tool @('-m','tools.abi_analysis',$xbePath,
        '--disasm-dir',$disasmRoot,'--func-id-dir',$funcIdRoot,'--output-dir',$abiRoot)

    if (-not $SkipLift) {
        Invoke-Tool @((Join-Path $projectRoot 'scripts\repair_function_metadata.py'))
        New-Item -ItemType Directory -Force -Path $liftRoot,$genRoot | Out-Null
        $liftArguments = @('-m','tools.recomp',$xbePath,'--all','--split','500',
            '--output-dir',$liftRoot,'--gen-dir',$genRoot,
            '--disasm-dir',$disasmRoot,'--func-id-dir',$funcIdRoot,'--abi-dir',$abiRoot,
            '--functions',(Join-Path $disasmRoot 'functions_repaired.json'),
            '--exclude-manual',$manualOverridePath)
        if (Test-Path -LiteralPath $traceFunctionPath) {
            $liftArguments += @('--trace-functions',$traceFunctionPath)
        }
        $liftArguments += @('--memory-read-polls',(Join-Path $projectRoot 'config/memory_read_polls.json'))
        $liftArguments += @('--mmio-store32',(Join-Path $projectRoot 'config/mmio_store32.json'))
        Invoke-Tool $liftArguments
    }
} finally {
    Pop-Location
}
