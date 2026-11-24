import 'dart:async';

import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../jobs/jobs_screen.dart';
import 'action_style.dart';
import 'lock_dialog.dart';
import 'matrix_model.dart';
import 'strategy_matrix.dart';

/// Walks through a job's solution: pick actions and cards to move down the
/// tree, and see each player's range and strategy where you are.
class SolutionScreen extends StatefulWidget {
  const SolutionScreen({
    super.key,
    required this.api,
    required this.job,
    this.refreshInterval = const Duration(seconds: 3),
    this.onResolved,
  });

  final GtoApi api;
  final Job job;

  /// How often a solve that is still running is read again.
  final Duration refreshInterval;

  /// Called with the new job when a re-solve under locks was queued.
  final ValueChanged<Job>? onResolved;

  @override
  State<SolutionScreen> createState() => _SolutionScreenState();
}

class _SolutionScreenState extends State<SolutionScreen> {
  late Job _job = widget.job;
  final List<String> _path = [];
  GetNodeResponse? _node;
  String? _error;
  bool _loading = false;
  int _request = 0;

  /// Cell picked in the matrix, and whose range is shown; null follows the
  /// player to act.
  int? _selected;
  Player? _view;

  /// The locks to re-solve with: this solve's own, plus what the user adds.
  late List<NodeLock> _locks = [...widget.job.locks];
  bool _resolving = false;

