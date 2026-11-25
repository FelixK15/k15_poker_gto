import 'dart:convert';

import 'package:flutter_riverpod/flutter_riverpod.dart';

/// A server whose address and credentials were compiled into this build, so
/// that a private build connects without typing anything in.
///
/// Set with `--dart-define-from-file` (see tools/server.defines.example.json):
/// GTO_BUILTIN_HOST, GTO_BUILTIN_PORT, GTO_BUILTIN_TOKEN, GTO_BUILTIN_TLS and
/// GTO_BUILTIN_CERT (the server's PEM certificate, base64-encoded).
///
/// Anyone who has such a build can read the token out of it. Only make one
/// for yourself, and never commit the defines file.
class BuiltInServer {
  const BuiltInServer({
    required this.host,
    this.port = 50051,
    this.token = '',
    this.tls = false,
    this.certificate,
  });

  final String host;
  final int port;
  final String token;
  final bool tls;

  /// The server's own certificate as PEM bytes, for a self-signed one.
  final List<int>? certificate;

  /// The server compiled into this build, or null when there is none.
  static BuiltInServer? fromEnvironment() {
    const host = String.fromEnvironment('GTO_BUILTIN_HOST');
    const port = int.fromEnvironment('GTO_BUILTIN_PORT', defaultValue: 50051);
    const token = String.fromEnvironment('GTO_BUILTIN_TOKEN');
    const tls = bool.fromEnvironment('GTO_BUILTIN_TLS');
    const certificate = String.fromEnvironment('GTO_BUILTIN_CERT');
    if (host.isEmpty) return null;
    return BuiltInServer(
      host: host,
      port: port,
      token: token,
      tls: tls || certificate.isNotEmpty,
      certificate: certificate.isEmpty ? null : base64.decode(certificate),
    );
  }
}

/// Overridden in tests.
final builtInServerProvider = Provider<BuiltInServer?>(
  (ref) => BuiltInServer.fromEnvironment(),
);
