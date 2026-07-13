$ErrorActionPreference='Stop';$root=Split-Path -Parent $PSScriptRoot
foreach($tool in 'cmake','git','ctest'){if(!(Get-Command $tool -ErrorAction SilentlyContinue)){throw "Ferramenta obrigatória ausente do PATH: $tool"}}
& "$PSScriptRoot\clean.ps1";& "$PSScriptRoot\build-release.ps1";if($LASTEXITCODE){exit $LASTEXITCODE}
& "$PSScriptRoot\run-tests.ps1";if($LASTEXITCODE){exit $LASTEXITCODE}
& "$PSScriptRoot\build-portable.ps1";& "$PSScriptRoot\build-installer.ps1"
$files=@("$root\dist\portable\ModificadorDeVoz.exe","$root\dist\installer\Modificador-de-Voz-Setup-x64.exe");foreach($f in $files){if(!(Test-Path -LiteralPath $f)){throw "Pacote ausente: $f"}}
New-Item -ItemType Directory -Force -Path "$root\dist\checksums"|Out-Null;$lines=$files|ForEach-Object{$h=Get-FileHash -Algorithm SHA256 -LiteralPath $_;"$($h.Hash)  $([IO.Path]::GetFileName($_))"};Set-Content -LiteralPath "$root\dist\checksums\SHA256.txt" -Value $lines -Encoding ASCII
$files;"$root\dist\checksums\SHA256.txt"
