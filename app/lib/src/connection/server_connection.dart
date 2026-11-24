import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import 'local_server.dart';

/// A server the user asked to connect to.
class RemoteTarget {
  const RemoteTarget({required this.host, required this.port, this.token = ''});

  final String host;
  final int port;
  final String token;
}

typedef RemoteConnector = Future<GtoApi> Function(RemoteTarget target);
typedef LocalConnector = Future<GtoApi> Function();

/// How the app reaches a remote server. Overridden in tests.
final remoteConnectorProvider = Provider<RemoteConnector>(
  (ref) =>
      (target) => GrpcGtoApi.connect(
        host: target.host,
        port: target.port,
        token: target.token,
      ),
);

/// How the app starts a server on this computer. Overridden in tests.
final localConnectorProvider = Provider<LocalConnector>(
  (ref) => LocalServer.start,
);

/// The server the app is connected to, or null on the connect screen.
class ConnectionNotifier extends Notifier<GtoApi?> {
  @override
  GtoApi? build() => null;

  void connected(GtoApi api) => state = api;

  Future<void> disconnect() async {
    final api = state;
    state = null;
    await api?.close();
  }
}

final connectionProvider = NotifierProvider<ConnectionNotifier, GtoApi?>(
  ConnectionNotifier.new,
);
