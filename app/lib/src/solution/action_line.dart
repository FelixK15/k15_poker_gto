import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/playing_card.dart';
import 'action_style.dart';

const _streets = ['Flop', 'Turn', 'River'];

/// The line played so far as a strip of tiles, one per decision or dealt
/// card, ending in the node on screen with its choices. Every option in the
/// strip can be tapped: the current tile moves forward, earlier tiles jump to
/// another line from that point.
class ActionLine extends StatefulWidget {
  const ActionLine({
    super.key,
    required this.path,
    required this.board,
    required this.current,
    required this.nodeAt,
    required this.effectiveStack,
    required this.onGo,
    this.compact = false,
  });

  /// Steps from the start to the node on screen.
  final List<String> path;

  /// The cards the spot starts with.
  final List<String> board;

  /// The node at the end of [path].
  final GetNodeResponse current;

  /// A node along the path by its depth, if it has been read.
  final GetNodeResponse? Function(int depth) nodeAt;

  final double effectiveStack;

  /// Called with the path to show.
  final ValueChanged<List<String>> onGo;

  /// Larger touch targets for phones.
  final bool compact;

  @override
  State<ActionLine> createState() => _ActionLineState();
}

class _ActionLineState extends State<ActionLine> {
  final _scroll = ScrollController();

  @override
  void initState() {
    super.initState();
    _scrollToEnd();
  }

  @override
  void didUpdateWidget(ActionLine old) {
    super.didUpdateWidget(old);
    if (old.path.length != widget.path.length ||
        old.current != widget.current) {
      _scrollToEnd();
    }
  }

  @override
  void dispose() {
    _scroll.dispose();
    super.dispose();
  }

  void _scrollToEnd() {
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (!_scroll.hasClients) return;
      _scroll.animateTo(
        _scroll.position.maxScrollExtent,
        duration: const Duration(milliseconds: 250),
        curve: Curves.easeOut,
      );
    });
  }

  @override
  Widget build(BuildContext context) {
    final path = widget.path;
    final tiles = <Widget>[_boardTile(context)];
    var dealt = 0;
    for (var depth = 0; depth <= path.length; ++depth) {
      final isCurrent = depth == path.length;
      final node = isCurrent ? widget.current : widget.nodeAt(depth);
      final step = isCurrent ? null : path[depth];
      if (step != null && cardTitle(step) != null) {
        ++dealt;
        tiles.add(_cardTile(context, depth, step, dealt));
      } else if (isCurrent && node!.type == NodeType.NODE_TYPE_CHANCE) {
        tiles.add(_dealTile(context, dealt + 1));
      } else if (isCurrent && node!.type != NodeType.NODE_TYPE_ACTION) {
        tiles.add(_endTile(context, node));
      } else {
        tiles.add(_decisionTile(context, depth, node, step));
      }
    }

    return Material(
      color: Theme.of(context).colorScheme.surface,
      child: SingleChildScrollView(
        controller: _scroll,
        scrollDirection: Axis.horizontal,
        padding: const EdgeInsets.fromLTRB(8, 8, 8, 8),
        child: IntrinsicHeight(
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              for (var i = 0; i < tiles.length; ++i) ...[
                if (i > 0) const SizedBox(width: 6),
                tiles[i],
              ],
            ],
          ),
        ),
      ),
    );
  }

  String _street(int dealt) {
    final index = widget.board.length - 3 + dealt;
    return index >= 0 && index < _streets.length ? _streets[index] : 'Card';
  }

  Widget _boardTile(BuildContext context) {
    final theme = Theme.of(context);
    final dealt = [
      for (final step in widget.path)
        if (cardTitle(step) != null) step,
    ];
    return _Tile(
      header: Text(
        'Pot ${formatChips(widget.current.pot)}',
        key: const Key('line-pot'),
        style: theme.textTheme.labelMedium?.copyWith(
          fontWeight: FontWeight.w700,
        ),
      ),
      child: Padding(
        padding: const EdgeInsets.all(6),
        child: Center(
          child: CardRow([
            ...widget.board,
            ...dealt,
          ], width: widget.compact ? 24 : 26),
        ),
      ),
    );
  }

  Widget _cardTile(BuildContext context, int depth, String card, int dealt) {
    return _Tile(
      header: _HeaderButton(
        key: Key('crumb-$depth'),
        label: _street(dealt),
        onTap: () => widget.onGo(widget.path.sublist(0, depth)),
      ),
      child: Padding(
        padding: const EdgeInsets.all(6),
        child: Center(
          child: PlayingCard(card, width: widget.compact ? 26 : 28),
        ),
      ),
    );
  }

  Widget _dealTile(BuildContext context, int dealt) {
    final theme = Theme.of(context);
    return _Tile(
      current: true,
      header: _HeaderButton(label: _street(dealt)),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
        child: Center(
          child: Text(
            'Deal a card',
            style: theme.textTheme.bodySmall?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
          ),
        ),
      ),
    );
  }

  Widget _endTile(BuildContext context, GetNodeResponse node) {
    final theme = Theme.of(context);
    final fold = node.type == NodeType.NODE_TYPE_FOLD;
    return _Tile(
      current: true,
      width: 150,
      header: _HeaderButton(label: fold ? 'Hand over' : 'Showdown'),
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
        child: Center(
          child: Text(
            fold
                ? '${node.player == Player.PLAYER_OOP ? 'OOP' : 'IP'} folds. '
                      'The hand is over.'
                : 'The hand goes to showdown.',
            key: const Key('line-end'),
            style: theme.textTheme.bodySmall,
          ),
        ),
      ),
    );
  }

  Widget _decisionTile(
    BuildContext context,
    int depth,
    GetNodeResponse? node,
    String? taken,
  ) {
    final theme = Theme.of(context);
    final isCurrent = taken == null;
    final prefix = widget.path.sublist(0, depth);
    final known = node != null && node.type == NodeType.NODE_TYPE_ACTION;

    String header;
    if (known) {
      final oop = node.player == Player.PLAYER_OOP;
      final behind =
          widget.effectiveStack - (oop ? node.oopCommitted : node.ipCommitted);
      header = '${oop ? 'OOP' : 'IP'} · ${formatChips(behind)}';
    } else {
      header = 'Decision';
    }

    final rows = <Widget>[];
    if (known) {
      final colors = actionColors(node.actions);
      for (var i = 0; i < node.actions.length; ++i) {
        final action = node.actions[i];
        rows.add(
          _OptionRow(
            key: isCurrent ? Key('action-${action.label}') : null,
            title: actionTitle(action),
            frequency: action.frequency,
            color: colors[i],
            chosen: action.label == taken,
            dimmed: !isCurrent && action.label != taken,
            tall: widget.compact && isCurrent,
            onTap: () => widget.onGo([...prefix, action.label]),
          ),
        );
      }
    } else if (taken != null) {
      rows.add(
        _OptionRow(
          title: pathStepTitle(taken),
          color: theme.colorScheme.primary,
          chosen: true,
          onTap: () => widget.onGo([...prefix, taken]),
        ),
      );
    }

    return _Tile(
      current: isCurrent,
      width: isCurrent ? (widget.compact ? 168 : 156) : 132,
      header: _HeaderButton(
        key: Key('crumb-$depth'),
        label: header,
        onTap: isCurrent ? null : () => widget.onGo(prefix),
      ),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(4, 2, 4, 4),
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: rows,
        ),
      ),
    );
  }
}

