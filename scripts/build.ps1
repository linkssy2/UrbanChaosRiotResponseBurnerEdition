[CmdletBinding()]
param(
    [ValidateSet('Debug','Release','RelWithDebInfo')]
    [string]$Configuration = 'RelWithDebInfo',
    [ValidateRange(1, 16)]
    [int]$Jobs = 2,
    [ValidateSet('OFF','GENERATE','USE')][string]$PgoPhase = 'OFF',
    [string]$PgoProfile = '',
    [ValidateSet('GRAPHICS','GUEST')][string]$PgoScope = 'GRAPHICS',
    [switch]$XmvSimd = $true,
    [switch]$GraphicsLto,
    [switch]$DmaDiagnostic,
    [switch]$CompactPalette,
    [string]$LocalPaletteSource = '',
    [switch]$GuardedPalette = $true,
    [switch]$PaletteGuardTrace,
    [switch]$PaletteVectorGuard,
    [switch]$PaletteForwardScratch,
    [switch]$PaletteForwardAb,
    [switch]$BoundedVertex,
    [switch]$BoundedVertexTrace,
    [switch]$InlineVertex,
    [switch]$InlineVertexTrace,
    [switch]$GuestStreamSwitch,
    [switch]$TextureLookupHint,
    [switch]$TextureLookupAb,
    [switch]$RamWatchShadow,
    [switch]$BodyRetention,
    [switch]$HierarchyLocal,
    [switch]$HierarchyGuarded,
    [switch]$HierarchyForward,
    [switch]$HierarchyRam,
    [ValidateSet('DEFAULT','PALETTE_LAST')][string]$GuestLayout = 'DEFAULT'
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if($GraphicsLto -and ($PgoPhase -ne 'OFF' -or $Configuration -ne 'RelWithDebInfo')) {
    throw 'Graphics LTO comparison requires RelWithDebInfo and PGO OFF'
}
if($PgoPhase -ne 'OFF' -and $Configuration -ne 'RelWithDebInfo') {
    throw 'PGO is supported only in RelWithDebInfo'
}
$buildRoot = Join-Path $projectRoot 'build'
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) {
    $cmakeExe = $cmakeCommand.Source
} else {
    $cmakeExe = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
}
if (-not (Test-Path -LiteralPath $cmakeExe)) {
    throw 'CMake was not found. Install the C++ CMake tools in Visual Studio 2022.'
}

function Invoke-CleanProcess([string]$FilePath, [string[]]$Arguments, [string]$Step) {
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $projectRoot
    $startInfo.UseShellExecute = $false
    foreach ($argument in $Arguments) {
        [void]$startInfo.ArgumentList.Add($argument)
    }

    # Codex sessions can contain both `Path` and `PATH`. MSBuild treats those
    # as duplicate dictionary keys and fails before invoking cl.exe. Rebuild a
    # case-insensitive child environment without changing this process or the
    # user's machine settings.
    $cleanEnvironment = [System.Collections.Generic.Dictionary[string,string]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in [Environment]::GetEnvironmentVariables().GetEnumerator()) {
        $cleanEnvironment[[string]$entry.Key] = [string]$entry.Value
    }
    $startInfo.Environment.Clear()
    foreach ($entry in $cleanEnvironment.GetEnumerator()) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) {
        throw "$Step failed with exit code $($process.ExitCode)"
    }
}

