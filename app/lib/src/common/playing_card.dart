import 'package:flutter/material.dart';

const _suitSymbols = {'s': '♠', 'h': '♥', 'd': '♦', 'c': '♣'};

/// Background of a card in the four-colour deck: spades grey, hearts red,
/// diamonds blue, clubs green.
Color cardFaceColor(String card) {
  switch (card.isEmpty ? '' : card[card.length - 1].toLowerCase()) {
    case 'h':
      return const Color(0xFFC8423C);
    case 'd':
      return const Color(0xFF2F6FC0);
    case 'c':
      return const Color(0xFF2F8F4E);
    default:
      return const Color(0xFF4A505C);
  }
}

/// A card such as "Ks" drawn as a small coloured tile, rank above suit.
class PlayingCard extends StatelessWidget {
  const PlayingCard(
    this.card, {
    super.key,
    this.width = 28,
    this.dimmed = false,
  });

  /// Rank and suit, e.g. "Ks".
  final String card;
  final double width;

  /// Drawn faded, e.g. a card that cannot come.
  final bool dimmed;

  @override
  Widget build(BuildContext context) {
    final rank = card.isEmpty ? '?' : card[0].toUpperCase();
    final suit = card.length < 2
        ? ''
        : _suitSymbols[card[1].toLowerCase()] ?? '';
    final height = width * 1.32;
    return Opacity(
      opacity: dimmed ? 0.3 : 1,
      child: Container(
        width: width,
        height: height,
        decoration: BoxDecoration(
          color: cardFaceColor(card),
          borderRadius: BorderRadius.circular(width * 0.14),
          boxShadow: const [
            BoxShadow(
              color: Color(0x40000000),
              blurRadius: 2,
              offset: Offset(0, 1),
            ),
          ],
        ),
        padding: EdgeInsets.all(width * 0.06),
        // Shrinks rather than overflows with large fonts.
        child: FittedBox(
          fit: BoxFit.scaleDown,
          child: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              Text(
                rank == 'T' ? '10' : rank,
                style: TextStyle(
                  color: Colors.white,
                  fontWeight: FontWeight.w700,
                  fontSize: width * (rank == 'T' ? 0.5 : 0.58),
                  height: 1.0,
                ),
              ),
              Text(
                suit,
                style: TextStyle(
                  color: Colors.white.withValues(alpha: 0.9),
                  fontSize: width * 0.42,
                  height: 1.05,
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

/// Cards next to each other, e.g. a board.
class CardRow extends StatelessWidget {
  const CardRow(this.cards, {super.key, this.width = 28, this.gap = 3});

  final List<String> cards;
  final double width;
  final double gap;

  @override
  Widget build(BuildContext context) {
    return Row(
      mainAxisSize: MainAxisSize.min,
      children: [
        for (var i = 0; i < cards.length; ++i) ...[
          if (i > 0) SizedBox(width: gap),
          PlayingCard(cards[i], width: width),
        ],
      ],
    );
  }
}

/// "Ks Td 7h" or "KsTd7h" as a list of cards.
List<String> splitCards(String text) {
  final compact = text.replaceAll(RegExp(r'[\s,]'), '');
  return [
    for (var i = 0; i + 1 < compact.length; i += 2) compact.substring(i, i + 2),
  ];
}
