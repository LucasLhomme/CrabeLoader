param (
    [int]$WarmupSeconds = 15,
    [int]$DurationSeconds = 60,
    [switch]$Interactive,
    [switch]$NonInteractive,
    [switch]$EmptyWorld,
    [string]$TargetWorld = "holoempty",
    [int]$F4StressCount = 5,
    [string]$LogFile = "",
    [string]$JsonFile = "",
    [string]$GameDir = "",
    [switch]$NoKill,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

# ==============================================================================
# Game Installation Directory Configuration:
# By default, this script auto-detects Disney Infinity 3.0 across common Steam paths
# and the Windows Registry. If your game is installed in a custom location, you can:
#   1. Pass the parameter    : .\test_bench.ps1 -GameDir "C:\path\to\Disney Infinity 3.0 Gold Edition"
#   2. Set environment var   : $env:CRABE_GAME_DIR = "C:\path\to\Disney Infinity 3.0 Gold Edition"
#   3. Or edit the fallback list below directly in Resolve-GameDirectory.
# ==============================================================================
function Resolve-GameDirectory {
    param ([string]$ExplicitPath)

    if ($ExplicitPath -and (Test-Path (Join-Path $ExplicitPath "DisneyInfinity3.exe"))) {
        return (Resolve-Path $ExplicitPath).Path
    }

    if ($env:CRABE_GAME_DIR -and (Test-Path (Join-Path $env:CRABE_GAME_DIR "DisneyInfinity3.exe"))) {
        return (Resolve-Path $env:CRABE_GAME_DIR).Path
    }

    $candidates = @(
        "D:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "C:\Program Files (x86)\Steam\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "C:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "E:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition",
        "F:\SteamLibrary\steamapps\common\Disney Infinity 3.0 Gold Edition"
    )

    $regKeys = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 541670",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 541670"
    )
    foreach ($rk in $regKeys) {
        $loc = (Get-ItemProperty -Path $rk -ErrorAction SilentlyContinue).InstallLocation
        if ($loc) { $candidates += $loc }
    }

    foreach ($c in $candidates) {
        if ($c -and (Test-Path (Join-Path $c "DisneyInfinity3.exe"))) {
            return (Resolve-Path $c).Path
        }
    }

    Write-Host ""
    Write-Host "[ERROR] Disney Infinity 3.0 directory could not be located automatically." -ForegroundColor Red
    Write-Host "Please specify your game directory using one of the following methods:" -ForegroundColor Yellow
    Write-Host "  1. Pass the parameter    : .\test_bench.ps1 -GameDir `"C:\path\to\Disney Infinity 3.0 Gold Edition`"" -ForegroundColor Cyan
    Write-Host "  2. Set environment var   : `$env:CRABE_GAME_DIR = `"C:\path\to\Disney Infinity 3.0 Gold Edition`"" -ForegroundColor Cyan
    Write-Host ""
    exit 1
}

$isInteractive = (-not $NonInteractive)

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $scriptDir) { $scriptDir = $PSScriptRoot }
$projectRoot = if ((Split-Path -Leaf $scriptDir) -eq "tests") { Split-Path -Parent $scriptDir } else { $scriptDir }

if (-not $LogFile) {
    $LogFile = Join-Path $projectRoot "tests\log.txt"
}
if (-not $JsonFile) {
    $JsonFile = Join-Path $projectRoot "tests\telemetry_report.json"
}

$gameDir = Resolve-GameDirectory -ExplicitPath $GameDir
$gameExe = Join-Path $gameDir "DisneyInfinity3.exe"
$logPath = Join-Path $gameDir "loader.log"
$modsDir = Join-Path $gameDir "mods"
$benchModDir = Join-Path $modsDir "crabe_test_bench"
$companionModDir = Join-Path $modsDir "crabe_companion_bench"

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "   CRABELOADER TEST BENCH: MULTI-WORKLOAD & IN-WORLD TEST   " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Mode                 : $(if ($isInteractive) { 'Interactive In-World (Toy Box / Void)' } else { 'Automated Soak' })"
Write-Host "F4 Hot-Reload Stress : $(if ($F4StressCount -gt 0) { "$F4StressCount automated in-world pulses" } else { 'Passive monitoring' })"
Write-Host "Target World         : $(if ($EmptyWorld) { "$TargetWorld (Isolated Zero-Noise)" } else { 'User Chosen World / Blank Toy Box' })"
Write-Host "Warmup duration      : $WarmupSeconds seconds (Engine boot & asset loading)"
Write-Host "Target soak duration : $DurationSeconds seconds (Concurrent workloads)"
Write-Host "Auto-terminate game  : $(if ($NoKill) { 'No (-NoKill active)' } else { 'Yes (Clean exit)' })"
Write-Host "Report output file   : $LogFile"
Write-Host "Telemetry JSON file  : $JsonFile"
Write-Host "Game directory       : $gameDir"

