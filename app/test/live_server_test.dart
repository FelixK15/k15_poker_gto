// Talks to a real gto-server. Skipped unless GTO_SERVER_PORT is set:
//
//   gto-server --port 50051
//   $env:GTO_SERVER_PORT = "50051"; flutter test test/live_server_test.dart
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/connection/server_connection.dart';

void main() {
  final port = int.tryParse(Platform.environment['GTO_SERVER_PORT'] ?? '');
  final host = Platform.environment['GTO_SERVER_HOST'] ?? '127.0.0.1';

  test(
    'GetInfo against a running server',
    () async {
      final info = await fetchServerInfo(host, port!);
      expect(info.apiMajor, clientApiMajor);
      expect(info.serverVersion, isNotEmpty);
      expect(info.engineVersion, isNotEmpty);
      expect(info.os, anyOf('windows', 'linux'));
      expect(info.logicalCores, greaterThan(0));
      expect(info.totalMemoryBytes.toInt(), greaterThan(0));
      expect(
        info.availableMemoryBytes.toInt(),
        lessThanOrEqualTo(info.totalMemoryBytes.toInt()),
      );
    },
    skip: port == null ? 'GTO_SERVER_PORT not set' : false,
  );
}
