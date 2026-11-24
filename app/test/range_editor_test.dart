import 'dart:math';

import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/range/hand_range.dart';
import 'package:k15_gto/src/range/range_editor.dart';
import 'package:k15_gto/src/range/range_store.dart';
import 'package:k15_gto/src/spot/board_picker.dart';
import 'package:k15_gto/src/spot/spot_setup_screen.dart';

import 'fake_api.dart';

/// Opens the editor from a button and records what it returns.
Future<List<String?>> pumpEditor(
  WidgetTester tester, {
  String initial = '',
  RangeStore? store,
  Size size = const Size(1300, 900),
}) async {
  tester.view.physicalSize = size;
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);

  final results = <String?>[];
  await tester.pumpWidget(
    MaterialApp(
      home: Builder(
        builder: (context) => TextButton(
          onPressed: () async => results.add(
            await showRangeEditor(
              context,
              title: 'Test range',
              initialText: initial,
              store: store ?? MemoryRangeStore(),
            ),
          ),
          child: const Text('open'),
        ),
      ),
    ),
  );
  await tester.tap(find.text('open'));
  await tester.pumpAndSettle();
  return results;
}

Offset cellCentre(WidgetTester tester, String name) {
  final hand = HandClass.tryParse(name)!;
  final box = tester.getRect(find.byKey(const Key('range-grid')));
  final cell = box.width / 13;
  return box.topLeft +
      Offset((hand.index % 13 + 0.5) * cell, (hand.index ~/ 13 + 0.5) * cell);
}

String editorText(WidgetTester tester) => tester
    .widget<TextField>(find.byKey(const Key('range-text')))
    .controller!
    .text;