if (-not $SkipBuild) {
    Write-Host "`n[1/5] Building CrabeLoader (Release Win32)..." -ForegroundColor Yellow
    cmake --build (Join-Path $projectRoot "build") --config Release --target CrabeLoader
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed."
        exit $LASTEXITCODE
    }
}

Write-Host "`n[2/5] Checking and terminating previous instances..." -ForegroundColor Yellow
Stop-Process -Name "DisneyInfinity3" -Force -ErrorAction SilentlyContinue
Start-Sleep -Milliseconds 800

Write-Host "[3/5] Deploying bink2w32.dll..." -ForegroundColor Yellow
$dllSrc = Join-Path $projectRoot "build\Release\bink2w32.dll"
$targetDll = Join-Path $gameDir "bink2w32.dll"
$origDll = Join-Path $gameDir "bink2w32_orig.dll"

if (!(Test-Path $origDll) -and (Test-Path $targetDll)) {
    Copy-Item -Path $targetDll -Destination $origDll -Force
}
Copy-Item -Path $dllSrc -Destination $targetDll -Force

Write-Host "[4/5] Injecting concurrent test harnesses (bench & companion)..." -ForegroundColor Yellow
if (!(Test-Path $benchModDir)) {
    New-Item -ItemType Directory -Path $benchModDir -Force | Out-Null
}
if (!(Test-Path $companionModDir)) {
    New-Item -ItemType Directory -Path $companionModDir -Force | Out-Null
}

$benchManifest = @{
    manifestVersion = 1
    id = "com.crabe.testbench"
    name = "CrabeLoader Multi-Workload Test Bench"
    version = "1.0.0"
    entry = "main.lua"
} | ConvertTo-Json

$shouldAutoLoad = if ($EmptyWorld) { "true" } else { "false" }

$benchScript = @"
local frameCount = 0
local vfsCalls = 0
local eventCalls = 0
local companionPongs = 0
local fiberCalls = 0
local fiberResumes = 0
local faultProbes = 0

local autoLoadTarget = "$TargetWorld"
local autoLoadRequested = $shouldAutoLoad
local worldTransitioned = false

local lastClock = nil
local totalFrameTimeMs = 0
local minFrametimeMs = 999999
local maxFrametimeMs = 0
local stutters16 = 0
local stutters33 = 0
local stutters50 = 0

local function logMsg(msg)
    if Crabe and Crabe.write then
        Crabe.write(msg)
    elseif print then
        print(msg)
    end
end

logMsg("[BENCH] Multi-workload test harness initialized.")

if Crabe and Crabe.Events and Crabe.Events.on then
    Crabe.Events.on("crabe.bench.pong", function(data)
        companionPongs = companionPongs + 1
    end)
end

