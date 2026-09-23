param(
    [string]$EngineRoot = "D:/UE_5.8",
    [switch]$SkipBuild,
    [switch]$MigrateAssets
)
Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
$projectFile = Join-Path $projectRoot "ChopIt.uproject"
$build = Join-Path $EngineRoot "Engine/Build/BatchFiles/Build.bat"
$editor = Join-Path $EngineRoot "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
$report = Join-Path $projectRoot "Saved/ChainV2Reports"
$common = @("-unattended", "-nop4", "-nosplash", "-NullRHI", "-NoSound",
    "-DDC=InstalledNoZenLocalFallback", "-LocalDataCachePath=$projectRoot/Saved/LocalDDC")
if (-not $SkipBuild) {
    & $build ChopItEditor Win64 Development "-Project=$projectFile" -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw "Editor build failed." }
    & $build ChopIt Win64 Development "-Project=$projectFile" -NoHotReloadFromIDE
    if ($LASTEXITCODE -ne 0) { throw "Game build failed." }
}
if ($MigrateAssets) {
    & $editor $projectFile "-run=ChopItBootstrap" "-ChainV2" @common
    if ($LASTEXITCODE -ne 0) { throw "Chain-only asset migration failed." }
}
$started = Get-Date
& $editor $projectFile /Engine/Maps/Entry @common "-ExecCmds=Automation RunTests ChopIt.Chain" "-TestExit=Automation Test Queue Empty" "-ReportExportPath=$report" "-abslog=$projectRoot/Saved/Logs/ChainV2-Tests.log"
if ($LASTEXITCODE -ne 0) { throw "Automation process failed." }
# Unreal may return zero even when individual automation tests fail.
$index = Join-Path $report "index.json"
if (-not (Test-Path -LiteralPath $index) -or (Get-Item -LiteralPath $index).LastWriteTime -lt $started) {
    throw "No fresh automation report was produced."
}
$result = Get-Content -LiteralPath $index -Raw | ConvertFrom-Json
$failed = @($result.tests | Where-Object { $_.state -ne "Success" })
if ($result.tests.Count -lt 14 -or $failed.Count -gt 0) {
    $failed | Select-Object fullTestPath, state | Format-Table
    throw "Chain V2 acceptance tests failed or did not all run."
}
Write-Host "Chain V2: $($result.tests.Count) tests passed. Report: $index"
