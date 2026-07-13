$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$localeDir=Join-Path $root 'resources\locales'
$referenceFile=Join-Path $localeDir 'pt-BR.json'
if(!(Test-Path -LiteralPath $referenceFile)){throw "Locale de referência ausente: $referenceFile"}
try{$reference=Get-Content -Raw -LiteralPath $referenceFile -Encoding UTF8|ConvertFrom-Json}catch{throw "JSON inválido: pt-BR.json - $($_.Exception.Message)"}
$referenceKeys=@($reference.PSObject.Properties.Name|Sort-Object)
$expected='pt-BR','en-US','es-ES','fr-FR','de-DE','it-IT','ja-JP','ko-KR','zh-CN','ru-RU'
foreach($code in $expected){
  $file=Join-Path $localeDir "$code.json"
  if(!(Test-Path -LiteralPath $file)){throw "Locale ausente: $code.json"}
  try{$locale=Get-Content -Raw -LiteralPath $file -Encoding UTF8|ConvertFrom-Json}catch{throw "JSON inválido: $code.json - $($_.Exception.Message)"}
  $keys=@($locale.PSObject.Properties.Name|Sort-Object)
  $missing=@($referenceKeys|Where-Object{$_ -notin $keys})
  $extra=@($keys|Where-Object{$_ -notin $referenceKeys})
  if($missing.Count -or $extra.Count){throw "$code.json: ausentes=[$($missing -join ', ')] extras=[$($extra -join ', ')]"}
}
Write-Output "Locales válidos: $($expected.Count) idiomas, $($referenceKeys.Count) chaves."