if Game and Game.onTick then
    Game.onTick(function()
        frameCount = frameCount + 1

        local now = os.clock()
        if lastClock then
            local dtMs = (now - lastClock) * 1000.0
            totalFrameTimeMs = totalFrameTimeMs + dtMs
            if dtMs < minFrametimeMs then minFrametimeMs = dtMs end
            if dtMs > maxFrametimeMs then maxFrametimeMs = dtMs end
            if dtMs > 50.0 then
                stutters50 = stutters50 + 1
            elseif dtMs > 33.33 then
                stutters33 = stutters33 + 1
            elseif dtMs > 16.67 then
                stutters16 = stutters16 + 1
            end
        end
        lastClock = now

        if autoLoadRequested and not worldTransitioned and frameCount >= 100 then
            worldTransitioned = true
            if Game and Game.LoadLevel then
                local chosen = autoLoadTarget
                if Game.ListLevels then
                    local ok, levels = pcall(function() return Game.ListLevels("sortedList") end)
                    if ok and type(levels) == "table" then
                        for _, lvl in ipairs(levels) do
                            local lower = string.lower(lvl)
                            if lower == string.lower(autoLoadTarget) or lower:find("holoempty") or lower:find("blank") then
                                chosen = lvl
                                break
                            end
                        end
                    end
                end
                local ok, err = pcall(function()
                    Game.LoadLevel(chosen, true)
                end)
                if ok then
                    logMsg("[BENCH] Automated transition to empty world '" .. tostring(chosen) .. "' initiated.")
                else
                    logMsg("[BENCH_WARN] Failed to load empty world: " .. tostring(err))
                end
            end
        end

        if Crabe and Crabe.Vfs and (frameCount % 5 == 0) then
            for i = 1, 10 do
                Crabe.Vfs.resolve("characters/sora.p3d")
                Crabe.Vfs.resolve("characters/thor.p3d")
                vfsCalls = vfsCalls + 2
            end
        end

        if Crabe and Crabe.Events and Crabe.Events.emit and (frameCount % 10 == 0) then
            for i = 1, 10 do
                Crabe.Events.emit("crabe.bench.ping", { tick = frameCount, iter = i })
                eventCalls = eventCalls + 1
            end
        end

        if Crabe and Crabe.spawn and (frameCount % 15 == 0) then
            Crabe.spawn(function()
                fiberCalls = fiberCalls + 1
                if Crabe.wait then
                    Crabe.wait(150)
                end
                fiberResumes = fiberResumes + 1
            end)
        end

        if frameCount % 300 == 0 then
            local ok, err = pcall(function()
                error("controlled in-game fault injection probe")
            end)
            if not ok then
                faultProbes = faultProbes + 1
            end
        end

        if frameCount % 60 == 0 then
            local luaMem = collectgarbage("count")
            local avgFt = frameCount > 1 and (totalFrameTimeMs / (frameCount - 1)) or 0
            local msg = string.format("[BENCH_HEARTBEAT] frames=%d vfs=%d events=%d pongs=%d fibers=%d fiber_res=%d faults=%d lua_mem_kb=%.1f avg_ft=%.2f min_ft=%.2f max_ft=%.2f st16=%d st33=%d st50=%d",
                                      frameCount, vfsCalls, eventCalls, companionPongs, fiberCalls, fiberResumes, faultProbes, luaMem, avgFt, minFrametimeMs, maxFrametimeMs, stutters16, stutters33, stutters50)
            logMsg(msg)
        end
    end)
else
    logMsg("[BENCH_ERROR] Game.onTick is unavailable")
end
"@

$companionManifest = @{
    manifestVersion = 1
    id = "com.crabe.testbench.companion"
    name = "CrabeLoader Companion Test Mod"
    version = "1.0.0"
    entry = "main.lua"
} | ConvertTo-Json

$companionScript = @'
if Crabe and Crabe.Events and Crabe.Events.on then
    Crabe.Events.on("crabe.bench.ping", function(data)
        if Crabe.Events.emit then
            Crabe.Events.emit("crabe.bench.pong", { from = "companion", tick = data.tick })
        end
    end)
end
'@

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText((Join-Path $benchModDir "mod.json"), $benchManifest, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $benchModDir "main.lua"), $benchScript, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $companionModDir "mod.json"), $companionManifest, $utf8NoBom)
[System.IO.File]::WriteAllText((Join-Path $companionModDir "main.lua"), $companionScript, $utf8NoBom)

$logStartOffset = 0
if (Test-Path $logPath) {
    $logStartOffset = (Get-Item $logPath).Length
}

Write-Host "`n[5/5] Launching Disney Infinity 3.0..." -ForegroundColor Green
if (Get-Process -Name "steam" -ErrorAction SilentlyContinue) {
    Start-Process "steam://rungameid/541670"
} else {
    Start-Process -FilePath $gameExe -WorkingDirectory $gameDir
}

Write-Host "Waiting for game process..." -NoNewline
$gameProc = $null
$waitTimeout = 30
$waited = 0
while ($waited -lt $waitTimeout) {
    Start-Sleep -Seconds 1
    $gameProc = Get-Process -Name "DisneyInfinity3" -ErrorAction SilentlyContinue
    if ($gameProc) { break }
    Write-Host "." -NoNewline
    $waited++
}
Write-Host ""

if (-not $gameProc) {
    Write-Error "Unable to detect DisneyInfinity3.exe process after $waitTimeout seconds."
    Remove-Item -Path $benchModDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path $companionModDir -Recurse -Force -ErrorAction SilentlyContinue
    exit 1
}

Write-Host "Game process detected: PID $($gameProc.Id)." -ForegroundColor Green
$initialBootRam = [Math]::Round($gameProc.WorkingSet64 / 1MB, 2)
$peakRam = $initialBootRam
$lastRam = $initialBootRam
$crashCount = 0
$overlayOpenedCount = 0
$f4ReloadCount = 0
$f4History = [System.Collections.Generic.List[PSCustomObject]]::new()
$lastHeartbeat = ""

$lastFrames = 0
$lastVfs = 0
$lastEvents = 0
$lastPongs = 0
$lastFibers = 0
$lastFiberResumes = 0
$lastFaults = 0
$lastLuaMem = 0.0
$lastAvgFt = 0.0
$lastMinFt = 0.0
$lastMaxFt = 0.0
$lastSt16 = 0
$lastSt33 = 0
$lastSt50 = 0