  /// The same node in the solve this one started from, for comparison.
  GetNodeResponse? _parentNode;

  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _load();
    _timer = Timer.periodic(widget.refreshInterval, (_) => _refreshIfRunning());
  }

  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }

  bool get _running =>
      _job.state == JobState.JOB_STATE_RUNNING ||
      _job.state == JobState.JOB_STATE_QUEUED;

  Future<void> _refreshIfRunning() async {
    if (!_running || _loading) return;
    try {
      final job = await widget.api.getJob(_job.id);
      if (!mounted) return;
      setState(() => _job = job);
      await _load(quiet: true);
    } catch (_) {
      // A missed refresh is not worth an error; the next one may succeed.
    }
  }

  /// Reads the node at the current path. [quiet] keeps what is on screen
  /// until the new data is there.
  Future<void> _load({bool quiet = false}) async {
    final request = ++_request;
    setState(() {
      _loading = true;
      if (!quiet) _error = null;
    });
    try {
      final path = _path.join('/');
      final node = await widget.api.getNode(_job.id, path);
      GetNodeResponse? parentNode;
      if (_job.parentId.isNotEmpty) {
        try {
          parentNode = await widget.api.getNode(_job.parentId, path);
        } catch (_) {
          // The parent may have been deleted; then there is nothing to compare.
        }
      }
      if (!mounted || request != _request) return;
      setState(() {
        _node = node;
        _parentNode = parentNode;
        _error = null;
        _loading = false;
      });
    } catch (error) {
      if (!mounted || request != _request) return;
      setState(() {
        if (!quiet) _error = describeError(error);
        _loading = false;
      });
    }
  }

  void _go(List<String> path) {
    setState(() {
      _path
        ..clear()
        ..addAll(path);
      _selected = null;
      _view = null;
    });
    _load();
  }

  Future<void> _addLock(GetNodeResponse node) async {
    final actor = node.player == Player.PLAYER_OOP ? node.oop : node.ip;
    final model = MatrixModel.fromRange(
      actor,
      strategy: node.strategy,
      actionCount: node.actions.length,
      locked: node.locked,
    );
    // A class picked in the grid counts only if the grid shows the actor.
    final selected = _selected != null && _viewedPlayer(node) == node.player
        ? model.cells[_selected!]
        : null;
    final lock = await showLockDialog(
      context,
      node: node,
      model: model,
      selected: selected,
    );
    if (lock == null || !mounted) return;
    setState(() => _locks = mergeLock(_locks, lock));
  }

  Future<void> _resolve() async {
    setState(() => _resolving = true);
    final messenger = ScaffoldMessenger.of(context);
    try {
      final job = await widget.api.resolve(
        _job.id,
        _locks,
        SolveSettings(
          maxIterations: 300,
          targetExploitabilityPct: _job.settings.targetExploitabilityPct,
        ),
      );
      if (!mounted) return;
      widget.onResolved?.call(job);
      messenger.showSnackBar(
        SnackBar(content: Text('Re-solve queued as "${job.name}".')),
      );
      await Navigator.of(context).maybePop();
    } catch (error) {
      if (!mounted) return;
      messenger.showSnackBar(SnackBar(content: Text(describeError(error))));
    } finally {
      if (mounted) setState(() => _resolving = false);
    }
  }

  Widget _locksCard(BuildContext context) {
    final theme = Theme.of(context);
    final changed = !sameLocks(_locks, widget.job.locks);
    return Card(
      key: const Key('locks-card'),
      margin: const EdgeInsets.only(top: 12),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 12, 4, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Locks', style: theme.textTheme.titleSmall),
            for (final lock in _locks)
              Row(
                children: [
                  Expanded(
                    child: InkWell(
                      onTap: () => _go(
                        lock.path.isEmpty ? const [] : lock.path.split('/'),
                      ),
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 6),
                        child: Text(
                          '${lock.path.isEmpty ? 'Start' : lock.path.split('/').map(pathStepTitle).join(' › ')}'
                          ' · ${lock.hands.length} hands',
                        ),
                      ),
                    ),
                  ),
                  IconButton(
                    key: Key('remove-lock-${lock.path}'),
                    tooltip: 'Remove',
                    icon: const Icon(Icons.close, size: 18),
                    onPressed: () => setState(
                      () => _locks = [
                        for (final other in _locks)
                          if (!identical(other, lock)) other,
                      ],
                    ),
                  ),
                ],
              ),
            const SizedBox(height: 4),
            if (!changed)
              Text(
                'These locks are in force in this solve.',
                style: theme.textTheme.bodySmall?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
              )
            else ...[
              FilledButton.icon(
                key: const Key('resolve-button'),
                onPressed: _running || _resolving ? null : _resolve,
                icon: const Icon(Icons.lock, size: 18),
                label: const Text('Re-solve with these locks'),
              ),
              if (_running)
                Padding(
                  padding: const EdgeInsets.only(top: 4),
                  child: Text(
                    'Pause this solve first, or wait until it is done.',
                    style: theme.textTheme.bodySmall,
                  ),
                ),
            ],
          ],
        ),
      ),
    );
  }

  /// How this node differs from the same node in the parent solve.
  Widget _compareCard(BuildContext context, GetNodeResponse node) {
    final parent = _parentNode!;
    final theme = Theme.of(context);
    String signed(double value) =>
        '${value >= 0 ? '+' : '−'}${value.abs().toStringAsFixed(2)}';
    final parentFrequency = {
      for (final action in parent.actions) action.label: action.frequency,
    };
    return Card(
      key: const Key('compare-card'),
      margin: const EdgeInsets.only(top: 12),
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Compared with the solve it started from',
              style: theme.textTheme.titleSmall,
            ),
            const SizedBox(height: 4),
            Text(
              'EV: OOP ${signed(node.oop.rangeEv - parent.oop.rangeEv)} · '
              'IP ${signed(node.ip.rangeEv - parent.ip.rangeEv)}',
              key: const Key('compare-ev'),
            ),
            for (final action in node.actions)
              if (parentFrequency.containsKey(action.label))
                Text(
                  '${actionTitle(action)}: '
                  '${formatPercent(parentFrequency[action.label]!)} → '
                  '${formatPercent(action.frequency)}',
                ),
          ],
        ),
      ),
    );
  }

  Player _viewedPlayer(GetNodeResponse node) =>
      _view ??
      (node.type == NodeType.NODE_TYPE_ACTION
          ? node.player
          : Player.PLAYER_OOP);

  @override
  Widget build(BuildContext context) {
    final node = _node;
    return Scaffold(
      appBar: AppBar(
        title: Text(_job.name.isEmpty ? _job.spot.board : _job.name),
        actions: [
          Padding(
            padding: const EdgeInsets.symmetric(horizontal: 12),
            child: Center(
              child: Text(
                _running
                    ? '${jobStateLabel(_job.state)} · iteration ${_job.iteration}'
                    : jobStateLabel(_job.state),
                style: Theme.of(context).textTheme.bodySmall,
              ),
            ),
          ),
          IconButton(
            tooltip: 'Read again',
            icon: const Icon(Icons.refresh),
            onPressed: _loading ? null : () => _load(quiet: true),
          ),
        ],
        bottom: _loading
            ? const PreferredSize(
                preferredSize: Size.fromHeight(2),
                child: LinearProgressIndicator(minHeight: 2),
              )
            : null,
      ),
      body: _error != null
          ? _ErrorView(message: _error!, onBack: () => _go(const []))
          : node == null
          ? const Center(child: CircularProgressIndicator())
          : _buildNode(context, node),
    );
  }

  Widget _buildNode(BuildContext context, GetNodeResponse node) {
    final viewed = _viewedPlayer(node);
    final range = viewed == Player.PLAYER_OOP ? node.oop : node.ip;
    final acting =
        node.type == NodeType.NODE_TYPE_ACTION && node.player == viewed;
    final model = MatrixModel.fromRange(
      range,
      strategy: acting ? node.strategy : const [],
      actionCount: acting ? node.actions.length : 0,
      locked: acting ? node.locked : const [],
    );
    final colors = actionColors(node.actions);
    final lockedHere = node.locked.where((flag) => flag).length;

    final header = Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        _Breadcrumb(
          path: _path,
          onTap: (length) => _go(_path.sublist(0, length)),
        ),
        const SizedBox(height: 12),
        _NextStep(
          node: node,
          colors: colors,
          onStep: (step) => _go([..._path, step]),
        ),
        if (node.type == NodeType.NODE_TYPE_ACTION) ...[
          const SizedBox(height: 8),
          Wrap(
            spacing: 12,
            crossAxisAlignment: WrapCrossAlignment.center,
            children: [
              OutlinedButton.icon(
                key: const Key('lock-button'),
                onPressed: () => _addLock(node),
                icon: const Icon(Icons.lock_outline, size: 18),
                label: const Text('Lock this decision…'),
              ),
              if (lockedHere > 0)
                Text(
                  '$lockedHere of ${node.locked.length} hands are locked here',
                  key: const Key('locked-here'),
                  style: Theme.of(context).textTheme.bodySmall,
                ),
            ],
          ),
        ],
      ],
    );
    final matrix = StrategyMatrix(
      key: const Key('matrix'),
      model: model,
      colors: colors,
      selected: _selected,
      onSelected: (index) =>
          setState(() => _selected = _selected == index ? null : index),
    );
    final details = Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        _Details(
          node: node,
          viewed: viewed,
          model: model,
          colors: colors,
          selected: _selected,
          onView: (player) => setState(() {
            _view = player;
            _selected = null;
          }),
        ),
        if (_locks.isNotEmpty) _locksCard(context),
        if (_parentNode != null && node.type == NodeType.NODE_TYPE_ACTION)
          _compareCard(context, node),
      ],
    );

    return LayoutBuilder(
      builder: (context, constraints) {
        if (constraints.maxWidth >= 900) {
          return Padding(
            padding: const EdgeInsets.all(16),
            child: Row(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      header,
                      const SizedBox(height: 12),
                      Expanded(
                        child: Align(
                          alignment: Alignment.topLeft,
                          child: matrix,
                        ),
                      ),
                    ],
                  ),
                ),
                const SizedBox(width: 16),
                SizedBox(
                  width: 340,
                  child: SingleChildScrollView(child: details),
                ),
              ],
            ),
          );
        }
        return ListView(
          padding: const EdgeInsets.all(12),
          children: [
            header,
            const SizedBox(height: 12),
            matrix,
            const SizedBox(height: 12),
            details,
          ],
        );
      },
    );
  }
}

