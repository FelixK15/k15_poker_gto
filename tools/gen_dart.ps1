# Regenerates the Dart gRPC code in app/lib/src/gen from proto/.
#
# Needs:
#   - protoc: taken from vcpkg_installed (built with the C++ dependencies) or PATH
#   - protoc-gen-dart: dart pub global activate protoc_plugin
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$protoDir = Join-Path $root 'proto'
$outDir = Join-Path $root 'app\lib\src\gen'

$protoc = Get-ChildItem -Path (Join-Path $root 'vcpkg_installed') -Recurse -Filter 'protoc.exe' -ErrorAction SilentlyContinue |
  Select-Object -First 1 -ExpandProperty FullName
if (-not $protoc) {
  $cmd = Get-Command protoc -ErrorAction SilentlyContinue
  if ($cmd) { $protoc = $cmd.Source }
}
if (-not $protoc) { throw 'protoc not found. Build the C++ project once (vcpkg provides protoc) or put protoc on PATH.' }

$plugin = Join-Path $env:LOCALAPPDATA 'Pub\Cache\bin\protoc-gen-dart.bat'
if (-not (Test-Path $plugin)) { throw 'protoc-gen-dart not found. Run: dart pub global activate protoc_plugin' }

if (Test-Path $outDir) { Remove-Item -Recurse -Force $outDir }
New-Item -ItemType Directory -Force $outDir | Out-Null

# Run from proto/ with relative paths: protoc splits "grpc:<path>" and proto
# arguments on ':' and trips over Windows drive letters otherwise.
Push-Location $protoDir
try {
  $protos = @(Get-ChildItem -Recurse -Filter '*.proto' |
    ForEach-Object { (Resolve-Path -Relative $_.FullName).TrimStart('.', '\').Replace('\', '/') })
  & $protoc "--plugin=protoc-gen-dart=$plugin" '--dart_opt=grpc' "--dart_out=$outDir" '-I.' $protos
  if ($LASTEXITCODE -ne 0) { throw "protoc failed with exit code $LASTEXITCODE" }
} finally { Pop-Location }

Push-Location (Join-Path $root 'app')
try { dart format lib/src/gen | Out-Null } finally { Pop-Location }
Write-Host "Generated Dart code in $outDir"
