$ErrorActionPreference = "Stop"

cmake --preset windows-vs2022-dll
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build --preset build-vs2022-dll-release
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

New-Item -ItemType Directory -Force -Path "Release" | Out-Null
Copy-Item -Path "build\vs2022-dll\Release\bink2w32.dll" -Destination "Release\bink2w32.dll" -Force

exit 0
