import 'package:fixnum/fixnum.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/spot/spot_form.dart';
import 'package:k15_gto/src/spot/spot_setup_screen.dart';

import 'fake_api.dart';

Future<List<Job>> pumpSetup(WidgetTester tester, FakeApi api) async {
  tester.view.physicalSize = const Size(1200, 2400);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);

  final submitted = <Job>[];
  await tester.pumpWidget(
    MaterialApp(
      home: Scaffold(
        body: SpotSetupScreen(
          api: api,
          onSubmitted: submitted.add,
          estimateDelay: Duration.zero,
        ),
      ),
    ),
  );
  await tester.pumpAndSettle();
  return submitted;
}

FilledButton solveButton(WidgetTester tester) =>
    tester.widget<FilledButton>(find.byKey(const Key('solve-button')));

void main() {
  group('parsePercents', () {
    test('reads lists in the usual spellings', () {
      expect(parsePercents('33, 75'), [33, 75]);
      expect(parsePercents('33 75;150'), [33, 75, 150]);
      expect(parsePercents(' 50% '), [50]);
      expect(parsePercents(''), isEmpty);
    });
    test('rejects what is not a positive number', () {
      expect(parsePercents('33, x'), isNull);
      expect(parsePercents('-5'), isNull);
      expect(parsePercents('0'), isNull);
    });
  });

  test('countBoardCards', () {
    expect(countBoardCards('Ks Td 7h'), 3);
    expect(countBoardCards('KsTd7h4c'), 4);
    expect(countBoardCards('ks, td, 7h, 4c, 2s'), 5);
    expect(countBoardCards(''), 0);
    expect(countBoardCards('Ks T'), isNull);
    expect(countBoardCards('Kx Td 7h'), isNull);
  });

  group('SpotInput', () {
    test('turns the defaults into a request', () {
      final parsed = SpotInput().toSpot();
      expect(parsed.error, isNull);
      final spot = parsed.spot!;
      expect(spot.board, 'Ks Td 7h 4c');
      expect(spot.pot, 10);
      expect(spot.effectiveStack, 95);
      expect(spot.tree.turn.ip.betPct, [66]);
      expect(spot.tree.turn.ip.raisePct, [60]);
      expect(spot.tree.river.oop.allin, isTrue);
      expect(spot.tree.raiseCap, 2);
      expect(spot.tree.allinThresholdPct, 67);

      final settings = SpotInput().toSettings().settings!;
      expect(settings.maxIterations, 500);
      expect(settings.targetExploitabilityPct, 0.5);
    });

    test('names the field that cannot be read', () {
      expect((SpotInput()..pot = '0').toSpot().error, contains('pot'));
      expect((SpotInput()..stack = 'x').toSpot().error, contains('stack'));
      expect(
        (SpotInput()..raiseCap = '2.5').toSpot().error,
        contains('raise cap'),
      );
      final badSize = SpotInput()..sizing[1][0].bet = '66, big';
      expect(badSize.toSpot().error, contains('turn sizes'));
      expect(
        (SpotInput()..maxIterations = '0').toSettings().error,
        contains('Iterations'),
      );
    });
  });

  testWidgets('shows what the solve takes', (tester) async {
    final api = FakeApi();
    await pumpSetup(tester, api);

    expect(api.estimated, hasLength(1));
    expect(
      find.text('15,774 nodes · 233 vs 234 hands · 28.0 MB'),
      findsOneWidget,
    );
    expect(solveButton(tester).onPressed, isNotNull);
    // A turn board: flop sizes are not asked for.
    expect(find.text('Flop'), findsNothing);
    expect(find.text('Turn'), findsOneWidget);
    expect(find.text('River'), findsOneWidget);
  });

  testWidgets('measures again after an edit and asks for flop sizes', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);

    // The board shows as cards; the keyboard button switches to typing.
    expect(find.byKey(const Key('board-cards')), findsOneWidget);
    await tester.tap(find.byKey(const Key('board-type')));
    await tester.pumpAndSettle();
    await tester.enterText(find.byKey(const Key('field-board')), 'Ks Td 7h');
    await tester.pumpAndSettle();

    expect(api.estimated, hasLength(2));
    expect(api.estimated.last.board, 'Ks Td 7h');
    expect(find.text('Flop'), findsOneWidget);
  });

  testWidgets('points out a bad number without asking the server', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);

    await tester.enterText(find.byKey(const Key('field-pot')), 'ten');
    await tester.pumpAndSettle();

    expect(api.estimated, hasLength(1));
    expect(find.text('The pot must be a number above 0.'), findsOneWidget);
    expect(solveButton(tester).onPressed, isNull);
  });

  testWidgets('shows what the server objects to', (tester) async {
    final api = FakeApi()
      ..estimateError = const GrpcError.invalidArgument(
        "ip_range: cannot read range item 'banana'",
      );
    await pumpSetup(tester, api);

    expect(
      find.text("Ip_range: cannot read range item 'banana'."),
      findsOneWidget,
    );
    expect(solveButton(tester).onPressed, isNull);
  });

  testWidgets('refuses a solve that does not fit', (tester) async {
    final api = FakeApi();
    api.estimateResult = EstimateTreeResponse(
      nodes: Int64(40000000),
      oopHands: 300,
      ipHands: 310,
      strategyBytes: Int64(60) << 30,
      treeBytes: Int64(2) << 30,
      canSolve: false,
      memoryBudgetBytes: Int64(24) << 30,
    );
    await pumpSetup(tester, api);

    expect(find.textContaining('62.0 GB'), findsOneWidget);
    expect(
      find.text('Needs more memory than this server allows (24.0 GB).'),
      findsOneWidget,
    );
    expect(solveButton(tester).onPressed, isNull);

    api.estimateResult = EstimateTreeResponse(
      nodes: Int64(100000000),
      tooLarge: true,
      canSolve: false,
    );
    await tester.enterText(find.byKey(const Key('field-pot')), '11');
    await tester.pumpAndSettle();
    expect(find.textContaining('More than 100,000,000 nodes'), findsOneWidget);
    expect(find.textContaining('Too large for this server'), findsOneWidget);
  });

  testWidgets('submits what was typed', (tester) async {
    final api = FakeApi();
    final submitted = await pumpSetup(tester, api);

    await tester.enterText(find.byKey(const Key('field-pot')), '20');
    await tester.enterText(find.byKey(const Key('field-oop-range')), 'AA, KK');
    await tester.enterText(
      find.byKey(const Key('field-river-ip-bet')),
      '50, 100',
    );
    await tester.enterText(find.byKey(const Key('field-iterations')), '300');
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();

    expect(api.submitted, hasLength(1));
    final request = api.submitted.single;
    expect(request.spot.pot, 20);
    expect(request.spot.oopRange, 'AA, KK');
    expect(request.spot.tree.river.ip.betPct, [50, 100]);
    expect(request.settings.maxIterations, 300);
    expect(request.name, 'Ks Td 7h 4c · pot 20');
    expect(submitted.single.id, 'job1');
  });

  testWidgets('says when the server reuses an existing job', (tester) async {
    final api = FakeApi()
      ..reuseJob = Job(id: 'old', name: 'Ks Td 7h 4c · pot 6');
    final submitted = await pumpSetup(tester, api);

    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();

    expect(submitted.single.id, 'old');
    expect(find.textContaining('already solves this spot'), findsOneWidget);
  });

  testWidgets('reports a refused submission', (tester) async {
    final api = FakeApi()
      ..submitError = const GrpcError.resourceExhausted(
        'the solve needs about 900 MB, more than the server\'s limit of 512 MB',
      );
    final submitted = await pumpSetup(tester, api);

    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();

    expect(submitted, isEmpty);
    expect(find.textContaining('The solve needs about 900 MB'), findsOneWidget);
  });
}
