import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import 'action_style.dart';
import 'matrix_model.dart';

/// Asks how the acting player should be made to play at [node], for the whole
/// range or for the hand class [selected]. Returns the lock, or null if
/// cancelled.
Future<NodeLock?> showLockDialog(
  BuildContext context, {
  required GetNodeResponse node,
  required MatrixModel model,
  MatrixCell? selected,
}) {
  return showDialog<NodeLock>(
    context: context,
    builder: (context) =>
        LockDialog(node: node, model: model, selected: selected),
  );
}

class LockDialog extends StatefulWidget {
  const LockDialog({
    super.key,
    required this.node,
    required this.model,
    this.selected,
  });

  final GetNodeResponse node;

  /// The acting player's range at the node, with its strategy.
  final MatrixModel model;
  final MatrixCell? selected;

  @override
  State<LockDialog> createState() => _LockDialogState();
}

class _LockDialogState extends State<LockDialog> {
  /// Whether the lock covers only the selected class.
  late bool _classOnly = _selectable;

  /// Raw slider positions, one per action; they are scaled to add up to one.
  late List<double> _weights = _currentFrequencies();

  bool get _selectable =>
      widget.selected != null && widget.selected!.hands.isNotEmpty;

  List<HandView> get _hands => _classOnly
      ? widget.selected!.hands
      : [for (final cell in widget.model.cells) ...cell.hands];

  /// What the hands in scope do now, as the starting point.
  List<double> _currentFrequencies() {
    final actions = widget.node.actions.length;
    final totals = List<double>.filled(actions, 0);
    var reach = 0.0;
    for (final hand in _hands) {
      if (hand.frequencies.length != actions) continue;
      for (var a = 0; a < actions; ++a) {
        totals[a] += hand.reach * hand.frequencies[a];
      }
      reach += hand.reach;
    }
    if (reach <= 0) return List<double>.filled(actions, 1 / actions);
    return [for (final total in totals) total / reach];
  }

  double get _sum => _weights.fold(0.0, (a, b) => a + b);

  NodeLock _lock() {
    final sum = _sum;
    final frequencies = [for (final weight in _weights) weight / sum];
    return NodeLock(
      path: widget.node.path,
      hands: [
        for (final hand in _hands)
          HandLock(hand: hand.name, frequencies: frequencies),
      ],
    );
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final node = widget.node;
    final colors = actionColors(node.actions);
    final player = node.player == Player.PLAYER_OOP ? 'OOP' : 'IP';
    final sum = _sum;

    return AlertDialog(
      title: Text('Lock $player\'s strategy here'),
      content: SizedBox(
        width: 460,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'The locked hands will play exactly as set. Re-solving then '
              'finds the best play for everything else.',
              style: theme.textTheme.bodySmall?.copyWith(
                color: theme.colorScheme.onSurfaceVariant,
              ),
            ),
            const SizedBox(height: 12),
            SegmentedButton<bool>(
              segments: [
                ButtonSegment(
                  value: false,
                  label: Text(
                    'Whole range (${[for (final c in widget.model.cells) ...c.hands].length} hands)',
                  ),
                ),
                ButtonSegment(
                  value: true,
                  enabled: _selectable,
                  label: Text(
                    _selectable
                        ? 'Only ${widget.selected!.label}'
                        : 'One hand class',
                  ),
                ),
              ],
              selected: {_classOnly},
              onSelectionChanged: (selection) => setState(() {
                _classOnly = selection.first;
                _weights = _currentFrequencies();
              }),
            ),
            if (!_selectable)
              Padding(
                padding: const EdgeInsets.only(top: 4),
                child: Text(
                  'To lock a single hand class, pick it in the grid first.',
                  style: theme.textTheme.bodySmall?.copyWith(
                    color: theme.colorScheme.onSurfaceVariant,
                  ),
                ),
              ),
            const SizedBox(height: 16),
            for (var a = 0; a < node.actions.length; ++a)
              Row(
                children: [
                  Container(width: 12, height: 12, color: colors[a]),
                  const SizedBox(width: 8),
                  SizedBox(
                    width: 110,
                    child: Text(actionTitle(node.actions[a])),
                  ),
                  Expanded(
                    child: Slider(
                      key: Key('lock-slider-$a'),
                      value: _weights[a].clamp(0.0, 1.0),
                      divisions: 20,
                      onChanged: (value) => setState(() => _weights[a] = value),
                    ),
                  ),
                  SizedBox(
                    width: 52,
                    child: Text(
                      sum > 0
                          ? formatPercent(_weights[a] / sum, digits: 0)
                          : '–',
                      key: Key('lock-share-$a'),
                      textAlign: TextAlign.end,
                    ),
                  ),
                ],
              ),
            const SizedBox(height: 8),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: [
                // One tap for the common "always do this".
                for (var a = 0; a < node.actions.length; ++a)
                  OutlinedButton(
                    key: Key('lock-always-$a'),
                    onPressed: () => setState(() {
                      _weights = [
                        for (var i = 0; i < node.actions.length; ++i)
                          i == a ? 1.0 : 0.0,
                      ];
                    }),
                    child: Text(
                      'Always ${actionTitle(node.actions[a]).toLowerCase()}',
                    ),
                  ),
              ],
            ),
          ],
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.pop(context),
          child: const Text('Cancel'),
        ),
        FilledButton(
          key: const Key('lock-add'),
          onPressed: sum > 0 ? () => Navigator.pop(context, _lock()) : null,
          child: const Text('Add lock'),
        ),
      ],
    );
  }
}

/// Puts [lock] into [locks]: hands it names replace the same hands of an
/// earlier lock on the same node; the rest of that lock stays.
List<NodeLock> mergeLock(List<NodeLock> locks, NodeLock lock) {
  final result = <NodeLock>[];
  var merged = false;
  for (final existing in locks) {
    if (existing.path != lock.path) {
      result.add(existing);
      continue;
    }
    final named = {for (final hand in lock.hands) hand.hand};
    result.add(
      NodeLock(
        path: lock.path,
        hands: [
          for (final hand in existing.hands)
            if (!named.contains(hand.hand)) hand,
          ...lock.hands,
        ],
      ),
    );
    merged = true;
  }
  if (!merged) result.add(lock);
  return result;
}

/// Whether two sets of locks fix the same hands to the same frequencies.
bool sameLocks(List<NodeLock> a, List<NodeLock> b) {
  String key(List<NodeLock> locks) {
    final lines = <String>[
      for (final lock in locks)
        for (final hand in lock.hands)
          '${lock.path}|${hand.hand}|'
              '${hand.frequencies.map((f) => f.toStringAsFixed(4)).join(',')}',
    ]..sort();
    return lines.join('\n');
  }

  return key(a) == key(b);
}
