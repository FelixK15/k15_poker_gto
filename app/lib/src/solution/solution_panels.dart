import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/playing_card.dart';
import '../common/solve_labels.dart';
import 'action_style.dart';
import 'matrix_model.dart';
import 'node_view.dart';
import 'strategy_matrix.dart';

/// A titled block of the side panel.
class PanelSection extends StatelessWidget {
  const PanelSection({
    super.key,
    required this.title,
    required this.child,
    this.trailing,
  });

  final String title;
  final Widget child;
  final Widget? trailing;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Card(
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 10, 12, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text(
                    title.toUpperCase(),
                    style: theme.textTheme.labelSmall?.copyWith(
                      color: theme.colorScheme.onSurfaceVariant,
                      fontWeight: FontWeight.w700,
                    ),
                  ),
                ),
                ?trailing,
              ],
            ),
            const SizedBox(height: 8),
            child,
          ],
        ),
      ),
    );
  }
}

/// A bar split by how often each action is taken.
class FrequencyBar extends StatelessWidget {
  const FrequencyBar({
    super.key,
    required this.frequencies,
    required this.colors,
    this.height = 8,
  });

  final List<double> frequencies;
  final List<Color> colors;
  final double height;

  @override
  Widget build(BuildContext context) {
    return ClipRRect(
      borderRadius: BorderRadius.circular(height / 3),
      child: SizedBox(
        height: height,
        child: Row(
          // Without this the coloured boxes have no height.
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            for (var a = 0; a < frequencies.length; ++a)
              if (frequencies[a] > 0.0005)
                Expanded(
                  flex: (frequencies[a] * 1000).round(),
                  child: ColoredBox(color: colors[a]),
                ),
          ],
        ),
      ),
    );
  }
}

/// Pot, and every player's EV, equity, combos and chips at this node.
class NodeSummary extends StatelessWidget {
  const NodeSummary({super.key, required this.node, required this.job});

  final GetNodeResponse node;
  final Job job;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final seats = NodeSeats(node, job);
    final positions = seats.positions;
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: theme.colorScheme.onSurfaceVariant,
    );
    final number = theme.textTheme.bodyMedium?.copyWith(
      fontWeight: FontWeight.w600,
      fontFeatures: const [FontFeature.tabularFigures()],
    );
    final approx = node.evEstimated ? '≈' : '';
    Widget cell(String text, {TextStyle? style, Key? key}) => Padding(
      padding: const EdgeInsets.symmetric(vertical: 3),
      child: Text(
        text,
        key: key,
        style: style ?? number,
        textAlign: TextAlign.right,
      ),
    );

    String status(int seat) {
      if (seats.folded(seat)) return 'folded';
      if (seats.allIn(seat)) return 'all-in';
      if (seats.isAction && seats.acting == seat) return 'to act';
      return '';
    }

    final rows = <TableRow>[
      TableRow(
        children: [
          const SizedBox(),
          cell('EV', style: muted, key: const Key('node-ev')),
          cell('Equity', style: muted, key: const Key('node-equity')),
          cell('Combos', style: muted),
          cell('In / behind', style: muted),
        ],
      ),
      for (var seat = 0; seat < seats.count; ++seat)
        TableRow(
          key: ValueKey('seat-$seat'),
          children: [
            Padding(
              padding: const EdgeInsets.symmetric(vertical: 3),
              child: Text.rich(
                TextSpan(
                  children: [
                    TextSpan(
                      text: positions[seat],
                      style: theme.textTheme.bodyMedium?.copyWith(
                        fontWeight: seats.isAction && seats.acting == seat
                            ? FontWeight.w700
                            : FontWeight.w500,
                        color: seats.folded(seat)
                            ? theme.colorScheme.onSurfaceVariant
                            : null,
                      ),
                    ),
                    if (status(seat).isNotEmpty)
                      TextSpan(text: ' ${status(seat)}', style: muted),
                  ],
                ),
              ),
            ),
            cell(
              seats.folded(seat)
                  ? '–'
                  : '$approx${seats.ranges[seat].rangeEv.toStringAsFixed(2)}',
            ),
            cell(
              seats.folded(seat)
                  ? '–'
                  : formatPercent(seats.ranges[seat].rangeEquity),
            ),
            cell(
              seats.folded(seat) ? '–' : seats.combos(seat).toStringAsFixed(0),
            ),
            cell(
              '${formatChips(seats.committed(seat))} / '
              '${formatChips(seats.behind(seat))}',
              style: muted,
            ),
          ],
        ),
    ];

    return PanelSection(
      title: 'Spot',
      trailing: Text(
        'Pot ${formatChips(node.pot)}',
        key: const Key('node-pot'),
        style: theme.textTheme.titleSmall,
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Table(
            columnWidths: const {
              0: FlexColumnWidth(1.4),
              1: FlexColumnWidth(),
              2: FlexColumnWidth(),
              3: FlexColumnWidth(),
              4: FlexColumnWidth(1.3),
            },
            defaultVerticalAlignment: TableCellVerticalAlignment.middle,
            children: rows,
          ),
          if (job.measured) ...[
            const SizedBox(height: 6),
            Tooltip(
              message: exploitabilityMeaning(job.exploitabilityKind),
              child: Text(
                exploitabilityLabel(
                  job.exploitabilityPct,
                  job.exploitabilityKind,
                ),
                key: const Key('node-exploitability'),
                style: muted,
              ),
            ),
          ],
        ],
      ),
    );
  }
}

