import 'package:grpc/grpc.dart';

import '../gen/gto/v1/common.pb.dart';
import '../gen/gto/v1/jobs.pbgrpc.dart';
import '../gen/gto/v1/locks.pbgrpc.dart';
import '../gen/gto/v1/server.pbgrpc.dart';
import '../gen/gto/v1/solutions.pbgrpc.dart';
import '../gen/gto/v1/trees.pbgrpc.dart';

export '../gen/gto/v1/common.pb.dart';
export '../gen/gto/v1/jobs.pb.dart';
export '../gen/gto/v1/locks.pb.dart';
export '../gen/gto/v1/server.pb.dart';
export '../gen/gto/v1/solutions.pb.dart';
export '../gen/gto/v1/trees.pb.dart';

/// API contract major version this client speaks. See proto/gto/v1/server.proto.
const int clientApiMajor = 1;

/// Everything the app asks of a solver server. The screens depend on this
/// interface only, so tests can hand them a fake.
abstract class GtoApi {
  /// Where this server is, for display: "This computer" or "host:port".
  String get label;

  /// Whether the app started this server itself.
  bool get isLocal;

  /// What the server said about itself when connecting.
  GetInfoResponse get info;

  Future<EstimateTreeResponse> estimate(Spot spot);

  Future<Job> submit(Spot spot, SolveSettings settings, String name);
  Future<List<Job>> listJobs();
  Future<Job> getJob(String id);
  Future<Job> pause(String id);
  Future<Job> resume(String id);
  Future<Job> cancel(String id);
  Future<void> delete(String id);

  /// The job now and after every change, until it ends.
  Stream<Job> watch(String id);

  Future<GetNodeResponse> getNode(String jobId, String path);

  /// How each card that can come at the chance node [path] changes the spot.
  Future<List<Runout>> getRunouts(String jobId, String path);

  /// Completes with a message if the connection ends on its own, which so
  /// far only a server the app started can report. Null if that cannot be
  /// noticed.
  Future<String>? get lost;

  /// Queues a re-solve of [parentId] with [locks] in force, as a new job.
  /// [settings] counts iterations on top of the parent's.
  Future<Job> resolve(
    String parentId,
    List<NodeLock> locks,
    SolveSettings settings,
  );

  /// Ends the connection. A server the app started is shut down.
  Future<void> close();
}

/// Thrown when a server answers but speaks a different API major version.
class IncompatibleServerException implements Exception {
  IncompatibleServerException(this.serverApiMajor);

  final int serverApiMajor;

  @override
  String toString() =>
      'Server speaks API version $serverApiMajor, this app needs version $clientApiMajor.';
}

/// [GtoApi] over gRPC.
class GrpcGtoApi implements GtoApi {
  GrpcGtoApi._(
    this._channel,
    this._token,
    this.label,
    this.isLocal,
    this._onClose,
    this.lost,
  ) : _server = ServerServiceClient(_channel),
      _trees = TreeServiceClient(_channel),
      _jobs = JobServiceClient(_channel),
      _solutions = SolutionServiceClient(_channel),
      _locks = LockServiceClient(_channel);

  /// Opens a channel and checks that a compatible server answers with this
  /// token. [onClose] runs after the channel is shut down.
  static Future<GrpcGtoApi> connect({
    required String host,
    required int port,
    String token = '',
    String? label,
    bool isLocal = false,
    bool tls = false,
    List<int>? trustedCertificate,
    Future<void> Function()? onClose,
    Future<String>? lost,
  }) async {
    final channel = ClientChannel(
      host,
      port: port,
      options: ChannelOptions(
        // With TLS the server must present a certificate for [host] that the
        // system trusts, or exactly [trustedCertificate] for a self-signed one.
        credentials: tls
            ? ChannelCredentials.secure(certificates: trustedCertificate)
            : const ChannelCredentials.insecure(),
        connectTimeout: const Duration(seconds: 5),
      ),
    );
    final api = GrpcGtoApi._(
      channel,
      token,
      label ?? '$host:$port',
      isLocal,
      onClose,
      lost,
    );
    try {
      api.info = await api._server.getInfo(
        GetInfoRequest(),
        options: api._options(const Duration(seconds: 5)),
      );
      if (api.info.apiMajor != clientApiMajor) {
        throw IncompatibleServerException(api.info.apiMajor);
      }
      return api;
    } catch (_) {
      // The caller keeps whatever [onClose] would have cleaned up.
      await channel.shutdown();
      rethrow;
    }
  }

