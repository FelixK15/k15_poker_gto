# K15 GTO

A GTO solver for No-Limit Hold'em: a headless C++ solver server (Windows, Linux) and a Flutter
client (Windows, Android). See [PLAN.md](PLAN.md) for the design and milestones.

## Layout

| Path | Contents |
|---|---|
| `proto/` | gRPC API contract, the source of truth for client and server |
| `engine/` | `libsolver`, the solver engine, with unit tests |
| `server/` | `gto-server`, the gRPC server |
| `cli/` | `gto-cli`, runs the engine without a server |
| `app/` | Flutter client |
| `examples/` | Spot files for `gto-cli` |
| `tools/` | Code generation scripts |

## Building the server

Needs CMake 3.25+, a C++20 compiler (Visual Studio 2022 or GCC/Clang) and a current
[vcpkg](https://github.com/microsoft/vcpkg) checkout. The vcpkg bundled with Visual Studio is too
old. The first configure builds gRPC and takes a while; later ones reuse vcpkg's binary cache.

The build targets processors with AVX2 (Intel from 2013, AMD from 2015). Add
`-DGTO_ENABLE_AVX2=OFF` to the configure step for older ones, at some cost in speed.

Windows (PowerShell):

```powershell
$env:VCPKG_ROOT = "D:\repos\vcpkg"
cmake --preset windows
cmake --build --preset windows
ctest --preset windows
```

Linux (also needs Ninja):

```sh
export VCPKG_ROOT=~/vcpkg
cmake --preset linux
cmake --build --preset linux
ctest --preset linux
```

## Running the server

```powershell
build\windows\server\Release\gto-server.exe --port 50051
```

The server solves one job at a time and keeps jobs and their solutions in `./gto-data`
(`--data-dir` changes that), so they survive a restart. `--help` lists all options.

It listens on `127.0.0.1` unless `--host` says otherwise. To accept clients from other machines it
needs an access token, which clients send with every call:

```powershell
$env:GTO_SERVER_TOKEN = "choose-a-long-random-string"
build\windows\server\Release\gto-server.exe --host 0.0.0.0 --port 50051
```

To encrypt connections, give the server a certificate and its key; clients then switch on
"Encrypted connection (TLS)":

```powershell
build\windows\server\Release\gto-server.exe --host 0.0.0.0 --tls-cert server.pem --tls-key server.key
```

With a certificate you made yourself (self-signed), the client also needs that `.pem` file and asks
for its path. The certificate must be for the name or address clients connect to. Without TLS,
reach a server on a network you do not trust through a VPN or an SSH tunnel.

`docker/Dockerfile` builds the server for Linux. It has not been built yet.

The API is defined in `proto/gto/v1/`: `TreeService` (size estimate), `JobService` (submit, list,
pause, resume, cancel, delete, watch), `SolutionService` (browse a solution node by node) and
`LockService` (re-solve under locks).

## Solving a spot from the command line

`gto-cli` runs the engine without a server. A spot is a JSON file with the board (three, four or
five cards), both ranges, pot, stack and bet sizes per street; see [examples/](examples/).

```powershell
build\windows\cli\Release\gto-cli.exe solve examples\turn_srp.json --node check --node check/check/2c
```

It prints the tree size and memory, the convergence, both players' EV and a strategy table per hand
class for the root and for each `--node` line. A line is a `/`-separated list of actions and dealt
cards, e.g. `bet5/call/Ah/check`. If a step does not exist, the error lists what is available
there. `--threads <n>` limits the threads used (default: all).

Flop trees grow quickly with ranges and bet sizes; the tool prints the memory it needs before it
starts. `examples/flop_3bet.json` is a small one.

## Running the app

```powershell
cd app
flutter run -d windows
```

On the first screen choose **This computer** to solve locally, or **A server** to connect to a
`gto-server` elsewhere (host, port and, if it has one, its access token).

Solving locally starts `gto-server` in the background. The app looks for it next to its own
executable, then in the C++ build output of this checkout (`build\windows\server\Release`), so
build the server first. `GTO_SERVER_PATH` overrides the search. Local jobs and solutions are kept
in `%LOCALAPPDATA%\K15GTO\data`; the solver stops when the app closes and saves a running solve,
which shows up as paused the next time.

When setting up a solve, the **Preflop** row fills in both ranges, the pot and the stacks for a
line such as "BTN opens, BB calls" at 6-max or 9-max and 100, 200 or 300BB. Those ranges are rough,
rule-based starting points, not solver output: edit them on the grid (the button next to each
range), paste your own in PioSOLVER or GTO+ text form, and save the ones you want to keep.

In a solution, **Lock this decision…** fixes how the player to act plays there, for the whole
range or for the hand class picked in the grid. **Re-solve with these locks** then queues a new
solve that starts from this one and finds the best play around the locks; the original stays, and
the new one shows what changed against it.

Tests that need a real server are skipped by default; `app/test/live_server_test.dart` says how to
run them. `app/test/screenshots_test.dart` renders the main screens to PNG files.

## Packaging

```powershell
$env:VCPKG_ROOT = "D:\repos\vcpkg"
tools\package_windows.ps1
```

builds and tests everything and writes two zip files to `dist\`: the app with the solver inside,
and the solver alone for use as a server. For Android, `flutter build apk` in `app` builds the
client; it connects to a server and cannot solve by itself.

The **Release** GitHub workflow (`.github/workflows/release.yml`) builds all three packages (the
two Windows zips and the APK) and publishes them as a GitHub release. Push a tag that matches the
version in `CMakeLists.txt` (`git tag v0.1.0`, then `git push origin v0.1.0`), or run the workflow
by hand from the Actions tab, which tags the commit itself. Release builds carry no
`server.defines.json`.

## Changing the API

Edit the `.proto` files, then regenerate the Dart code (C++ code is generated by CMake):

```powershell
dart pub global activate protoc_plugin
tools\gen_dart.ps1
```

The generated Dart files in `app/lib/src/gen` are committed.
