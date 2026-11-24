import 'dart:typed_data';

/// Ranks from ace down, the order of rows and columns in the range grid.
const String gridRanks = 'AKQJT98765432';

const String _rankOrder = '23456789TJQKA';
const String _suitOrder = 'cdhs';

/// Thrown by [HandRange.parse] with a message naming the item it cannot read.
class RangeFormatException implements Exception {
  RangeFormatException(this.message);

  final String message;

  @override
  String toString() => message;
}

/// One of the 169 hand classes: a pair, a suited or an offsuit combination.
class HandClass {
  const HandClass._(this.index, this.high, this.low, this.suited);

  /// Position in the 13×13 grid, row by row: pairs on the diagonal, suited
  /// hands above it, offsuit hands below.
  final int index;

  /// Rank indices, 0 = deuce … 12 = ace, [high] >= [low].
  final int high;
  final int low;
  final bool suited;

  bool get isPair => high == low;

  /// Number of exact hands: 6 for pairs, 4 suited, 12 offsuit.
  int get combos => isPair ? 6 : (suited ? 4 : 12);

  /// "AA", "AKs", "T9o".
  String get name {
    final ranks = '${_rankOrder[high]}${_rankOrder[low]}';
    return isPair ? ranks : '$ranks${suited ? 's' : 'o'}';
  }

  /// All 169, in grid order.
  static final List<HandClass> all = List.generate(169, (index) {
    final row = index ~/ 13;
    final col = index % 13;
    // Grid rows and columns run from ace (0) down to deuce (12).
    final a = 12 - row;
    final b = 12 - col;
    return HandClass._(index, a > b ? a : b, a > b ? b : a, col > row);
  });

  /// The class of rank indices [a] and [b].
  static HandClass of(int a, int b, {required bool suited}) {
    final high = a > b ? a : b;
    final low = a > b ? b : a;
    final row = 12 - high;
    final col = 12 - low;
    if (high == low) return all[row * 13 + row];
    return all[suited ? row * 13 + col : col * 13 + row];
  }

  /// Reads "AA", "AKs", "AKo"; null for anything else, including "AK".
  static HandClass? tryParse(String text) {
    if (text.length < 2 || text.length > 3) return null;
    final a = _rankOrder.indexOf(text[0].toUpperCase());
    final b = _rankOrder.indexOf(text[1].toUpperCase());
    if (a < 0 || b < 0) return null;
    if (a == b) return text.length == 2 ? of(a, b, suited: false) : null;
    if (text.length != 3) return null;
    final kind = text[2].toLowerCase();
    if (kind != 's' && kind != 'o') return null;
    return of(a, b, suited: kind == 's');
  }

  /// Indices of this class's exact hands in [HandRange.weights].
  List<int> get comboIndices => _comboIndices[index];

  static final List<List<int>> _comboIndices = List.generate(169, (index) {
    final hand = all[index];
    final result = <int>[];
    for (var s1 = 0; s1 < 4; ++s1) {
      for (var s2 = 0; s2 < 4; ++s2) {
        final include = hand.isPair
            ? s1 < s2
            : (hand.suited ? s1 == s2 : s1 != s2);
        if (include) {
          result.add(
            HandRange.comboIndex(hand.high * 4 + s1, hand.low * 4 + s2),
          );
        }
      }
    }
    return result;
  });
}

/// A Hold'em range: a weight from 0 to 1 for each of the 1326 exact hands.
///
/// Reads and writes the text form used by PioSOLVER, and reads the bracket
/// form GTO+ exports. Mirrors the server's parser, which has the last word.
class HandRange {
  HandRange() : weights = Float64List(comboCount);

  HandRange._(this.weights);

  static const int comboCount = 1326;

  /// Indexed by [comboIndex].
  final Float64List weights;

  HandRange copy() => HandRange._(Float64List.fromList(weights));

  /// Index of the hand made of cards [a] and [b], each rank * 4 + suit with
  /// suits in the order clubs, diamonds, hearts, spades.
  static int comboIndex(int a, int b) {
    final low = a < b ? a : b;
    final high = a < b ? b : a;
    return high * (high - 1) ~/ 2 + low;
  }

