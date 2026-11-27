import 'package:fixnum/fixnum.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/solution/matrix_model.dart';
import 'package:k15_gto/src/solution/solution_screen.dart';
import 'package:k15_gto/src/solution/strategy_matrix.dart';

import 'fake_api.dart';

Future<void> pumpJob(
  WidgetTester tester,
  FakeApi api,
  Job job, {
  Size size = const Size(1400, 900),
}) async {
  tester.view.physicalSize = size;
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);
  api.jobs.add(job);
  await tester.pumpWidget(
    MaterialApp(
      theme: ThemeData(brightness: Brightness.dark),
      home: SolutionScreen(api: api, job: job),
    ),
  );
  await tester.pumpAndSettle();
}

PlayerRange seatRange(String position, Map<String, double> classes) =>
    rangeOf(classes)..position = position;

void main() {
  testWidgets('browses a three-way spot seat by seat', (tester) async {
    final api = FakeApi();
    final bb = seatRange('BB', {'AA': 5, 'KK': 4});
    final co = seatRange('CO', {'QQ': 3, 'AKs': 2});
    final btn = seatRange('BTN', {'JJ': 2, 'T9s': 1});
    final hands = co.hands.length;
    api.nodes['check'] = GetNodeResponse(
      type: NodeType.NODE_TYPE_ACTION,
      path: 'check',
      pot: 10,
      players: [bb, co, btn],
      actingPlayer: 1,
      committed: [0, 0, 0],
      folded: [false, false, false],
      allIn: [false, false, false],
      evEstimated: true,
      actions: [check()..frequency = 0.6, bet(5, 'bet5')..frequency = 0.4],
      strategy: [
        for (var h = 0; h < hands; ++h) 0.6,
        for (var h = 0; h < hands; ++h) 0.4,
      ],
    );
    api.nodes[''] = GetNodeResponse(
      type: NodeType.NODE_TYPE_ACTION,
      path: '',
      pot: 10,
      players: [bb, co, btn],
      actingPlayer: 0,
      committed: [0, 0, 0],
      folded: [false, false, false],
      allIn: [false, false, false],
      actions: [check()..frequency = 1],
      strategy: [for (var h = 0; h < bb.hands.length; ++h) 1.0],
    );
    await pumpJob(
      tester,
      api,
      Job(
        id: 'mw',
        name: '3-way · Ks Td 7h',
        state: JobState.JOB_STATE_DONE,
        spot: Spot(board: 'Ks Td 7h', pot: 10, effectiveStack: 95),
        engine: Engine.ENGINE_MONTE_CARLO,
        positions: ['BB', 'CO', 'BTN'],
        playerCount: 3,
        measured: true,
        exploitabilityPct: 0.8,
        exploitabilityKind:
            ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY,
        hasSolution: true,
        mcIterations: Int64(5000000),
      ),
    );

    // The line names the seat to act.
    expect(find.text('BB · 95'), findsOneWidget);
    await tester.tap(find.byKey(const Key('action-check')));
    await tester.pumpAndSettle();
    expect(find.text('CO · 95'), findsOneWidget);

    // The grid shows the acting seat; every seat can be picked.
    expect(find.text('CO strategy'.toUpperCase()), findsOneWidget);
    expect(find.byType(StrategyMatrix), findsOneWidget);
    expect(
      find.descendant(
        of: find.byKey(const Key('player-view')),
        matching: find.text('BTN'),
      ),
      findsOneWidget,
    );
    await tester.tap(
      find.descendant(
        of: find.byKey(const Key('player-view')),
        matching: find.text('BTN'),
      ),
    );
    await tester.pumpAndSettle();
    expect(find.text('BTN range'.toUpperCase()), findsOneWidget);

    // Every seat in the summary, with the estimate labelled.
    expect(find.textContaining('to act'), findsOneWidget);
    expect(
      find.text(
        '≈ 0.80% of pot (estimate; no equilibrium guarantee with 3+ players)',
      ),
      findsOneWidget,
    );
    // No locking on the Monte Carlo engine.
    expect(find.byKey(const Key('lock-button')), findsNothing);
  });

  testWidgets('lists PLO hands instead of a grid', (tester) async {
    final api = FakeApi();
    final oop = PlayerRange(
      position: 'OOP',
      hands: ['AsKsQhJh', 'AhAdKcQc', '9s8s7h6h'],
      reach: [0.5, 1.0, 0.8],
      ev: [3, 7, 1],
      equity: [0.4, 0.7, 0.2],
      totalHands: 120000,
      shownReachFraction: 0.5,
    );
    final ip = PlayerRange(
      position: 'IP',
      hands: ['KsKdJcTc'],
      reach: [1],
      ev: [5],
      equity: [0.5],
    );
    api.nodes[''] = GetNodeResponse(
      type: NodeType.NODE_TYPE_ACTION,
      path: '',
      pot: 10,
      players: [oop, ip],
      oop: oop,
      ip: ip,
      actingPlayer: 0,
      actions: [check()..frequency = 0.5, bet(10, 'bet10')..frequency = 0.5],
      // check: 0.9, 0.2, 1.0; bet: 0.1, 0.8, 0.0
      strategy: [0.9, 0.2, 1.0, 0.1, 0.8, 0.0],
    );
    await pumpJob(
      tester,
      api,
      Job(
        id: 'plo',
        name: 'PLO · Ks Td 7h 4c',
        state: JobState.JOB_STATE_DONE,
        spot: Spot(
          board: 'Ks Td 7h 4c',
          pot: 10,
          effectiveStack: 95,
          variant: Variant.VARIANT_OMAHA4,
        ),
        engine: Engine.ENGINE_EXACT,
        hasSolution: true,
      ),
    );

    expect(api.nodeOptions.first.maxHands, 1000);
    expect(api.nodeOptions.first.detail, HandDetail.HAND_DETAIL_HANDS);
    expect(find.byType(StrategyMatrix), findsNothing);
    expect(find.byKey(const Key('matrix-view')), findsNothing);
    expect(
      tester.widget<Text>(find.byKey(const Key('plo-coverage'))).data,
      'Top 3 of 120,000 hands · 50.0% of the range\'s weight',
    );

    double top(String hand) =>
        tester.getTopLeft(find.byKey(Key('plo-hand-$hand'))).dy;
    // By weight first.
    expect(top('AhAdKcQc'), lessThan(top('9s8s7h6h')));
    expect(top('9s8s7h6h'), lessThan(top('AsKsQhJh')));

    // Picking the bet in the summary sorts by it.
    await tester.tap(find.byKey(const Key('summary-bet10')));
    await tester.pumpAndSettle();
    expect(top('AhAdKcQc'), lessThan(top('AsKsQhJh')));
    expect(top('AsKsQhJh'), lessThan(top('9s8s7h6h')));

    // Sorting by EV, then the other way round.
    await tester.tap(find.byKey(const Key('plo-sort-ev')));
    await tester.pumpAndSettle();
    expect(top('AhAdKcQc'), lessThan(top('AsKsQhJh')));
    await tester.tap(find.byKey(const Key('plo-sort-ev')));
    await tester.pumpAndSettle();
    expect(top('9s8s7h6h'), lessThan(top('AsKsQhJh')));

    // Asking for more hands reads the node again.
    await tester.tap(find.byKey(const Key('plo-max-hands')));
    await tester.pumpAndSettle();
    await tester.tap(find.text('Up to 5,000').last);
    await tester.pumpAndSettle();
    expect(api.nodeOptions.last.maxHands, 5000);
    expect(find.byKey(const Key('lock-button')), findsNothing);
  });

  testWidgets('shows the grid of the acting seat of a preflop solve', (
    tester,
  ) async {
    final api = FakeApi();
    List<HandClass> classes(Map<String, List<double>> strategy) => [
      for (final cell in MatrixModel.fromRange(PlayerRange()).cells)
        HandClass(
          name: cell.label,
          weight: 1,
          strategy: strategy[cell.label] ?? const [1, 0],
        ),
    ];
    final positions = ['SB', 'BB', 'UTG', 'HJ', 'CO', 'BTN'];
    api.nodes[''] = GetNodeResponse(
      type: NodeType.NODE_TYPE_ACTION,
      path: '',
      pot: 1.5,
      players: [
        for (var i = 0; i < 6; ++i)
          PlayerRange(
            position: positions[i],
            classes: classes(
              i == 2
                  ? {
                      'AA': [0, 1],
                    }
                  : const {},
            ),
          ),
      ],
      actingPlayer: 2,
      committed: [0.5, 1, 0, 0, 0, 0],
      folded: List.filled(6, false),
      allIn: List.filled(6, false),
      actions: [
        fold()..frequency = 0.85,
        raise(2.5, 'raise2.5')..frequency = 0.15,
      ],
    );
    await pumpJob(
      tester,
      api,
      Job(
        id: 'pf',
        name: 'Preflop · 6-max · 100 BB',
        state: JobState.JOB_STATE_DONE,
        spot: Spot(preflop: PreflopSettings(players: 6, stackBb: 100)),
        engine: Engine.ENGINE_MONTE_CARLO,
        positions: positions,
        playerCount: 6,
        hasSolution: true,
      ),
    );

    expect(api.nodeOptions.first.detail, HandDetail.HAND_DETAIL_CLASSES);
    expect(find.byType(StrategyMatrix), findsOneWidget);
    expect(find.text('UTG strategy'.toUpperCase()), findsOneWidget);
    expect(find.text('UTG · 100'), findsOneWidget);
    expect(find.text('Preflop'), findsOneWidget);
    // Six seats: a menu rather than segments.
    expect(
      find.byWidgetPredicate(
        (w) => w is DropdownButton<int> && w.key == const Key('player-view'),
      ),
      findsOneWidget,
    );
    final matrix = tester.widget<StrategyMatrix>(find.byType(StrategyMatrix));
    expect(matrix.model.cells[0].frequency(1), 1);
    expect(matrix.model.cells[1].frequency(0), 1);
  });
}
