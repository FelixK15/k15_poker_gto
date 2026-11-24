/// "512 B", "3.4 MB", "12.0 GB".
String formatBytes(num bytes) {
  const units = ['B', 'KB', 'MB', 'GB', 'TB'];
  var value = bytes.toDouble();
  var unit = 0;
  while (value >= 1024 && unit < units.length - 1) {
    value /= 1024;
    ++unit;
  }
  return unit == 0
      ? '${value.toStringAsFixed(0)} B'
      : '${value.toStringAsFixed(1)} ${units[unit]}';
}

/// "1,234,567".
String formatCount(num count) {
  final digits = count.toStringAsFixed(0);
  final out = StringBuffer();
  for (var i = 0; i < digits.length; ++i) {
    if (i > 0 && (digits.length - i) % 3 == 0) out.write(',');
    out.write(digits[i]);
  }
  return out.toString();
}

/// A chip amount without trailing zeros: "5", "6.93", "97.5".
String formatChips(double chips) {
  final text = chips.toStringAsFixed(2);
  if (!text.contains('.')) return text;
  return text.replaceFirst(RegExp(r'\.?0+$'), '');
}

/// "42.5%".
String formatPercent(double fraction, {int digits = 1}) =>
    '${(fraction * 100).toStringAsFixed(digits)}%';
