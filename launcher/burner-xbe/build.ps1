param([Parameter(Mandatory=$true)][string]$Package,[string]$PreviewExecutable)
$ErrorActionPreference = 'Stop'
$packagePath = (Resolve-Path -LiteralPath $Package).Path
if (!(Test-Path -LiteralPath (Join-Path $packagePath 'tools/runtime-manifest.json'))) { throw 'Assemble the package before building its launchers.' }
$icon = Join-Path (Split-Path $PSScriptRoot -Parent) 'burner/burner.ico'
$compiler = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
foreach ($direct in @($false,$true)) {
    $name = if ($direct) { 'Play Burner Edition.exe' } else { 'Burner Edition Setup.exe' }
    $arguments = @('/nologo','/target:winexe','/platform:x64','/optimize+',
        '/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.Web.Extensions.dll',
        ('/win32icon:'+$icon),('/out:'+(Join-Path $packagePath $name)))
    if ($direct) { $arguments += '/define:DIRECT' }
    & $compiler @arguments (Join-Path $PSScriptRoot 'Launcher.cs')
    if ($LASTEXITCODE) { throw 'Launcher compilation failed' }
}
if ($PreviewExecutable) {
    & $compiler /nologo /target:winexe /platform:x64 /optimize+ /define:LAUNCHER_PREVIEW /reference:System.Windows.Forms.dll /reference:System.Drawing.dll /reference:System.Web.Extensions.dll ('/win32icon:'+$icon) ('/out:'+$PreviewExecutable) (Join-Path $PSScriptRoot 'Launcher.cs')
    if ($LASTEXITCODE) { throw 'Preview launcher compilation failed' }
}
