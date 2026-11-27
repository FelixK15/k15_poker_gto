import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/playing_card.dart';
import 'action_style.dart';
import 'plo_hands.dart';
import 'solution_panels.dart';

/// How many PLO hands per player the browser asks the server for.
const List<int> ploHandLimits = [200, 1000, 5000, 20000];

/// A PLO range at a node as a sortable list of its hands, with each hand's
/// weight, EV, equity and, for the acting player, its action frequencies.
class PloHandTable extends StatefulWidget {
  const PloHandTable({
    super.key,
    required this.list,
    required this.actions,
    required this.colors,
    required this.maxHands,
    required this.onMaxHands,
    this.highlightAction,
    this.expand = false,
  });

  final PloHandList list;

  /// The node's actions; their frequencies are in the hands when the list
  /// belongs to the acting player.
  final List<NodeAction> actions;
  final List<Color> colors;

  /// The current request limit, and a call to ask for another.
  final int maxHands;
  final ValueChanged<int> onMaxHands;

  /// Action picked in the summary: the list is then sorted by it.
  final int? highlightAction;

  /// Fill the height given (desktop); otherwise list a limited number of
  /// rows inside the page's own scrolling.
  final bool expand;

  @override
  State<PloHandTable> createState() => _PloHandTableState();
}

class _PloHandTableState extends State<PloHandTable> {
  PloSort _sort = PloSort.weight;
  int _action = 0;
  bool _ascending = false;

  /// Rows shown inside a scrolling page before "show more".
  static const _pageRows = 100;
  int _rows = _pageRows;

  @override
  void didUpdateWidget(PloHandTable old) {
    super.didUpdateWidget(old);
    final highlight = widget.highlightAction;
    if (highlight != null && highlight != old.highlightAction) {
      _sort = PloSort.action;
      _action = highlight;
      _ascending = false;
    }
    if (_action >= widget.list.actionCount && _sort == PloSort.action) {
      _sort = PloSort.weight;
    }
  }

  void _sortBy(PloSort sort, {int action = 0}) {
    setState(() {
      if (_sort == sort && (sort != PloSort.action || _action == action)) {
        _ascending = !_ascending;
      } else {
        _sort = sort;
        _action = action;
        _ascending = false;
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final list = widget.list;
    final hands = list.sorted(_sort, action: _action, ascending: _ascending);
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: theme.colorScheme.onSurfaceVariant,
    );

    Widget chip(String label, PloSort sort, {int action = 0, Color? color}) {
      final selected =
          _sort == sort && (sort != PloSort.action || _action == action);
      return ChoiceChip(
        key: Key(
          'plo-sort-${sort.name}${sort == PloSort.action ? '-$action' : ''}',
        ),
        label: Text('$label${selected ? (_ascending ? ' ↑' : ' ↓') : ''}'),
        avatar: color == null
            ? null
            : Container(
                width: 10,
                height: 10,
                decoration: BoxDecoration(
                  color: color,
                  borderRadius: BorderRadius.circular(2),
                ),
              ),
        selected: selected,
        visualDensity: VisualDensity.compact,
        onSelected: (_) => _sortBy(sort, action: action),
      );
    }

    final header = Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          children: [
            Expanded(
              child: Text(
                list.coverage,
                key: const Key('plo-coverage'),
                style: muted,
              ),
            ),
            DropdownButton<int>(
              key: const Key('plo-max-hands'),
              value: ploHandLimits.contains(widget.maxHands)
                  ? widget.maxHands
                  : null,
              isDense: true,
              underline: const SizedBox.shrink(),
              hint: const Text('Hands'),
              items: [
                for (final limit in ploHandLimits)
                  DropdownMenuItem(
                    value: limit,
                    child: Text('Up to ${formatCount(limit)}'),
                  ),
              ],
              onChanged: (limit) {
                if (limit != null) widget.onMaxHands(limit);
              },
            ),
          ],
        ),
        const SizedBox(height: 6),
        Wrap(
          spacing: 6,
          runSpacing: 6,
          children: [
            chip('Weight', PloSort.weight),
            chip('EV', PloSort.ev),
            chip('Equity', PloSort.equity),
            for (var a = 0; a < list.actionCount; ++a)
              chip(
                actionTitle(widget.actions[a]),
                PloSort.action,
                action: a,
                color: widget.colors[a],
              ),
          ],
        ),
        const SizedBox(height: 6),
      ],
    );

