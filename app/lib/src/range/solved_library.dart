import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart' show rootBundle;

import '../common/format.dart';
import 'preflop_library.dart';

/// The preflop range library solved by K15 GTO's own preflop solver,
/// shipped with the app as assets/ranges/*.txt (copies of the repository's
/// ranges/ folder; the format is described in ranges/README.txt and read by
/// gto::mc::read_preflop_library on the C++ side).
///
/// Caveat that comes with it: there is no postflop play. When the preflop
/// betting ends the board is run out, so every player realises all of their
/// equity, and calling ranges (the big blind's above all) come out wider
/// than a full solution's.
class SolvedLibrary {
  SolvedLibrary({
    required this.format,
    required this.players,
    required this.stackBb,
    required this.settings,
    required this.iterations,
    required this.exploitabilityPct,
    required this.open,
    required this.scenarios,
  });

  /// "6max" or "9max".
  final String format;
  final int players;
  final double stackBb;

  /// Free text: the sizes the tree used.
  final String settings;
  final int iterations;

  /// The engine's sampled estimate, % of the starting pot. No Nash
  /// guarantee with more than two players.
  final double exploitabilityPct;

  /// Share of all hands each position raises first in.
  final Map<String, double> open;
  final List<SolvedScenario> scenarios;

  /// "1.77 G iterations · ≈ 0.27% exploitable (estimate)".
  String get quality {
    final its = iterations >= 1e9
        ? '${(iterations / 1e9).toStringAsFixed(2)} G'
        : iterations >= 1e6
        ? '${(iterations / 1e6).toStringAsFixed(0)} M'
        : formatCount(iterations);
    return '$its iterations · ≈ ${exploitabilityPct.toStringAsFixed(2)}% of '
        'pot exploitable (estimate, no equilibrium guarantee)';
  }

  /// The heads-up line in which [opener] raises first and [defender] is the
  /// only other player to act (everyone else folds), for the pot type:
  /// open and call, open 3-bet call, or open 3-bet 4-bet call. Positions may
  /// be spelled as the app does ("UTG+1") or as the library ("UTG1").
  SolvedScenario? find(PotType potType, String opener, String defender) {
    final a = libraryPosition(opener);
    final b = libraryPosition(defender);
    final want = switch (potType) {
      PotType.singleRaised => ['$a:raise', '$b:call'],
      PotType.threeBet => ['$a:raise', '$b:raise', '$a:call'],
      PotType.fourBet => ['$a:raise', '$b:raise', '$a:raise', '$b:call'],
    };
    for (final scenario in scenarios) {
      final acts = scenario.actions;
      if (acts.length != want.length) continue;
      var match = true;
      for (var i = 0; i < acts.length && match; ++i) {
        final w = want[i];
        match = w.endsWith(':raise')
            ? acts[i].startsWith(w) || acts[i] == '${w.split(':')[0]}:allin'
            : acts[i] == w;
      }
      if (match) return scenario;
    }
    return null;
  }

  /// Asset path of a format and stack, e.g. assets/ranges/6max_100bb.txt.
  static String assetFor(TableFormat format, int stack) =>
      'assets/ranges/${format == TableFormat.nineMax ? '9max' : '6max'}_${stack}bb.txt';

  static final Map<String, Future<SolvedLibrary>> _cache = {};

  /// Reads and parses the bundled file for [format] and [stack], once.
  static Future<SolvedLibrary> load(TableFormat format, int stack) {
    final asset = assetFor(format, stack);
    return _cache[asset] ??= rootBundle
        .loadString(asset)
        .then(parse)
        .catchError((Object error) {
          _cache.remove(asset);
          throw error;
        });
  }

  /// Hands [load] an already parsed library, for tests that cannot wait for
  /// the asset bundle.
  @visibleForTesting
  static void preload(TableFormat format, int stack, SolvedLibrary library) =>
      _cache[assetFor(format, stack)] = SynchronousFuture(library);

