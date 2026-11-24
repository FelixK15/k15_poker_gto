import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';

const Color foldColor = Color(0xFF3D7CB8);
const Color passiveColor = Color(0xFF45A36B);
const Color smallBetColor = Color(0xFFF0A860);
const Color bigBetColor = Color(0xFFD9534F);
const Color allinColor = Color(0xFF8E2A4A);

/// Colour for a range shown without a strategy (the player not acting).
const Color rangeColor = Color(0xFF7A8CA6);

/// One colour per action: blue to fold, green to check or call, and orange
/// through red for bets and raises, darker the larger they are.
List<Color> actionColors(List<NodeAction> actions) {
  final aggressive = [
    for (final action in actions)
      if (action.type == ActionType.ACTION_TYPE_BET ||
          action.type == ActionType.ACTION_TYPE_RAISE)
        action,
  ];
  var seen = 0;
  return [
    for (final action in actions)
      switch (action.type) {
        ActionType.ACTION_TYPE_FOLD => foldColor,
        ActionType.ACTION_TYPE_CHECK ||
        ActionType.ACTION_TYPE_CALL => passiveColor,
        ActionType.ACTION_TYPE_ALLIN => allinColor,
        // Actions come ordered by size.
        _ => Color.lerp(
          smallBetColor,
          bigBetColor,
          aggressive.length <= 1 ? 0.5 : seen++ / (aggressive.length - 1),
        )!,
      },
  ];
}

/// "Check", "Bet 6.93", "Raise to 25", "All-in".
String actionTitle(NodeAction action) {
  switch (action.type) {
    case ActionType.ACTION_TYPE_FOLD:
      return 'Fold';
    case ActionType.ACTION_TYPE_CHECK:
      return 'Check';
    case ActionType.ACTION_TYPE_CALL:
      return 'Call';
    case ActionType.ACTION_TYPE_BET:
      return 'Bet ${formatChips(action.amount)}';
    case ActionType.ACTION_TYPE_RAISE:
      return 'Raise to ${formatChips(action.amount)}';
    case ActionType.ACTION_TYPE_ALLIN:
      return 'All-in';
    default:
      return action.label;
  }
}

/// A step of a path for the breadcrumb: "bet6.93" becomes "Bet 6.93", a card
/// such as "Ts" becomes "T♠".
String pathStepTitle(String step) {
  final card = cardTitle(step);
  if (card != null) return card;
  final match = RegExp(r'^([a-z]+)([\d.]*)$').firstMatch(step.toLowerCase());
  if (match == null) return step;
  final word = match.group(1)!;
  final amount = match.group(2)!;
  final title = word == 'allin'
      ? 'All-in'
      : '${word[0].toUpperCase()}${word.substring(1)}';
  return amount.isEmpty ? title : '$title $amount';
}

const _suitSymbols = {'s': '♠', 'h': '♥', 'd': '♦', 'c': '♣'};

/// "T♠" for "Ts", or null if [text] is not a card.
String? cardTitle(String text) {
  if (text.length != 2) return null;
  final suit = _suitSymbols[text[1].toLowerCase()];
  if (suit == null || !'23456789TJQKA'.contains(text[0].toUpperCase())) {
    return null;
  }
  return '${text[0].toUpperCase()}$suit';
}

/// The usual four-colour deck, readable on a dark background.
Color suitColor(String card) {
  switch (card.isEmpty ? '' : card[card.length - 1].toLowerCase()) {
    case 'h':
      return const Color(0xFFE5645F);
    case 'd':
      return const Color(0xFF5B9BE5);
    case 'c':
      return const Color(0xFF5DB878);
    default:
      return const Color(0xFFD0D4DA);
  }
}
