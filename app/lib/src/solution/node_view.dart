import '../api/gto_api.dart';
import '../common/solve_labels.dart';

/// A node read per seat, whatever the server filled in: servers since API
/// 1.4 send `players`, `acting_player`, `committed` and so on for any number
/// of players; older ones (and heads-up shortcuts) only `oop` / `ip`.
class NodeSeats {
  NodeSeats(this.node, this.job);

  final GetNodeResponse node;
  final Job job;

  /// Every player's range in seat order.
  List<PlayerRange> get ranges =>
      node.players.isNotEmpty ? node.players : [node.oop, node.ip];

  int get count => ranges.length;

  /// Position names in seat order.
  List<String> get positions {
    final fromJob = jobPositions(job);
    return [
      for (var i = 0; i < count; ++i)
        ranges[i].position.isNotEmpty
            ? ranges[i].position
            : i < fromJob.length
            ? fromJob[i]
            : 'P${i + 1}',
    ];
  }

  /// Who acts (action node) or folded (fold node).
  int get acting => node.players.isNotEmpty
      ? node.actingPlayer
      : (node.player == Player.PLAYER_OOP ? 0 : 1);

  bool get isAction => node.type == NodeType.NODE_TYPE_ACTION;

  double committed(int seat) {
    if (seat < node.committed.length) return node.committed[seat];
    if (seat == 0) return node.oopCommitted;
    if (seat == 1) return node.ipCommitted;
    return 0;
  }

  bool folded(int seat) => seat < node.folded.length && node.folded[seat];
  bool allIn(int seat) => seat < node.allIn.length && node.allIn[seat];

  /// Chips the player had behind when the spot started.
  double startingStack(int seat) {
    final spot = job.spot;
    if (spot.hasPreflop()) return spot.preflop.stackBb;
    if (seat < spot.players.length && spot.players[seat].stack > 0) {
      return spot.players[seat].stack;
    }
    return spot.effectiveStack;
  }

  /// Chips the player has behind at this node.
  double behind(int seat) => startingStack(seat) - committed(seat);

  /// The player's combos at this node: the sum of reach over the listed
  /// hands, scaled up when the server cut the list, or over the classes.
  double combos(int seat) {
    final range = ranges[seat];
    if (range.hands.isEmpty) {
      return range.classes.fold(0.0, (sum, c) => sum + c.combos);
    }
    final shown = range.reach.fold(0.0, (sum, r) => sum + r);
    final fraction = range.shownReachFraction;
    return fraction > 0 && fraction < 1 ? shown / fraction : shown;
  }
}