  /// Parses a library file. Throws a [FormatException] naming the line for
  /// anything it cannot read; keys it does not know are skipped.
  static SolvedLibrary parse(String text) {
    final lines = text.split(RegExp(r'\r?\n'));
    var index = 0;
    // The first line names the format and its version.
    while (index < lines.length && lines[index].trim().isEmpty) {
      ++index;
    }
    if (index >= lines.length ||
        !lines[index].trim().startsWith('k15-preflop-library')) {
      throw const FormatException('not a K15 preflop library');
    }
    final version = int.tryParse(
      lines[index].trim().split(RegExp(r'\s+')).last,
    );
    if (version != 1) {
      throw FormatException('unsupported library version ${lines[index]}');
    }
    ++index;

    var format = '';
    var players = 0;
    var stack = 0.0;
    var settings = '';
    var iterations = 0;
    var exploitability = 0.0;
    final open = <String, double>{};
    final scenarios = <SolvedScenario>[];
    Map<String, String>? block;

    double number(String value, int line) {
      final parsed = double.tryParse(value);
      if (parsed == null) {
        throw FormatException('line ${line + 1}: "$value" is not a number');
      }
      return parsed;
    }

    void endBlock(int line) {
      final b = block;
      block = null;
      if (b == null) return;
      for (final key in ['oop', 'ip', 'pot', 'effective_stack']) {
        if (!b.containsKey(key)) {
          throw FormatException(
            'scenario ${b['scenario']} before line ${line + 1} has no $key',
          );
        }
      }
      scenarios.add(
        SolvedScenario(
          path: b['scenario']!,
          name: b['name'] ?? b['scenario']!,
          kind: b['kind'] ?? '',
          oop: b['oop']!,
          ip: b['ip']!,
          aggressor: b['aggressor'] ?? '',
          pot: number(b['pot']!, line),
          effectiveStack: number(b['effective_stack']!, line),
          frequency: double.tryParse(b['frequency'] ?? '') ?? 0,
          oopRange: b['oop_range'] ?? '',
          ipRange: b['ip_range'] ?? '',
        ),
      );
    }

    for (; index < lines.length; ++index) {
      final line = lines[index].trim();
      if (line.isEmpty) {
        endBlock(index);
        continue;
      }
      if (line.startsWith('#')) continue;
      final colon = line.indexOf(':');
      if (colon <= 0) {
        throw FormatException('line ${index + 1}: expected "key: value"');
      }
      final key = line.substring(0, colon).trim();
      final value = line.substring(colon + 1).trim();
      if (key == 'scenario') {
        endBlock(index);
        block = {'scenario': value};
        continue;
      }
      final b = block;
      if (b != null) {
        b[key] = value;
        continue;
      }
      switch (key) {
        case 'format':
          format = value;
        case 'players':
          players = number(value, index).round();
        case 'stack_bb':
          stack = number(value, index);
        case 'settings':
          settings = value;
        case 'iterations':
          iterations = number(value, index).round();
        case 'exploitability_pct':
          exploitability = number(value, index);
        case 'open':
          final parts = value.split(RegExp(r'\s+'));
          if (parts.length != 2) {
            throw FormatException('line ${index + 1}: "open: POS share"');
          }
          open[parts[0]] = number(parts[1], index);
        default:
          // Keys this reader does not know are skipped.
          break;
      }
    }
    endBlock(lines.length);
    if (players < 2 || stack <= 0) {
      throw const FormatException('the header has no players or stack_bb');
    }
    return SolvedLibrary(
      format: format,
      players: players,
      stackBb: stack,
      settings: settings,
      iterations: iterations,
      exploitabilityPct: exploitability,
      open: open,
      scenarios: scenarios,
    );
  }
}

/// One heads-up flop line of the solved library.
class SolvedScenario {
  SolvedScenario({
    required this.path,
    required this.name,
    required this.kind,
    required this.oop,
    required this.ip,
    required this.aggressor,
    required this.pot,
    required this.effectiveStack,
    required this.frequency,
    required this.oopRange,
    required this.ipRange,
  });

  /// Positional path, e.g. "BTN:raise2.3/BB:call".
  final String path;

  /// E.g. "BTN open 2.3, BB call".
  final String name;

  /// limped, srp, 3bet, 4bet or 5bet+.
  final String kind;
  final String oop;
  final String ip;
  final String aggressor;

  /// At the flop, in big blinds.
  final double pot;
  final double effectiveStack;

  /// Share of deals that follow this line.
  final double frequency;
  final String oopRange;
  final String ipRange;

  /// The path's actions other than folds, e.g. ["BTN:raise2.3", "BB:call"].
  List<String> get actions => [
    for (final segment in path.split('/'))
      if (!segment.endsWith(':fold')) segment,
  ];
}

/// The library's spelling of a position: "UTG+1" becomes "UTG1".
String libraryPosition(String position) => position.replaceAll('+', '');
