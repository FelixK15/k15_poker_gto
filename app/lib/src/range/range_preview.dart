import 'package:flutter/material.dart';

import 'hand_range.dart';

/// A small 13×13 picture of a range typed as text. Each class is filled from
/// the bottom as far as it is in the range. Tapping it calls [onTap].
class RangePreview extends StatelessWidget {
  const RangePreview({
    super.key,
    required this.text,
    required this.onTap,
    this.size = 88,
    this.tooltip,
  });

  final String text;
  final VoidCallback onTap;
  final double size;
  final String? tooltip;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    final range = HandRange.tryParse(text);
    final weights = range == null
        ? null
        : [for (final hand in HandClass.all) range.classWeight(hand)];
    final preview = Material(
      color: scheme.surfaceContainerHigh,
      borderRadius: BorderRadius.circular(6),
      child: InkWell(
        borderRadius: BorderRadius.circular(6),
        onTap: onTap,
        child: Container(
          width: size,
          height: size,
          padding: const EdgeInsets.all(3),
          decoration: BoxDecoration(
            borderRadius: BorderRadius.circular(6),
            border: Border.all(
              color: weights == null ? scheme.error : scheme.outlineVariant,
            ),
          ),
          child: CustomPaint(
            painter: _PreviewPainter(
              weights: weights,
              fill: scheme.primary,
              empty: scheme.surfaceContainerHighest,
            ),
          ),
        ),
      ),
    );
    return tooltip == null
        ? preview
        : Tooltip(message: tooltip, child: preview);
  }
}

class _PreviewPainter extends CustomPainter {
  _PreviewPainter({
    required this.weights,
    required this.fill,
    required this.empty,
  });

  /// One per class in grid order; null if the text is not a range.
  final List<double>? weights;
  final Color fill;
  final Color empty;

  @override
  void paint(Canvas canvas, Size size) {
    final cell = size.width / 13;
    const gap = 0.5;
    final paint = Paint();
    for (var index = 0; index < 169; ++index) {
      final rect = Rect.fromLTWH(
        (index % 13) * cell + gap,
        (index ~/ 13) * cell + gap,
        cell - 2 * gap,
        cell - 2 * gap,
      );
      canvas.drawRect(rect, paint..color = empty);
      final weight = weights?[index] ?? 0;
      if (weight <= 0) continue;
      canvas.drawRect(
        Rect.fromLTRB(
          rect.left,
          rect.bottom - rect.height * weight.clamp(0.0, 1.0),
          rect.right,
          rect.bottom,
        ),
        paint..color = fill,
      );
    }
  }

  @override
  bool shouldRepaint(_PreviewPainter old) =>
      old.fill != fill ||
      old.empty != empty ||
      !_sameWeights(old.weights, weights);

  static bool _sameWeights(List<double>? a, List<double>? b) {
    if (a == null || b == null) return a == b;
    for (var i = 0; i < a.length; ++i) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }
}
