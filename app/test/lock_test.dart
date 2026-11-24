import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/jobs/jobs_screen.dart';
import 'package:k15_gto/src/solution/lock_dialog.dart';
import 'package:k15_gto/src/solution/matrix_model.dart';
import 'package:k15_gto/src/solution/solution_screen.dart';

import 'fake_api.dart';

NodeLock lockOf(String path, Map<String, List<double>> hands) => NodeLock(
  path: path,
  hands: [
    for (final entry in hands.entries)
      HandLock(hand: entry.key, frequencies: entry.value),
  ],
);

Future<({FakeApi api, List<Job> resolved})> pumpSolution(
  WidgetTester tester, {
  Job? job,
  void Function(FakeApi api)? setUp,
}) async {
  tester.view.physicalSize = const Size(1400, 1000);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);

  final api = FakeApi();
  addSampleSolution(api);
  final shown = job ?? sampleJob();
  api.jobs.add(shown);
  setUp?.call(api);
  final resolved = <Job>[];
  await tester.pumpWidget(
    MaterialApp(
      home: Navigator(
        onGenerateRoute: (_) => MaterialPageRoute<void>(
          builder: (_) =>
              SolutionScreen(api: api, job: shown, onResolved: resolved.add),
        ),
      ),
    ),
  );
  await tester.pumpAndSettle();
  return (api: api, resolved: resolved);
}

Future<void> tapCell(WidgetTester tester, String hand) async {
  final index = MatrixModel.cellIndex(hand)!;
  final box = tester.getRect(find.byKey(const Key('matrix')));
  final cell = box.width / 13;
  await tester.tapAt(
    box.topLeft + Offset((index % 13 + 0.5) * cell, (index ~/ 13 + 0.5) * cell),
  );
  await tester.pumpAndSettle();
}

