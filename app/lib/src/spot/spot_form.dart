import 'package:fixnum/fixnum.dart';

import '../api/gto_api.dart';
import '../range/omaha_syntax.dart';

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

/// Position names of a [players]-handed table in seat order, which is also
/// the postflop order (first to act first), as the server names them:
/// 2: BB, BTN; 3: SB, BB, BTN; 4: + CO; 5: + HJ; 6: SB, BB, UTG, HJ, CO, BTN;
/// 7: + LJ; 8: + UTG1; 9: SB, BB, UTG, UTG1, UTG2, LJ, HJ, CO, BTN.
List<String> seatPositions(int players) {
  const full = ['SB', 'BB', 'UTG', 'UTG1', 'UTG2', 'LJ', 'HJ', 'CO', 'BTN'];
  if (players <= 2) return const ['BB', 'BTN'];
  // Seats drop from the early positions as the table gets shorter.
  const dropOrder = ['UTG2', 'UTG1', 'LJ', 'UTG', 'HJ', 'CO'];
  final drop = dropOrder.take(9 - players.clamp(3, 9)).toSet();
  return [
    for (final p in full)
      if (!drop.contains(p)) p,
  ];
}

/// Positions for a multiway postflop spot: the big blind and the latest
/// seats, as in a typical pot that reached the flop (3: BB, CO, BTN).
List<String> multiwayPositions(int players) {
  final seats = seatPositions(9);
  if (players >= 9) return seats;
  return ['BB', ...seats.sublist(9 - (players - 1))];
}

/// Default ranges for a fresh multiway seat.
const String multiwayHoldemRange =
    'TT-22, AJs-A2s, KTs+, QTs+, J9s+, T8s+, 97s+, 86s+, 75s+, 65s, 54s, '
    'AQo-ATo, KJo+, QJo';
const String multiwayOmahaRange = 'top25%';

/// One seat of a multiway spot as typed.
class PlayerInput {
  PlayerInput({required this.position, required this.range, this.stack = ''});

  String position;
  String range;

  /// Empty: the spot's effective stack.
  String stack;
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

/// Reads the shared solve budget fields. Null values mean "not set".
({double? seconds, int? mcIterations, String? error}) parseBudget(
  String maxSeconds,
  String mcMillions,
) {
  final seconds = maxSeconds.trim().isEmpty
      ? 0.0
      : double.tryParse(maxSeconds.trim());
  if (seconds == null || seconds < 0 || !seconds.isFinite) {
    return (
      seconds: null,
      mcIterations: null,
      error: 'The time limit must be 0 or more seconds.',
    );
  }
  final millions = mcMillions.trim().isEmpty
      ? 0.0
      : double.tryParse(mcMillions.trim());
  if (millions == null || millions < 0 || millions > 1e6) {
    return (
      seconds: null,
      mcIterations: null,
      error: 'Monte Carlo iterations must be 0 to 1,000,000 million.',
    );
  }
  return (
    seconds: seconds,
    mcIterations: (millions * 1e6).round(),
    error: null,
  );
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

  /// Hold'em or PLO4. Change it with [setVariant], which keeps each game's
  /// ranges apart.
  Variant variant = Variant.VARIANT_HOLDEM;

  /// Pot-limit betting; on by default for PLO.
  bool potLimit = false;

  /// Two players: [oopRange], [ipRange] and [stack]. More: [players].
  int playerCount = 2;
  final List<PlayerInput> players = [];

  Engine engine = Engine.ENGINE_AUTO;

  /// Monte Carlo budget in millions of sampled iterations; empty for the
  /// server's default.
  String mcIterations = '';

  /// Time limit in seconds for either engine; empty for none.
  String maxSeconds = '';

  /// Indexed [street][player]: flop, turn, river and OOP, IP.
  final List<List<SizingInput>> sizing = [
    [SizingInput(bet: '33'), SizingInput(bet: '33', raise: '60')],
    [SizingInput(bet: '66'), SizingInput(bet: '66', raise: '60')],
    [
      SizingInput(bet: '75', raise: '60', allin: true),
      SizingInput(bet: '75', raise: '60', allin: true),
    ],
  ];

  /// Multiway bet sizes, the same for every player, per street.
  final List<SizingInput> multiwaySizing = [
    SizingInput(bet: '33'),
    SizingInput(bet: '66'),
    SizingInput(bet: '75', allin: true),
  ];

  /// The other game's ranges while this one is shown.
  final Map<Variant, ({String oop, String ip, List<String> seats})> _stashed = {
    Variant.VARIANT_OMAHA4: (oop: 'top5%', ip: 'top8%', seats: const []),
  };

  bool get isOmaha => variant == Variant.VARIANT_OMAHA4;
  bool get isMultiway => playerCount > 2;

  /// Switches the game, putting this game's ranges aside and bringing back
  /// the other's. Pot-limit follows the game's usual default.
  void setVariant(Variant value) {
    if (value == variant) return;
    _stashed[variant] = (
      oop: oopRange,
      ip: ipRange,
      seats: [for (final p in players) p.range],
    );
    variant = value;
    potLimit = isOmaha;
    final back = _stashed[value];
    if (back != null) {
      oopRange = back.oop;
      ipRange = back.ip;
    }
    final seatDefault = isOmaha ? multiwayOmahaRange : multiwayHoldemRange;
    for (var i = 0; i < players.length; ++i) {
      players[i].range = back != null && i < back.seats.length
          ? back.seats[i]
          : seatDefault;
    }
  }

  /// Sets the number of players, adding seats with default positions and
  /// ranges or dropping the last ones. Seats the user named keep the name.
  void setPlayerCount(int count) {
    playerCount = count.clamp(2, 9);
    if (playerCount == 2) return;
    final seatDefault = isOmaha ? multiwayOmahaRange : multiwayHoldemRange;
    while (players.length < playerCount) {
      players.add(PlayerInput(position: '', range: seatDefault));
    }
    players.length = playerCount;
    final positions = multiwayPositions(playerCount);
    final defaults = {'', for (var n = 3; n <= 9; ++n) ...multiwayPositions(n)};
    for (var i = 0; i < playerCount; ++i) {
      if (defaults.contains(players[i].position)) {
        players[i].position = positions[i];
      }
    }
    // Renaming can leave a default name twice; give those their seat's.
    final used = <String>{};
    for (var i = 0; i < playerCount; ++i) {
      if (!used.add(players[i].position)) players[i].position = 'P${i + 1}';
    }
  }

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
      final sizingError = (
        spot: null,
        error:
            'The ${streetNames[street]} sizes must be positive numbers, '
            'separated by commas.',
      );
      if (isMultiway) {
        final all = multiwaySizing[street].toProto();
        if (all == null) return sizingError;
        streets.add(StreetSizing(all: all));
      } else {
        final oop = sizing[street][0].toProto();
        final ip = sizing[street][1].toProto();
        if (oop == null || ip == null) return sizingError;
        streets.add(StreetSizing(oop: oop, ip: ip));
      }
    }

