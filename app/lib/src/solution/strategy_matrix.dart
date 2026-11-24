import 'package:flutter/material.dart';

import 'action_style.dart';
import 'matrix_model.dart';

/// The 13×13 grid of hand classes. Each cell is filled from the bottom as far
/// as the class is in the range, split left to right by how often it takes
/// each action.
class StrategyMatrix extends StatelessWidget {
  const StrategyMatrix({
    super.key,
    required this.model,
    required this.colors,
    this.selected,
    this.onSelected,
  });

  final MatrixModel model;

  /// One colour per action of [model].
  final List<Color> colors;

  /// Index of the highlighted cell, 0 to 168.
  final int? selected;
  final ValueChanged<int>? onSelected;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return AspectRatio(
      aspectRatio: 1,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final side = constraints.biggest.shortestSide;
          return GestureDetector(
            onTapDown: onSelected == null
                ? null
                : (details) {
                    final cell = side / 13;
                    final col = (details.localPosition.dx / cell).floor();
                    final row = (details.localPosition.dy / cell).floor();
                    if (row < 0 || row > 12 || col < 0 || col > 12) return;
                    onSelected!(row * 13 + col);
                  },
            child: CustomPaint(
              size: Size.square(side),
              painter: _MatrixPainter(
                model: model,
                colors: colors,
                selected: selected,
                fontFamily: DefaultTextStyle.of(context).style.fontFamily,
                emptyColor: scheme.surfaceContainerHighest,
                gridColor: scheme.surface,
                selectionColor: scheme.onSurface,
              ),
            ),
          );
        },
      ),
    );
  }
}

class _MatrixPainter extends CustomPainter {
  _MatrixPainter({
    required this.model,
    required this.colors,
    required this.selected,
    required this.fontFamily,
    required this.emptyColor,
    required this.gridColor,
    required this.selectionColor,
  });

  final MatrixModel model;
  final List<Color> colors;
  final int? selected;
  final String? fontFamily;
  final Color emptyColor;
  final Color gridColor;
  final Color selectionColor;

  @override
  void paint(Canvas canvas, Size size) {
    final cell = size.width / 13;
    final gap = cell > 24 ? 1.0 : 0.5;
    final paint = Paint();
    final fontSize = (cell * 0.3).clamp(7.0, 15.0);

    for (var row = 0; row < 13; ++row) {
      for (var col = 0; col < 13; ++col) {
        final data = model.cell(row, col);
        final rect = Rect.fromLTWH(
          col * cell + gap,
          row * cell + gap,
          cell - 2 * gap,
          cell - 2 * gap,
        );
        canvas.drawRect(rect, paint..color = emptyColor);

        if (!data.isEmpty) {
          // Fill from the bottom as far as the class is in the range.
          final height = rect.height * data.fraction;
          final top = rect.bottom - height;
          if (model.actionCount == 0) {
            canvas.drawRect(
              Rect.fromLTRB(rect.left, top, rect.right, rect.bottom),
              paint..color = rangeColor,
            );
          } else {
            var left = rect.left;
            for (var a = 0; a < model.actionCount; ++a) {
              final width = rect.width * data.frequency(a);
              if (width <= 0) continue;
              // The last segment takes whatever rounding left over.
              final right = a == model.actionCount - 1
                  ? rect.right
                  : (left + width).clamp(rect.left, rect.right);
              canvas.drawRect(
                Rect.fromLTRB(left, top, right, rect.bottom),
                paint..color = colors[a],
              );
              left = right;
            }
          }
        }

        // A corner mark on classes whose strategy is locked.
        if (data.hasLock) {
          final mark = cell * 0.28;
          canvas.drawPath(
            Path()
              ..moveTo(rect.right - mark, rect.top)
              ..lineTo(rect.right, rect.top)
              ..lineTo(rect.right, rect.top + mark)
              ..close(),
            paint..color = const Color(0xFFFFFFFF),
          );
        }

        if (cell >= 18) {
          final text = TextPainter(
            text: TextSpan(
              text: data.label,
              style: TextStyle(
                fontFamily: fontFamily,
                fontSize: fontSize,
                fontWeight: FontWeight.w600,
                color: data.isEmpty
                    ? const Color(0x66FFFFFF)
                    : const Color(0xFFFFFFFF),
                shadows: data.isEmpty
                    ? null
                    : const [Shadow(blurRadius: 2, color: Color(0xCC000000))],
              ),
            ),
            textDirection: TextDirection.ltr,
          )..layout(maxWidth: rect.width);
          text.paint(canvas, rect.topLeft + const Offset(3, 2));
        }
      }
    }

    final index = selected;
    if (index != null) {
      final rect = Rect.fromLTWH(
        (index % 13) * cell,
        (index ~/ 13) * cell,
        cell,
        cell,
      );
      canvas.drawRect(
        rect.deflate(1),
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 2
          ..color = selectionColor,
      );
    }
  }

  @override
  bool shouldRepaint(_MatrixPainter old) =>
      old.model != model ||
      old.selected != selected ||
      old.colors != colors ||
      old.emptyColor != emptyColor;
}
