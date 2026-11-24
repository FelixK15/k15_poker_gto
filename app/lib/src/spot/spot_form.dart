import '../api/gto_api.dart';

/// Reads a list of percentages such as "33, 75" or "33 75". Returns null if
/// any entry is not a positive number.
List<double>? parsePercents(String text) {
  final values = <double>[];
  for (final part in text.split(RegExp(r'[,;\s]+'))) {
    if (part.isEmpty) continue;
    final value = double.tryParse(part.replaceAll('%', ''));
    if (value == null || value <= 0 || !value.isFinite) return null;
    values.add(value);
  }
  return values;
}

/// Number of cards in a board text such as "Ks Td 7h", or null if it does not
/// look like a list of cards. The server does the real validation.
int? countBoardCards(String text) {
  final compact = text.replaceAll(RegExp(r'[\s,]+'), '');
  if (compact.length.isOdd) return null;
  if (!RegExp(r'^([2-9TJQKAtjqka][cdhsCDHS])*$').hasMatch(compact)) return null;
  return compact.length ~/ 2;
}

/// What the user typed for one player on one street.
class SizingInput {
  SizingInput({this.bet = '', this.raise = '', this.allin = false});

  String bet;
  String raise;
  bool allin;

  /// Null if a size is not a positive number.
  BetSizing? toProto() {
    final bets = parsePercents(bet);
    final raises = parsePercents(raise);
    if (bets == null || raises == null) return null;
    return BetSizing(betPct: bets, raisePct: raises, allin: allin);
  }
}

/// Everything on the spot setup form, as text. [toSpot] turns it into the
/// request, or says what is wrong.
class SpotInput {
  String board = 'Ks Td 7h 4c';
  String oopRange =
      'TT-22, AQo-ATo, KQo-KTo, QJo, AJs-A2s, KQs-K2s, QJs-Q8s, JTs-J8s, '
      'T9s-T7s, 98s-97s, 87s, 76s, 65s';
  String ipRange =
      'AA:0.25, KK:0.5, QQ-22, AKs-A2s, AKo-ATo, KQs-K8s, KQo-KTo, QJs-Q9s, '
      'QJo, JTs-J9s, T9s, 98s, 87s, 76s';
  String pot = '10';
  String stack = '95';
  String raiseCap = '2';
  String allinThreshold = '67';
  String maxIterations = '500';
  String targetExploitability = '0.5';

  /// Indexed [street][player]: flop, turn, river and OOP, IP.
  final List<List<SizingInput>> sizing = [
    [SizingInput(bet: '33'), SizingInput(bet: '33', raise: '60')],
    [SizingInput(bet: '66'), SizingInput(bet: '66', raise: '60')],
    [
      SizingInput(bet: '75', raise: '60', allin: true),
      SizingInput(bet: '75', raise: '60', allin: true),
    ],
  ];

  /// The spot, or a message naming the first field that cannot be read.
  ({Spot? spot, String? error}) toSpot() {
    final potValue = double.tryParse(pot.trim());
    if (potValue == null || potValue <= 0) {
      return (spot: null, error: 'The pot must be a number above 0.');
    }
    final stackValue = double.tryParse(stack.trim());
    if (stackValue == null || stackValue < 0) {
      return (spot: null, error: 'The stack must be a number, 0 or more.');
    }
    final cap = int.tryParse(raiseCap.trim());
    if (cap == null || cap < 0 || cap > 20) {
      return (
        spot: null,
        error: 'The raise cap must be a whole number, 0 to 20.',
      );
    }
    final threshold = double.tryParse(allinThreshold.trim());
    if (threshold == null || threshold < 0 || threshold > 100) {
      return (spot: null, error: 'The all-in threshold must be 0 to 100.');
    }

    const streetNames = ['flop', 'turn', 'river'];
    final streets = <StreetSizing>[];
    for (var street = 0; street < 3; ++street) {
      final oop = sizing[street][0].toProto();
      final ip = sizing[street][1].toProto();
      if (oop == null || ip == null) {
        return (
          spot: null,
          error:
              'The ${streetNames[street]} sizes must be positive numbers, '
              'separated by commas.',
        );
      }
      streets.add(StreetSizing(oop: oop, ip: ip));
    }

    return (
      spot: Spot(
        board: board.trim(),
        oopRange: oopRange.trim(),
        ipRange: ipRange.trim(),
        pot: potValue,
        effectiveStack: stackValue,
        tree: TreeSettings(
          flop: streets[0],
          turn: streets[1],
          river: streets[2],
          raiseCap: cap,
          allinThresholdPct: threshold,
        ),
      ),
      error: null,
    );
  }

  /// The solve settings, or a message.
  ({SolveSettings? settings, String? error}) toSettings() {
    final iterations = int.tryParse(maxIterations.trim());
    if (iterations == null || iterations < 1 || iterations > 1000000) {
      return (
        settings: null,
        error: 'Iterations must be a whole number from 1 to 1,000,000.',
      );
    }
    final target = double.tryParse(targetExploitability.trim());
    if (target == null || target < 0) {
      return (settings: null, error: 'The target must be a number, 0 or more.');
    }
    return (
      settings: SolveSettings(
        maxIterations: iterations,
        targetExploitabilityPct: target,
      ),
      error: null,
    );
  }
}