    final spot = Spot(
      board: board.trim(),
      pot: potValue,
      effectiveStack: stackValue,
      tree: TreeSettings(
        flop: streets[0],
        turn: streets[1],
        river: streets[2],
        raiseCap: cap,
        allinThresholdPct: threshold,
      ),
    );
    // Hold'em heads-up on auto stays a v1 request, so older servers take it.
    if (isOmaha) spot.variant = variant;
    if (potLimit != isOmaha) spot.potLimit = potLimit;
    if (engine != Engine.ENGINE_AUTO) spot.engine = engine;

    if (isMultiway) {
      final seen = <String>{};
      for (var i = 0; i < playerCount; ++i) {
        final player = players[i];
        final position = player.position.trim();
        final name = position.isEmpty ? 'Player ${i + 1}' : position;
        if (position.isNotEmpty &&
            !RegExp(r'^[A-Za-z0-9+_-]{1,12}$').hasMatch(position)) {
          return (
            spot: null,
            error: '$name: a position is 1 to 12 letters, digits, +, _ or -.',
          );
        }
        if (position.isNotEmpty && !seen.add(position.toUpperCase())) {
          return (spot: null, error: 'Two players are both $position.');
        }
        final playerStack = player.stack.trim().isEmpty
            ? 0.0
            : double.tryParse(player.stack.trim());
        if (playerStack == null || playerStack < 0) {
          return (spot: null, error: '$name: the stack must be 0 or more.');
        }
        final rangeError = _checkRange(player.range);
        if (rangeError != null) {
          return (spot: null, error: '$name: $rangeError');
        }
        spot.players.add(
          PlayerSpot(
            position: position,
            range: player.range.trim(),
            stack: playerStack,
          ),
        );
      }
    } else {
      if (isOmaha) {
        final oopError = _checkRange(oopRange);
        if (oopError != null) {
          return (spot: null, error: 'Out of position range: $oopError');
        }
        final ipError = _checkRange(ipRange);
        if (ipError != null) {
          return (spot: null, error: 'In position range: $ipError');
        }
      }
      spot
        ..oopRange = oopRange.trim()
        ..ipRange = ipRange.trim();
    }
    return (spot: spot, error: null);
  }

  /// PLO text is checked for shape here; Hold'em ranges are left to the
  /// server, as before.
  String? _checkRange(String text) {
    if (text.trim().isEmpty) return 'the range is empty.';
    if (!isOmaha) return null;
    return checkOmahaRange(text);
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
    final budget = parseBudget(maxSeconds, mcIterations);
    if (budget.error != null) return (settings: null, error: budget.error);
    final settings = SolveSettings(
      maxIterations: iterations,
      targetExploitabilityPct: target,
    );
    if (budget.seconds! > 0) settings.maxSeconds = budget.seconds!;
    if (budget.mcIterations! > 0) {
      settings.mcIterations = Int64(budget.mcIterations!);
    }
    return (settings: settings, error: null);
  }
}
