/// A quick check of PLO4 range text, so the form can point at a typo before
/// asking the server. The server's parser (OmahaRange::parse) decides.
///
/// Items are separated by commas, each with an optional ":weight" (0 to 1):
///   "*" or "all"     every combo
///   "AsKsQhJh"       one exact combo
///   "AAKK", "AKQJ"   every combo with these four ranks
///   "AAxx", "KQJx"   'x' stands for any rank
///   "ds", "ss", "r"  suffix: double-suited, single-suited or rainbow only
///   "top15%"         the strongest 15% of combos
library;

const String omahaRangeHelp =
    'PLO ranges, separated by commas:\n'
    '• * or all: every hand\n'
    '• AsKsQhJh: one exact hand\n'
    '• AAKK, AKQJ: every hand with these ranks; x is any rank (AAxx, KQJx)\n'
    '• ds, ss, r after the ranks: double-suited, single-suited or rainbow '
    'only (AAxxds)\n'
    '• top15%: the strongest 15% of hands\n'
    '• :0.5 after an item: held half of the time\n'
    'Later items override earlier ones.';

final _exact = RegExp(r'^([2-9TJQKA][cdhs]){4}$', caseSensitive: false);
final _ranks = RegExp(r'^[2-9TJQKAX]{4}(DS|SS|R)?$', caseSensitive: false);
final _top = RegExp(r'^TOP(\d+(\.\d+)?)%$', caseSensitive: false);

/// Null if [text] looks like a PLO4 range, else what is wrong with it.
String? checkOmahaRange(String text) {
  if (text.trim().isEmpty) return 'The range is empty.';
  for (final raw in text.split(',')) {
    final item = raw.trim();
    if (item.isEmpty) continue;
    var body = item;
    final colon = item.indexOf(':');
    if (colon >= 0) {
      body = item.substring(0, colon).trim();
      final weight = double.tryParse(item.substring(colon + 1).trim());
      if (weight == null || weight < 0 || weight > 1) {
        return '"$item": the weight after ":" must be 0 to 1.';
      }
    }
    if (body == '*' || body.toLowerCase() == 'all') continue;
    final top = _top.firstMatch(body);
    if (top != null) {
      final pct = double.parse(top.group(1)!);
      if (pct <= 0 || pct > 100) {
        return '"$item": top needs a share from 0 to 100%.';
      }
      continue;
    }
    if (_exact.hasMatch(body)) {
      final cards = {
        for (var i = 0; i < 8; i += 2) body.substring(i, i + 2).toLowerCase(),
      };
      if (cards.length != 4) return '"$item" holds the same card twice.';
      continue;
    }
    if (_ranks.hasMatch(body)) continue;
    return '"$item" is not a PLO hand. Use four ranks such as AAxx or KQJTds, '
        'an exact hand such as AsKsQhJh, or top15%.';
  }
  return null;
}