class _Tile extends StatelessWidget {
  const _Tile({
    required this.header,
    required this.child,
    this.current = false,
    this.width,
  });

  final Widget header;
  final Widget child;
  final bool current;
  final double? width;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final tile = Container(
      width: width,
      decoration: BoxDecoration(
        color: current ? scheme.surfaceContainerHigh : scheme.surfaceContainer,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: current ? scheme.primary : scheme.outlineVariant,
          width: current ? 1.5 : 1,
        ),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        mainAxisSize: MainAxisSize.min,
        children: [header, child],
      ),
    );
    // Without a width, as wide as the content.
    return width == null ? IntrinsicWidth(child: tile) : tile;
  }
}

class _HeaderButton extends StatelessWidget {
  const _HeaderButton({super.key, required this.label, this.onTap});

  final String label;
  final VoidCallback? onTap;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return InkWell(
      onTap: onTap,
      borderRadius: const BorderRadius.vertical(top: Radius.circular(8)),
      child: Padding(
        padding: const EdgeInsets.fromLTRB(8, 5, 8, 3),
        child: Text(
          label,
          maxLines: 1,
          overflow: TextOverflow.ellipsis,
          style: theme.textTheme.labelMedium?.copyWith(
            fontWeight: FontWeight.w700,
            color: onTap == null
                ? theme.colorScheme.onSurface
                : theme.colorScheme.onSurfaceVariant,
          ),
        ),
      ),
    );
  }
}

class _OptionRow extends StatelessWidget {
  const _OptionRow({
    super.key,
    required this.title,
    required this.color,
    required this.onTap,
    this.frequency,
    this.chosen = false,
    this.dimmed = false,
    this.tall = false,
  });

  final String title;
  final double? frequency;
  final Color color;
  final bool chosen;
  final bool dimmed;
  final bool tall;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final style = theme.textTheme.bodySmall?.copyWith(
      fontWeight: chosen ? FontWeight.w700 : FontWeight.w500,
      color: dimmed
          ? theme.colorScheme.onSurfaceVariant
          : theme.colorScheme.onSurface,
    );
    return Padding(
      padding: const EdgeInsets.only(top: 2),
      child: Material(
        color: chosen ? color.withValues(alpha: 0.32) : Colors.transparent,
        borderRadius: BorderRadius.circular(5),
        child: InkWell(
          borderRadius: BorderRadius.circular(5),
          onTap: onTap,
          child: Container(
            constraints: BoxConstraints(minHeight: tall ? 38 : 26),
            padding: const EdgeInsets.symmetric(horizontal: 6),
            child: Row(
              children: [
                Container(
                  width: 4,
                  height: tall ? 20 : 14,
                  decoration: BoxDecoration(
                    color: dimmed ? color.withValues(alpha: 0.5) : color,
                    borderRadius: BorderRadius.circular(2),
                  ),
                ),
                const SizedBox(width: 6),
                Expanded(
                  child: Text(
                    title,
                    style: style,
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                ),
                if (frequency != null)
                  Text(
                    formatPercent(frequency!, digits: 0),
                    style: style?.copyWith(
                      fontFeatures: const [FontFeature.tabularFigures()],
                    ),
                  ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
