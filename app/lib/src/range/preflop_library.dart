import 'hand_range.dart';

/// Baseline preflop ranges for the usual heads-up postflop spots.
///
/// These are **approximations built from simple rules**, not solver output:
/// hands are ranked by a strength score, each position opens the top slice
/// of that ranking, and defending, 3-betting and 4-betting ranges are cut
/// from it by rules of thumb. They are meant as a sensible starting point to
/// edit, and are to be replaced by solved ranges once the solver can produce
/// them.

enum TableFormat {
  sixMax('6-max', ['UTG', 'HJ', 'CO', 'BTN', 'SB', 'BB']),
  nineMax('9-max', [
    'UTG',
    'UTG+1',
    'UTG+2',
    'LJ',
    'HJ',
    'CO',
    'BTN',
    'SB',
    'BB',
  ]);

  const TableFormat(this.label, this.positions);

  final String label;

  /// In the order they act before the flop.
  final List<String> positions;
}

enum PotType {
  singleRaised('Single-raised pot'),
  threeBet('3-bet pot'),
  fourBet('4-bet pot');

  const PotType(this.label);

  final String label;
}

/// Stack depths the library covers, in big blinds.
const List<int> stackDepths = [100, 200, 300];

/// A spot to solve, as the library fills it in.
class LibraryScenario {
  LibraryScenario({
    required this.description,
    required this.oopPosition,
    required this.ipPosition,
    required this.oopRange,
    required this.ipRange,
    required this.pot,
    required this.effectiveStack,
  });

  /// E.g. "BTN opens 2.5, BB calls".
  final String description;
  final String oopPosition;
  final String ipPosition;
  final HandRange oopRange;
  final HandRange ipRange;

  /// In big blinds, at the start of the flop.
  final double pot;
  final double effectiveStack;
}

class PreflopLibrary {
  const PreflopLibrary._();

  /// Positions that can open the pot: everyone but the big blind.
  static List<String> openers(TableFormat format) =>
      format.positions.sublist(0, format.positions.length - 1);

  /// Positions still to act behind [opener].
  static List<String> defenders(TableFormat format, String opener) {
    final index = format.positions.indexOf(opener);
    return index < 0 ? const [] : format.positions.sublist(index + 1);
  }

  /// Builds the spot in which [opener] raises first in and [defender]
  /// continues, for the given pot type and stack depth.
  static LibraryScenario scenario({
    required TableFormat format,
    required int stack,
    required PotType potType,
    required String opener,
    required String defender,
  }) {
    final positions = format.positions;
    final openerIndex = positions.indexOf(opener);
    final defenderIndex = positions.indexOf(defender);
    if (openerIndex < 0 || defenderIndex <= openerIndex || opener == 'BB') {
      throw ArgumentError('$defender cannot defend against $opener');
    }

    final openPct = _openPercent(format, opener);
    final openSize = opener == 'SB' ? 3.0 : 2.5;
    // Blinds that neither player posted stay in the pot.
    final dead =
        (opener == 'SB' || defender == 'SB' ? 0.0 : 0.5) +
        (defender == 'BB' ? 0.0 : 1.0);

    // After the flop the blinds act first, then the others in table order.
    // So a blind defending is out of position, except the big blind against
    // a small blind open.
    final defenderInBlinds = defender == 'SB' || defender == 'BB';
    final defenderIsOop = defenderInBlinds && opener != 'SB';

    final openRange = _top(openPct);
    final threeBetRange = _threeBetRange(openPct);
    final fourBetValue = _classes(_fourBetValue[stack]!);

    final HandRange openerRange, defenderRange;
    final double invested;
    final String description;
    final threeBetSize = openSize * (defenderIsOop ? 4 : 3);
    final fourBetSize = threeBetSize * 2.3;

    switch (potType) {
      case PotType.singleRaised:
        openerRange = openRange;
        // Hands good enough to continue, minus those that re-raise.
        defenderRange = _minus(
          _top(_continuePercent(defender, openPct)),
          _top(_threeBetValuePercent(openPct)),
        );
        invested = openSize;
        description = '$opener opens ${_chips(openSize)}, $defender calls';
      case PotType.threeBet:
        defenderRange = threeBetRange;
        // The better half of the opening range, minus what 4-bets.
        openerRange = _minus(_top(openPct * 0.5), fourBetValue);
        invested = threeBetSize;
        description =
            '$opener opens ${_chips(openSize)}, $defender 3-bets '
            '${_chips(threeBetSize)}, $opener calls';
      case PotType.fourBet:
        openerRange = _union(fourBetValue, _classes(_fourBetBluffs));
        defenderRange = _classes(_callFourBet[stack]!);
        invested = fourBetSize;
        description =
            '$opener opens ${_chips(openSize)}, $defender 3-bets '
            '${_chips(threeBetSize)}, $opener 4-bets ${_chips(fourBetSize)}, '
            '$defender calls';
    }

    return LibraryScenario(
      description: description,
      oopPosition: defenderIsOop ? defender : opener,
      ipPosition: defenderIsOop ? opener : defender,
      oopRange: defenderIsOop ? defenderRange : openerRange,
      ipRange: defenderIsOop ? openerRange : defenderRange,
      pot: 2 * invested + dead,
      effectiveStack: stack - invested,
    );
  }

  static String _chips(double value) =>
      value == value.roundToDouble() ? value.toStringAsFixed(0) : '$value';

  // --- The rules -----------------------------------------------------------