class _ErrorView extends StatelessWidget {
  const _ErrorView({required this.message, required this.onBack});

  final String message;
  final VoidCallback onBack;

  @override
  Widget build(BuildContext context) {
    return Center(
      child: Padding(
        padding: const EdgeInsets.all(24),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            Text(
              message,
              key: const Key('solution-error'),
              textAlign: TextAlign.center,
              style: TextStyle(color: Theme.of(context).colorScheme.error),
            ),
            const SizedBox(height: 16),
            OutlinedButton(
              onPressed: onBack,
              child: const Text('Back to the start'),
            ),
          ],
        ),
      ),
    );
  }
}

/// The line taken so far; tapping a step goes back to it.
class _Breadcrumb extends StatelessWidget {
  const _Breadcrumb({required this.path, required this.onTap});

  final List<String> path;

  /// Called with the number of steps to keep.
  final ValueChanged<int> onTap;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    Widget crumb(int length, String label, {Color? color}) {
      final current = length == path.length;
      return InkWell(
        key: Key('crumb-$length'),
        borderRadius: BorderRadius.circular(6),
        onTap: current ? null : () => onTap(length),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
          child: Text(
            label,
            style: theme.textTheme.bodyMedium?.copyWith(
              fontWeight: current ? FontWeight.w700 : FontWeight.w400,
              color: color ?? (current ? null : theme.colorScheme.primary),
            ),
          ),
        ),
      );
    }

    return Wrap(
      crossAxisAlignment: WrapCrossAlignment.center,
      children: [
        crumb(0, 'Start'),
        for (var i = 0; i < path.length; ++i) ...[
          Icon(
            Icons.chevron_right,
            size: 16,
            color: theme.colorScheme.onSurfaceVariant,
          ),
          crumb(
            i + 1,
            pathStepTitle(path[i]),
            color: cardTitle(path[i]) != null ? suitColor(path[i]) : null,
          ),
        ],
      ],
    );
  }
}

