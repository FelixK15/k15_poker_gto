import 'dart:io';

import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/range/hand_range.dart';
import 'package:k15_gto/src/range/preflop_library.dart';
import 'package:k15_gto/src/range/range_store.dart';

double combos(String text) => HandRange.parse(text).totalCombos;

HandClass hand(String name) => HandClass.tryParse(name)!;

void main() {
  group('HandClass', () {
    test('covers the grid', () {
      expect(HandClass.all, hasLength(169));
      expect(HandClass.all[0].name, 'AA');
      expect(HandClass.all[1].name, 'AKs');
      expect(HandClass.all[13].name, 'AKo');
      expect(HandClass.all[168].name, '22');
      expect(
        HandClass.all.map((h) => h.combos).reduce((a, b) => a + b),
        HandRange.comboCount,
      );
      // No exact hand belongs to two classes.
      final seen = <int>{};
      for (final h in HandClass.all) {
        for (final index in h.comboIndices) {
          expect(seen.add(index), isTrue);
        }
      }
      expect(seen, hasLength(HandRange.comboCount));
    });

    test('parses its own names', () {
      for (final h in HandClass.all) {
        expect(HandClass.tryParse(h.name), same(h));
      }
      expect(HandClass.tryParse('kas'), same(hand('AKs')));
      expect(HandClass.tryParse('AK'), isNull);
      expect(HandClass.tryParse('AAs'), isNull);
      expect(HandClass.tryParse('AKx'), isNull);
    });
  });

  group('HandRange', () {
    test('counts combos per item like the server does', () {
      expect(combos(''), 0);
      expect(combos('AA'), 6);
      expect(combos('AKs'), 4);
      expect(combos('AKo'), 12);
      expect(combos('AK'), 16);
      expect(combos('KA'), 16);
      expect(combos('AhKh'), 1);
      expect(combos('QQ+'), 18);
      expect(combos('22+'), 78);
      expect(combos('ATs+'), 16);
      expect(combos('KTo+'), 36);
      expect(combos('AT+'), 64);
      expect(combos('QQ-TT'), 18);
      expect(combos('TT-QQ'), 18);
      expect(combos('A5s-A2s'), 16);
      expect(combos('AK-AJ'), 48);
      expect(combos('22+, A2s+, K2s+, A2o+'), 78 + 48 + 44 + 144);
    });

    test('reads weights; later items override', () {
      final range = HandRange.parse(' AA:0.5 , AKs:0.25, AhKh ,KQ:0 ');
      expect(range.classWeight(hand('AA')), 0.5);
      expect(range.isMixed(hand('AKs')), isTrue);
      expect(range.totalCombos, closeTo(3 + 0.75 + 1, 1e-9));
    });

    test('reads the GTO+ bracket form', () {
      final range = HandRange.parse('[50]AQo,KQs[/50],AA,[25.0]77[/25.0],KK');
      expect(range.classWeight(hand('AQo')), 0.5);
      expect(range.classWeight(hand('KQs')), 0.5);
      expect(range.classWeight(hand('AA')), 1);
      expect(range.classWeight(hand('77')), 0.25);
      expect(range.classWeight(hand('KK')), 1);
    });

    test('names what it cannot read', () {
      for (final text in [
        'AX',
        'AAs',
        'AKx',
        'AA:1.5',
        'AA:-0.1',
        'AA:',
        'AA:x',
        'AhAh',
        'AKs-KQs',
        'AKs-AQo',
        'QQ-AKs',
        'A',
        'hello',
        '[150]AA[/150]',
      ]) {
        expect(
          () => HandRange.parse(text),
          throwsA(isA<RangeFormatException>()),
          reason: text,
        );
        expect(HandRange.tryParse(text), isNull, reason: text);
      }
      expect(
        () => HandRange.parse('AA, banana'),
        throwsA(
          isA<RangeFormatException>().having(
            (e) => e.message,
            'message',
            contains('banana'),
          ),
        ),
      );
    });

    test('writes text that reads back the same', () {
      for (final text in [
        '',
        'AA',
        'QQ+, AKs, AQo:0.5, A5s-A2s, 98s',
        'AhKh:0.25, AsKs, 72o',
        'AsAh:0.5, 22+, KTo+',
      ]) {
        final original = HandRange.parse(text);
        final again = HandRange.parse(original.toString());
        for (var i = 0; i < HandRange.comboCount; ++i) {
          expect(
            again.weights[i],
            closeTo(original.weights[i], 1e-4),
            reason: text,
          );
        }
      }
      expect(HandRange.parse('AKs, AA:0.5').toString(), 'AA:0.5, AKs');
      expect(HandRange.parse('AhKh, AsKs:0.5').toString(), 'AhKh, AsKs:0.5');
    });

    test('edits whole classes', () {
      final range = HandRange()..setClassWeight(hand('T9s'), 0.75);
      expect(range.totalCombos, 3);
      expect(range.toString(), 'T9s:0.75');
      range.clear();
      expect(range.isEmpty, isTrue);
    });
  });

  group('PreflopLibrary', () {
    test('ranks the obvious hands first and last', () {
      expect(PreflopLibrary.ranking.first.name, 'AA');
      expect(PreflopLibrary.ranking[1].name, 'KK');
      expect(PreflopLibrary.ranking, hasLength(169));
      expect(
        PreflopLibrary.ranking.last.name,
        anyOf('72o', '82o', '32o', '83o'),
      );
    });

    test('builds every scenario with sane numbers', () {
      var count = 0;
      for (final format in TableFormat.values) {
        for (final stack in stackDepths) {
          for (final potType in PotType.values) {
            for (final opener in PreflopLibrary.openers(format)) {
              for (final defender in PreflopLibrary.defenders(format, opener)) {
                final s = PreflopLibrary.scenario(
                  format: format,
                  stack: stack,
                  potType: potType,
                  opener: opener,
                  defender: defender,
                );
                final where =
                    '${format.label} $stack $potType $opener/$defender';
                ++count;
                expect(s.oopRange.isEmpty, isFalse, reason: where);
                expect(s.ipRange.isEmpty, isFalse, reason: where);
                expect(s.pot, greaterThan(4), reason: where);
                expect(
                  s.effectiveStack,
                  greaterThan(stack * 0.7),
                  reason: where,
                );
                // Chips are conserved: stacks plus pot is what both started
                // with, plus at most the two blinds.
                final total = s.pot + 2 * s.effectiveStack;
                expect(total, greaterThanOrEqualTo(2.0 * stack), reason: where);
                expect(
                  total,
                  lessThanOrEqualTo(2.0 * stack + 1.5),
                  reason: where,
                );
                // The ranges survive the trip through text, which is how
                // they reach the form and the server.
                expect(
                  HandRange.parse(s.oopRange.toString()).totalCombos,
                  closeTo(s.oopRange.totalCombos, 1e-9),
                  reason: where,
                );
                expect(
                  {s.oopPosition, s.ipPosition},
                  {opener, defender},
                  reason: where,
                );
              }
            }
          }
        }
      }
      // 15 seat pairs at 6-max and 36 at 9-max, three pot types, three depths.
      expect(count, (15 + 36) * 3 * 3);
    });

    test('puts the right player out of position', () {
      PreflopScenario s(String opener, String defender) =>
          PreflopLibrary.scenario(
            format: TableFormat.sixMax,
            stack: 100,
            potType: PotType.singleRaised,
            opener: opener,
            defender: defender,
          );
      expect(s('BTN', 'BB').oopPosition, 'BB');
      expect(s('BTN', 'SB').oopPosition, 'SB');
      expect(s('CO', 'BTN').oopPosition, 'CO');
      expect(s('UTG', 'HJ').ipPosition, 'HJ');
      // Blind against blind: the small blind acts first after the flop.
      expect(s('SB', 'BB').oopPosition, 'SB');
    });

    test('sizes the pots as the lines say', () {
      PreflopScenario s(PotType type, String opener, String defender) =>
          PreflopLibrary.scenario(
            format: TableFormat.sixMax,
            stack: 100,
            potType: type,
            opener: opener,
            defender: defender,
          );
      // 2.5 + 2.5 + the small blind.
      final srp = s(PotType.singleRaised, 'BTN', 'BB');
      expect(srp.pot, 5.5);
      expect(srp.effectiveStack, 97.5);
      expect(srp.description, 'BTN opens 2.5, BB calls');
      // Both blinds dead.
      expect(s(PotType.singleRaised, 'CO', 'BTN').pot, 6.5);
      // SB opens to 3, nobody else in.
      expect(s(PotType.singleRaised, 'SB', 'BB').pot, 6);
      // BB 3-bets to 10 out of position.
      final threeBet = s(PotType.threeBet, 'BTN', 'BB');
      expect(threeBet.pot, 20.5);
      expect(threeBet.effectiveStack, 90);
      // BTN 3-bets to 7.5 in position.
      expect(s(PotType.threeBet, 'CO', 'BTN').pot, 16.5);
      expect(s(PotType.fourBet, 'BTN', 'BB').pot, closeTo(46.5, 1e-9));
    });

    test('opens wider from later seats', () {
      double opens(String seat) =>
          PreflopLibrary.scenario(
            format: TableFormat.nineMax,
            stack: 100,
            potType: PotType.singleRaised,
            opener: seat,
            defender: 'BB',
          ).ipRange.totalCombos /
          HandRange.comboCount *
          100;
      expect(opens('UTG'), closeTo(10, 2));
      expect(opens('CO'), closeTo(27, 2));
      expect(opens('BTN'), closeTo(43, 2));
      expect(opens('UTG'), lessThan(opens('HJ')));
    });

    test('splits the defender into callers and 3-bettors', () {
      PreflopScenario s(PotType type) => PreflopLibrary.scenario(
        format: TableFormat.sixMax,
        stack: 100,
        potType: type,
        opener: 'BTN',
        defender: 'BB',
      );
      // Aces re-raise, so they are not in the calling range.
      final call = s(PotType.singleRaised).oopRange;
      expect(call.classWeight(hand('AA')), 0);
      expect(call.classWeight(hand('T9s')), 1);
      final threeBet = s(PotType.threeBet).oopRange;
      expect(threeBet.classWeight(hand('AA')), 1);
      expect(threeBet.classWeight(hand('A5s')), 1);
      expect(threeBet.totalCombos, lessThan(call.totalCombos));
      // The opener does not just call a 3-bet with aces at 100BB…
      expect(s(PotType.threeBet).ipRange.classWeight(hand('AA')), 0);
      // …but 4-bets them.
      expect(s(PotType.fourBet).ipRange.classWeight(hand('AA')), 1);
    });

    test('rejects impossible seats', () {
      expect(
        () => PreflopLibrary.scenario(
          format: TableFormat.sixMax,
          stack: 100,
          potType: PotType.singleRaised,
          opener: 'BTN',
          defender: 'CO',
        ),
        throwsArgumentError,
      );
    });
  });

  group('FileRangeStore', () {
    late Directory dir;
    setUp(() => dir = Directory.systemTemp.createTempSync('gto-ranges-'));
    tearDown(() => dir.deleteSync(recursive: true));

    test('saves, lists by name, replaces and deletes', () async {
      final store = FileRangeStore(() async => dir);
      expect(await store.load(), isEmpty);

      await store.save('BTN open', 'AA, KK');
      await store.save('bb defend', '22+');
      await store.save('BTN open', 'AA');
      expect((await store.load()).keys, ['bb defend', 'BTN open']);
      expect((await store.load())['BTN open'], 'AA');

      // A second store on the same folder sees them: they are on disk.
      expect(await FileRangeStore(() async => dir).load(), hasLength(2));

      await store.delete('bb defend');
      await store.delete('never existed');
      expect((await store.load()).keys, ['BTN open']);
    });

    test('creates the folder it needs', () async {
      final nested = Directory('${dir.path}/a/b');
      await FileRangeStore(() async => nested).save('x', 'AA');
      expect(File('${nested.path}/ranges.json').existsSync(), isTrue);
    });

    test('survives a damaged file', () async {
      File('${dir.path}/ranges.json').writeAsStringSync('{ not json');
      final store = FileRangeStore(() async => dir);
      expect(await store.load(), isEmpty);
      await store.save('x', 'AA');
      expect(await store.load(), {'x': 'AA'});
    });
  });
}