/// What the viewed player does here: one coloured box per action with how
/// often and with how many combos. Tapping a box shows only that action in
/// the grid; tapping it again shows all of them.
class ActionSummary extends StatelessWidget {
  const ActionSummary({
    super.key,
    required this.node,
    required this.who,
    required this.totalCombos,
    required this.comboCounts,
    required this.colors,
    required this.highlight,
    required this.onHighlight,
  });

  /// Combos per action of the viewed player's grid, or null if the player
  /// does not act here.
  static List<double>? matrixCombos(MatrixModel model) {
    if (model.actionCount == 0) return null;
    final counts = List.filled(model.actionCount, 0.0);
    for (final cell in model.cells) {
      for (var a = 0; a < model.actionCount; ++a) {
        counts[a] += cell.actionReach[a];
      }
    }
    return counts;
  }

  final GetNodeResponse node;

  /// The viewed player's position name.
  final String who;
  final double totalCombos;

  /// Combos taking each action; null if [who] does not act here.
  final List<double>? comboCounts;
  final List<Color> colors;
  final int? highlight;
  final ValueChanged<int?> onHighlight;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final combos = Text(
      '${totalCombos.toStringAsFixed(1)} combos in $who\'s range here',
      key: const Key('range-combos'),
      style: theme.textTheme.bodySmall?.copyWith(
        color: theme.colorScheme.onSurfaceVariant,
      ),
    );
    final counts = comboCounts;
    if (counts == null || counts.length != node.actions.length) {
      return PanelSection(title: '$who range', child: combos);
    }

    return PanelSection(
      title: '$who strategy',
      trailing: highlight == null
          ? null
          : InkWell(
              onTap: () => onHighlight(null),
              child: Text(
                'Show all',
                style: theme.textTheme.labelSmall?.copyWith(
                  color: theme.colorScheme.primary,
                ),
              ),
            ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          FrequencyBar(
            frequencies: [for (final a in node.actions) a.frequency],
            colors: colors,
            height: 10,
          ),
          const SizedBox(height: 8),
          LayoutBuilder(
            builder: (context, constraints) {
              const gap = 6.0;
              final perRow = node.actions.length <= 3
                  ? node.actions.length
                  : (constraints.maxWidth / 100).floor().clamp(2, 4);
              final width =
                  (constraints.maxWidth - gap * (perRow - 1)) / perRow;
              return Wrap(
                spacing: gap,
                runSpacing: gap,
                children: [
                  for (var i = 0; i < node.actions.length; ++i)
                    SizedBox(
                      width: width,
                      child: _ActionBox(
                        key: Key('summary-${node.actions[i].label}'),
                        title: actionTitle(node.actions[i]),
                        frequency: node.actions[i].frequency,
                        combos: counts[i],
                        color: colors[i],
                        selected: highlight == i,
                        dimmed: highlight != null && highlight != i,
                        onTap: () => onHighlight(highlight == i ? null : i),
                      ),
                    ),
                ],
              );
            },
          ),
          const SizedBox(height: 8),
          combos,
        ],
      ),
    );
  }
}

class _ActionBox extends StatelessWidget {
  const _ActionBox({
    super.key,
    required this.title,
    required this.frequency,
    required this.combos,
    required this.color,
    required this.selected,
    required this.dimmed,
    required this.onTap,
  });

