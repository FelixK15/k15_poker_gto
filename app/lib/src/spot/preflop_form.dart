import 'package:fixnum/fixnum.dart';

import '../api/gto_api.dart';
import '../common/solve_labels.dart';
import 'spot_form.dart';

/// Table sizes offered for a preflop solve.
const List<int> preflopTables = [2, 6, 9];

/// Stack depths offered for a preflop solve, in big blinds.
const List<int> preflopStacks = [100, 200, 300];

/// Everything on the preflop solve form, as text. [toSpot] turns it into the
/// request, or says what is wrong.
class PreflopInput {
  int players = 6;
  int stackBb = 100;

  /// Raise-to sizes in big blinds; several separated by commas.
  String openSizes = '2.5';
  String sbOpenSizes = '3';
  String threeBetIp = '3';
  String threeBetOop = '4';
  String fourBet = '2.3';
  bool allowLimp = false;
  bool allowColdCalls = true;
  String rakePct = '0';
  String rakeCap = '0';

  /// Millions of sampled iterations; empty for the server's default.
  String mcIterations = '';

  /// Seconds; empty for no limit.
  String maxSeconds = '';

  /// Position names of the table in seat order.
  List<String> get positions => seatPositions(players);

  String get jobName => 'Preflop · ${tableLabel(players)} · $stackBb BB';

  ({Spot? spot, String? error}) toSpot() {
    final opens = parsePercents(openSizes);
    if (opens == null || opens.isEmpty || opens.any((v) => v <= 1)) {
      return (
        spot: null,
        error: 'Open sizes must be raise-to amounts above 1 BB, e.g. 2.5.',
      );
    }
    final sbOpens = parsePercents(sbOpenSizes);
    if (sbOpens == null || sbOpens.isEmpty || sbOpens.any((v) => v <= 1)) {
      return (
        spot: null,
        error: 'Small blind open sizes must be above 1 BB, e.g. 3.',
      );
    }
    double? multiplier(String text) {
      final value = double.tryParse(text.trim());
      return value == null || value <= 1 || value > 20 ? null : value;
    }

    final ip = multiplier(threeBetIp);
    final oop = multiplier(threeBetOop);
    final four = multiplier(fourBet);
    if (ip == null || oop == null) {
      return (
        spot: null,
        error: '3-bet sizes are multiples of the open, above 1, e.g. 3.',
      );
    }
    if (four == null) {
      return (
        spot: null,
        error: 'The 4-bet size is a multiple of the 3-bet, above 1, e.g. 2.3.',
      );
    }
    final rake = double.tryParse(rakePct.trim());
    if (rake == null || rake < 0 || rake > 100) {
      return (spot: null, error: 'The rake must be 0 to 100%.');
    }
    final cap = double.tryParse(rakeCap.trim());
    if (cap == null || cap < 0) {
      return (spot: null, error: 'The rake cap must be 0 or more big blinds.');
    }
    return (
      spot: Spot(
        rakePct: rake,
        rakeCap: cap,
        preflop: PreflopSettings(
          players: players,
          stackBb: stackBb.toDouble(),
          openSizesBb: opens,
          sbOpenSizesBb: sbOpens,
          threeBetIpMultiplier: ip,
          threeBetOopMultiplier: oop,
          fourBetMultiplier: four,
          allowLimp: allowLimp,
          allowColdCalls: allowColdCalls,
        ),
      ),
      error: null,
    );
  }

  ({SolveSettings? settings, String? error}) toSettings() {
    final budget = parseBudget(maxSeconds, mcIterations);
    if (budget.error != null) return (settings: null, error: budget.error);
    final settings = SolveSettings();
    if (budget.seconds! > 0) settings.maxSeconds = budget.seconds!;
    if (budget.mcIterations! > 0) {
      settings.mcIterations = Int64(budget.mcIterations!);
    }
    return (settings: settings, error: null);
  }
}
