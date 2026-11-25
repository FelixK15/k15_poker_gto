import 'dart:io';

import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import 'local_server.dart';

/// A server the user asked to connect to.
class RemoteTarget {
  const RemoteTarget({
    required this.host,
    required this.port,
    this.token = '',
    this.tls = false,
    this.certificatePath = '',
    this.certificate,
  });

  final String host;
  final int port;
  final String token;

  /// Whether to encrypt the connection.
  final bool tls;

  /// With [tls]: a PEM file holding the server's own certificate, for servers
  /// whose certificate no public authority signed. Empty to rely on the
  /// system's trusted authorities.
  final String certificatePath;

  /// With [tls]: that certificate's PEM bytes when the app already has them.
  /// Used when [certificatePath] is empty.
  final List<int>? certificate;
}

typedef RemoteConnector = Future<GtoApi> Function(RemoteTarget target);
typedef LocalConnector = Future<GtoApi> Function();

/// How the app reaches a remote server. Overridden in tests.
final remoteConnectorProvider = Provider<RemoteConnector>(
  (ref) => (target) async {
    var certificate = target.tls ? target.certificate : null;
    if (target.tls && target.certificatePath.isNotEmpty) {
      try {
        certificate = await File(target.certificatePath).readAsBytes();
      } on FileSystemException {
        throw CertificateFileException(target.certificatePath);
      }
    }
    return GrpcGtoApi.connect(
      host: target.host,
      port: target.port,
      token: target.token,
      tls: target.tls,
      trustedCertificate: certificate,
    );
  },
);

/// Thrown when the certificate file the user named cannot be read.
class CertificateFileException implements Exception {
  CertificateFileException(this.path);

  final String path;

  @override
  String toString() => 'The certificate file "$path" cannot be read.';
}

/// How the app starts a server on this computer. Overridden in tests.
final localConnectorProvider = Provider<LocalConnector>(
  (ref) => LocalServer.start,
);

/// The server the app is connected to, or null on the connect screen.
class ConnectionNotifier extends Notifier<GtoApi?> {
  @override
  GtoApi? build() => null;

  void connected(GtoApi api) {
    state = api;
    ref.read(connectionLostProvider.notifier).clear();
    // A server we started may stop on its own; do not leave the user on
    // screens that can no longer do anything.
    api.lost?.then((message) {
      if (!identical(state, api)) return;
      state = null;
      ref.read(connectionLostProvider.notifier).set(message);
    });
  }

  Future<void> disconnect() async {
    final api = state;
    state = null;
    await api?.close();
  }
}

final connectionProvider = NotifierProvider<ConnectionNotifier, GtoApi?>(
  ConnectionNotifier.new,
);

/// Why the last connection ended, if it ended on its own.
class ConnectionLostNotifier extends Notifier<String?> {
  @override
  String? build() => null;

  void set(String message) => state = message;
  void clear() => state = null;
}

final connectionLostProvider =
    NotifierProvider<ConnectionLostNotifier, String?>(
      ConnectionLostNotifier.new,
    );
