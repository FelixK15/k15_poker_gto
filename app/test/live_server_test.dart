// Talks to a real gto-server. Skipped unless GTO_SERVER_PORT is set:
//
//   gto-server --port 50051
//   $env:GTO_SERVER_PORT = "50051"; flutter test test/live_server_test.dart
//
// Set GTO_SERVER_TOKEN as well if the server was started with a token.
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/connection/server_connection.dart';
import 'package:k15_gto/src/gen/gto/v1/common.pb.dart';
import 'package:k15_gto/src/gen/gto/v1/jobs.pbgrpc.dart';
import 'package:k15_gto/src/gen/gto/v1/solutions.pbgrpc.dart';
import 'package:k15_gto/src/gen/gto/v1/trees.pbgrpc.dart';

void main() {
  final port = int.tryParse(Platform.environment['GTO_SERVER_PORT'] ?? '');
  final host = Platform.environment['GTO_SERVER_HOST'] ?? '127.0.0.1';
  final token = Platform.environment['GTO_SERVER_TOKEN'] ?? '';
  final skip = port == null ? 'GTO_SERVER_PORT not set' : false;

  CallOptions options({bool withToken = true}) => CallOptions(
    timeout: const Duration(seconds: 30),
    metadata: withToken && token.isNotEmpty
        ? {'authorization': 'Bearer $token'}
        : null,
  );

  ClientChannel open() => ClientChannel(
    host,
    port: port!,
    options: const ChannelOptions(credentials: ChannelCredentials.insecure()),
  );

  // Nuts or air against bluff-catchers; the answer is known exactly.
  Spot quickSpot() => Spot(
    board: 'Kc 8d 5h 3s 2d',
    oopRange: '88',
    ipRange: 'KK, QJo',
    pot: 100,
    effectiveStack: 100,
    tree: TreeSettings(
      river: StreetSizing(ip: BetSizing(betPct: [100])),
    ),
  );

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
    // fetchServerInfo cannot send a token yet; that arrives with the client
    // milestone.
    skip: port == null
        ? skip
        : (token.isNotEmpty ? 'server needs a token' : false),
  );

  test('estimate, solve, watch and browse a spot', () async {
    final channel = open();
    addTearDown(channel.shutdown);
    final trees = TreeServiceClient(channel);
    final jobs = JobServiceClient(channel);
    final solutions = SolutionServiceClient(channel);

    final estimate = await trees.estimateTree(
      EstimateTreeRequest(spot: quickSpot()),
      options: options(),
    );
    expect(estimate.nodes.toInt(), 6);
    expect(estimate.canSolve, isTrue);

    final submitted = await jobs.submit(
      SubmitJobRequest(
        spot: quickSpot(),
        name: 'dart live test',
        settings: SolveSettings(
          maxIterations: 1000,
          targetExploitabilityPct: 0,
        ),
      ),
      options: options(),
    );
    expect(submitted.id, isNotEmpty);
    addTearDown(
      () => jobs.delete(DeleteJobRequest(id: submitted.id), options: options()),
    );

    // The stream ends by itself once the job is done.
    final updates = await jobs
        .watch(WatchJobRequest(id: submitted.id), options: options())
        .toList();
    expect(updates, isNotEmpty);
    final done = updates.last;
    expect(done.state, JobState.JOB_STATE_DONE);
    expect(done.iteration, 1000);
    expect(done.oopEv, closeTo(70, 0.2));
    expect(done.ipEv, closeTo(30, 0.2));

    final node = await solutions.getNode(
      GetNodeRequest(jobId: submitted.id, path: 'check'),
      options: options(),
    );
    expect(node.type, NodeType.NODE_TYPE_ACTION);
    expect(node.player, Player.PLAYER_IP);
    expect(node.actions.map((a) => a.label), ['check', 'allin']);
    expect(node.actions[1].frequency, closeTo(0.3, 0.01));
    expect(node.ip.hands, hasLength(15));
    expect(node.strategy, hasLength(30));

    final listed = await jobs.list(ListJobsRequest(), options: options());
    expect(listed.jobs.map((j) => j.id), contains(submitted.id));
  }, skip: skip);

  test(
    'a server with a token turns away calls without it',
    () async {
      final channel = open();
      addTearDown(channel.shutdown);
      final jobs = JobServiceClient(channel);
      await expectLater(
        jobs.list(ListJobsRequest(), options: options(withToken: false)),
        throwsA(
          isA<GrpcError>().having(
            (e) => e.code,
            'code',
            StatusCode.unauthenticated,
          ),
        ),
      );
    },
    skip: port == null ? skip : (token.isEmpty ? 'server has no token' : false),
  );
}