  final String title;
  final double frequency;
  final double combos;
  final Color color;
  final bool selected;
  final bool dimmed;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    const white = Colors.white;
    return Opacity(
      opacity: dimmed ? 0.45 : 1,
      child: Material(
        color: color,
        borderRadius: BorderRadius.circular(6),
        child: InkWell(
          borderRadius: BorderRadius.circular(6),
          onTap: onTap,
          child: Container(
            padding: const EdgeInsets.fromLTRB(8, 6, 8, 6),
            decoration: BoxDecoration(
              borderRadius: BorderRadius.circular(6),
              border: Border.all(
                color: selected ? white : Colors.transparent,
                width: 2,
              ),
            ),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text(
                  title,
                  maxLines: 1,
                  overflow: TextOverflow.ellipsis,
                  style: const TextStyle(
                    color: white,
                    fontSize: 12,
                    fontWeight: FontWeight.w600,
                  ),
                ),
                Text(
                  formatPercent(frequency),
                  style: const TextStyle(
                    color: white,
                    fontSize: 18,
                    fontWeight: FontWeight.w700,
                    height: 1.2,
                  ),
                ),
                Text(
                  '${combos.toStringAsFixed(1)} combos',
                  style: TextStyle(
                    color: white.withValues(alpha: 0.85),
                    fontSize: 11,
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

/// The hand class picked or pointed at in the grid, combo by combo.
class HandDetails extends StatelessWidget {
  const HandDetails({
    super.key,
    required this.cell,
    required this.node,
    required this.colors,
    required this.actionCount,
  });

  final MatrixCell cell;
  final GetNodeResponse node;
  final List<Color> colors;

  /// Actions the cell has frequencies for; 0 for a range without strategy.
  final int actionCount;

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
      child: Padding(
        padding: const EdgeInsets.fromLTRB(12, 10, 12, 12),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Text(cell.label, style: theme.textTheme.titleMedium),
            if (cell.isEmpty)
              Text('Not in the range here.', style: muted)
            else ...[
              Text(
                '${cell.reach.toStringAsFixed(2)} of ${cell.combos} combos · '
                'EV ${cell.ev.toStringAsFixed(2)} · '
                'equity ${formatPercent(cell.equity, digits: 0)}',
                style: muted,
              ),
              if (actionCount > 0) ...[
                const SizedBox(height: 8),
                FrequencyBar(
                  frequencies: [
                    for (var a = 0; a < actionCount; ++a) cell.frequency(a),
                  ],
                  colors: colors,
                  height: 10,
                ),
                const SizedBox(height: 4),
                Wrap(
                  spacing: 10,
                  runSpacing: 2,
                  children: [
                    for (var a = 0; a < actionCount; ++a)
                      if (cell.frequency(a) >= 0.005)
                        _Legend(
                          color: colors[a],
                          text:
                              '${actionTitle(node.actions[a])} '
                              '${formatPercent(cell.frequency(a), digits: 0)}',
                        ),
                  ],
                ),
              ],
              const SizedBox(height: 10),
              LayoutBuilder(
                builder: (context, constraints) {
                  final perRow = (constraints.maxWidth / 150).floor().clamp(
                    1,
                    4,
                  );
                  final width =
                      (constraints.maxWidth - 6 * (perRow - 1)) / perRow;
                  return Wrap(
                    spacing: 6,
                    runSpacing: 6,
                    children: [
                      for (final hand in present)
                        SizedBox(
                          width: width,
                          child: _comboTile(context, hand),
                        ),
                    ],
                  );
                },
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _comboTile(BuildContext context, HandView hand) {
    final theme = Theme.of(context);
    final small = theme.textTheme.bodySmall;
    return Container(
      padding: const EdgeInsets.all(6),
      decoration: BoxDecoration(
        color: theme.colorScheme.surfaceContainer,
        borderRadius: BorderRadius.circular(6),
        border: hand.locked
            ? Border.all(color: theme.colorScheme.primary)
            : null,
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            children: [
              CardRow(
                [hand.name.substring(0, 2), hand.name.substring(2, 4)],
                width: 18,
                gap: 2,
              ),
              const SizedBox(width: 8),
              Expanded(
                child: Text(
                  '${hand.locked ? 'Locked · ' : ''}'
                  'EV ${hand.ev.toStringAsFixed(2)}'
                  '${hand.reach < 0.995 ? ' · ${formatPercent(hand.reach, digits: 0)} here' : ''}',
                  style: small,
                ),
              ),
            ],
          ),
          if (hand.frequencies.isNotEmpty) ...[
            const SizedBox(height: 4),
            FrequencyBar(frequencies: hand.frequencies, colors: colors),
            const SizedBox(height: 2),
            Text(
              [
                for (var a = 0; a < hand.frequencies.length; ++a)
                  if (hand.frequencies[a] >= 0.005)
                    '${actionTitle(node.actions[a])} '
                        '${formatPercent(hand.frequencies[a], digits: 0)}',
              ].join(' · '),
              style: small?.copyWith(fontSize: 11),
            ),
          ],
        ],
      ),
    );
  }
}

class _Legend extends StatelessWidget {
  const _Legend({required this.color, required this.text});

  final Color color;
  final String text;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        Container(
          width: 8,
          height: 8,
          decoration: BoxDecoration(
            color: color,
            borderRadius: BorderRadius.circular(2),
          ),
        ),
        const SizedBox(width: 4),
        Text(text, style: Theme.of(context).textTheme.bodySmall),
      ],
    );
  }
}

/// The deck as four rows of cards; the ones that can come are tappable.
class CardPicker extends StatelessWidget {
  const CardPicker({
    super.key,
    required this.cards,
    required this.onPick,
    this.compact = false,
  });

  final List<String> cards;
  final ValueChanged<String> onPick;
  final bool compact;

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        const gap = 3.0;
        final width = ((constraints.maxWidth - 12 * gap) / 13).clamp(
          16.0,
          compact ? 40.0 : 32.0,
        );
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            for (final suit in const ['s', 'h', 'd', 'c'])
              Padding(
                padding: const EdgeInsets.only(bottom: gap),
                child: Row(
                  children: [
                    for (final rank in matrixRanks.split('')) ...[
                      if (rank != 'A') const SizedBox(width: gap),
                      _pickable(context, '$rank$suit', width),
                    ],
                  ],
                ),
              ),
          ],
        );
      },
    );
  }