  /// Share of hands a position opens, by how many seats it is from the button.
  static double _openPercent(TableFormat format, String position) {
    if (position == 'SB') return 40;
    const byDistance = [43.0, 27.0, 20.0, 16.0, 13.0, 11.0, 10.0];
    final distance =
        format.positions.indexOf('BTN') - format.positions.indexOf(position);
    return byDistance[distance.clamp(0, byDistance.length - 1)];
  }

  /// Share of hands [defender] plays at all against an open of [openPct].
  static double _continuePercent(String defender, double openPct) {
    switch (defender) {
      case 'BB':
        return 10 + openPct; // closing the action, already a blind in
      case 'SB':
        return 4 + 0.3 * openPct;
      default:
        return 3 + 0.35 * openPct;
    }
  }

  /// Share of hands that 3-bet for value against an open of [openPct].
  static double _threeBetValuePercent(double openPct) => 2 + 0.12 * openPct;

  /// A 3-betting range: the best hands plus suited bluffs, more of them the
  /// wider the open.
  static HandRange _threeBetRange(double openPct) {
    const bluffs = [
      'A5s',
      'A4s',
      'A3s',
      'A2s',
      'K9s',
      'Q9s',
      'J9s',
      'T8s',
      '97s',
      '86s',
      '76s',
      '65s',
    ];
    final count = (2 + (openPct - 16) / 24 * (bluffs.length - 2)).round().clamp(
      2,
      bluffs.length,
    );
    return _union(_top(3 + 0.15 * openPct), _classes(bluffs.sublist(0, count)));
  }

  // The top of the ranges is too important to leave to a score, so these are
  // spelled out. Deeper stacks 4-bet a tighter range and call more often.
  static const Map<int, List<String>> _fourBetValue = {
    100: ['AA', 'KK', 'QQ', 'AKs', 'AKo'],
    200: ['AA', 'KK', 'QQ', 'AKs'],
    300: ['AA', 'KK', 'AKs'],
  };
  static const List<String> _fourBetBluffs = ['A5s', 'A4s'];
  static const Map<int, List<String>> _callFourBet = {
    // At 100BB aces and kings go all-in instead of calling.
    100: ['QQ', 'JJ', 'TT', 'AKs', 'AKo', 'AQs', 'AJs', 'KQs'],
    200: [
      'AA',
      'KK',
      'QQ',
      'JJ',
      'TT',
      'AKs',
      'AKo',
      'AQs',
      'AJs',
      'KQs',
      'A5s',
    ],
    300: [
      'AA',
      'KK',
      'QQ',
      'JJ',
      'TT',
      '99',
      'AKs',
      'AKo',
      'AQs',
      'AJs',
      'KQs',
      'KJs',
      'QJs',
      'JTs',
      'A5s',
    ],
  };

  // --- Hand ranking --------------------------------------------------------

  /// All classes from strongest to weakest by [handScore].
  static final List<HandClass> ranking = () {
    final classes = [...HandClass.all];
    classes.sort((a, b) {
      final byScore = handScore(b).compareTo(handScore(a));
      if (byScore != 0) return byScore;
      // Ties: higher cards first, suited before offsuit.
      final byHigh = b.high.compareTo(a.high);
      if (byHigh != 0) return byHigh;
      final byLow = b.low.compareTo(a.low);
      if (byLow != 0) return byLow;
      return (b.suited ? 1 : 0).compareTo(a.suited ? 1 : 0);
    });
    return classes;
  }();

  /// Preflop strength of a class by the Chen formula: high card, pairs,
  /// suitedness and connectedness.
  static double handScore(HandClass hand) {
    double card(int rank) => switch (rank) {
      12 => 10,
      11 => 8,
      10 => 7,
      9 => 6,
      _ => (rank + 2) / 2,
    };
    if (hand.isPair) {
      final score = card(hand.high) * 2;
      return score < 5 ? 5 : score;
    }
    var score = card(hand.high);
    if (hand.suited) score += 2;
    final gap = hand.high - hand.low - 1;
    score -= switch (gap) {
      0 => 0,
      1 => 1,
      2 => 2,
      3 => 4,
      _ => 5,
    };
    // Low connected cards make more straights.
    if (gap <= 1 && hand.high < 10) score += 1;
    return score;
  }

  /// The best [percent] of all hands, as whole classes.
  static HandRange _top(double percent) {
    final range = HandRange();
    final target = percent.clamp(0, 100) / 100 * HandRange.comboCount;
    var combos = 0;
    for (final hand in ranking) {
      // Stop at whichever side of the target is closer.
      if (combos + hand.combos / 2 > target) break;
      range.setClassWeight(hand, 1);
      combos += hand.combos;
    }
    return range;
  }

  static HandRange _classes(List<String> names) {
    final range = HandRange();
    for (final name in names) {
      range.setClassWeight(HandClass.tryParse(name)!, 1);
    }
    return range;
  }

  static HandRange _union(HandRange a, HandRange b) {
    final result = a.copy();
    for (var i = 0; i < HandRange.comboCount; ++i) {
      if (b.weights[i] > result.weights[i]) result.weights[i] = b.weights[i];
    }
    return result;
  }

  static HandRange _minus(HandRange a, HandRange b) {
    final result = a.copy();
    for (var i = 0; i < HandRange.comboCount; ++i) {
      if (b.weights[i] > 0) result.weights[i] = 0;
    }
    return result;
  }
}