/// What can happen next at a node: the actions, the cards, or nothing.
class _NextStep extends StatelessWidget {
  const _NextStep({
    required this.node,
    required this.colors,
    required this.onStep,
  });

  final GetNodeResponse node;
  final List<Color> colors;

  /// Called with the path step of the chosen action or card.
  final ValueChanged<String> onStep;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    switch (node.type) {
      case NodeType.NODE_TYPE_ACTION:
        return Wrap(
          spacing: 8,
          runSpacing: 8,
          crossAxisAlignment: WrapCrossAlignment.center,
          children: [
            Text(
              node.player == Player.PLAYER_OOP ? 'OOP acts:' : 'IP acts:',
              style: theme.textTheme.titleSmall,
            ),
            for (var i = 0; i < node.actions.length; ++i)
              OutlinedButton(
                key: Key('action-${node.actions[i].label}'),
                onPressed: () => onStep(node.actions[i].label),
                style: OutlinedButton.styleFrom(
                  side: BorderSide(color: colors[i], width: 2),
                  padding: const EdgeInsets.symmetric(
                    horizontal: 12,
                    vertical: 10,
                  ),
                ),
                child: Text(
                  '${actionTitle(node.actions[i])}  '
                  '${formatPercent(node.actions[i].frequency)}',
                ),
              ),
          ],
        );
      case NodeType.NODE_TYPE_CHANCE:
        return _CardPicker(cards: node.nextCards, onPick: onStep);
      case NodeType.NODE_TYPE_FOLD:
        return Text(
          '${node.player == Player.PLAYER_OOP ? 'OOP' : 'IP'} folds. '
          'The hand is over.',
          style: theme.textTheme.titleSmall,
        );
      default:
        return Text(
          'The hand goes to showdown.',
          style: theme.textTheme.titleSmall,
        );
    }
  }
}

/// The cards that can come next, one row per suit.
class _CardPicker extends StatelessWidget {
  const _CardPicker({required this.cards, required this.onPick});

  final List<String> cards;
  final ValueChanged<String> onPick;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text('Pick the next card:', style: theme.textTheme.titleSmall),
        const SizedBox(height: 8),
        for (final suit in const ['s', 'h', 'd', 'c'])
          Padding(
            padding: const EdgeInsets.only(bottom: 4),
            child: Wrap(
              spacing: 4,
              runSpacing: 4,
              children: [
                // Ace first, like the matrix.
                for (final rank in matrixRanks.split(''))
                  if (cards.contains('$rank$suit'))
                    SizedBox(
                      width: 44,
                      height: 34,
                      child: OutlinedButton(
                        key: Key('card-$rank$suit'),
                        onPressed: () => onPick('$rank$suit'),
                        style: OutlinedButton.styleFrom(
                          padding: EdgeInsets.zero,
                          foregroundColor: suitColor(suit),
                        ),
                        child: Text(cardTitle('$rank$suit')!),
                      ),
                    ),
              ],
            ),
          ),
      ],
    );
  }
}

/// Numbers for the node and for the hand class picked in the matrix.
class _Details extends StatelessWidget {
  const _Details({
    required this.node,
    required this.viewed,
    required this.model,
    required this.colors,
    required this.selected,
    required this.onView,
  });

