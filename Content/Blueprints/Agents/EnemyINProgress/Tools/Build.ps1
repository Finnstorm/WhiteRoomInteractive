param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.5',
    [switch]$PackageTargets
)
$ErrorActionPreference = 'Stop'
$enemyRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$pluginRoot = Join-Path $enemyRoot 'Plugins\ProgressEnemy'
$packageRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'DistributionBuild'))
if (-not $packageRoot.StartsWith($enemyRoot + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'Build output must remain inside EnemyINProgress.'
}
$buildArgs = @('BuildPlugin', "-Plugin=$pluginRoot\ProgressEnemy.uplugin", "-Package=$packageRoot", '-Rocket')
if ($PackageTargets) { $buildArgs += '-TargetPlatforms=Win64' } else { $buildArgs += '-NoTargetPlatforms' }
& (Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat') @buildArgs
if ($LASTEXITCODE -ne 0) { throw "Unreal plugin build failed ($LASTEXITCODE)." }
Copy-Item -LiteralPath (Join-Path $packageRoot 'Binaries') -Destination $pluginRoot -Recurse -Force
Write-Output 'ProgressEnemy compiled and installed. Open WhiteRoomInteractive.uproject.'
