import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/solution/action_style.dart';
import 'package:k15_gto/src/solution/matrix_model.dart';
import 'package:k15_gto/src/solution/solution_screen.dart';
import 'package:k15_gto/src/solution/strategy_matrix.dart';

import 'fake_api.dart';

Future<FakeApi> pumpSolution(
  WidgetTester tester, {
  Size size = const Size(1400, 900),
  FakeApi? api,
}) async {
  tester.view.physicalSize = size;
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);

  final fake = api ?? FakeApi();
  if (api == null) addSampleSolution(fake);
  fake.jobs.add(sampleJob());
  await tester.pumpWidget(
    MaterialApp(
      theme: ThemeData(brightness: Brightness.dark),
      home: SolutionScreen(api: fake, job: sampleJob()),
    ),
  );
  await tester.pumpAndSettle();
  return fake;
}

/// Taps the centre of a matrix cell.
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
  group('MatrixModel', () {
    test('places hands in the usual grid', () {
      expect(MatrixModel.cellIndex('AsAh'), 0);
      expect(MatrixModel.cellIndex('2s2h'), 168);
      // Suited above the diagonal, offsuit below.
      expect(MatrixModel.cellIndex('AhKh'), 1);
      expect(MatrixModel.cellIndex('AhKd'), 13);
      expect(MatrixModel.cellIndex('KdAh'), 13);
      expect(MatrixModel.cellIndex('7c6c'), 7 * 13 + 8);
      expect(MatrixModel.cellIndex('Ah'), isNull);
      expect(MatrixModel.cellIndex('XhKd'), isNull);
    });

    test('labels all 169 cells', () {
      final model = MatrixModel.fromRange(PlayerRange());
      expect(model.cells, hasLength(169));
      expect(model.cell(0, 0).label, 'AA');
      expect(model.cell(0, 1).label, 'AKs');
      expect(model.cell(1, 0).label, 'AKo');
      expect(model.cell(12, 12).label, '22');
      expect(model.cell(0, 0).combos, 6);
      expect(model.cell(0, 1).combos, 4);
      expect(model.cell(1, 0).combos, 12);
      expect(model.cells.map((c) => c.combos).reduce((a, b) => a + b), 1326);
      expect(model.cells.every((c) => c.isEmpty), isTrue);
    });

    test('adds up reach, frequencies and value per class', () {
      // Two of the four AKs combos, one of them only half there.
      final range = PlayerRange(
        hands: ['AhKh', 'AsKs', 'QdQc'],
        reach: [1.0, 0.5, 1.0],
        ev: [10.0, 4.0, 2.0],
      );
      // Actions: check, bet. Laid out [action][hand].
      final strategy = [0.2, 1.0, 0.0, 0.8, 0.0, 1.0];
      final model = MatrixModel.fromRange(
        range,
        strategy: strategy,
        actionCount: 2,
      );

      final aks = model.cell(0, 1);
      expect(aks.hands, hasLength(2));
      expect(aks.reach, 1.5);
      expect(aks.fraction, closeTo(1.5 / 4, 1e-9));
      // (1.0 * 0.2 + 0.5 * 1.0) / 1.5
      expect(aks.frequency(0), closeTo(0.7 / 1.5, 1e-9));
      expect(aks.frequency(1), closeTo(0.8 / 1.5, 1e-9));
      expect(aks.ev, closeTo((10 + 2) / 1.5, 1e-9));

      final queens = model.cell(2, 2);
      expect(queens.frequency(1), 1.0);
      expect(queens.fraction, closeTo(1 / 6, 1e-9));
      expect(model.totalReach, 2.5);
      expect(model.actionCount, 2);
    });

    test('ignores a strategy of the wrong size', () {
      final range = PlayerRange(hands: ['AhKh'], reach: [1.0], ev: [1.0]);
      final model = MatrixModel.fromRange(
        range,
        strategy: [1.0],
        actionCount: 2,
      );
      expect(model.actionCount, 0);
      expect(model.cell(0, 1).reach, 1.0);
    });
  });

  group('action style', () {
    test('names path steps', () {
      expect(pathStepTitle('check'), 'Check');
      expect(pathStepTitle('bet6.93'), 'Bet 6.93');
      expect(pathStepTitle('raise25'), 'Raise 25');
      expect(pathStepTitle('allin'), 'All-in');
      expect(pathStepTitle('Ts'), 'T♠');
      expect(cardTitle('2h'), '2♥');
      expect(cardTitle('check'), isNull);
    });

    test('colours bets from light to dark', () {
      final colors = actionColors([
        check(),
        bet(5, 'bet5'),
        bet(10, 'bet10'),
        allin(95),
      ]);
      expect(colors[0], passiveColor);
      expect(colors[1], smallBetColor);
      expect(colors[2], bigBetColor);
      expect(colors[3], allinColor);
      expect(actionColors([fold(), call(5)]), [foldColor, passiveColor]);
    });
  });

  testWidgets('shows the root with its actions and numbers', (tester) async {
    final api = await pumpSolution(tester);

    expect(api.nodeRequests, ['']);
    expect(find.text('OOP acts:'), findsOneWidget);
    expect(find.byKey(const Key('action-check')), findsOneWidget);
    expect(find.byKey(const Key('action-bet5')), findsOneWidget);
    expect(find.byKey(const Key('action-bet10')), findsOneWidget);
    expect(find.text('Pot 10'), findsOneWidget);
    expect(find.byType(StrategyMatrix), findsOneWidget);
    expect(find.textContaining('combos in OOP\'s range here'), findsOneWidget);
    expect(
      find.text('Tap a hand in the grid for its details.'),
      findsOneWidget,
    );
  });

  testWidgets('walks down a line and back up the breadcrumb', (tester) async {
    final api = await pumpSolution(tester);

    await tester.tap(find.byKey(const Key('action-bet5')));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, 'bet5');
    expect(find.text('IP acts:'), findsOneWidget);
    expect(find.text('Pot 15'), findsOneWidget);
    expect(find.byKey(const Key('action-raise17')), findsOneWidget);
    expect(find.textContaining('Raise to 17'), findsWidgets);
    expect(find.text('Bet 5'), findsOneWidget); // the breadcrumb step

    await tester.tap(find.byKey(const Key('action-fold')));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, 'bet5/fold');
    expect(find.text('IP folds. The hand is over.'), findsOneWidget);

    // Back to the start.
    await tester.tap(find.byKey(const Key('crumb-0')));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, '');
    expect(find.text('OOP acts:'), findsOneWidget);
  });

  testWidgets('deals a card at a chance node', (tester) async {
    final api = await pumpSolution(tester);

    await tester.tap(find.byKey(const Key('action-check')));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('action-check')));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, 'check/check');
    expect(find.text('Pick the next card:'), findsOneWidget);
    // Cards on the board are not offered.
    expect(find.byKey(const Key('card-Ks')), findsNothing);
    expect(find.byKey(const Key('card-As')), findsOneWidget);

    await tester.tap(find.byKey(const Key('card-2c')));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, 'check/check/2c');
    expect(find.text('2♣'), findsOneWidget); // in the breadcrumb
    expect(find.byKey(const Key('action-bet7.5')), findsOneWidget);
  });

  testWidgets('shows the hands of a tapped class', (tester) async {
    await pumpSolution(tester);

    await tapCell(tester, 'AhKh');
    final details = find.byKey(const Key('cell-details'));
    expect(details, findsOneWidget);
    expect(
      find.descendant(of: details, matching: find.text('AKs')),
      findsOneWidget,
    );
    expect(
      find.descendant(
        of: details,
        matching: find.text('4.00 of 4 combos · EV 9.40'),
      ),
      findsOneWidget,
    );
    // One line per combo, with what it does.
    expect(
      find.descendant(
        of: details,
        matching: find.text('Check 40% · Bet 5 60%'),
      ),
      findsNWidgets(4),
    );

    // A class outside the range.
    await tapCell(tester, '7h2d');
    expect(find.text('Not in the range here.'), findsOneWidget);

    // Tapping the same cell again clears the selection.
    await tapCell(tester, '7h2d');
    expect(find.byKey(const Key('cell-details')), findsNothing);
  });

  testWidgets('switches to the other player\'s range', (tester) async {
    await pumpSolution(tester);

    await tester.tap(find.text('IP range'));
    await tester.pumpAndSettle();
    expect(find.textContaining('combos in IP\'s range here'), findsOneWidget);

    // The player not acting has no strategy here, only a range.
    await tapCell(tester, 'KsQs');
    final details = find.byKey(const Key('cell-details'));
    expect(
      find.descendant(of: details, matching: find.text('KQs')),
      findsOneWidget,
    );
    expect(
      find.descendant(of: details, matching: find.textContaining('Check')),
      findsNothing,
    );
  });

  testWidgets('offers a way back when a line cannot be read', (tester) async {
    final api = await pumpSolution(tester);

    await tester.tap(find.byKey(const Key('action-bet10')));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('solution-error')), findsOneWidget);
    expect(find.textContaining('no such line'), findsOneWidget);

    await tester.tap(find.text('Back to the start'));
    await tester.pumpAndSettle();
    expect(api.nodeRequests.last, '');
    expect(find.text('OOP acts:'), findsOneWidget);
  });

  testWidgets('fits a phone screen', (tester) async {
    await pumpSolution(tester, size: const Size(390, 844));
    expect(tester.takeException(), isNull);
    expect(find.byType(StrategyMatrix), findsOneWidget);
    expect(find.byKey(const Key('action-check')), findsOneWidget);
    await tester.scrollUntilVisible(find.byKey(const Key('node-ev')), 200);
    expect(find.byKey(const Key('node-ev')), findsOneWidget);
  });

  testWidgets('keeps reading a solve that is still running', (tester) async {
    tester.view.physicalSize = const Size(1400, 900);
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.reset);

    final api = FakeApi();
    addSampleSolution(api);
    final running = sampleJob(state: JobState.JOB_STATE_RUNNING, iteration: 40);
    api.jobs.add(running);
    await tester.pumpWidget(
      MaterialApp(
        home: SolutionScreen(api: api, job: running),
      ),
    );
    await tester.pump();
    await tester.pump();
    expect(api.nodeRequests, ['']);
    expect(find.textContaining('iteration 40'), findsOneWidget);

    api.jobs.single.iteration = 90;
    await tester.pump(const Duration(seconds: 3));
    await tester.pump();
    await tester.pump();
    expect(api.nodeRequests, ['', '']);
    expect(find.textContaining('iteration 90'), findsOneWidget);

    // Once it is done there is nothing more to fetch.
    api.jobs.single.state = JobState.JOB_STATE_DONE;
    await tester.pump(const Duration(seconds: 3));
    await tester.pump();
    await tester.pump();
    await tester.pump(const Duration(seconds: 3));
    await tester.pump();
    expect(api.nodeRequests, hasLength(3));
  });
}
