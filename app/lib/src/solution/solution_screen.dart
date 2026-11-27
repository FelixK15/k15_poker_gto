import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/playing_card.dart';
import '../common/solve_labels.dart';
import '../jobs/jobs_screen.dart';
import 'action_line.dart';
import 'action_style.dart';
import 'lock_dialog.dart';
import 'matrix_model.dart';
import 'node_view.dart';
import 'plo_hand_view.dart';
import 'plo_hands.dart';
import 'solution_panels.dart';
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
  int? _view;

  /// PLO: hands per player to ask for.
  int _ploMaxHands = 1000;

  /// Cell under the mouse, shown in the details while it is there.
  int? _hovered;

  /// Action picked in the summary, the only one the grid then shows.
  int? _highlight;

  /// Nodes read so far by path, for the tiles of the action line.
  final Map<String, GetNodeResponse> _nodes = {};
  MatrixView _matrixView = MatrixView.strategy;

  /// The runout report of the chance node on screen, once asked for.
  List<Runout>? _runouts;
  bool _loadingRunouts = false;

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

  bool get _isOmaha => _job.spot.variant == Variant.VARIANT_OMAHA4;
  bool get _isPreflop => _job.spot.hasPreflop();

  /// Locking works on the exact heads-up Hold'em grid.
  bool _canLock(NodeSeats seats) =>
      !_isOmaha &&
      !_isPreflop &&
      seats.count == 2 &&
      _job.engine != Engine.ENGINE_MONTE_CARLO;

  Future<GetNodeResponse> _read(String jobId, String path) =>
      widget.api.getNode(
        jobId,
        path,
        maxHands: _isOmaha ? _ploMaxHands : 0,
        // Preflop nodes hold every hand of every player: classes are enough.
        detail: _isPreflop
            ? HandDetail.HAND_DETAIL_CLASSES
            : HandDetail.HAND_DETAIL_HANDS,
      );

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
      final node = await _read(_job.id, path);
      GetNodeResponse? parentNode;
      if (_job.parentId.isNotEmpty) {
        try {
          parentNode = await _read(_job.parentId, path);
        } catch (_) {
          // The parent may have been deleted; then there is nothing to compare.
        }
      }
      if (!mounted || request != _request) return;
      setState(() {
        // A solve that is still running changes everywhere; keep only what
        // was just read.
        if (_running) _nodes.clear();
        _nodes[path] = node;
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
    final cached = _nodes[path.join('/')];
    setState(() {
      _path
        ..clear()
        ..addAll(path);
      _selected = null;
      _hovered = null;
      _highlight = null;
      _view = null;
      _runouts = null;
      // A node seen before shows at once, and is read again behind it.
      if (cached != null) {
        _node = cached;
        _error = null;
      }
    });
    _load(quiet: cached != null);
  }

  void _back() {
    if (_path.isNotEmpty) _go(_path.sublist(0, _path.length - 1));
  }

  Future<void> _loadRunouts() async {
    final path = _path.join('/');
    setState(() => _loadingRunouts = true);
    try {
      final runouts = await widget.api.getRunouts(_job.id, path);
      // The user may have moved on while it was computed.
      if (!mounted || path != _path.join('/')) return;
      setState(() => _runouts = runouts);
    } catch (error) {
      if (!mounted) return;
      ScaffoldMessenger.of(context)
          .showSnackBar(SnackBar(content: Text(describeError(error))));
    } finally {
      if (mounted) setState(() => _loadingRunouts = false);
    }
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
    final selected =
        _selected != null && _viewedSeat(node) == NodeSeats(node, _job).acting
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
    return PanelSection(
      key: const Key('locks-card'),
      title: 'Locks',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          for (final lock in _locks)
            Row(
              children: [
                const Icon(Icons.lock_outline, size: 16),
                const SizedBox(width: 6),
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
    );
  }

  /// How this node differs from the same node in the parent solve.
  Widget _compareCard(BuildContext context, GetNodeResponse node) {
    final parent = _parentNode!;
    String signed(double value) =>
        '${value >= 0 ? '+' : '−'}${value.abs().toStringAsFixed(2)}';
    final parentFrequency = {
      for (final action in parent.actions) action.label: action.frequency,
    };
    return PanelSection(
      key: const Key('compare-card'),
      title: 'Compared with the solve it started from',
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
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
    );
  }

  /// The seat whose range is shown: the user's pick, else the player to
  /// act, else the first.
  int _viewedSeat(GetNodeResponse node) {
    final seats = NodeSeats(node, _job);
    final view = _view;
    if (view != null && view < seats.count) return view;
    return seats.isAction && seats.acting < seats.count ? seats.acting : 0;
  }

  @override
  Widget build(BuildContext context) {
    final node = _node;
    final theme = Theme.of(context);
    final board = splitCards(_job.spot.board);
    final progress = isMonteCarlo(_job)
        ? '${formatSamples(_job.mcIterations.toInt())} samples'
        : 'iteration ${_job.iteration}';
    return Scaffold(
      appBar: AppBar(
        titleSpacing: 0,
        title: Row(
          children: [
            if (board.isNotEmpty) ...[
              CardRow(board, width: 20, gap: 2),
              const SizedBox(width: 10),
            ],
            Flexible(
              child: Text(jobTitle(_job), overflow: TextOverflow.ellipsis),
            ),
          ],
        ),
        actions: [
          _StatusChip(
            label: _running
                ? '${jobStateLabel(_job.state)} · $progress'
                : jobStateLabel(_job.state),
            active: _running,
          ),
          IconButton(
            tooltip: 'Read again',
            icon: const Icon(Icons.refresh),
            onPressed: _loading ? null : () => _load(quiet: true),
          ),
          const SizedBox(width: 4),
        ],
        bottom: PreferredSize(
          preferredSize: const Size.fromHeight(2),
          child: _loading
              ? const LinearProgressIndicator(minHeight: 2)
              : const SizedBox(height: 2),
        ),
      ),
      backgroundColor: theme.colorScheme.surfaceContainerLowest,
      body: _error != null
          ? _ErrorView(message: _error!, onBack: () => _go(const []))
          : node == null
          ? const Center(child: CircularProgressIndicator())
          : CallbackShortcuts(
              bindings: {
                const SingleActivator(LogicalKeyboardKey.backspace): _back,
                const SingleActivator(LogicalKeyboardKey.arrowLeft): _back,
                const SingleActivator(LogicalKeyboardKey.escape): () =>
                    setState(() {
                      _selected = null;
                      _highlight = null;
                    }),
              },
              child: Focus(autofocus: true, child: _buildNode(context, node)),
            ),
    );
  }

  Widget _buildNode(BuildContext context, GetNodeResponse node) {
    final seats = NodeSeats(node, _job);
    final positions = seats.positions;
    final viewed = _viewedSeat(node);
    final range = seats.ranges[viewed];
    final acting = seats.isAction && seats.acting == viewed;
    final actionCount = acting ? node.actions.length : 0;
    final colors = actionColors(node.actions);
    final shown = _hovered ?? _selected;

    // PLO has no grid: its hands are listed. Hold'em shows the grid, built
    // from the hands, or from the classes where only those are sent.
    final PloHandList? ploList = _isOmaha
        ? PloHandList.fromRange(
            range,
            strategy: acting ? node.strategy : const [],
            actionCount: actionCount,
            locked: acting ? node.locked : const [],
          )
        : null;
    final model = ploList != null
        ? MatrixModel.fromRange(PlayerRange())
        : range.hands.isEmpty && range.classes.isNotEmpty
        ? MatrixModel.fromClasses(range.classes, actionCount: actionCount)
        : MatrixModel.fromRange(
            range,
            strategy: acting ? node.strategy : const [],
            actionCount: actionCount,
            locked: acting ? node.locked : const [],
          );
    final totalCombos = ploList != null
        ? seats.combos(viewed)
        : model.totalReach;
    final comboCounts = ploList != null
        ? (acting
              ? [for (final a in node.actions) a.frequency * totalCombos]
              : null)
        : ActionSummary.matrixCombos(model);

    return LayoutBuilder(
      builder: (context, constraints) {
        final wide = constraints.maxWidth >= 960;
        final line = ActionLine(
          path: List.unmodifiable(_path),
          board: splitCards(_job.spot.board),
          current: node,
          nodeAt: (depth) => _nodes[_path.sublist(0, depth).join('/')],
          job: _job,
          onGo: _go,
          compact: !wide,
        );

        final toolbar = _MatrixToolbar(
          positions: positions,
          viewed: viewed,
          acting: seats.isAction ? seats.acting : null,
          folded: [for (var i = 0; i < seats.count; ++i) seats.folded(i)],
          onView: (seat) => setState(() {
            _view = seat;
            _selected = null;
            _highlight = null;
          }),
          matrixView: _matrixView,
          onMatrixView: ploList != null
              ? null
              : (view) => setState(() => _matrixView = view),
          compact: !wide,
        );
        final Widget matrix = ploList != null
            ? PloHandTable(
                key: const Key('plo-table'),
                list: ploList,
                actions: node.actions,
                colors: colors,
                maxHands: _ploMaxHands,
                highlightAction: _highlight,
                expand: wide,
                onMaxHands: (limit) {
                  setState(() {
                    _ploMaxHands = limit;
                    _nodes.clear();
                  });
                  _load(quiet: true);
                },
              )
            : StrategyMatrix(
                key: const Key('matrix'),
                model: model,
                colors: colors,
                view: _matrixView,
                selected: _selected,
                hovered: wide ? _hovered : null,
                highlightAction: _highlight,
                onSelected: (index) => setState(
                  () => _selected = _selected == index ? null : index,
                ),
                onHover: wide
                    ? (index) => setState(() => _hovered = index)
                    : null,
              );

        final deal = node.type == NodeType.NODE_TYPE_CHANCE
            ? _dealSection(context, node, compact: !wide)
            : null;
        final summary = ActionSummary(
          node: node,
          who: positions[viewed],
          totalCombos: totalCombos,
          comboCounts: comboCounts,
          colors: colors,
          highlight: _highlight,
          onHighlight: (action) => setState(() => _highlight = action),
        );
        final Widget? details = ploList != null
            ? null
            : shown == null
            ? _hint(
                context,
                wide
                    ? 'Point at or click a hand in the grid for its combos.'
                    : 'Tap a hand in the grid for its combos.',
              )
            : HandDetails(
                cell: model.cells[shown],
                node: node,
                colors: colors,
                actionCount: model.actionCount,
              );
        final lockRow = seats.isAction && _canLock(seats)
            ? _lockRow(context, node)
            : null;
        // Locking is one task: the button, then what is locked so far.
        final locking = [?lockRow, if (_locks.isNotEmpty) _locksCard(context)];
        final extras = [
          NodeSummary(node: node, job: _job),
          if (_parentNode != null && node.type == NodeType.NODE_TYPE_ACTION)
            _compareCard(context, node),
        ];

        if (wide) {
          final panelWidth = constraints.maxWidth >= 1280 ? 420.0 : 360.0;
          final panel = [?deal, summary, ...locking, ?details, ...extras];
          return Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              line,
              const Divider(),
              Expanded(
                child: Row(
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    Expanded(
                      child: Padding(
                        padding: const EdgeInsets.fromLTRB(16, 12, 8, 16),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            toolbar,
                            const SizedBox(height: 10),
                            Expanded(
                              child: ploList != null
                                  ? matrix
                                  : Align(
                                      alignment: Alignment.topCenter,
                                      child: matrix,
                                    ),
                            ),
                          ],
                        ),
                      ),
                    ),
                    SizedBox(
                      width: panelWidth,
                      child: _ScrollStack(
                        padding: const EdgeInsets.fromLTRB(8, 12, 16, 16),
                        children: panel,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          );
        }

        // Phones: the line stays at the top; everything else scrolls.
        final items = [
          ?deal,
          toolbar,
          matrix,
          summary,
          ?details,
          ...locking,
          ...extras,
        ];
        return Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            line,
            const Divider(),
            Expanded(
              child: _ScrollStack(
                padding: const EdgeInsets.fromLTRB(12, 10, 12, 24),
                children: items,
              ),
            ),
          ],
        );
      },
    );
  }

  Widget _hint(BuildContext context, String text) {
    final theme = Theme.of(context);
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 4, vertical: 2),
      child: Row(
        children: [
          Icon(
            Icons.touch_app_outlined,
            size: 16,
            color: theme.colorScheme.onSurfaceVariant,
          ),
          const SizedBox(width: 6),
          Expanded(
            child: Text(
              text,
              key: const Key('details-hint'),
              style: theme.textTheme.bodySmall?.copyWith(
                color: theme.colorScheme.onSurfaceVariant,
              ),
            ),
          ),
        ],
      ),
    );
  }

  Widget _lockRow(BuildContext context, GetNodeResponse node) {
    final lockedHere = node.locked.where((flag) => flag).length;
    return Wrap(
      spacing: 12,
      runSpacing: 4,
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
    );
  }

  Widget _dealSection(
    BuildContext context,
    GetNodeResponse node, {
    required bool compact,
  }) {
    return PanelSection(
      title: 'Deal the next card',
      trailing: _runouts != null
          ? null
          : TextButton.icon(
              key: const Key('runouts-button'),
              onPressed: _loadingRunouts ? null : _loadRunouts,
              style: TextButton.styleFrom(
                visualDensity: VisualDensity.compact,
                padding: const EdgeInsets.symmetric(horizontal: 8),
              ),
              icon: _loadingRunouts
                  ? const SizedBox.square(
                      dimension: 14,
                      child: CircularProgressIndicator(strokeWidth: 2),
                    )
                  : const Icon(Icons.bar_chart, size: 16),
              label: const Text('Compare the cards'),
            ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          CardPicker(
            cards: node.nextCards,
            compact: compact,
            onPick: (card) => _go([..._path, card]),
          ),
          if (_runouts != null) ...[
            const SizedBox(height: 10),
            RunoutReport(
              runouts: _runouts!,
              firstSeat: NodeSeats(node, _job).positions.first,
              onPick: (card) => _go([..._path, card]),
            ),
          ],
        ],
      ),
    );
  }
}

