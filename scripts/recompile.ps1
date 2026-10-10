[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$pythonExe = Join-Path $projectRoot '.tools\python\python.exe'
Push-Location $projectRoot
try {
    & $pythonExe scripts/repair_function_metadata.py
    if ($LASTEXITCODE -ne 0) { throw 'Reviewed metadata repair failed' }
    & $pythonExe -m tools.recomp game_files/Default.xbe --all --split 500 `
        --output-dir analysis/lift_full --gen-dir src/recomp/gen `
        --disasm-dir analysis/disasm --func-id-dir analysis/func_id --abi-dir analysis/abi `
        --functions analysis/disasm/functions_repaired.json --exclude-manual src/recomp_manual.c `
        --trace-functions config/trace_functions.json --memory-read-polls config/memory_read_polls.json `
        --mmio-store32 config/mmio_store32.json
    if ($LASTEXITCODE -ne 0) { throw 'Recompilation failed' }
    & $pythonExe scripts/apply_audio_list_hooks.py
    if ($LASTEXITCODE -ne 0) { throw 'Audio list hook installation failed' }
    & $pythonExe scripts/apply_menu_pointer_hooks.py
    if ($LASTEXITCODE -ne 0) { throw 'Menu pointer hook installation failed' }
    & $pythonExe scripts/apply_pc_button_prompt_hooks.py
    if ($LASTEXITCODE -ne 0) { throw 'PC button prompt hook installation failed' }
    & $pythonExe scripts/apply_pc_hud_prompt_hooks.py
    if ($LASTEXITCODE -ne 0) { throw 'PC HUD prompt hook installation failed' }
    & $pythonExe scripts/apply_loading_present_hook.py
    if ($LASTEXITCODE -ne 0) { throw 'Loading presentation hook installation failed' }
    & $pythonExe scripts/install_pc_camera_hook.py
    if ($LASTEXITCODE -ne 0) { throw 'PC camera hook installation failed' }
    & $pythonExe scripts/install_sound_request_probe.py
    if ($LASTEXITCODE -ne 0) { throw 'Sound request probe installation failed' }
    & $pythonExe scripts/install_hostage_trace.py
    if ($LASTEXITCODE -ne 0) { throw 'Hostage trace installation failed' }
    & $pythonExe scripts/install_solver_cost_probe.py
    if ($LASTEXITCODE -ne 0) { throw 'Solver cost probe installation failed' }
    & $pythonExe scripts/prune_suppressed_esp_probes.py
    if ($LASTEXITCODE -ne 0) { throw 'Suppressed ESP probe pruning failed' }
} finally { Pop-Location }