void main() {
  testWidgets('shows the range it was given', (tester) async {
    await pumpEditor(tester, initial: 'AA, AKs');
    expect(find.text('Test range'), findsOneWidget);
    expect(find.text('10.0 combos · 0.8% of hands'), findsOneWidget);
    expect(editorText(tester), 'AA, AKs');
  });

  testWidgets('adds and removes hands by tapping', (tester) async {
    final results = await pumpEditor(tester, initial: 'AA');

    await tester.tapAt(cellCentre(tester, 'KK'));
    await tester.pump();
    expect(editorText(tester), 'AA, KK');
    expect(find.text('12.0 combos · 0.9% of hands'), findsOneWidget);

    // Tapping a hand that is fully in removes it.
    await tester.tapAt(cellCentre(tester, 'AA'));
    await tester.pump();
    expect(editorText(tester), 'KK');

    await tester.tap(find.byKey(const Key('range-use')));
    await tester.pumpAndSettle();
    expect(results, ['KK']);
  });

  testWidgets('paints a row of hands in one drag', (tester) async {
    await pumpEditor(tester);
    final gesture = await tester.startGesture(cellCentre(tester, 'AKs'));
    await gesture.moveTo(cellCentre(tester, 'AQs'));
    await gesture.moveTo(cellCentre(tester, 'AJs'));
    await gesture.up();
    await tester.pump();
    expect(editorText(tester), 'AKs, AQs, AJs');
  });

  testWidgets('paints with the chosen weight', (tester) async {
    await pumpEditor(tester);
    final state = tester.widget<Slider>(find.byKey(const Key('range-brush')));
    state.onChanged!(0.5);
    await tester.pump();
    expect(find.text('Paint with weight 50%'), findsOneWidget);

    await tester.tapAt(cellCentre(tester, 'T9s'));
    await tester.pump();
    expect(editorText(tester), 'T9s:0.5');
    expect(find.text('2.0 combos · 0.2% of hands'), findsOneWidget);
  });

  testWidgets('follows the text and blocks what it cannot read', (
    tester,
  ) async {
    final results = await pumpEditor(tester);

    await tester.enterText(find.byKey(const Key('range-text')), 'QQ+, AKs');
    await tester.pump();
    expect(find.text('22.0 combos · 1.7% of hands'), findsOneWidget);

    await tester.enterText(find.byKey(const Key('range-text')), 'QQ+, banana');
    await tester.pump();
    expect(find.text('Cannot read "banana".'), findsOneWidget);
    expect(
      tester.widget<FilledButton>(find.byKey(const Key('range-use'))).onPressed,
      isNull,
    );

    // The GTO+ form is accepted and comes back in the usual one.
    await tester.enterText(
      find.byKey(const Key('range-text')),
      '[50]AQo[/50],AA',
    );
    await tester.pump();
    await tester.tap(find.byKey(const Key('range-use')));
    await tester.pumpAndSettle();
    expect(results, ['AA, AQo:0.5']);
  });

  testWidgets('clears, and cancel returns nothing', (tester) async {
    final results = await pumpEditor(tester, initial: '22+');
    await tester.tap(find.byKey(const Key('range-clear')));
    await tester.pump();
    expect(editorText(tester), '');
    expect(find.text('0.0 combos · 0.0% of hands'), findsOneWidget);

    await tester.tap(find.text('Cancel'));
    await tester.pumpAndSettle();
    expect(results, [null]);
  });

  testWidgets('saves, loads and deletes named ranges', (tester) async {
    final store = MemoryRangeStore({'Old one': 'KK, QQ'});
    await pumpEditor(tester, initial: 'AA', store: store);
    expect(find.byKey(const Key('saved-Old one')), findsOneWidget);

    await tester.enterText(find.byKey(const Key('range-save-name')), 'Aces');
    await tester.pump();
    await tester.tap(find.byKey(const Key('range-save')));
    await tester.pumpAndSettle();
    expect(await store.load(), {'Aces': 'AA', 'Old one': 'KK, QQ'});
    expect(find.byKey(const Key('saved-Aces')), findsOneWidget);

    // Loading replaces what is on the grid.
    await tester.tap(find.byKey(const Key('saved-Old one')));
    await tester.pump();
    expect(editorText(tester), 'KK, QQ');
    expect(find.text('12.0 combos · 0.9% of hands'), findsOneWidget);

    final chip = tester.widget<InputChip>(find.byKey(const Key('saved-Aces')));
    chip.onDeleted!();
    await tester.pumpAndSettle();
    expect((await store.load()).keys, ['Old one']);
    expect(find.byKey(const Key('saved-Aces')), findsNothing);
  });

  testWidgets('fits a phone screen', (tester) async {
    await pumpEditor(tester, initial: 'AA', size: const Size(390, 844));
    expect(tester.takeException(), isNull);
    expect(find.byKey(const Key('range-grid')), findsOneWidget);
  });

  group('board picker', () {
    test('parseBoard', () {
      expect(parseBoard('Ks Td 7h'), ['Ks', 'Td', '7h']);
      expect(parseBoard('kstd7h4c'), ['Ks', 'Td', '7h', '4c']);
      expect(parseBoard(''), isEmpty);
      expect(parseBoard('Ks Ks 7h'), isEmpty);
      expect(parseBoard('Kx Td 7h'), isEmpty);
    });

    Future<List<String?>> pumpPicker(WidgetTester tester, String board) async {
      tester.view.physicalSize = const Size(1000, 800);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final results = <String?>[];
      await tester.pumpWidget(
        MaterialApp(
          home: Builder(
            builder: (context) => TextButton(
              onPressed: () async => results.add(
                await showDialog<String>(
                  context: context,
                  builder: (_) => BoardPicker(
                    initial: parseBoard(board),
                    random: Random(7),
                  ),
                ),
              ),
              child: const Text('open'),
            ),
          ),
        ),
      );
      await tester.tap(find.text('open'));
      await tester.pumpAndSettle();
      return results;
    }

    FilledButton okButton(WidgetTester tester) =>
        tester.widget<FilledButton>(find.byKey(const Key('board-ok')));

    testWidgets('adds and removes cards, three to five', (tester) async {
      final results = await pumpPicker(tester, 'Ks Td');
      expect(okButton(tester).onPressed, isNull);

      await tester.tap(find.byKey(const Key('board-card-7h')));
      await tester.pump();
      expect(okButton(tester).onPressed, isNotNull);

      await tester.tap(find.byKey(const Key('board-card-Ks')));
      await tester.pump();
      expect(okButton(tester).onPressed, isNull);

      for (final card in ['4c', '2s', 'As']) {
        await tester.tap(find.byKey(Key('board-card-$card')));
        await tester.pump();
      }
      // Five cards: the rest of the deck is locked.
      expect(
        tester
            .widget<OutlinedButton>(find.byKey(const Key('board-card-Qd')))
            .onPressed,
        isNull,
      );
      await tester.tap(find.byKey(const Key('board-ok')));
      await tester.pumpAndSettle();
      expect(results, ['Td 7h 4c 2s As']);
    });

    testWidgets('deals a random flop', (tester) async {
      final results = await pumpPicker(tester, '');
      await tester.tap(find.byKey(const Key('board-random')));
      await tester.pump();
      await tester.tap(find.byKey(const Key('board-ok')));
      await tester.pumpAndSettle();
      expect(parseBoard(results.single!), hasLength(3));
    });
  });

  group('spot setup', () {
    Future<FakeApi> pumpSetup(WidgetTester tester, {RangeStore? store}) async {
      tester.view.physicalSize = const Size(1300, 2600);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final api = FakeApi();
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: SpotSetupScreen(
              api: api,
              store: store,
              onSubmitted: (_) {},
              estimateDelay: Duration.zero,
            ),
          ),
        ),
      );
      await tester.pumpAndSettle();
      return api;
    }

    String field(WidgetTester tester, String key) => tester
        .widget<EditableText>(
          find.descendant(
            of: find.byKey(Key('field-$key')),
            matching: find.byType(EditableText),
          ),
        )
        .controller
        .text;

    Future<void> choose(WidgetTester tester, String key, String item) async {
      await tester.tap(find.byKey(Key('scenario-$key')));
      await tester.pumpAndSettle();
      await tester.tap(find.text(item).last);
      await tester.pumpAndSettle();
    }

    testWidgets('fills in a preflop line', (tester) async {
      final api = await pumpSetup(tester);

      await choose(tester, 'pot-type', '3-bet pot');
      expect(field(tester, 'pot'), '20.5');
      expect(field(tester, 'stack'), '90');
      expect(
        find.textContaining('BTN opens 2.5, BB 3-bets 10, BTN calls'),
        findsOneWidget,
      );
      expect(find.textContaining('OOP is BB, IP is BTN'), findsOneWidget);
      // The 3-bettor's range, with aces, went to the out-of-position box.
      expect(field(tester, 'oop-range'), startsWith('AA'));
      expect(api.estimated.last.pot, 20.5);

      await choose(tester, 'stack', '200 BB');
      expect(field(tester, 'stack'), '190');

      // A seat that cannot defend against the new opener is replaced.
      await choose(tester, 'opener', 'SB');
      expect(find.textContaining('OOP is SB, IP is BB'), findsOneWidget);
    });

    testWidgets('offers nine-handed seats', (tester) async {
      await pumpSetup(tester);
      await choose(tester, 'format', '9-max');
      await choose(tester, 'opener', 'UTG+2');
      expect(find.textContaining('UTG+2 opens 2.5'), findsOneWidget);
    });

    testWidgets('fills in a tree preset', (tester) async {
      final api = await pumpSetup(tester);
      await tester.tap(find.byKey(const Key('preset-Detailed')));
      await tester.pumpAndSettle();

      expect(field(tester, 'turn-oop-bet'), '33, 66, 125');
      expect(field(tester, 'river-ip-raise'), '50, 100');
      expect(field(tester, 'raise-cap'), '3');
      final tree = api.estimated.last.tree;
      expect(tree.river.oop.betPct, [33, 75, 150]);
      expect(tree.river.oop.allin, isTrue);
      expect(tree.turn.oop.allin, isFalse);
      expect(tree.raiseCap, 3);
    });

    testWidgets('edits a range on the grid', (tester) async {
      final api = await pumpSetup(
        tester,
        store: MemoryRangeStore({'Mine': 'AA, KK'}),
      );
      await tester.tap(find.byKey(const Key('edit-ip-range')));
      await tester.pumpAndSettle();
      expect(find.text('In position range'), findsWidgets);

      await tester.tap(find.byKey(const Key('saved-Mine')));
      await tester.pump();
      await tester.tap(find.byKey(const Key('range-use')));
      await tester.pumpAndSettle();

      expect(field(tester, 'ip-range'), 'AA, KK');
      expect(api.estimated.last.ipRange, 'AA, KK');
    });

    testWidgets('picks the board from cards', (tester) async {
      final api = await pumpSetup(tester);
      await tester.tap(find.byKey(const Key('pick-board')));
      await tester.pumpAndSettle();
      // Remove the turn card: a flop spot.
      await tester.tap(find.byKey(const Key('board-card-4c')));
      await tester.pump();
      await tester.tap(find.byKey(const Key('board-ok')));
      await tester.pumpAndSettle();

      expect(field(tester, 'board'), 'Ks Td 7h');
      expect(api.estimated.last.board, 'Ks Td 7h');
      expect(find.text('Flop'), findsOneWidget);
    });
  });
}
