$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
cmake -S . -B build -A x64
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host 'VST3: build/CLAInspiredGuitar_artefacts/Release/VST3/Guitar Rack.vst3'
