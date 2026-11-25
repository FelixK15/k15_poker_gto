import 'package:flutter/material.dart';

import 'action_style.dart';
import 'matrix_model.dart';

/// What the grid's colours stand for.
enum MatrixView {
  /// How often each class takes each action.
  strategy('Strategy'),

  /// What each class is worth, in chips.
  ev('EV'),

  /// Each class's share of the pot at showdown.
  equity('Equity');

  const MatrixView(this.label);

  final String label;
}

/// Colour for a value between [low] (red) and [high] (green).
Color valueColor(double value, double low, double high) {
  const bad = Color(0xFFD9534F);
  const middle = Color(0xFFE0B94F);
  const good = Color(0xFF45A36B);
  final t = high > low ? ((value - low) / (high - low)).clamp(0.0, 1.0) : 0.5;
  return t < 0.5
      ? Color.lerp(bad, middle, t * 2)!
      : Color.lerp(middle, good, (t - 0.5) * 2)!;
}

/// The 13×13 grid of hand classes. Each cell is filled from the bottom as far
/// as the class is in the range, split left to right by how often it takes
/// each action.
class StrategyMatrix extends StatelessWidget {
  const StrategyMatrix({
    super.key,
    required this.model,
    required this.colors,
    this.view = MatrixView.strategy,
    this.selected,
    this.onSelected,
    this.hovered,
    this.onHover,
    this.highlightAction,
  });

  final MatrixModel model;

  /// Strategy splits each cell by action; the other views colour it by value.
  final MatrixView view;

  /// One colour per action of [model].
  final List<Color> colors;

  /// Index of the highlighted cell, 0 to 168.
  final int? selected;
  final ValueChanged<int>? onSelected;

  /// Cell under the mouse, outlined lightly; [onHover] reports it, null once
  /// the mouse leaves the grid.
  final int? hovered;
  final ValueChanged<int?>? onHover;

  /// In the strategy view, show only how often each class takes this action.
  final int? highlightAction;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return AspectRatio(
      aspectRatio: 1,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final side = constraints.biggest.shortestSide;
          int? indexAt(Offset position) {
            final cell = side / 13;
            final col = (position.dx / cell).floor();
            final row = (position.dy / cell).floor();
            if (row < 0 || row > 12 || col < 0 || col > 12) return null;
            return row * 13 + col;
          }

          final grid = GestureDetector(
            onTapDown: onSelected == null
                ? null
                : (details) {
                    final index = indexAt(details.localPosition);
                    if (index != null) onSelected!(index);
                  },
            child: CustomPaint(
              size: Size.square(side),
              painter: _MatrixPainter(
                model: model,
                colors: colors,
                view: view,
                selected: selected,
                hovered: hovered,
                highlightAction: highlightAction,
                fontFamily: DefaultTextStyle.of(context).style.fontFamily,
                emptyColor: scheme.surfaceContainerHigh,
                emptyLabelColor: scheme.onSurfaceVariant.withValues(
                  alpha: 0.55,
                ),
                selectionColor: scheme.onSurface,
              ),
            ),
          );
          if (onHover == null) return grid;
          return MouseRegion(
            onHover: (event) {
              final index = indexAt(event.localPosition);
              if (index != hovered) onHover!(index);
            },
            onExit: (_) => onHover!(null),
            child: grid,
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
    required this.view,
    required this.selected,
    required this.hovered,
    required this.highlightAction,
    required this.fontFamily,
    required this.emptyColor,
    required this.emptyLabelColor,
    required this.selectionColor,
  });

  final MatrixModel model;
  final List<Color> colors;
  final MatrixView view;
  final int? selected;
  final int? hovered;
  final int? highlightAction;
  final String? fontFamily;
  final Color emptyColor;
  final Color emptyLabelColor;
  final Color selectionColor;

  @override
  void paint(Canvas canvas, Size size) {
    final cell = size.width / 13;
    final gap = cell > 24 ? 1.0 : 0.5;
    final paint = Paint();
    final fontSize = (cell * 0.28).clamp(7.0, 14.0);
    final highlight =
        view == MatrixView.strategy &&
            highlightAction != null &&
            highlightAction! < model.actionCount
        ? highlightAction
        : null;

    // EV is coloured relative to the hands on screen; equity runs 0 to 1.
    var low = 0.0, high = 1.0;
    if (view == MatrixView.ev) {
      final values = [
        for (final c in model.cells)
          if (!c.isEmpty) c.ev,
      ];
      if (values.isNotEmpty) {
        low = values.reduce((a, b) => a < b ? a : b);
        high = values.reduce((a, b) => a > b ? a : b);
      }
    }

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
          if (view != MatrixView.strategy) {
            canvas.drawRect(
              Rect.fromLTRB(rect.left, top, rect.right, rect.bottom),
              paint
                ..color = valueColor(
                  view == MatrixView.ev ? data.ev : data.equity,
                  low,
                  high,
                ),
            );
          } else if (model.actionCount == 0) {
            canvas.drawRect(
              Rect.fromLTRB(rect.left, top, rect.right, rect.bottom),
              paint..color = rangeColor,
            );
          } else if (highlight != null) {
            // Only the chosen action, from the left, as wide as it is taken.
            final width = rect.width * data.frequency(highlight);
            if (width > 0) {
              canvas.drawRect(
                Rect.fromLTRB(rect.left, top, rect.left + width, rect.bottom),
                paint..color = colors[highlight],
              );
            }
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
                color: data.isEmpty ? emptyLabelColor : const Color(0xFFFFFFFF),
                shadows: data.isEmpty
                    ? null
                    : const [Shadow(blurRadius: 2, color: Color(0xCC000000))],
              ),
            ),
            textDirection: TextDirection.ltr,
          )..layout(maxWidth: rect.width);
          text.paint(canvas, rect.topLeft + const Offset(3, 2));

          // The number itself, where there is room for it.
          if (view != MatrixView.strategy && !data.isEmpty && cell >= 34) {
            final number = TextPainter(
              text: TextSpan(
                text: view == MatrixView.ev
                    ? data.ev.toStringAsFixed(1)
                    : '${(data.equity * 100).round()}%',
                style: TextStyle(
                  fontFamily: fontFamily,
                  fontSize: fontSize * 0.9,
                  color: const Color(0xFFFFFFFF),
                  shadows: const [
                    Shadow(blurRadius: 2, color: Color(0xCC000000)),
                  ],
                ),
              ),
              textDirection: TextDirection.ltr,
            )..layout(maxWidth: rect.width);
            number.paint(
              canvas,
              Offset(rect.left + 3, rect.bottom - number.height - 2),
            );
          }
        }
      }
    }

    final over = hovered;
    if (over != null && over != selected) {
      canvas.drawRect(
        Rect.fromLTWH(
          (over % 13) * cell,
          (over ~/ 13) * cell,
          cell,
          cell,
        ).deflate(1),
        Paint()
          ..style = PaintingStyle.stroke
          ..strokeWidth = 1.5
          ..color = selectionColor.withValues(alpha: 0.5),
      );
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
      old.view != view ||
      old.selected != selected ||
      old.hovered != hovered ||
      old.highlightAction != highlightAction ||
      old.colors != colors ||
      old.emptyColor != emptyColor;
}
