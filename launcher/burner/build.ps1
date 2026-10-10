Add-Type -AssemblyName System.Drawing
$root = Split-Path $PSScriptRoot -Parent
$release = Get-Content (Join-Path $PSScriptRoot 'release.json') -Raw | ConvertFrom-Json
$package = Join-Path (Split-Path $root -Parent) $release.package_directory
$image = [System.Drawing.Image]::FromFile((Join-Path $PSScriptRoot 'cover.png'))
$images = @()
foreach ($size in @(16,32,48,64,128,256)) {
 $bitmap = [System.Drawing.Bitmap]::new($size,$size)
 $g = [System.Drawing.Graphics]::FromImage($bitmap)
 $g.Clear([System.Drawing.Color]::Black)
 $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
 $width = [int]($size * $image.Width / $image.Height)
 $g.DrawImage($image,[System.Drawing.Rectangle]::new([int](($size-$width)/2),0,$width,$size))
 $stream = [System.IO.MemoryStream]::new();$bitmap.Save($stream,[System.Drawing.Imaging.ImageFormat]::Png)
 $images += ,@($size,$stream.ToArray());$stream.Dispose();$g.Dispose();$bitmap.Dispose()
}
$image.Dispose()
$stream = [System.IO.File]::Create((Join-Path $PSScriptRoot 'burner.ico'));$writer = [System.IO.BinaryWriter]::new($stream)
$writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]$images.Count);$offset=6+16*$images.Count
foreach($row in $images){$size=$row[0];$bytes=$row[1];$dim=if($size -eq 256){0}else{$size};$writer.Write([byte]$dim);$writer.Write([byte]$dim);$writer.Write([uint16]0);$writer.Write([uint16]1);$writer.Write([uint16]32);$writer.Write([uint32]$bytes.Length);$writer.Write([uint32]$offset);$offset+=$bytes.Length}
foreach($row in $images){$writer.Write([byte[]]$row[1])};$writer.Dispose();$stream.Dispose()
$csc = 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe'
foreach($direct in @($false,$true)){
 $name=if($direct){'Play Burner Edition.exe'}else{'Burner Edition Setup.exe'}
 $args=@('/nologo','/target:winexe','/platform:x64','/optimize+','/reference:System.Windows.Forms.dll','/reference:System.Drawing.dll','/reference:System.Web.Extensions.dll',('/win32icon:'+(Join-Path $PSScriptRoot 'burner.ico')),('/out:'+(Join-Path $package $name)))
 if($direct){$args+='/define:DIRECT'}
 & $csc @args (Join-Path $PSScriptRoot 'Launcher.cs')
 if($LASTEXITCODE){throw 'Launcher compilation failed'}
}
