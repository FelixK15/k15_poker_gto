import '../api/gto_api.dart';
import '../common/format.dart';

/// How the PLO hand list is ordered.
enum PloSort {
  weight('Weight'),
  ev('EV'),
  equity('Equity'),

  /// By how often the hand takes the chosen action.
  action('Action');

  const PloSort(this.label);

  final String label;
}

/// One PLO hand at a node.
class PloHand {
  PloHand({
    required this.name,
    required this.reach,
    required this.ev,
    required this.equity,
    required this.frequencies,
    this.locked = false,
  });

  /// Eight characters, cards by rank: "AsKsQhJh".
  final String name;
  final double reach;
  final double ev;
  final double equity;

  /// Per action; empty if the hand's owner does not act here.
  final List<double> frequencies;
  final bool locked;

  /// The four cards: ["As", "Ks", "Qh", "Jh"].
  List<String> get cards => [
    for (var i = 0; i + 1 < name.length; i += 2) name.substring(i, i + 2),
  ];
}

/// A PLO range at a node as a list (a 13×13 grid has no room for 270,725
/// four-card hands). The server sends the hands with the most reach,
/// up to GetNodeRequest.max_hands.
class PloHandList {
  PloHandList._(
    this.hands,
    this.actionCount,
    this.totalHands,
    this.shown,
    this.hasEquity,
  );

  /// Whether the hands carry equities (exact engine only).
  final bool hasEquity;

  factory PloHandList.fromRange(
    PlayerRange range, {
    List<double> strategy = const [],
    int actionCount = 0,
    List<bool> locked = const [],
  }) {
    final count = range.hands.length;
    final actions = actionCount > 0 && strategy.length == actionCount * count
        ? actionCount
        : 0;
    final hands = [
      for (var h = 0; h < count; ++h)
        PloHand(
          name: range.hands[h],
          reach: h < range.reach.length ? range.reach[h] : 0,
          ev: h < range.ev.length ? range.ev[h] : 0,
          equity: h < range.equity.length ? range.equity[h] : 0,
          frequencies: [
            for (var a = 0; a < actions; ++a) strategy[a * count + h],
          ],
          locked: actions > 0 && h < locked.length && locked[h],
        ),
    ];
    final total = range.totalHands > 0 ? range.totalHands : count;
    // Monte Carlo sends no equities.
    final hasEquity = range.equity.isNotEmpty;
    final shown = range.shownReachFraction > 0 ? range.shownReachFraction : 1.0;
    return PloHandList._(hands, actions, total, shown, hasEquity);
  }

  /// In the order the server sent them (the range's own order).
  final List<PloHand> hands;
  final int actionCount;

  /// Hands in the whole range, listed or not.
  final int totalHands;

  /// Share of the range's reach the listed hands hold, 0 to 1.
  final double shown;

  /// The hands still in the range here, ordered by [sort], largest first.
  /// [action] picks the action for [PloSort.action].
  List<PloHand> sorted(PloSort sort, {int action = 0, bool ascending = false}) {
    double key(PloHand hand) => switch (sort) {
      PloSort.weight => hand.reach,
      PloSort.ev => hand.ev,
      PloSort.equity => hand.equity,
      PloSort.action =>
        action < hand.frequencies.length ? hand.frequencies[action] : 0,
    };
    final present = [
      for (final hand in hands)
        if (hand.reach > 0) hand,
    ];
    present.sort((a, b) {
      final order = key(b).compareTo(key(a));
      // Ties by weight, so the list does not jump about.
      final result = order != 0 ? order : b.reach.compareTo(a.reach);
      return ascending ? -result : result;
    });
    return present;
  }

  /// "All 1,204 hands" or "Top 2,000 of 270,725 hands · 93.1% of the
  /// range's weight".
  String get coverage {
    if (hands.length >= totalHands) {
      return 'All ${formatCount(totalHands)} hands';
    }
    return 'Top ${formatCount(hands.length)} of ${formatCount(totalHands)} '
        'hands · ${formatPercent(shown)} of the range\'s weight';
  }
}