    Widget row(int index) => _PloHandRow(
      key: Key('plo-hand-${hands[index].name}'),
      hand: hands[index],
      actions: widget.actions,
      colors: widget.colors,
    );

    if (hands.isEmpty) {
      return Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          header,
          Text('No hands get here.', style: muted),
        ],
      );
    }

    if (widget.expand) {
      return Column(
        key: const Key('plo-hands'),
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          header,
          Expanded(
            child: ListView.builder(
              itemCount: hands.length,
              itemExtent: 44,
              itemBuilder: (context, index) => row(index),
            ),
          ),
        ],
      );
    }

    final shown = hands.length < _rows ? hands.length : _rows;
    return Column(
      key: const Key('plo-hands'),
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        header,
        for (var i = 0; i < shown; ++i) SizedBox(height: 44, child: row(i)),
        if (shown < hands.length)
          TextButton(
            key: const Key('plo-more'),
            onPressed: () => setState(() => _rows += _pageRows),
            child: Text(
              'Show more (${formatCount(hands.length - shown)} left)',
            ),
          ),
      ],
    );
  }
}

class _PloHandRow extends StatelessWidget {
  const _PloHandRow({
    super.key,
    required this.hand,
    required this.actions,
    required this.colors,
  });

  final PloHand hand;
  final List<NodeAction> actions;
  final List<Color> colors;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final small = theme.textTheme.bodySmall?.copyWith(
      fontFeatures: const [FontFeature.tabularFigures()],
    );
    final top = <String>[
      for (var a = 0; a < hand.frequencies.length; ++a)
        if (hand.frequencies[a] >= 0.005)
          '${actionTitle(actions[a])} '
              '${formatPercent(hand.frequencies[a], digits: 0)}',
    ];
    return Container(
      margin: const EdgeInsets.only(bottom: 2),
      padding: const EdgeInsets.symmetric(horizontal: 6, vertical: 4),
      decoration: BoxDecoration(
        color: theme.colorScheme.surfaceContainer,
        borderRadius: BorderRadius.circular(6),
        border: hand.locked
            ? Border.all(color: theme.colorScheme.primary)
            : null,
      ),
      child: Row(
        children: [
          CardRow(hand.cards, width: 18, gap: 2),
          const SizedBox(width: 8),
          SizedBox(
            width: 48,
            child: Text(
              formatPercent(hand.reach, digits: 0),
              style: small,
              textAlign: TextAlign.right,
            ),
          ),
          SizedBox(
            width: 64,
            child: Text(
              hand.ev.toStringAsFixed(2),
              style: small,
              textAlign: TextAlign.right,
            ),
          ),
          SizedBox(
            width: 52,
            child: Text(
              formatPercent(hand.equity, digits: 0),
              style: small,
              textAlign: TextAlign.right,
            ),
          ),
          const SizedBox(width: 10),
          if (hand.frequencies.isNotEmpty)
            Expanded(
              child: Tooltip(
                message: top.join(' · '),
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  crossAxisAlignment: CrossAxisAlignment.stretch,
                  children: [
                    FrequencyBar(frequencies: hand.frequencies, colors: colors),
                    const SizedBox(height: 2),
                    Text(
                      top.join(' · '),
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: small?.copyWith(fontSize: 11),
                    ),
                  ],
                ),
              ),
            )
          else
            const Spacer(),
        ],
      ),
    );
  }
}
