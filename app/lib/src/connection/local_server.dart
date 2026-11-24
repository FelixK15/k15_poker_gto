import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'dart:math';

import 'package:flutter/foundation.dart';

import '../api/gto_api.dart';

/// Thrown when the solver cannot be started on this computer.
class LocalServerException implements Exception {
  LocalServerException(this.message);

  final String message;

  @override
  String toString() => message;
}

/// Local solving needs the server executable, which only ships for desktop.
bool get localSolvingSupported =>
    !kIsWeb && defaultTargetPlatform == TargetPlatform.windows;

/// Starts `gto-server` as a child process and connects to it.
///
/// The server listens on loopback only, on a port the system picks, and
/// requires a token generated for this run. Its standard input is tied to
/// this process: closing it, or the app dying, shuts the server down, and the
/// server saves a running solve on the way out.
class LocalServer {
  static const _startupTimeout = Duration(seconds: 15);

  /// Where the executable is looked for, in order.
  static List<String> candidatePaths() {
    final name = Platform.isWindows ? 'gto-server.exe' : 'gto-server';
    final paths = <String>[];

    final override = Platform.environment['GTO_SERVER_PATH'];
    if (override != null && override.isNotEmpty) paths.add(override);

    // Shipped next to the app.
    final appDir = File(Platform.resolvedExecutable).parent;
    paths.add('${appDir.path}${Platform.pathSeparator}$name');

    // Running from a source checkout: the C++ build output further up.
    var dir = appDir;
    for (var i = 0; i < 10; ++i) {
      final sep = Platform.pathSeparator;
      paths.add(
        '${dir.path}${sep}build${sep}windows${sep}server${sep}Release$sep$name',
      );
      final parent = dir.parent;
      if (parent.path == dir.path) break;
      dir = parent;
    }
    return paths;
  }

  static String? findExecutable() {
    for (final path in candidatePaths()) {
      if (File(path).existsSync()) return path;
    }
    return null;
  }

  /// Where a local server keeps jobs and solutions.
  static String dataDirectory() {
    final sep = Platform.pathSeparator;
    final base =
        Platform.environment['LOCALAPPDATA'] ??
        Platform.environment['HOME'] ??
        Directory.systemTemp.path;
    return '$base${sep}K15GTO${sep}data';
  }

  static String _newToken() {
    final random = Random.secure();
    return List.generate(
      32,
      (_) => random.nextInt(256).toRadixString(16).padLeft(2, '0'),
    ).join();
  }

  /// Starts the server and returns a connection that stops it when closed.
  static Future<GtoApi> start({
    String? executable,
    String? dataDir,
    String label = 'This computer',
  }) async {
    final path = executable ?? findExecutable();
    if (path == null) {
      throw LocalServerException(
        'The solver (gto-server) was not found next to the app.',
      );
    }

    final token = _newToken();
    final Process process;
    try {
      process = await Process.start(
        path,
        [
          '--port',
          '0',
          '--data-dir',
          dataDir ?? dataDirectory(),
          '--exit-with-stdin',
        ],
        // The environment, unlike the command line, is not visible to other
        // users of the machine.
        environment: {'GTO_SERVER_TOKEN': token},
      );
    } on ProcessException catch (e) {
      throw LocalServerException(
        'The solver could not be started: ${e.message}',
      );
    }

    // Completes if the server ends without us asking it to.
    var stopping = false;
    final lost = process.exitCode.then((code) async {
      if (stopping) await Completer<void>().future;
      return 'The solver on this computer stopped unexpectedly '
          '(exit code $code). Start it again to continue; solves that were '
          'saved are still there.';
    });

    Future<void> stop() async {
      stopping = true;
      // Closing stdin asks for a clean shutdown; give it time to save.
      try {
        await process.stdin.close();
      } catch (_) {}
      await process.exitCode.timeout(
        const Duration(seconds: 20),
        onTimeout: () {
          process.kill();
          return -1;
        },
      );
    }

    try {
      final port = await _readPort(process).timeout(
        _startupTimeout,
        onTimeout: () => throw LocalServerException(
          'The solver did not start within ${_startupTimeout.inSeconds} seconds.',
        ),
      );
      return await GrpcGtoApi.connect(
        host: '127.0.0.1',
        port: port,
        token: token,
        label: label,
        isLocal: true,
        onClose: stop,
        lost: lost,
      );
    } catch (_) {
      process.kill();
      rethrow;
    }
  }

  /// Waits for the line on which the server announces its port.
  static Future<int> _readPort(Process process) {
    final port = Completer<int>();
    final errors = StringBuffer();

    process.stdout
        .transform(utf8.decoder)
        .transform(const LineSplitter())
        .listen((line) {
          final match = RegExp(r'^LISTENING (\d+)$').firstMatch(line.trim());
          if (match != null && !port.isCompleted) {
            port.complete(int.parse(match.group(1)!));
          }
        });
    // Drain stderr so the server never blocks on a full pipe; keep the last
    // of it for the error message.
    process.stderr.transform(utf8.decoder).listen((text) {
      if (errors.length < 2000) errors.write(text);
    });
    process.exitCode.then((code) {
      if (!port.isCompleted) {
        final detail = errors.toString().trim();
        port.completeError(
          LocalServerException(
            'The solver stopped right after starting'
            '${detail.isEmpty ? ' (exit code $code).' : ': $detail'}',
          ),
        );
      }
    });
    return port.future;
  }
}