  static int? _card(String text) {
    if (text.length != 2) return null;
    final rank = _rankOrder.indexOf(text[0].toUpperCase());
    final suit = _suitOrder.indexOf(text[1].toLowerCase());
    return rank < 0 || suit < 0 ? null : rank * 4 + suit;
  }

  static String _cardName(int card) =>
      '${_rankOrder[card ~/ 4]}${_suitOrder[card % 4]}';

  /// Average weight of a class's hands.
  double classWeight(HandClass hand) {
    var sum = 0.0;
    for (final index in hand.comboIndices) {
      sum += weights[index];
    }
    return sum / hand.combos;
  }

  /// Whether a class's hands do not all have the same weight.
  bool isMixed(HandClass hand) {
    final indices = hand.comboIndices;
    final first = weights[indices.first];
    return indices.any((index) => weights[index] != first);
  }

  void setClassWeight(HandClass hand, double weight) {
    for (final index in hand.comboIndices) {
      weights[index] = weight;
    }
  }

  /// Sum of all weights: the number of hands in the range.
  double get totalCombos {
    var sum = 0.0;
    for (final weight in weights) {
      sum += weight;
    }
    return sum;
  }

  bool get isEmpty => weights.every((weight) => weight == 0);

  void clear() => weights.fillRange(0, comboCount, 0);

  /// Reads a range such as
  ///   "QQ+, AKs, AQo:0.5, A5s-A2s, 98s, KhQh:0.25"
  /// or, as GTO+ writes it, "[50]AQo,KQs[/50],AA,KK". Later items override
  /// earlier ones. Throws [RangeFormatException].
  static HandRange parse(String text) {
    final range = HandRange();
    var groupWeight = 1.0;
    // Bracket tags switch the weight for the items that follow.
    final pattern = RegExp(r'\[(/?)([\d.]*)\]');
    var start = 0;
    for (final tag in pattern.allMatches(text)) {
      _parseItems(range, text.substring(start, tag.start), groupWeight);
      if (tag.group(1) == '/') {
        groupWeight = 1.0;
      } else {
        final percent = double.tryParse(tag.group(2)!);
        if (percent == null || percent < 0 || percent > 100) {
          throw RangeFormatException(
            'Cannot read the weight in "${tag.group(0)}".',
          );
        }
        groupWeight = percent / 100;
      }
      start = tag.end;
    }
    _parseItems(range, text.substring(start), groupWeight);
    return range;
  }

  /// [parse] that returns null instead of throwing.
  static HandRange? tryParse(String text) {
    try {
      return parse(text);
    } on RangeFormatException {
      return null;
    }
  }

  static void _parseItems(HandRange range, String text, double groupWeight) {
    for (final raw in text.split(RegExp(r'[,\n]'))) {
      final token = raw.trim();
      if (token.isEmpty) continue;

      var item = token;
      var weight = groupWeight;
      final colon = token.lastIndexOf(':');
      if (colon >= 0) {
        item = token.substring(0, colon).trim();
        final value = double.tryParse(token.substring(colon + 1).trim());
        if (value == null || value < 0 || value > 1) {
          throw RangeFormatException('Cannot read the weight in "$token".');
        }
        weight = value * groupWeight;
      }
      if (!_applyItem(range, item, weight)) {
        throw RangeFormatException('Cannot read "$token".');
      }
    }
  }

  /// The classes an item such as "AK", "AKs" or "77" names.
  static List<HandClass>? _classes(String text) {
    if (text.length == 2) {
      final a = _rankOrder.indexOf(text[0].toUpperCase());
      final b = _rankOrder.indexOf(text[1].toUpperCase());
      if (a < 0 || b < 0) return null;
      if (a == b) return [HandClass.of(a, b, suited: false)];
      // "AK" means suited and offsuit.
      return [
        HandClass.of(a, b, suited: true),
        HandClass.of(a, b, suited: false),
      ];
    }
    final single = HandClass.tryParse(text);
    return single == null ? null : [single];
  }