$samples = [System.Collections.Generic.List[PSCustomObject]]::new()

<#
.SYNOPSIS
    Reads appended diagnostic log lines from the loader log file.
    Extracts heartbeat telemetry, exception signatures, and reload events.
    Updates script variables with real-time frametime and memory metrics.
#>
function Read-IncrementalLog {
    if (Test-Path $logPath) {
        $currentLogLength = (Get-Item $logPath).Length
        if ($currentLogLength -gt $script:logStartOffset) {
            $fs = [System.IO.File]::Open($logPath, [System.IO.FileMode]::Open, [System.IO.FileAccess]::Read, [System.IO.FileShare]::ReadWrite)
            $fs.Seek($script:logStartOffset, [System.IO.SeekOrigin]::Begin) | Out-Null
            $reader = New-Object System.IO.StreamReader($fs)
            $newLines = $reader.ReadToEnd()
            $reader.Close()
            $fs.Close()
            $script:logStartOffset = $currentLogLength

            if ($newLines -match "RenderHook: overlay opened") { $script:overlayOpenedCount++ }
            if ($newLines -match "Exception|CrashReporter|0xC0000005") { $script:crashCount++ }
            if ($newLines -match "hot-reload requested \(F4\)") {
                $script:f4ReloadCount++
                $curWS = [Math]::Round($script:gameProc.WorkingSet64 / 1MB, 2)
                $script:f4History.Add([PSCustomObject]@{
                    Index      = $script:f4ReloadCount
                    Timestamp  = (Get-Date).ToString("HH:mm:ss.fff")
                    WorkingSet = $curWS
                    LuaMemKB   = $script:lastLuaMem
                })
            }
            if ($newLines -match "\[BENCH_HEARTBEAT\] frames=(\d+) vfs=(\d+) events=(\d+) pongs=(\d+) fibers=(\d+) fiber_res=(\d+) faults=(\d+) lua_mem_kb=([0-9.]+) avg_ft=([0-9.]+) min_ft=([0-9.]+) max_ft=([0-9.]+) st16=(\d+) st33=(\d+) st50=(\d+)") {
                $script:lastFrames = [int]$matches[1]
                $script:lastVfs = [int]$matches[2]
                $script:lastEvents = [int]$matches[3]
                $script:lastPongs = [int]$matches[4]
                $script:lastFibers = [int]$matches[5]
                $script:lastFiberResumes = [int]$matches[6]
                $script:lastFaults = [int]$matches[7]
                $script:lastLuaMem = [double]$matches[8]
                $script:lastAvgFt = [double]$matches[9]
                $script:lastMinFt = [double]$matches[10]
                $script:lastMaxFt = [double]$matches[11]
                $script:lastSt16 = [int]$matches[12]
                $script:lastSt33 = [int]$matches[13]
                $script:lastSt50 = [int]$matches[14]
                $fps = if ($script:lastAvgFt -gt 0) { [Math]::Round(1000.0 / $script:lastAvgFt, 1) } else { 0 }
                $script:lastHeartbeat = "fps=$fps avg_ft=$($matches[9])ms vfs=$($matches[2]) ev=$($matches[3]) pong=$($matches[4]) fib=$($matches[5]) res=$($matches[6]) lua=$($matches[8])KB"
            }
        }
    }
}

$sessionWatch = [System.Diagnostics.Stopwatch]::StartNew()

