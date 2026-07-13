$root=Split-Path -Parent $PSScriptRoot
$build=Join-Path $root 'build'
if(Test-Path -LiteralPath $build){$resolved=(Resolve-Path -LiteralPath $build).Path;$expected=[IO.Path]::GetFullPath((Join-Path $root 'build'));if($resolved -ne $expected){throw "Caminho inesperado: $resolved"};Remove-Item -LiteralPath $resolved -Recurse -Force}

