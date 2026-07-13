param([ValidateSet('Debug','Release')][string]$Configuration='Release',[switch]$NoFetch)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$cmakeArgs=@('-S',$root,'-B',"$root/build",'-G','Visual Studio 17 2022','-A','x64')
if($NoFetch){$cmakeArgs+='-DVOXFORGE_FETCH_JUCE=OFF'}
& cmake @cmakeArgs
if($LASTEXITCODE){exit $LASTEXITCODE}
& cmake --build "$root/build" --config $Configuration --parallel
if($LASTEXITCODE){exit $LASTEXITCODE}
& ctest --test-dir "$root/build" -C $Configuration --output-on-failure
exit $LASTEXITCODE
