$ErrorActionPreference = "Stop"

cmake --preset windows-vs2022-dll
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build --preset build-vs2022-dll-release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path "Release" | Out-Null
Copy-Item -Path "build\vs2022-dll\Release\CrabeLoader.asi" -Destination "Release\CrabeLoader.asi" -Force
Copy-Item -Path "build\vs2022-dll\Release\dinput8.dll" -Destination "Release\dinput8.dll" -Force

exit 0
