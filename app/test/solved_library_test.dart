import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/range/hand_range.dart';
import 'package:k15_gto/src/range/preflop_library.dart';
import 'package:k15_gto/src/range/solved_library.dart';

/// Reads a bundled library file from disk, as the app reads the asset.
SolvedLibrary readBundled(TableFormat format, int stack) => SolvedLibrary.parse(
  File(SolvedLibrary.assetFor(format, stack)).readAsStringSync(),
);

/// Share of all 1,326 combos in a range text.
double share(String text) {
  final range = HandRange.parse(text);
  var combos = 0.0;
  for (final hand in HandClass.all) {
    combos += range.classWeight(hand) * hand.combos;
  }
  return combos / 1326;
}

void main() {
  test('the bundled copies match the repository\'s ranges/', () {
    for (final format in TableFormat.values) {
      for (final stack in stackDepths) {
        final asset = File(SolvedLibrary.assetFor(format, stack));
        final source = File('../ranges/${asset.uri.pathSegments.last}');
        if (!source.existsSync()) continue;
        expect(
          asset.readAsStringSync(),
          source.readAsStringSync(),
          reason: '${asset.path} is out of date; copy it from ../ranges',
        );
      }
    }
  });

  for (final format in TableFormat.values) {
    for (final stack in stackDepths) {
      test('parses ${format.label} $stack BB', () {
        final library = readBundled(format, stack);
        expect(library.players, format == TableFormat.sixMax ? 6 : 9);
        expect(library.stackBb, stack);
        expect(library.iterations, greaterThan(100000000));
        expect(library.exploitabilityPct, inInclusiveRange(0, 5));
        expect(library.scenarios.length, greaterThan(100));
        for (final scenario in library.scenarios) {
          expect(scenario.pot, greaterThan(1), reason: scenario.name);
          expect(
            scenario.effectiveStack,
            inInclusiveRange(0, stack),
            reason: scenario.name,
          );
          // Every range text reads with the app's own parser.
          expect(HandRange.tryParse(scenario.oopRange), isNotNull);
          expect(HandRange.tryParse(scenario.ipRange), isNotNull);
        }

        // Earlier positions open less.
        final open = library.open;
        expect(open['UTG']!, lessThan(open['CO']!));
        expect(open['CO']!, lessThan(open['BTN']!));

        // Nearly every opener and defender of the picker has a single-raised
        // pot (a few, like the SB against a BTN open at 6-max, never just
        // call), and the opening range narrows from the button to UTG.
        var pairs = 0, found = 0;
        for (final opener in PreflopLibrary.openers(format)) {
          for (final defender in PreflopLibrary.defenders(format, opener)) {
            ++pairs;
            if (library.find(PotType.singleRaised, opener, defender) != null) {
              ++found;
            }
          }
        }
        expect(found, greaterThanOrEqualTo(pairs * 0.85));
        final utg = library.find(PotType.singleRaised, 'UTG', 'BB')!;
        final btn = library.find(PotType.singleRaised, 'BTN', 'BB')!;
        expect(utg.ip, 'UTG');
        expect(utg.oop, 'BB');
        expect(share(utg.ipRange), lessThan(share(btn.ipRange)));
        // AA is always opened and continued.
        expect(
          HandRange.parse(btn.ipRange).classWeight(HandClass.tryParse('AA')!),
          1,
        );

        // 3-bet and 4-bet pots are narrower and deeper.
        final threeBet = library.find(PotType.threeBet, 'CO', 'BTN');
        final fourBet = library.find(PotType.fourBet, 'CO', 'BTN');
        expect(threeBet, isNotNull);
        expect(fourBet, isNotNull);
        final coSrp = library.find(PotType.singleRaised, 'CO', 'BTN')!;
        expect(threeBet!.pot, greaterThan(coSrp.pot));
        expect(fourBet!.pot, greaterThan(threeBet.pot));
        expect(share(fourBet.oopRange), lessThan(share(coSrp.oopRange)));
      });
    }
  }

  test('maps the app\'s 9-max seat names', () {
    final library = readBundled(TableFormat.nineMax, 100);
    final line = library.find(PotType.singleRaised, 'UTG+1', 'BB');
    expect(line, isNotNull);
    expect(line!.ip, 'UTG1');
  });

  test('rejects what is not a library', () {
    expect(() => SolvedLibrary.parse('hello'), throwsFormatException);
    expect(
      () => SolvedLibrary.parse('k15-preflop-library 2\n'),
      throwsFormatException,
    );
    expect(
      () => SolvedLibrary.parse(
        'k15-preflop-library 1\nplayers: 2\nstack_bb: 100\n\n'
        'scenario: BTN:raise2/BB:call\npot: 4\n',
      ),
      throwsFormatException,
    );
    final tiny = SolvedLibrary.parse(
      'k15-preflop-library 1\nplayers: 2\nstack_bb: 100\nfuture_key: x\n\n'
      'scenario: BTN:raise2/BB:call\noop: BB\nip: BTN\npot: 4\n'
      'effective_stack: 98\noop_range: 22+\nip_range: AA\n',
    );
    expect(tiny.scenarios.single.name, 'BTN:raise2/BB:call');
    expect(tiny.find(PotType.singleRaised, 'BTN', 'BB')!.ipRange, 'AA');
  });
}