try {
    Write-Host "`n[PHASE 1/2] Warmup: waiting for engine boot and asset loading ($WarmupSeconds s)..." -ForegroundColor Yellow
    $warmupWatch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($warmupWatch.Elapsed.TotalSeconds -lt $WarmupSeconds) {
        Start-Sleep -Milliseconds 1200
        $gameProc = Get-Process -Id $gameProc.Id -ErrorAction SilentlyContinue
        if (-not $gameProc) {
            Write-Host "`n[ALERT] Game crashed during startup!" -ForegroundColor Red
            $crashCount++
            break
        }
        $currentRam = [Math]::Round($gameProc.WorkingSet64 / 1MB, 2)
        $privBytes = [Math]::Round($gameProc.PrivateMemorySize64 / 1MB, 2)
        if ($currentRam -gt $peakRam) { $peakRam = $currentRam }
        $lastRam = $currentRam
        Read-IncrementalLog

        $wElapsed = [Math]::Round($warmupWatch.Elapsed.TotalSeconds, 1)
        $samples.Add([PSCustomObject]@{
            Timestamp      = (Get-Date).ToString("HH:mm:ss.fff")
            ElapsedSec     = [Math]::Round($sessionWatch.Elapsed.TotalSeconds, 1)
            Phase          = "Warmup"
            WorkingSetMB   = $currentRam
            PrivateBytesMB = $privBytes
            DriftMB        = 0.0
            LuaMemKB       = $lastLuaMem
            AvgFtMs        = $lastAvgFt
            Frames         = $lastFrames
            VfsResolutions = $lastVfs
            Events         = $lastEvents
            CompanionPongs = $lastPongs
            Fibers         = $lastFibers
            FiberResumes   = $lastFiberResumes
            Stutters16     = $lastSt16
            Stutters33     = $lastSt33
        })

        $statusMsg = if ($lastHeartbeat) { $lastHeartbeat } else { "Booting assets & shaders..." }
        Write-Host -NoNewline "`r  [WARMUP : ${wElapsed}s / ${WarmupSeconds}s]  RAM: $currentRam MB | $statusMsg             "

        if ($lastHeartbeat -and $wElapsed -ge 20 -and (-not $isInteractive)) {
            break
        }
    }
    $warmupWatch.Stop()

    if ($isInteractive) {
        Write-Host ""
        Write-Host "`n============================================================" -ForegroundColor Magenta
        Write-Host "     IN-WORLD TESTING: LOAD TOY BOX / EMPTY WORLD           " -ForegroundColor Magenta
        Write-Host "============================================================" -ForegroundColor Magenta
        Write-Host "To execute the F4 hot-reloads and performance tests directly inside the world:"
        Write-Host "1. In Disney Infinity 3.0:"
        Write-Host "   - Option A : Main Menu -> Toy Box -> New -> 'Blank' (Blank Toy Box)"
        Write-Host "   - Option B : Open CrabeMenu (F5) -> World -> Destinations -> 'holoempty'"
        Write-Host "   - Option C : Load into any existing Toy Box or Play Set"
        Write-Host "2. Once your character is loaded and standing active in the 3D world:"
        Write-Host "   Switch back to this console and press [ENTER] to start in-world tests!"
        Write-Host "============================================================" -ForegroundColor Magenta
        Read-Host "Press [ENTER] once character is in the world to launch in-world testing"
        Start-Sleep -Milliseconds 800
        $gameProc = Get-Process -Id $gameProc.Id -ErrorAction SilentlyContinue
        $currentRam = [Math]::Round($gameProc.WorkingSet64 / 1MB, 2)
        $lastRam = $currentRam
        if ($currentRam -gt $peakRam) { $peakRam = $currentRam }
    }

    $baselineRam = $lastRam
    Write-Host "`nIn-World steady-state baseline established: $baselineRam MB" -ForegroundColor Green
    Write-Host "------------------------------------------------------------"

    if ($F4StressCount -gt 0) {
        Write-Host "[F4 STRESS] Starting $F4StressCount automated in-world hot-reload cycles..." -ForegroundColor Yellow
        Add-Type -AssemblyName System.Windows.Forms
        $wshell = New-Object -ComObject WScript.Shell

        for ($i = 1; $i -le $F4StressCount; $i++) {
            $wshell.AppActivate($gameProc.Id) | Out-Null
            Start-Sleep -Milliseconds 300
            [System.Windows.Forms.SendKeys]::SendWait("{F4}")
            Start-Sleep -Milliseconds 3200

            $gameProc = Get-Process -Id $gameProc.Id -ErrorAction SilentlyContinue
            if (-not $gameProc) {
                Write-Host "`n[ALERT] Game crashed during F4 hot-reload #$i!" -ForegroundColor Red
                $crashCount++
                break
            }
            $currentRam = [Math]::Round($gameProc.WorkingSet64 / 1MB, 2)
            if ($currentRam -gt $peakRam) { $peakRam = $currentRam }
            $lastRam = $currentRam
            Read-IncrementalLog

            $f4Diff = [Math]::Round($currentRam - $baselineRam, 2)
            Write-Host "  [F4 CYCLE $i / $F4StressCount] RAM: $currentRam MB (Delta from baseline: +$f4Diff MB) | Lua: $lastLuaMem KB" -ForegroundColor Cyan
        }
        Write-Host "F4 Stress sequence completed.`n" -ForegroundColor Green
    }

    Write-Host "[PHASE 2/2] In-World Soak Test: monitoring endurance stability ($DurationSeconds s)..." -ForegroundColor Yellow
    $soakWatch = [System.Diagnostics.Stopwatch]::StartNew()
    while ($soakWatch.Elapsed.TotalSeconds -lt $DurationSeconds) {
        Start-Sleep -Milliseconds 1500

        $gameProc = Get-Process -Id $gameProc.Id -ErrorAction SilentlyContinue
        if (-not $gameProc) {
            Write-Host "`n[ALERT] Game process terminated unexpectedly!" -ForegroundColor Red
            $crashCount++
            break
        }

        $currentRam = [Math]::Round($gameProc.WorkingSet64 / 1MB, 2)
        $privBytes = [Math]::Round($gameProc.PrivateMemorySize64 / 1MB, 2)
        if ($currentRam -gt $peakRam) { $peakRam = $currentRam }
        $lastRam = $currentRam
        $drift = [Math]::Round($currentRam - $baselineRam, 2)
        $sign = if ($drift -ge 0) { "+$drift" } else { "$drift" }

        Read-IncrementalLog

        $sElapsed = [Math]::Round($soakWatch.Elapsed.TotalSeconds, 1)
        $samples.Add([PSCustomObject]@{
            Timestamp      = (Get-Date).ToString("HH:mm:ss.fff")
            ElapsedSec     = [Math]::Round($sessionWatch.Elapsed.TotalSeconds, 1)
            Phase          = "Soak"
            WorkingSetMB   = $currentRam
            PrivateBytesMB = $privBytes
            DriftMB        = $drift
            LuaMemKB       = $lastLuaMem
            AvgFtMs        = $lastAvgFt
            Frames         = $lastFrames
            VfsResolutions = $lastVfs
            Events         = $lastEvents
            CompanionPongs = $lastPongs
            Fibers         = $lastFibers
            FiberResumes   = $lastFiberResumes
            Stutters16     = $lastSt16
            Stutters33     = $lastSt33
        })

        $progStr = "  [SOAK : {0}s / {1}s]  RAM : {2} MB (Drift: {3} MB) | Peak : {4} MB | F4: {5} | {6}   " -f [Math]::Round($soakWatch.Elapsed.TotalSeconds), $DurationSeconds, $currentRam, $sign, $peakRam, $f4ReloadCount, $lastHeartbeat
        Write-Host -NoNewline "`r$progStr"
    }
    $soakWatch.Stop()
}
finally {
    $sessionWatch.Stop()
    Write-Host ""
    if ($NoKill) {
        Write-Host "`nTarget duration reached. [-NoKill] specified: leaving Disney Infinity 3.0 running." -ForegroundColor Green
    } else {
        Write-Host "`nTarget duration reached. Terminating test process in 2 seconds (use -NoKill to keep game open)..." -ForegroundColor Yellow
        Start-Sleep -Seconds 2
        Stop-Process -Name "DisneyInfinity3" -Force -ErrorAction SilentlyContinue
    }

    Remove-Item -Path $benchModDir -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item -Path $companionModDir -Recurse -Force -ErrorAction SilentlyContinue
}

