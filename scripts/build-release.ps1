$ErrorActionPreference='Stop'; & "$PSScriptRoot\build.ps1" -Configuration Release; exit $LASTEXITCODE