  static bool _applyItem(HandRange range, String item, double weight) {
    // An exact hand, e.g. "AhKh".
    if (item.length == 4) {
      final a = _card(item.substring(0, 2));
      final b = _card(item.substring(2, 4));
      if (a != null && b != null) {
        if (a == b) return false;
        range.weights[comboIndex(a, b)] = weight;
        return true;
      }
    }

    // "X+": everything from X upwards.
    if (item.endsWith('+')) {
      final classes = _classes(item.substring(0, item.length - 1));
      if (classes == null) return false;
      for (final hand in classes) {
        if (hand.isPair) {
          for (var rank = hand.high; rank < 13; ++rank) {
            range.setClassWeight(
              HandClass.of(rank, rank, suited: false),
              weight,
            );
          }
        } else {
          for (var low = hand.low; low < hand.high; ++low) {
            range.setClassWeight(
              HandClass.of(hand.high, low, suited: hand.suited),
              weight,
            );
          }
        }
      }
      return true;
    }

    // "X-Y": everything between X and Y.
    final dash = item.indexOf('-');
    if (dash >= 0) {
      final first = _classes(item.substring(0, dash).trim());
      final second = _classes(item.substring(dash + 1).trim());
      if (first == null || second == null || first.length != second.length) {
        return false;
      }
      for (var i = 0; i < first.length; ++i) {
        final a = first[i];
        final b = second[i];
        if (a.isPair != b.isPair) return false;
        if (a.isPair) {
          final low = a.high < b.high ? a.high : b.high;
          final high = a.high < b.high ? b.high : a.high;
          for (var rank = low; rank <= high; ++rank) {
            range.setClassWeight(
              HandClass.of(rank, rank, suited: false),
              weight,
            );
          }
        } else {
          if (a.high != b.high || a.suited != b.suited) return false;
          final low = a.low < b.low ? a.low : b.low;
          final high = a.low < b.low ? b.low : a.low;
          for (var kicker = low; kicker <= high; ++kicker) {
            range.setClassWeight(
              HandClass.of(a.high, kicker, suited: a.suited),
              weight,
            );
          }
        }
      }
      return true;
    }

    final classes = _classes(item);
    if (classes == null) return false;
    for (final hand in classes) {
      range.setClassWeight(hand, weight);
    }
    return true;
  }

  static String _formatWeight(double weight) {
    var text = weight.toStringAsFixed(4);
    text = text.replaceFirst(RegExp(r'0+$'), '');
    return text.endsWith('.') ? text.substring(0, text.length - 1) : text;
  }

  /// Text that [parse] reads back to the same weights (to four digits): one
  /// item per class where its hands agree, exact hands where they do not.
  @override
  String toString() {
    final items = <String>[];
    void add(String name, double weight) {
      items.add(weight == 1 ? name : '$name:${_formatWeight(weight)}');
    }

    // From aces down; within a high card: the pair, then suited and offsuit
    // by kicker.
    for (var high = 12; high >= 0; --high) {
      for (var low = high; low >= 0; --low) {
        final classes = low == high
            ? [HandClass.of(high, low, suited: false)]
            : [
                HandClass.of(high, low, suited: true),
                HandClass.of(high, low, suited: false),
              ];
        for (final hand in classes) {
          if (!isMixed(hand)) {
            final weight = weights[hand.comboIndices.first];
            if (weight > 0) add(hand.name, weight);
            continue;
          }
          for (var s1 = 0; s1 < 4; ++s1) {
            for (var s2 = 0; s2 < 4; ++s2) {
              final a = hand.high * 4 + s1;
              final b = hand.low * 4 + s2;
              final include = hand.isPair
                  ? s1 < s2
                  : (hand.suited ? s1 == s2 : s1 != s2);
              if (!include) continue;
              final weight = weights[comboIndex(a, b)];
              // Higher rank first; for pairs the higher suit.
              if (weight > 0) {
                add(
                  hand.isPair
                      ? '${_cardName(b)}${_cardName(a)}'
                      : '${_cardName(a)}${_cardName(b)}',
                  weight,
                );
              }
            }
          }
        }
      }
    }
    return items.join(', ');
  }
}
