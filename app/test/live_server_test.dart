// Tests against a real gto-server. Each group is skipped unless its
// environment variable is set.
//
// A server that is already running (set GTO_SERVER_TOKEN too if it has one):
//
//   gto-server --port 50051
//   $env:GTO_SERVER_PORT = "50051"; flutter test test/live_server_test.dart
//
// Starting the server from the app, as "This computer" does:
//
//   $env:GTO_SERVER_PATH = "..\build\windows\server\Release\gto-server.exe"
//   flutter test test/live_server_test.dart
import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/connection/local_server.dart';

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

/// Estimates, solves, watches and browses the quick spot.
Future<void> solveAndBrowse(GtoApi api) async {
  final estimate = await api.estimate(quickSpot());
  expect(estimate.nodes.toInt(), 6);
  expect(estimate.canSolve, isTrue);

  final submitted = await api.submit(
    quickSpot(),
    SolveSettings(maxIterations: 1000, targetExploitabilityPct: 0),
    'dart live test',
  );
  expect(submitted.id, isNotEmpty);
  addTearDown(() async {
    try {
      await api.delete(submitted.id);
    } catch (_) {
      // The connection may already be closed.
    }
  });

  // The stream ends by itself once the job is done.
  final updates = await api.watch(submitted.id).toList();
  expect(updates, isNotEmpty);
  final done = updates.last;
  expect(done.state, JobState.JOB_STATE_DONE);
  expect(done.iteration, 1000);
  expect(done.oopEv, closeTo(70, 0.2));
  expect(done.ipEv, closeTo(30, 0.2));

  final node = await api.getNode(submitted.id, 'check');
  expect(node.type, NodeType.NODE_TYPE_ACTION);
  expect(node.player, Player.PLAYER_IP);
  expect(node.actions.map((a) => a.label), ['check', 'allin']);
  expect(node.actions[1].frequency, closeTo(0.3, 0.01));
  expect(node.ip.hands, hasLength(15));
  expect(node.strategy, hasLength(30));

  expect((await api.listJobs()).map((j) => j.id), contains(submitted.id));
  expect((await api.getJob(submitted.id)).state, JobState.JOB_STATE_DONE);

  await expectLater(
    api.getNode(submitted.id, 'check/bet50'),
    throwsA(
      isA<GrpcError>().having((e) => e.code, 'code', StatusCode.notFound),
    ),
  );
}

void main() {
  final environment = Platform.environment;
  final port = int.tryParse(environment['GTO_SERVER_PORT'] ?? '');
  final host = environment['GTO_SERVER_HOST'] ?? '127.0.0.1';
  final token = environment['GTO_SERVER_TOKEN'] ?? '';
  final serverPath = environment['GTO_SERVER_PATH'] ?? '';

  group('a running server', () {
    test('describes itself and solves a spot', () async {
      final api = await GrpcGtoApi.connect(
        host: host,
        port: port!,
        token: token,
      );
      addTearDown(api.close);

      expect(api.info.apiMajor, clientApiMajor);
      expect(api.info.serverVersion, isNotEmpty);
      expect(api.info.os, anyOf('windows', 'linux'));
      expect(api.info.logicalCores, greaterThan(0));
      expect(api.label, '$host:$port');
      expect(api.isLocal, isFalse);

      await solveAndBrowse(api);
    });

    test('turns away a wrong token', () async {
      await expectLater(
        GrpcGtoApi.connect(host: host, port: port!, token: 'not-the-token'),
        throwsA(
          isA<GrpcError>().having(
            (e) => e.code,
            'code',
            StatusCode.unauthenticated,
          ),
        ),
      );
    }, skip: token.isEmpty ? 'the server has no token' : false);
  }, skip: port == null ? 'GTO_SERVER_PORT not set' : false);

  group('a server started by the app', () {
    late Directory dataDir;
    setUp(() => dataDir = Directory.systemTemp.createTempSync('gto-local-'));
    tearDown(() {
      try {
        dataDir.deleteSync(recursive: true);
      } catch (_) {}
    });

    test('starts, solves, and stops when closed', () async {
      final api = await LocalServer.start(
        executable: serverPath,
        dataDir: dataDir.path,
      );
      expect(api.isLocal, isTrue);
      expect(api.label, 'This computer');

      await solveAndBrowse(api);

      // Closing waits for the server process to end.
      final stopwatch = Stopwatch()..start();
      await api.close();
      expect(stopwatch.elapsed, lessThan(const Duration(seconds: 15)));
      // It saved its jobs on the way out.
      expect(dataDir.listSync().any((f) => f.path.endsWith('.job')), isTrue);
    });

    test('keeps an interrupted solve for the next start', () async {
      // A turn spot that would run for a long time.
      final slow = Spot(
        board: 'Ks Ts 7h 4h',
        oopRange: 'TT,77,44,AKo,KQs,KJs,QJs,J9s,98s,A5s,65s',
        ipRange: 'AA,KK,KTs,AQs,QTs,JTs,T9s,87s,A4s,99,88',
        pot: 100,
        effectiveStack: 300,
        tree: TreeSettings(
          turn: StreetSizing(
            oop: BetSizing(betPct: [66], raisePct: [75]),
            ip: BetSizing(betPct: [66], raisePct: [75]),
          ),
          river: StreetSizing(
            oop: BetSizing(betPct: [66], raisePct: [75]),
            ip: BetSizing(betPct: [66], raisePct: [75]),
          ),
          raiseCap: 1,
        ),
      );

      var api = await LocalServer.start(
        executable: serverPath,
        dataDir: dataDir.path,
      );
      final job = await api.submit(
        slow,
        SolveSettings(maxIterations: 1000000, targetExploitabilityPct: 0),
        'interrupted',
      );
      // Let it get going, then close the app's end.
      while ((await api.getJob(job.id)).iteration < 5) {
        await Future<void>.delayed(const Duration(milliseconds: 20));
      }
      await api.close();

      api = await LocalServer.start(
        executable: serverPath,
        dataDir: dataDir.path,
      );
      addTearDown(api.close);
      final restored = await api.getJob(job.id);
      expect(restored.state, JobState.JOB_STATE_PAUSED);
      expect(restored.iteration, greaterThanOrEqualTo(5));
      expect(restored.hasSolution, isTrue);
      // The solution it had reached is there to browse.
      final node = await api.getNode(job.id, '');
      expect(node.iteration, restored.iteration);
      await api.delete(job.id);
    });

    test('reports a missing executable', () async {
      await expectLater(
        LocalServer.start(
          executable: '${dataDir.path}${Platform.pathSeparator}nope.exe',
          dataDir: dataDir.path,
        ),
        throwsA(isA<LocalServerException>()),
      );
    });
  }, skip: serverPath.isEmpty ? 'GTO_SERVER_PATH not set' : false);
}