  Widget _pickable(BuildContext context, String card, double width) {
    final available = cards.contains(card);
    final face = PlayingCard(card, width: width, dimmed: !available);
    if (!available) return face;
    return Tooltip(
      message: 'Deal ${cardTitle(card)}',
      child: InkWell(
        key: Key('card-$card'),
        borderRadius: BorderRadius.circular(width * 0.14),
        onTap: () => onPick(card),
        child: face,
      ),
    );
  }
}

/// One tile per card that can come: what the spot is worth after it and what
/// the next player does. Tapping a tile deals the card.
class RunoutReport extends StatelessWidget {
  const RunoutReport({super.key, required this.runouts, required this.onPick});

  final List<Runout> runouts;
  final ValueChanged<String> onPick;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    // Best cards for the out-of-position player first.
    final sorted = [...runouts]..sort((a, b) => b.oopEv.compareTo(a.oopEv));
    final lowest = sorted.isEmpty ? 0.0 : sorted.last.oopEv;
    final highest = sorted.isEmpty ? 1.0 : sorted.first.oopEv;

    return Column(
      key: const Key('runout-report'),
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(
          'Cards by OOP\'s value after them. The bar shows what the next '
          'player does.',
          style: theme.textTheme.bodySmall?.copyWith(
            color: theme.colorScheme.onSurfaceVariant,
          ),
        ),
        const SizedBox(height: 8),
        Wrap(
          spacing: 6,
          runSpacing: 6,
          children: [
            for (final runout in sorted)
              Material(
                color: theme.colorScheme.surfaceContainer,
                borderRadius: BorderRadius.circular(6),
                child: InkWell(
                  key: Key('runout-${runout.card}'),
                  borderRadius: BorderRadius.circular(6),
                  onTap: () => onPick(runout.card),
                  child: Container(
                    width: 92,
                    padding: const EdgeInsets.all(6),
                    decoration: BoxDecoration(
                      borderRadius: BorderRadius.circular(6),
                      border: Border(
                        left: BorderSide(
                          width: 3,
                          color: valueColor(runout.oopEv, lowest, highest),
                        ),
                      ),
                    ),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            PlayingCard(runout.card, width: 20),
                            const SizedBox(width: 6),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    'EV ${runout.oopEv.toStringAsFixed(2)}',
                                    style: theme.textTheme.bodySmall,
                                  ),
                                  Text(
                                    'Eq ${formatPercent(runout.oopEquity, digits: 0)}',
                                    style: theme.textTheme.bodySmall,
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ),
                        if (runout.actions.isNotEmpty) ...[
                          const SizedBox(height: 4),
                          FrequencyBar(
                            frequencies: [
                              for (final a in runout.actions) a.frequency,
                            ],
                            colors: actionColors(runout.actions),
                            height: 6,
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
              ),
          ],
        ),
      ],
    );
  }
}