  final GetNodeResponse node;
  final Player viewed;
  final MatrixModel model;
  final List<Color> colors;
  final int? selected;
  final ValueChanged<Player> onView;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: theme.colorScheme.onSurfaceVariant,
    );
    final cell = selected == null ? null : model.cells[selected!];

    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        SegmentedButton<Player>(
          segments: const [
            ButtonSegment(value: Player.PLAYER_OOP, label: Text('OOP range')),
            ButtonSegment(value: Player.PLAYER_IP, label: Text('IP range')),
          ],
          selected: {viewed},
          onSelectionChanged: (selection) => onView(selection.first),
        ),
        const SizedBox(height: 12),
        Card(
          margin: EdgeInsets.zero,
          child: Padding(
            padding: const EdgeInsets.all(12),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  'Pot ${formatChips(node.pot)}',
                  key: const Key('node-pot'),
                  style: theme.textTheme.titleMedium,
                ),
                const SizedBox(height: 4),
                Text(
                  'EV: OOP ${node.oop.rangeEv.toStringAsFixed(2)} · '
                  'IP ${node.ip.rangeEv.toStringAsFixed(2)}',
                  key: const Key('node-ev'),
                ),
                Text(
                  '${model.totalReach.toStringAsFixed(1)} combos in '
                  '${viewed == Player.PLAYER_OOP ? 'OOP' : 'IP'}\'s range here',
                  style: muted,
                ),
                if (model.actionCount > 0) ...[
                  const SizedBox(height: 12),
                  for (var i = 0; i < node.actions.length; ++i)
                    Padding(
                      padding: const EdgeInsets.symmetric(vertical: 2),
                      child: Row(
                        children: [
                          Container(width: 12, height: 12, color: colors[i]),
                          const SizedBox(width: 8),
                          Expanded(child: Text(actionTitle(node.actions[i]))),
                          Text(formatPercent(node.actions[i].frequency)),
                        ],
                      ),
                    ),
                ],
              ],
            ),
          ),
        ),
        const SizedBox(height: 12),
        if (cell == null)
          Text('Tap a hand in the grid for its details.', style: muted)
        else
          _CellDetails(cell: cell, node: node, colors: colors),
      ],
    );
  }
}

class _CellDetails extends StatelessWidget {
  const _CellDetails({
    required this.cell,
    required this.node,
    required this.colors,
  });

  final MatrixCell cell;
  final GetNodeResponse node;
  final List<Color> colors;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: theme.colorScheme.onSurfaceVariant,
    );
    final present = [
      for (final hand in cell.hands)
        if (hand.reach > 0) hand,
    ];

    return Card(
      key: const Key('cell-details'),
      margin: EdgeInsets.zero,
      child: Padding(
        padding: const EdgeInsets.all(12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(cell.label, style: theme.textTheme.titleMedium),
            if (cell.isEmpty)
              Text('Not in the range here.', style: muted)
            else ...[
              Text(
                '${cell.reach.toStringAsFixed(2)} of ${cell.combos} combos · '
                'EV ${cell.ev.toStringAsFixed(2)}',
                style: muted,
              ),
              const SizedBox(height: 8),
              for (final hand in present) _handRow(context, hand),
            ],
          ],
        ),
      ),
    );
  }

  Widget _handRow(BuildContext context, HandView hand) {
    final first = hand.name.substring(0, 2);
    final second = hand.name.substring(2, 4);
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 3),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Row(
            children: [
              Text.rich(
                TextSpan(
                  children: [
                    TextSpan(
                      text: cardTitle(first),
                      style: TextStyle(color: suitColor(first)),
                    ),
                    TextSpan(
                      text: cardTitle(second),
                      style: TextStyle(color: suitColor(second)),
                    ),
                  ],
                ),
                style: const TextStyle(fontWeight: FontWeight.w600),
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  '${hand.locked ? 'Locked · ' : ''}'
                  'EV ${hand.ev.toStringAsFixed(2)}'
                  '${hand.reach < 0.995 ? ' · ${formatPercent(hand.reach, digits: 0)} here' : ''}',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
              ),
            ],
          ),
          if (hand.frequencies.isNotEmpty)
            Padding(
              padding: const EdgeInsets.only(top: 2),
              child: ClipRRect(
                borderRadius: BorderRadius.circular(2),
                child: SizedBox(
                  height: 8,
                  child: Row(
                    // Without this the coloured boxes have no height.
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      for (var a = 0; a < hand.frequencies.length; ++a)
                        if (hand.frequencies[a] > 0.0005)
                          Expanded(
                            flex: (hand.frequencies[a] * 1000).round(),
                            child: ColoredBox(color: colors[a]),
                          ),
                    ],
                  ),
                ),
              ),
            ),
          if (hand.frequencies.isNotEmpty)
            Text(
              [
                for (var a = 0; a < hand.frequencies.length; ++a)
                  if (hand.frequencies[a] >= 0.005)
                    '${actionTitle(node.actions[a])} '
                        '${formatPercent(hand.frequencies[a], digits: 0)}',
              ].join(' · '),
              style: Theme.of(context).textTheme.bodySmall,
            ),
        ],
      ),
    );
  }
}
