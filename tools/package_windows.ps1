# Builds the Windows release of the solver and the app and zips them into dist\.
#
#   $env:VCPKG_ROOT = "D:\repos\vcpkg"
#   tools\package_windows.ps1
#
# Produces:
#   dist\k15-gto-windows-<version>.zip          the app, with the solver inside
#   dist\k15-gto-server-windows-<version>.zip   the solver alone, for a server
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
if (-not $env:VCPKG_ROOT) { throw 'Set VCPKG_ROOT to a vcpkg checkout first.' }

function Invoke-Step([string]$What, [scriptblock]$Step) {
  Write-Host "== $What"
  & $Step
  if ($LASTEXITCODE -ne 0) { throw "$What failed with exit code $LASTEXITCODE" }
}

$version = (Select-String -Path (Join-Path $root 'CMakeLists.txt') `
    -Pattern 'project\(k15_gto VERSION ([0-9.]+)').Matches[0].Groups[1].Value

Push-Location $root
try {
  # The solver first: the app's build picks up gto-server.exe from here.
  Invoke-Step 'Configure the solver' { cmake --preset windows | Out-Null }
  Invoke-Step 'Build the solver' { cmake --build --preset windows | Out-Null }
  Invoke-Step 'Test the solver' { ctest --preset windows | Out-Null }

  Push-Location app
  try {
    Invoke-Step 'Test the app' { flutter test | Out-Null }
    Invoke-Step 'Build the app' { flutter build windows --release | Out-Null }
  } finally { Pop-Location }

  $bundle = Join-Path $root 'app\build\windows\x64\runner\Release'
  $server = Join-Path $root 'build\windows\server\Release\gto-server.exe'
  if (-not (Test-Path (Join-Path $bundle 'gto-server.exe'))) {
    throw 'The app bundle does not contain gto-server.exe.'
  }

  $dist = Join-Path $root 'dist'
  New-Item -ItemType Directory -Force $dist | Out-Null
  $appZip = Join-Path $dist "k15-gto-windows-$version.zip"
  $serverZip = Join-Path $dist "k15-gto-server-windows-$version.zip"
  Remove-Item $appZip, $serverZip -ErrorAction SilentlyContinue

  Compress-Archive -Path (Join-Path $bundle '*') -DestinationPath $appZip
  Compress-Archive -Path $server, (Join-Path $root 'LICENSE'), (Join-Path $root 'README.md') `
    -DestinationPath $serverZip

  Get-Item $appZip, $serverZip | ForEach-Object {
    Write-Host ("{0}  {1:N1} MB" -f $_.Name, ($_.Length / 1MB))
  }
} finally { Pop-Location }
