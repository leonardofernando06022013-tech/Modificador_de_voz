$ErrorActionPreference='Stop';$root=Split-Path -Parent $PSScriptRoot;& ctest --test-dir "$root\build" -C Release --output-on-failure;exit $LASTEXITCODE