if($GuardedPalette -and !$LocalPaletteSource) {
    if($CompactPalette -or $PgoPhase -ne 'OFF' -or $GuestLayout -ne 'DEFAULT') {
        throw 'Disable GuardedPalette explicitly when testing another guest-code candidate'
    }
    $palettePython = Get-Command python -ErrorAction SilentlyContinue
    $palettePythonPath = if($palettePython){$palettePython.Source}else{
        Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/python/python.exe'
    }
    if(!(Test-Path -LiteralPath $palettePythonPath)){throw 'Python is required to derive guarded palette sources; install Python or use -GuardedPalette:$false'}
    $LocalPaletteSource=Join-Path $buildRoot 'guest_candidates/recomp_0007_guarded.c'
    $paletteArgs=@((Join-Path $PSScriptRoot 'prepare_palette_local.py'),'--production-output',$LocalPaletteSource)
    if($PaletteGuardTrace){$paletteArgs+='--trace-guard'}
    if($PaletteVectorGuard){$paletteArgs+='--vector-guard'}
    if($PaletteForwardScratch){$paletteArgs+='--forward-scratch'}
    if($PaletteForwardAb){$paletteArgs+='--forward-ab'}
    Invoke-CleanProcess $palettePythonPath $paletteArgs 'Guarded palette generation'
}
if(([int]$HierarchyLocal.IsPresent+[int]$HierarchyGuarded.IsPresent+[int]$HierarchyForward.IsPresent+[int]$HierarchyRam.IsPresent) -gt 1){throw 'Select one hierarchy candidate'}
if($HierarchyLocal -or $HierarchyGuarded -or $HierarchyForward -or $HierarchyRam){
    if(!$GuardedPalette -or !$palettePythonPath){throw 'Hierarchy candidate requires default guarded palette generation'}
    $hierarchySource=Join-Path $buildRoot 'guest_candidates/recomp_0007_hierarchy.c'
    $hierarchyArgs=@((Join-Path $PSScriptRoot 'prepare_hierarchy_local.py'),'--base-source',$LocalPaletteSource,'--production-output',$hierarchySource)
    if($HierarchyGuarded){$hierarchyArgs+='--guarded'}
    if($HierarchyForward){$hierarchyArgs+='--forward'}
    if($HierarchyRam){$hierarchyArgs+='--ram'}
    Invoke-CleanProcess $palettePythonPath $hierarchyArgs 'Hierarchy candidate generation'
    $LocalPaletteSource=$hierarchySource
}
if($BoundedVertex){
    if(!$palettePythonPath){throw 'Bounded vertex generation requires the default Python-backed build path'}
    Invoke-CleanProcess $palettePythonPath @((Join-Path $PSScriptRoot 'prepare_vertex_decode_cache.py')) 'Bounded vertex generation'
}
if($InlineVertex){
    if(!$palettePythonPath){throw 'Inline vertex generation requires the default Python-backed build path'}
    Invoke-CleanProcess $palettePythonPath @((Join-Path $PSScriptRoot 'prepare_inline_specialized.py')) 'Inline vertex generation'
}
if($GuestStreamSwitch){
    if(!$palettePythonPath){throw 'Guest stream generation requires the default Python-backed build path'}
    Invoke-CleanProcess $palettePythonPath @((Join-Path $PSScriptRoot 'test_guest_stream_dispatch.py'),'--production-output',(Join-Path $buildRoot 'guest_candidates/recomp_0006_stream.c')) 'Guest stream dispatch generation'
}
Invoke-CleanProcess $cmakeExe @('-S',$projectRoot,'-B',$buildRoot,
    '-G','Visual Studio 17 2022','-A','x64',"-DUCRR_PGO_PHASE=$PgoPhase",
    "-DUCRR_PGO_PROFILE=$PgoProfile", "-DUCRR_PGO_SCOPE=$PgoScope", "-DUCRR_XMV_SIMD=$($XmvSimd.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_GRAPHICS_LTO=$($GraphicsLto.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_DMA_DIAGNOSTIC=$($DmaDiagnostic.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_COMPACT_PALETTE=$($CompactPalette.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_BOUNDED_VERTEX=$($BoundedVertex.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_BOUNDED_VERTEX_TRACE=$($BoundedVertexTrace.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_INLINE_VERTEX=$($InlineVertex.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_INLINE_VERTEX_TRACE=$($InlineVertexTrace.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_GUEST_STREAM_SWITCH=$($GuestStreamSwitch.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_TEXTURE_LOOKUP_HINT=$($TextureLookupHint.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_TEXTURE_LOOKUP_AB=$($TextureLookupAb.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_RAM_WATCH_SHADOW=$($RamWatchShadow.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_BODY_RETENTION=$($BodyRetention.IsPresent.ToString().ToUpperInvariant())",
    "-DUCRR_GUEST_LAYOUT=$GuestLayout", "-DUCRR_LOCAL_PALETTE_SOURCE=$LocalPaletteSource") 'CMake configure'
Invoke-CleanProcess $cmakeExe @('--build',$buildRoot,'--config',$Configuration,
    '--parallel',[string]$Jobs) 'Build'
