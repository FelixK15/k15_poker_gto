import '../api/gto_api.dart';

/// Ranks from ace down, the order of rows and columns in the matrix.
const String matrixRanks = 'AKQJT98765432';

/// One exact hand, e.g. AhKd, at a node.
class HandView {
  HandView({
    required this.name,
    required this.reach,
    required this.ev,
    required this.frequencies,
    this.locked = false,
  });

  /// Four characters, higher card first.
  final String name;

  /// How much of the hand gets to this node, 0 to 1.
  final double reach;
  final double ev;

  /// Per action; empty if the hand's owner is not the one acting.
  final List<double> frequencies;

  /// Whether the hand's strategy here is fixed by a lock.
  final bool locked;
}

/// One of the 169 hand classes (AA, AKs, AKo, …) at a node.
class MatrixCell {
  MatrixCell(this.label, this.combos, int actions)
    : actionReach = List.filled(actions, 0.0);

  final String label;

  /// Number of exact hands in the class: 6 for pairs, 4 suited, 12 offsuit.
  final int combos;
  final List<HandView> hands = [];

  /// Total reach of the class's hands.
  double reach = 0;

  /// Reach that takes each action.
  final List<double> actionReach;
  double _evTimesReach = 0;

  /// Share of the class that is in the range here, 0 to 1.
  double get fraction => (reach / combos).clamp(0.0, 1.0);

  /// How often the class takes [action], over the hands that get here.
  double frequency(int action) => reach > 0 ? actionReach[action] / reach : 0;

  /// Reach-weighted EV of the class.
  double get ev => reach > 0 ? _evTimesReach / reach : 0;

  bool get isEmpty => reach <= 0;

  /// Whether a lock fixes the strategy of any of the class's hands here.
  bool get hasLock => hands.any((hand) => hand.locked);
}

/// A player's range at a node, arranged as the familiar 13×13 grid: pairs on
/// the diagonal, suited hands above it, offsuit hands below.
class MatrixModel {
  MatrixModel._(this.cells, this.actionCount, this.totalReach);

  /// Builds the grid for [range]. Pass the node's [strategy] and
  /// [actionCount] if this is the acting player's range.
  factory MatrixModel.fromRange(
    PlayerRange range, {
    List<double> strategy = const [],
    int actionCount = 0,
    List<bool> locked = const [],
  }) {
    final handCount = range.hands.length;
    final hasStrategy =
        actionCount > 0 && strategy.length == actionCount * handCount;
    final actions = hasStrategy ? actionCount : 0;

    final cells = List<MatrixCell>.generate(169, (index) {
      final row = index ~/ 13;
      final col = index % 13;
      if (row == col) {
        return MatrixCell('${matrixRanks[row]}${matrixRanks[row]}', 6, actions);
      }
      // Higher rank first in the label, whichever side of the diagonal.
      final high = row < col ? row : col;
      final low = row < col ? col : row;
      final suited = col > row;
      return MatrixCell(
        '${matrixRanks[high]}${matrixRanks[low]}${suited ? 's' : 'o'}',
        suited ? 4 : 12,
        actions,
      );
    });

    var total = 0.0;
    for (var h = 0; h < handCount; ++h) {
      final name = range.hands[h];
      final index = cellIndex(name);
      if (index == null) continue;
      final reach = range.reach[h].toDouble();
      final ev = range.ev[h].toDouble();
      final frequencies = [
        for (var a = 0; a < actions; ++a)
          strategy[a * handCount + h].toDouble(),
      ];

      final cell = cells[index];
      cell.hands.add(
        HandView(
          name: name,
          reach: reach,
          ev: ev,
          frequencies: frequencies,
          locked: actions > 0 && h < locked.length && locked[h],
        ),
      );
      cell.reach += reach;
      cell._evTimesReach += ev * reach;
      for (var a = 0; a < actions; ++a) {
        cell.actionReach[a] += reach * frequencies[a];
      }
      total += reach;
    }
    return MatrixModel._(cells, actions, total);
  }

  /// 169 cells, row by row.
  final List<MatrixCell> cells;

  /// Number of actions the cells hold frequencies for; 0 if none.
  final int actionCount;
  final double totalReach;

  MatrixCell cell(int row, int col) => cells[row * 13 + col];

  /// Grid position of a hand such as "AhKd", or null if it is not one.
  static int? cellIndex(String hand) {
    if (hand.length != 4) return null;
    final first = matrixRanks.indexOf(hand[0].toUpperCase());
    final second = matrixRanks.indexOf(hand[2].toUpperCase());
    if (first < 0 || second < 0) return null;
    final high = first < second ? first : second;
    final low = first < second ? second : first;
    if (high == low) return high * 13 + high;
    final suited = hand[1].toLowerCase() == hand[3].toLowerCase();
    return suited ? high * 13 + low : low * 13 + high;
  }
}
