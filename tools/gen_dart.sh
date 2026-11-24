#!/usr/bin/env bash
# Regenerates the Dart gRPC code in app/lib/src/gen from proto/.
#
# Needs:
#   - protoc: taken from vcpkg_installed (built with the C++ dependencies) or PATH
#   - protoc-gen-dart: dart pub global activate protoc_plugin (and ~/.pub-cache/bin on PATH)
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
proto_dir="$root/proto"
out_dir="$root/app/lib/src/gen"

protoc="$(find "$root/vcpkg_installed" -type f -name protoc -perm -u+x 2>/dev/null | head -n 1 || true)"
if [ -z "$protoc" ]; then protoc="$(command -v protoc || true)"; fi
if [ -z "$protoc" ]; then
  echo "protoc not found. Build the C++ project once or put protoc on PATH." >&2
  exit 1
fi

export PATH="$PATH:$HOME/.pub-cache/bin"
if ! command -v protoc-gen-dart >/dev/null; then
  echo "protoc-gen-dart not found. Run: dart pub global activate protoc_plugin" >&2
  exit 1
fi

rm -rf "$out_dir"
mkdir -p "$out_dir"
find "$proto_dir" -name '*.proto' -print0 | xargs -0 "$protoc" --dart_out="grpc:$out_dir" -I"$proto_dir"

(cd "$root/app" && dart format lib/src/gen >/dev/null)
echo "Generated Dart code in $out_dir"