  final ClientChannel _channel;
  final String _token;
  final Future<void> Function()? _onClose;
  final ServerServiceClient _server;
  final TreeServiceClient _trees;
  final JobServiceClient _jobs;
  final SolutionServiceClient _solutions;
  final LockServiceClient _locks;

  @override
  final String label;

  @override
  final bool isLocal;

  @override
  late final GetInfoResponse info;

  @override
  final Future<String>? lost;

  CallOptions _options([Duration? timeout = const Duration(seconds: 30)]) =>
      CallOptions(
        timeout: timeout,
        metadata: _token.isEmpty ? null : {'authorization': 'Bearer $_token'},
      );

  @override
  Future<EstimateTreeResponse> estimate(Spot spot) =>
      _trees.estimateTree(EstimateTreeRequest(spot: spot), options: _options());

  @override
  Future<Job> submit(Spot spot, SolveSettings settings, String name) =>
      _jobs.submit(
        SubmitJobRequest(spot: spot, settings: settings, name: name),
        options: _options(),
      );

  @override
  Future<List<Job>> listJobs() async =>
      (await _jobs.list(ListJobsRequest(), options: _options())).jobs;

  @override
  Future<Job> getJob(String id) =>
      _jobs.get(GetJobRequest(id: id), options: _options());

  @override
  Future<Job> pause(String id) =>
      _jobs.pause(PauseJobRequest(id: id), options: _options());

  @override
  Future<Job> resume(String id) =>
      _jobs.resume(ResumeJobRequest(id: id), options: _options());

  @override
  Future<Job> cancel(String id) =>
      _jobs.cancel(CancelJobRequest(id: id), options: _options());

  @override
  Future<void> delete(String id) async {
    await _jobs.delete(DeleteJobRequest(id: id), options: _options());
  }

  // A solve can run for hours, so the stream has no deadline.
  @override
  Stream<Job> watch(String id) =>
      _jobs.watch(WatchJobRequest(id: id), options: _options(null));

  // Reading back a large solution from disk can take a while.
  @override
  Future<GetNodeResponse> getNode(String jobId, String path) =>
      _solutions.getNode(
        GetNodeRequest(jobId: jobId, path: path),
        options: _options(const Duration(minutes: 5)),
      );

  @override
  Future<List<Runout>> getRunouts(String jobId, String path) async =>
      (await _solutions.getRunoutReport(
        GetRunoutReportRequest(jobId: jobId, path: path),
        options: _options(const Duration(minutes: 5)),
      )).runouts;

  @override
  Future<Job> resolve(
    String parentId,
    List<NodeLock> locks,
    SolveSettings settings,
  ) => _locks.resolve(
    ResolveRequest(parentJobId: parentId, locks: locks, settings: settings),
    options: _options(const Duration(minutes: 5)),
  );

  @override
  Future<void> close() async {
    await _channel.shutdown();
    await _onClose?.call();
  }
}

/// Turns a failed call into a sentence a user can act on.
String describeError(Object error) {
  if (error is IncompatibleServerException) return error.toString();
  if (error is GrpcError) {
    final message = error.message ?? '';
    switch (error.code) {
      case StatusCode.unavailable:
        // A failed TLS handshake also ends up here.
        return message.toLowerCase().contains('certificate') ||
                message.toLowerCase().contains('handshake')
            ? 'The server\'s certificate was not accepted.'
            : 'No server reachable at that address.';
      case StatusCode.deadlineExceeded:
        return 'The server did not answer in time.';
      case StatusCode.unimplemented:
        return 'Something answered, but it is not a K15 GTO server.';
      case StatusCode.unauthenticated:
        return message.contains('wrong')
            ? 'The access token is wrong.'
            : 'This server needs an access token.';
      case StatusCode.invalidArgument:
      case StatusCode.notFound:
      case StatusCode.failedPrecondition:
      case StatusCode.resourceExhausted:
        // The server words these for the user already.
        return message.isEmpty ? error.codeName : _sentence(message);
      default:
        return 'Server error: ${message.isEmpty ? error.codeName : message}';
    }
  }
  return 'Something went wrong: $error';
}

String _sentence(String text) {
  final capitalised = text[0].toUpperCase() + text.substring(1);
  return capitalised.endsWith('.') ? capitalised : '$capitalised.';
}
