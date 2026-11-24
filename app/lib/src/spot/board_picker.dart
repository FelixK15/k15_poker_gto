import 'dart:math';

import 'package:flutter/material.dart';

import '../solution/action_style.dart';

const String _ranks = 'AKQJT98765432';
const String _suits = 'shdc';

/// The cards of a board text such as "Ks Td 7h", in order. Unreadable input
/// gives an empty list.
List<String> parseBoard(String text) {
  final compact = text.replaceAll(RegExp(r'[\s,]+'), '');
  if (compact.length.isOdd) return [];
  final cards = <String>[];
  for (var i = 0; i < compact.length; i += 2) {
    final rank = compact[i].toUpperCase();
    final suit = compact[i + 1].toLowerCase();
    if (!_ranks.contains(rank) || !_suits.contains(suit)) return [];
    final card = '$rank$suit';
    if (cards.contains(card)) return [];
    cards.add(card);
  }
  return cards;
}

/// Lets the user pick three to five board cards. Returns the board as text,
/// or null if cancelled.
Future<String?> showBoardPicker(BuildContext context, String current) {
  return showDialog<String>(
    context: context,
    builder: (context) => BoardPicker(initial: parseBoard(current)),
  );
}

class BoardPicker extends StatefulWidget {
  const BoardPicker({super.key, required this.initial, this.random});

  final List<String> initial;

  /// Source of randomness for "Random flop"; tests pass a seeded one.
  final Random? random;

  @override
  State<BoardPicker> createState() => _BoardPickerState();
}

class _BoardPickerState extends State<BoardPicker> {
  late final List<String> _cards = [...widget.initial.take(5)];

  void _toggle(String card) {
    setState(() {
      if (!_cards.remove(card) && _cards.length < 5) _cards.add(card);
    });
  }

  void _randomFlop() {
    final random = widget.random ?? Random();
    final deck = [
      for (final rank in _ranks.split(''))
        for (final suit in _suits.split('')) '$rank$suit',
    ]..shuffle(random);
    setState(() {
      _cards
        ..clear()
        ..addAll(deck.take(3));
    });
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final street = switch (_cards.length) {
      3 => 'Flop',
      4 => 'Turn',
      5 => 'River',
      _ => 'Pick three to five cards',
    };
    return AlertDialog(
      title: const Text('Board'),
      content: SizedBox(
        width: 620,
        child: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: Text.rich(
                    TextSpan(
                      children: [
                        for (final card in _cards)
                          TextSpan(
                            text: '${cardTitle(card)} ',
                            style: TextStyle(color: suitColor(card)),
                          ),
                        TextSpan(
                          text: _cards.isEmpty ? street : '· $street',
                          style: theme.textTheme.bodyMedium?.copyWith(
                            color: theme.colorScheme.onSurfaceVariant,
                          ),
                        ),
                      ],
                    ),
                    key: const Key('board-selection'),
                    style: theme.textTheme.titleLarge,
                  ),
                ),
                TextButton(
                  key: const Key('board-random'),
                  onPressed: _randomFlop,
                  child: const Text('Random flop'),
                ),
                TextButton(
                  onPressed: _cards.isEmpty
                      ? null
                      : () => setState(_cards.clear),
                  child: const Text('Clear'),
                ),
              ],
            ),
            const SizedBox(height: 12),
            for (final suit in _suits.split(''))
              Padding(
                padding: const EdgeInsets.only(bottom: 4),
                child: Wrap(
                  spacing: 4,
                  runSpacing: 4,
                  children: [
                    for (final rank in _ranks.split(''))
                      _cardButton('$rank$suit'),
                  ],
                ),
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
          key: const Key('board-ok'),
          onPressed: _cards.length >= 3
              ? () => Navigator.pop(context, _cards.join(' '))
              : null,
          child: const Text('Use this board'),
        ),
      ],
    );
  }

  Widget _cardButton(String card) {
    final selected = _cards.contains(card);
    final color = suitColor(card);
    return SizedBox(
      width: 42,
      height: 34,
      child: selected
          ? FilledButton(
              key: Key('board-card-$card'),
              onPressed: () => _toggle(card),
              style: FilledButton.styleFrom(
                padding: EdgeInsets.zero,
                backgroundColor: color,
                foregroundColor: Colors.black,
              ),
              child: Text(cardTitle(card)!),
            )
          : OutlinedButton(
              key: Key('board-card-$card'),
              // A full board takes no more cards.
              onPressed: _cards.length >= 5 ? null : () => _toggle(card),
              style: OutlinedButton.styleFrom(
                padding: EdgeInsets.zero,
                foregroundColor: color,
              ),
              child: Text(cardTitle(card)!),
            ),
    );
  }
}
