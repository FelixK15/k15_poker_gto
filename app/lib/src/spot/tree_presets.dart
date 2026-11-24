import 'spot_form.dart';

/// A ready-made set of bet sizes: how detailed the tree is.
class TreePreset {
  const TreePreset({
    required this.name,
    required this.description,
    required this.bets,
    required this.raises,
    required this.raiseCap,
  });

  final String name;
  final String description;

  /// Bet sizes for flop, turn and river, in % of the pot.
  final List<String> bets;

  /// Raise sizes for flop, turn and river.
  final List<String> raises;
  final String raiseCap;

  /// Writes the preset into the form. Both players get the same sizes, and
  /// all-in is offered on the river.
  void applyTo(SpotInput input) {
    for (var street = 0; street < 3; ++street) {
      for (var player = 0; player < 2; ++player) {
        input.sizing[street][player]
          ..bet = bets[street]
          ..raise = raises[street]
          ..allin = street == 2;
      }
    }
    input.raiseCap = raiseCap;
  }
}

/// From fastest to most detailed. Trees grow quickly with every size, most of
/// all on the flop.
const List<TreePreset> treePresets = [
  TreePreset(
    name: 'Simple',
    description: 'One bet size per street. Small and fast.',
    bets: ['33', '66', '75'],
    raises: ['60', '60', '60'],
    raiseCap: '1',
  ),
  TreePreset(
    name: 'Standard',
    description: 'Two bet sizes per street.',
    bets: ['33, 75', '50, 100', '50, 100'],
    raises: ['60', '60', '60'],
    raiseCap: '2',
  ),
  TreePreset(
    name: 'Detailed',
    description:
        'Three bet sizes and two raise sizes. Large; best for turn '
        'and river spots.',
    bets: ['25, 50, 100', '33, 66, 125', '33, 75, 150'],
    raises: ['50, 100', '50, 100', '50, 100'],
    raiseCap: '3',
  ),
];