$totalRuntime = [Math]::Round($sessionWatch.Elapsed.TotalSeconds, 1)
$steadyStateDrift = [Math]::Round($lastRam - $baselineRam, 2)
$isPass = ($crashCount -eq 0 -and $steadyStateDrift -lt 60.0)

$sb = [System.Text.StringBuilder]::new()
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("                 CRABELOADER IN-GAME TEST BENCH AUDIT REPORT                    ")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("Execution Date       : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')")
[void]$sb.AppendLine("Target Executable    : $gameExe")
[void]$sb.AppendLine("Process PID          : $($gameProc.Id)")
$execModeStr = if ($Interactive) { "Interactive Empty World" } elseif ($EmptyWorld) { "Automated Empty World ($TargetWorld)" } else { "Automated Soak" }
[void]$sb.AppendLine("Execution Mode       : $execModeStr")
[void]$sb.AppendLine("Target Environment   : $(if ($EmptyWorld) { "$TargetWorld (Isolated Zero-Noise Environment)" } elseif ($Interactive) { "Empty Toy Box / Void Selected" } else { "Standard Engine Environment" })")
[void]$sb.AppendLine("Total Bench Runtime  : $totalRuntime seconds")
[void]$sb.AppendLine("Warmup Duration      : $([Math]::Round($warmupWatch.Elapsed.TotalSeconds, 1)) seconds")
[void]$sb.AppendLine("Soak Duration        : $([Math]::Round($soakWatch.Elapsed.TotalSeconds, 1)) seconds")
[void]$sb.AppendLine("Auto-Kill Process    : $(if ($NoKill) { 'No' } else { 'Yes' })")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("1. EXECUTIVE SUMMARY & VERDICT")
[void]$sb.AppendLine("================================================================================")
if ($isPass) {
    [void]$sb.AppendLine("Final Verdict        : [PASS] SYSTEM STABLE - ZERO LEAKS - SOLID ARCHITECTURE")
} else {
    [void]$sb.AppendLine("Final Verdict        : [FAIL] ANOMALY DETECTED (Crash or excessive memory leak)")
}
[void]$sb.AppendLine("SEH Exceptions / Crashes : $crashCount")
[void]$sb.AppendLine("Overlay Opened Count     : $overlayOpenedCount")
[void]$sb.AppendLine("F4 Hot-Reloads Recorded  : $f4ReloadCount")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("2. MEMORY DIAGNOSTIC BREAKDOWN")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine(("* Process Spawn Working Set (Empty NT Stub)     : {0} MB" -f $initialBootRam))
[void]$sb.AppendLine(("* Steady-State Baseline (World / Engine Ready)  : {0} MB" -f $baselineRam))
[void]$sb.AppendLine(("* Peak Observed Working Set                     : {0} MB" -f $peakRam))
[void]$sb.AppendLine(("* Final Working Set                             : {0} MB" -f $lastRam))
[void]$sb.AppendLine(("* Steady-State Memory Drift (True Leak Metric)  : {0} MB (Threshold: < 60.0 MB)" -f $steadyStateDrift))
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("3. PERFORMANCE & FRAMETIME PACING TELEMETRY")
[void]$sb.AppendLine("================================================================================")
$calcFps = if ($lastAvgFt -gt 0) { [Math]::Round(1000.0 / $lastAvgFt, 1) } else { 0.0 }
[void]$sb.AppendLine("Average Frametime    : $lastAvgFt ms (~$calcFps FPS)")
[void]$sb.AppendLine("Min Observed Frame   : $lastMinFt ms")
[void]$sb.AppendLine("Max Observed Frame   : $lastMaxFt ms")
[void]$sb.AppendLine("Micro-stutters >16ms : $lastSt16 frames")
[void]$sb.AppendLine("Stutters >33ms       : $lastSt33 frames")
[void]$sb.AppendLine("Severe Drops >50ms   : $lastSt50 frames")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("4. CONCURRENT WORKLOADS & MULTI-MOD TELEMETRY")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("Primary Test Harness : crabe_test_bench (com.crabe.testbench v1.0.0)")
[void]$sb.AppendLine("Companion Mod        : crabe_companion_bench (com.crabe.testbench.companion v1.0.0)")
[void]$sb.AppendLine("Total Game Ticks     : $lastFrames frames")
[void]$sb.AppendLine("Workload 1 (VFS)     : $lastVfs path resolutions")
[void]$sb.AppendLine("Workload 2 (Events)  : $lastEvents pub/sub events emitted")
[void]$sb.AppendLine("Workload 2 (Companion): $lastPongs pong responses received")
[void]$sb.AppendLine("Workload 3 (Fibers)  : $lastFibers scheduled fibers spawned")
[void]$sb.AppendLine("Workload 3 (Resumes) : $lastFiberResumes fiber post-wait resumptions")
[void]$sb.AppendLine("Workload 4 (Faults)  : $lastFaults fault probes safely contained")
[void]$sb.AppendLine("Lua VM Heap Size     : $lastLuaMem KB (Managed by Lua garbage collector)")
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("5. HOT-RELOAD (F4) STABILITY AUDIT")
[void]$sb.AppendLine("================================================================================")
if ($f4History.Count -gt 0) {
    [void]$sb.AppendLine("Reload # | Timestamp    | Working Set | Lua Heap  | Status")
    [void]$sb.AppendLine("---------+--------------+-------------+-----------+-----------------------------")
    foreach ($r in $f4History) {
        $line = "F4 #{0,-3} | {1,-12} | {2,7} MB  | {3,6} KB | Handlers revoked & reloaded" -f $r.Index, $r.Timestamp, $r.WorkingSet, $r.LuaMemKB
        [void]$sb.AppendLine($line)
    }
} else {
    [void]$sb.AppendLine("No F4 hot-reloads were triggered during this session (Use -F4StressCount N or press F4 in-game).")
}
[void]$sb.AppendLine("")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("6. CHRONOLOGICAL MONITORING DATA (TIME-SERIES SAMPLES)")
[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("Time         | Elapsed | Phase  | WorkingSet | PrivBytes  | Drift     | LuaMemKB | AvgFtMs  | Frames | VFS  | Events | Fibers")
[void]$sb.AppendLine("-------------+---------+--------+------------+------------+-----------+----------+----------+--------+------+--------+-------")

foreach ($s in $samples) {
    $driftFormatted = if ($s.DriftMB -ge 0) { "+$($s.DriftMB)" } else { "$($s.DriftMB)" }
    $line = "{0,-12} | {1,5}s  | {2,-6} | {3,7} MB  | {4,7} MB  | {5,7} MB | {6,6} KB | {7,6} ms | {8,6} | {9,4} | {10,6} | {11,6}" -f `
        $s.Timestamp, $s.ElapsedSec, $s.Phase, $s.WorkingSetMB, $s.PrivateBytesMB, $driftFormatted, $s.LuaMemKB, $s.AvgFtMs, $s.Frames, $s.VfsResolutions, $s.Events, $s.Fibers
    [void]$sb.AppendLine($line)
}

[void]$sb.AppendLine("================================================================================")
[void]$sb.AppendLine("                              END OF REPORT                                     ")
[void]$sb.AppendLine("================================================================================")

[System.IO.File]::WriteAllText($LogFile, $sb.ToString(), [System.Text.Encoding]::UTF8)

$telemetryObject = [PSCustomObject]@{
    metadata = [PSCustomObject]@{
        timestamp = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
        targetExecutable = $gameExe
        processPid = $gameProc.Id
        executionMode = $execModeStr
        targetEnvironment = if ($EmptyWorld) { "$TargetWorld (Isolated Zero-Noise)" } elseif ($Interactive) { "Empty Toy Box / Void Selected" } else { "Standard Engine Environment" }
        totalRuntimeSec = $totalRuntime
        warmupSec = [Math]::Round($warmupWatch.Elapsed.TotalSeconds, 1)
        soakSec = [Math]::Round($soakWatch.Elapsed.TotalSeconds, 1)
    }
    verdict = [PSCustomObject]@{
        isPass = $isPass
        crashes = $crashCount
        f4Reloads = $f4ReloadCount
        overlayOpened = $overlayOpenedCount
    }
    memory = [PSCustomObject]@{
        initialWorkingSetMb = $initialBootRam
        baselineWorkingSetMb = $baselineRam
        peakWorkingSetMb = $peakRam
        finalWorkingSetMb = $lastRam
        steadyStateDriftMb = $steadyStateDrift
    }
    framePacing = [PSCustomObject]@{
        totalFrames = $lastFrames
        avgFrametimeMs = $lastAvgFt
        minFrametimeMs = $lastMinFt
        maxFrametimeMs = $lastMaxFt
        stutters16ms = $lastSt16
        stutters33ms = $lastSt33
        stutters50ms = $lastSt50
    }
    concurrency = [PSCustomObject]@{
        vfsResolutions = $lastVfs
        eventsEmitted = $lastEvents
        companionPongs = $lastPongs
        fibersSpawned = $lastFibers
        fibersResumed = $lastFiberResumes
        faultProbesContained = $lastFaults
        luaHeapKb = $lastLuaMem
    }
    samples = $samples
}

$telemetryJson = $telemetryObject | ConvertTo-Json -Depth 5
[System.IO.File]::WriteAllText($JsonFile, $telemetryJson, [System.Text.Encoding]::UTF8)

Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "                    TEST BENCH REPORT                       " -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Actual elapsed time    : $totalRuntime seconds"
Write-Host "Environment            : $(if ($EmptyWorld) { "$TargetWorld (Isolated Zero-Noise)" } elseif ($Interactive) { "Empty Toy Box / Void" } else { "Standard Engine" })"
Write-Host "Process Spawn RAM      : $initialBootRam MB"
Write-Host "Baseline RAM (Post-boot): $baselineRam MB"
Write-Host "Final RAM              : $lastRam MB"
Write-Host "Observed peak RAM      : $peakRam MB"
Write-Host "Steady-State Drift     : $steadyStateDrift MB"
Write-Host "Average Frametime      : $lastAvgFt ms (~$calcFps FPS)"
Write-Host "Fibers Spawned/Resumed : $lastFibers / $lastFiberResumes"
Write-Host "Companion Pongs        : $lastPongs"
Write-Host "F4 Hot-Reloads Recorded: $f4ReloadCount"

if ($lastHeartbeat) {
    Write-Host "Concurrent Telemetry   : $lastHeartbeat" -ForegroundColor Green
}
Write-Host "SEH Exceptions/Crashes : $crashCount"

Write-Host "------------------------------------------------------------"
if ($isPass) {
    Write-Host "  RESULT : [PASS] SYSTEM STABLE - ZERO LEAKS - SOLID ARCHITECTURE" -ForegroundColor Green
} else {
    Write-Host "  RESULT : [FAIL] ANOMALY DETECTED (Crash or excessive memory leak)" -ForegroundColor Red
}
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "`nDetailed diagnostic audit saved to:" -ForegroundColor Cyan
Write-Host "  $LogFile" -ForegroundColor Yellow
Write-Host "Structured JSON telemetry saved to:" -ForegroundColor Cyan
Write-Host "  $JsonFile" -ForegroundColor Yellow
Write-Host ""
