import 'dart:io';

import 'package:fixnum/fixnum.dart';
import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/common/format.dart';
import 'package:k15_gto/src/range/preflop_library.dart';
import 'package:k15_gto/src/range/solved_library.dart';
import 'package:k15_gto/src/spot/spot_setup_screen.dart';

import 'fake_api.dart';
import 'spot_setup_test.dart' show pumpSetup, solveButton;

String fieldText(WidgetTester tester, String key) => tester
    .widget<EditableText>(
      find.descendant(
        of: find.byKey(Key('field-$key')),
        matching: find.byType(EditableText),
      ),
    )
    .controller
    .text;

Future<void> pick(WidgetTester tester, String dropdown, String item) async {
  await tester.tap(find.byKey(Key(dropdown)));
  await tester.pumpAndSettle();
  await tester.tap(find.text(item).last);
  await tester.pumpAndSettle();
}

void main() {
  testWidgets('a PLO spot is sent with its game and Omaha ranges', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);

    await tester.tap(find.text('PLO4'));
    await tester.pumpAndSettle();

    // No grid for PLO, and no rule-based Hold'em library either.
    expect(find.byKey(const Key('edit-oop-range')), findsNothing);
    expect(find.byKey(const Key('preflop-source')), findsNothing);
    expect(fieldText(tester, 'oop-range'), 'top5%');
    final checkbox = tester.widget<Checkbox>(
      find.byKey(const Key('pot-limit')),
    );
    expect(checkbox.value, isTrue);
    expect(api.estimated.last.variant, Variant.VARIANT_OMAHA4);

    await tester.enterText(find.byKey(const Key('field-ip-range')), 'AKs');
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('spot-problem')), findsOneWidget);
    expect(find.textContaining('"AKs" is not a PLO hand'), findsOneWidget);

    await tester.enterText(
      find.byKey(const Key('field-ip-range')),
      'AAxx, KQJTds',
    );
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();
    expect(api.submitted.single.spot.ipRange, 'AAxx, KQJTds');
    expect(api.submitted.single.name, startsWith('PLO · '));
  });

  testWidgets('multiway spots get one row per player', (tester) async {
    final api = FakeApi()
      ..estimateResult = EstimateTreeResponse(
        nodes: Int64(900),
        playerHands: [400, 380, 420],
        infoSets: Int64(1234567),
        engine: Engine.ENGINE_MONTE_CARLO,
        exploitabilityKind:
            ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY,
        playerCount: 3,
        positions: ['BB', 'CO', 'BTN'],
        canSolve: true,
      );
    await pumpSetup(tester, api);

    await pick(tester, 'player-count', '3');
    expect(find.byKey(const Key('field-player-2-range')), findsOneWidget);
    expect(fieldText(tester, 'player-1-position'), 'CO');
    expect(find.byKey(const Key('field-oop-range')), findsNothing);
    // One set of sizes for everyone.
    expect(find.byKey(const Key('field-river-all-bet')), findsOneWidget);
    // Probably Monte Carlo: its budget is asked for.
    expect(find.byKey(const Key('field-mc-iterations')), findsOneWidget);

    final spot = api.estimated.last;
    expect([for (final p in spot.players) p.position], ['BB', 'CO', 'BTN']);
    expect(
      find.text('900 nodes · 400 vs 380 vs 420 hands · 0 B'),
      findsOneWidget,
    );
    expect(
      tester.widget<Text>(find.byKey(const Key('spot-engine'))).data,
      'Monte Carlo engine · BB, CO, BTN · 1,234,567 info sets · '
      'estimated exploitability, no equilibrium guarantee',
    );

    await tester.enterText(find.byKey(const Key('field-mc-iterations')), '5');
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();
    expect(api.submitted.single.settings.mcIterations, Int64(5000000));
    expect(api.submitted.single.name, startsWith('3-way · '));
  });

  testWidgets('a preflop solve is set up and submitted as a job', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);

    await tester.tap(find.text(SetupMode.preflop.label));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('field-board')), findsNothing);
    expect(
      tester.widget<Text>(find.byKey(const Key('preflop-positions'))).data,
      'Seats: UTG, HJ, CO, BTN, SB, BB',
    );

    await pick(tester, 'preflop-table', '9-max');
    await pick(tester, 'preflop-stack', '200 BB');
    expect(api.estimated.last.preflop.preset, '9max');
    expect(find.byKey(const Key('preflop-limp')), findsNothing);
    await tester.tap(find.text('Custom'));
    await tester.pumpAndSettle();
    await tester.tap(find.byKey(const Key('preflop-limp')));
    await tester.enterText(find.byKey(const Key('field-preflop-rake')), '5');
    await tester.pumpAndSettle();

    final spot = api.estimated.last;
    expect(spot.preflop.players, 9);
    expect(spot.preflop.stackBb, 200);
    expect(spot.preflop.allowLimp, isTrue);
    expect(spot.rakePct, 5);

    await tester.tap(find.byKey(const Key('solve-button')));
    await tester.pumpAndSettle();
    expect(api.submitted.single.name, 'Preflop · 9-max · 200 BB · custom');
    expect(api.submitted.single.spot.hasPreflop(), isTrue);
  });

  testWidgets('shows the server\'s refusal of a spot it cannot solve yet', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);
    api.estimateError = const GrpcError.failedPrecondition(
      'Monte Carlo engine not built yet',
    );
    await tester.tap(find.text(SetupMode.preflop.label));
    await tester.pumpAndSettle();
    expect(
      find.textContaining('Monte Carlo engine not built yet'),
      findsOneWidget,
    );
    expect(solveButton(tester).onPressed, isNull);
  });

  testWidgets('an older server is not sent spots it would misread', (
    tester,
  ) async {
    final api = FakeApi();
    api.info.apiMinor = 3;
    await pumpSetup(tester, api);
    final before = api.estimated.length;

    await tester.tap(find.text('PLO4'));
    await tester.pumpAndSettle();
    expect(api.estimated, hasLength(before));
    expect(find.textContaining('cannot solve PLO'), findsOneWidget);
  });

  testWidgets('fills the ranges from a solved preflop job', (tester) async {
    final api = FakeApi();
    api.jobs.add(
      Job(
        id: 'pf',
        name: 'Preflop · 6-max · 100 BB',
        state: JobState.JOB_STATE_DONE,
        hasSolution: true,
        spot: Spot(preflop: PreflopSettings(players: 6, stackBb: 100)),
      ),
    );
    api.preflopScenarios['pf'] = [
      PreflopScenario(
        name: 'BTN open, BB call',
        path: 'BTN:raise2.5/BB:call',
        pot: 5.5,
        effectiveStack: 97.5,
        frequency: 0.12,
      ),
    ];
    api.preflopRanges['pf:BTN:raise2.5/BB:call'] = GetPreflopRangesResponse(
      path: 'fold/fold/fold/raise2.5/fold/call',
      complete: true,
      pot: 5.5,
      effectiveStack: 97.5,
      iteration: 4200,
      players: [
        PreflopRange(position: 'BB'),
        PreflopRange(position: 'BTN'),
      ],
      spot: Spot(
        oopRange: 'QQ-22, AJs-A2s',
        ipRange: 'AA-22, AKs-A2s, KQo',
        pot: 5.5,
        effectiveStack: 97.5,
      ),
    );
    await pumpSetup(tester, api);

    await pick(tester, 'preflop-source', 'Server preflop job');
    expect(api.calls, contains('scenarios pf'));

    await pick(
      tester,
      'preflop-line',
      'BTN open, BB call · pot 5.5 · 12% of deals',
    );
    expect(api.calls, contains('ranges pf:BTN:raise2.5/BB:call'));
    expect(fieldText(tester, 'oop-range'), 'QQ-22, AJs-A2s');
    expect(fieldText(tester, 'ip-range'), 'AA-22, AKs-A2s, KQo');
    expect(fieldText(tester, 'pot'), '5.5');
    expect(fieldText(tester, 'stack'), '97.5');
    final description = tester
        .widget<Text>(find.byKey(const Key('scenario-description')))
        .data!;
    expect(description, contains('OOP is BB, IP is BTN'));
    expect(description, contains('"Preflop · 6-max · 100 BB"'));
    expect(description, contains('iteration 4200'));

    // Back to the library: it is still there, and says what it is.
    await pick(tester, 'preflop-source', 'Built-in rules');
    expect(find.byKey(const Key('scenario-format')), findsOneWidget);
    expect(
      tester.widget<Text>(find.byKey(const Key('preflop-source-label'))).data,
      contains('not solver output'),
    );
  });

  testWidgets('says so when the server has no preflop solve', (tester) async {
    final api = FakeApi();
    await pumpSetup(tester, api);
    await pick(tester, 'preflop-source', 'Server preflop job');
    expect(find.byKey(const Key('no-preflop-jobs')), findsOneWidget);
  });

  testWidgets('fills the ranges from the bundled solved library', (
    tester,
  ) async {
    final api = FakeApi();
    await pumpSetup(tester, api);
    final ruleRange = fieldText(tester, 'oop-range');

    // The bundled file as the app ships it, read from disk: the asset
    // bundle answers outside the test's fake clock.
    final library = SolvedLibrary.parse(
      File(SolvedLibrary.assetFor(TableFormat.sixMax, 100)).readAsStringSync(),
    );
    SolvedLibrary.preload(TableFormat.sixMax, 100, library);
    await pick(tester, 'preflop-source', 'Solved library (bundled)');

    final line = library.find(PotType.singleRaised, 'BTN', 'BB')!;
    // The form still holds the defaults, so the line fills them in.
    expect(fieldText(tester, 'oop-range'), line.oopRange);
    expect(fieldText(tester, 'oop-range'), isNot(ruleRange));
    expect(fieldText(tester, 'ip-range'), line.ipRange);
    expect(fieldText(tester, 'pot'), formatChips(line.pot));
    expect(
      tester.widget<Text>(find.byKey(const Key('scenario-description'))).data,
      'BTN open 2.3, BB call. OOP is BB, IP is BTN.',
    );
    final caveat = tester
        .widget<Text>(find.byKey(const Key('preflop-source-label')))
        .data!;
    expect(caveat, contains('100% of their equity'));
    expect(caveat, contains('G iterations'));
    expect(caveat, contains('no equilibrium guarantee'));
  });
}