void main() {
  group('mergeLock', () {
    test('adds a lock on a new node', () {
      final locks = mergeLock(
        [
          lockOf('check', {
            'AhKh': [1, 0],
          }),
        ],
        lockOf('bet5', {
          'KsKh': [0, 1],
        }),
      );
      expect(locks.map((l) => l.path), ['check', 'bet5']);
    });

    test('replaces the named hands of a lock on the same node', () {
      final locks = mergeLock(
        [
          lockOf('check', {
            'AhKh': [1, 0],
            'AsKs': [1, 0],
          }),
        ],
        lockOf('check', {
          'AsKs': [0, 1],
          'QsQh': [0.5, 0.5],
        }),
      );
      expect(locks, hasLength(1));
      final byHand = {
        for (final h in locks.single.hands) h.hand: h.frequencies,
      };
      expect(byHand, {
        'AhKh': [1.0, 0.0],
        'AsKs': [0.0, 1.0],
        'QsQh': [0.5, 0.5],
      });
    });

    test('sameLocks ignores order', () {
      final a = [
        lockOf('check', {
          'AhKh': [1, 0],
          'AsKs': [0, 1],
        }),
        lockOf('bet5', {
          'KsKh': [0, 1],
        }),
      ];
      final b = [
        lockOf('bet5', {
          'KsKh': [0, 1],
        }),
        lockOf('check', {
          'AsKs': [0, 1],
          'AhKh': [1, 0],
        }),
      ];
      expect(sameLocks(a, b), isTrue);
      expect(sameLocks(a, [b.first]), isFalse);
      expect(
        sameLocks(a, [
          b.first,
          lockOf('check', {
            'AsKs': [0, 1],
            'AhKh': [0.9, 0.1],
          }),
        ]),
        isFalse,
      );
      expect(sameLocks(const [], const []), isTrue);
    });
  });

  testWidgets('locks the whole range and re-solves', (tester) async {
    final setup = await pumpSolution(tester);
    final api = setup.api;
    expect(find.byKey(const Key('locks-card')), findsNothing);

    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    expect(find.text('Lock OOP\'s strategy here'), findsOneWidget);
    expect(find.text('Whole range (48 hands)'), findsOneWidget);
    // It starts from what the range does now.
    expect(find.byKey(const Key('lock-share-0')), findsOneWidget);
    expect(
      tester.widget<Text>(find.byKey(const Key('lock-share-0'))).data,
      '51%',
    );

    await tester.tap(find.byKey(const Key('lock-always-0')));
    await tester.pump();
    expect(
      tester.widget<Text>(find.byKey(const Key('lock-share-0'))).data,
      '100%',
    );
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle();

    expect(find.byKey(const Key('locks-card')), findsOneWidget);
    expect(find.text('Start · 48 hands'), findsOneWidget);

    await tester.tap(find.byKey(const Key('resolve-button')));
    await tester.pumpAndSettle();

    final request = api.resolved.single;
    expect(request.parentId, 'job1');
    expect(request.locks.single.path, '');
    expect(request.locks.single.hands, hasLength(48));
    expect(request.locks.single.hands.first.frequencies, [1.0, 0.0, 0.0]);
    expect(request.settings.maxIterations, 300);
    expect(request.settings.targetExploitabilityPct, 0.5);
    expect(setup.resolved.single.parentId, 'job1');
    expect(find.textContaining('Re-solve queued'), findsOneWidget);
  });

  testWidgets('locks only the class picked in the grid', (tester) async {
    final setup = await pumpSolution(tester);

    await tapCell(tester, 'AhKh');
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    expect(find.text('Only AKs'), findsOneWidget);
    // AKs checks 40% and bets small 60% in the sample.
    expect(
      tester.widget<Text>(find.byKey(const Key('lock-share-1'))).data,
      '60%',
    );

    await tester.tap(find.byKey(const Key('lock-always-2')));
    await tester.pump();
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle();
    expect(find.text('Start · 4 hands'), findsOneWidget);

    // A second lock on another class of the same node joins the first.
    await tapCell(tester, 'AhKh');
    await tapCell(tester, 'QsQh');
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('lock-always-0')));
    await tester.pump();
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle();
    expect(find.text('Start · 10 hands'), findsOneWidget);

    await tester.tap(find.byKey(const Key('resolve-button')));
    await tester.pumpAndSettle();
    final hands = {
      for (final h in setup.api.resolved.single.locks.single.hands)
        h.hand: h.frequencies,
    };
    expect(hands, hasLength(10));
    expect(hands['AhKh'], [0.0, 0.0, 1.0]);
    expect(hands['QsQh'], [1.0, 0.0, 0.0]);
  });

  testWidgets('a lock can be removed again', (tester) async {
    await pumpSolution(tester);
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('locks-card')), findsOneWidget);

    await tester.tap(find.byKey(const Key('remove-lock-')));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('locks-card')), findsNothing);
  });

  testWidgets('cancelling the dialog adds nothing', (tester) async {
    await pumpSolution(tester);
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Cancel'));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('locks-card')), findsNothing);
  });

  testWidgets('shows the locks a solve was made with and what they changed', (
    tester,
  ) async {
    final child = sampleJob(id: 'job2')
      ..parentId = 'job1'
      ..locks.add(
        lockOf('', {
          'AhKh': [1, 0, 0],
        }),
      );
    final setup = await pumpSolution(
      tester,
      job: child,
      setUp: (api) {
        api.jobs.add(sampleJob());
        // The child's root has AKs locked and plays differently overall.
        final root = api.nodes['']!.deepCopy();
        final hands = root.oop.hands;
        root.locked.addAll([
          for (final hand in hands)
            hand.startsWith('A') && hand[2] == 'K' && hand[1] == hand[3],
        ]);
        root.actions[0].frequency = 0.62;
        root.actions[1].frequency = 0.20;
        root.actions[2].frequency = 0.18;
        root.oop.rangeEv = api.nodes['']!.oop.rangeEv + 0.25;
        root.ip.rangeEv = api.nodes['']!.ip.rangeEv - 0.25;
        api.childNodes[''] = root;
      },
    );

    expect(find.text('4 of 48 hands are locked here'), findsOneWidget);
    expect(find.text('Start · 1 hands'), findsOneWidget);
    expect(
      find.text('These locks are in force in this solve.'),
      findsOneWidget,
    );
    expect(find.byKey(const Key('resolve-button')), findsNothing);

    // The same node of the parent was fetched for the comparison.
    expect(setup.api.nodeRequestsByJob, containsAll(['job2:', 'job1:']));
    expect(find.byKey(const Key('compare-card')), findsOneWidget);
    expect(find.text('EV: OOP +0.25 · IP −0.25'), findsOneWidget);
    expect(find.text('Check: 50.8% → 62.0%'), findsOneWidget);

    await tapCell(tester, 'AhKh');
    expect(find.textContaining('Locked · EV'), findsNWidgets(4));
  });

  testWidgets('cannot re-solve while the solve is running', (tester) async {
    await pumpSolution(
      tester,
      job: sampleJob(state: JobState.JOB_STATE_RUNNING, iteration: 40),
    );
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));

    expect(
      tester
          .widget<FilledButton>(find.byKey(const Key('resolve-button')))
          .onPressed,
      isNull,
    );
    expect(find.textContaining('Pause this solve first'), findsOneWidget);
  });

  testWidgets('reports a refused re-solve and stays', (tester) async {
    final setup = await pumpSolution(
      tester,
      setUp: (api) => api.resolveError = const GrpcError.invalidArgument(
        "lock on '': the hand 'AsAh' is not in the OOP range",
      ),
    );
    await tester.tap(find.byKey(const Key('lock-button')));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('lock-add')));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('resolve-button')));
    await tester.pumpAndSettle();

    expect(setup.resolved, isEmpty);
    expect(find.textContaining("is not in the OOP range"), findsOneWidget);
    expect(find.byKey(const Key('locks-card')), findsOneWidget);
  });

  testWidgets('the jobs list marks re-solves', (tester) async {
    tester.view.physicalSize = const Size(1000, 900);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.reset);
    final api = FakeApi()
      ..jobs.addAll([
        sampleJob(id: 'child')
          ..parentId = 'parent'
          ..locks.addAll([
            lockOf('', {
              'AhKh': [1, 0, 0],
            }),
            lockOf('check', {
              'KsKh': [1, 0],
            }),
          ]),
        sampleJob(id: 'parent'),
      ]);
    await tester.pumpWidget(
      MaterialApp(
        home: Scaffold(
          body: JobsScreen(api: api, onOpen: (_) {}),
        ),
      ),
    );
    await tester.pump();
    await tester.pump();

    expect(find.text('Re-solve with 2 locked decisions'), findsOneWidget);
    expect(find.byKey(const Key('locks-parent')), findsNothing);
  });
}
