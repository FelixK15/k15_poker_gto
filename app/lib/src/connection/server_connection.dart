import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:grpc/grpc.dart';

import '../gen/gto/v1/server.pbgrpc.dart';

/// API contract major version this client speaks. See proto/gto/v1/server.proto.
const int clientApiMajor = 1;

/// Thrown when a server answers but speaks a different API major version.
class IncompatibleServerException implements Exception {
  IncompatibleServerException(this.serverApiMajor);

  final int serverApiMajor;

  @override
  String toString() =>
      'Server speaks API version $serverApiMajor, this app needs version $clientApiMajor.';
}

/// Asks the server at [host]:[port] who it is.
typedef ServerInfoFetcher = Future<GetInfoResponse> Function(
  String host,
  int port,
);

/// Overridden in tests.
final serverInfoFetcherProvider = Provider<ServerInfoFetcher>(
  (ref) => fetchServerInfo,
);

/// Opens a channel, calls GetInfo and closes the channel again.
///
/// Transport is plaintext for now; TLS and token auth arrive with the server
/// milestone (M3). A long-lived connection replaces this once there are jobs
/// to watch.
Future<GetInfoResponse> fetchServerInfo(String host, int port) async {
  final channel = ClientChannel(
    host,
    port: port,
    options: const ChannelOptions(
      credentials: ChannelCredentials.insecure(),
      connectTimeout: Duration(seconds: 5),
    ),
  );
  try {
    final client = ServerServiceClient(channel);
    final info = await client.getInfo(
      GetInfoRequest(),
      options: CallOptions(timeout: const Duration(seconds: 5)),
    );
    if (info.apiMajor != clientApiMajor) {
      throw IncompatibleServerException(info.apiMajor);
    }
    return info;
  } finally {
    await channel.shutdown();
  }
}

/// Turns a connection failure into a sentence a user can act on.
String describeConnectionError(Object error) {
  if (error is IncompatibleServerException) return error.toString();
  if (error is GrpcError) {
    switch (error.code) {
      case StatusCode.unavailable:
        return 'No server reachable at that address.';
      case StatusCode.deadlineExceeded:
        return 'The server did not answer in time.';
      case StatusCode.unimplemented:
        return 'Something answered, but it is not a K15 GTO server.';
      default:
        return 'Server error: ${error.message ?? error.codeName}';
    }
  }
  return 'Could not connect: $error';
}