/// Blocks one under the other, scrolling together.
class _ScrollStack extends StatelessWidget {
  const _ScrollStack({required this.padding, required this.children});

  final EdgeInsets padding;
  final List<Widget> children;

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: padding,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          for (var i = 0; i < children.length; ++i) ...[
            if (i > 0) const SizedBox(height: 10),
            children[i],
          ],
        ],
      ),
    );
  }
}

/// Whose range is shown, and what the grid's colours mean.
class _MatrixToolbar extends StatelessWidget {
  const _MatrixToolbar({
    required this.positions,
    required this.viewed,
    required this.acting,
    required this.folded,
    required this.onView,
    required this.matrixView,
    required this.onMatrixView,
    this.compact = false,
  });

  /// Position names in seat order.
  final List<String> positions;
  final int viewed;
  final bool compact;

  /// The seat to act, marked in the picker; null if nobody acts here.
  final int? acting;
  final List<bool> folded;
  final ValueChanged<int> onView;
  final MatrixView matrixView;

  /// Null where there is no grid (PLO).
  final ValueChanged<MatrixView>? onMatrixView;

  @override
  Widget build(BuildContext context) {
    // Tighter segments on a phone, so the labels fit on one line.
    final style = compact
        ? const ButtonStyle(
            padding: WidgetStatePropertyAll(
              EdgeInsets.symmetric(horizontal: 4),
            ),
          )
        : null;
    final Widget players;
    if (positions.length <= 3) {
      players = SegmentedButton<int>(
        style: style,
        key: const Key('player-view'),
        showSelectedIcon: false,
        segments: [
          for (var seat = 0; seat < positions.length; ++seat)
            ButtonSegment(
              value: seat,
              label: Text(positions[seat]),
              icon: acting == seat && !compact
                  ? const Icon(Icons.play_arrow_rounded, size: 16)
                  : null,
            ),
        ],
        selected: {viewed},
        onSelectionChanged: (selection) => onView(selection.first),
      );
    } else {
      // Too many seats for segments: a menu, the player to act marked.
      players = DropdownButton<int>(
        key: const Key('player-view'),
        value: viewed,
        isDense: true,
        items: [
          for (var seat = 0; seat < positions.length; ++seat)
            DropdownMenuItem(
              value: seat,
              child: Text(
                '${positions[seat]}'
                '${acting == seat ? ' ▸ to act' : ''}'
                '${folded[seat] ? ' (folded)' : ''}',
              ),
            ),
        ],
        onChanged: (seat) {
          if (seat != null) onView(seat);
        },
      );
    }
    final onViews = onMatrixView;
    final views = onViews == null
        ? null
        : SegmentedButton<MatrixView>(
            style: style,
            key: const Key('matrix-view'),
            showSelectedIcon: false,
            segments: [
              for (final view in MatrixView.values)
                ButtonSegment(value: view, label: Text(view.label)),
            ],
            selected: {matrixView},
            onSelectionChanged: (selection) => onViews(selection.first),
          );
    if (compact && views != null) {
      // One row on a phone, sharing the width.
      return Row(
        children: [
          Expanded(flex: 3, child: players),
          const SizedBox(width: 8),
          Expanded(flex: 7, child: views),
        ],
      );
    }
    return Wrap(spacing: 8, runSpacing: 8, children: [players, ?views]);
  }
}

class _StatusChip extends StatelessWidget {
  const _StatusChip({required this.label, required this.active});

  final String label;
  final bool active;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final color = active
        ? theme.colorScheme.primary
        : theme.colorScheme.onSurfaceVariant;
    return Container(
      margin: const EdgeInsets.symmetric(horizontal: 4),
      padding: const EdgeInsets.symmetric(horizontal: 10, vertical: 4),
      decoration: BoxDecoration(
        color: color.withValues(alpha: 0.14),
        borderRadius: BorderRadius.circular(20),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Container(
            width: 7,
            height: 7,
            decoration: BoxDecoration(color: color, shape: BoxShape.circle),
          ),
          const SizedBox(width: 6),
          Text(
            label,
            style: theme.textTheme.labelSmall?.copyWith(color: color),
          ),
        ],
      ),
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
