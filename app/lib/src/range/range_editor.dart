import 'package:flutter/material.dart';

import 'hand_range.dart';
import 'range_store.dart';

/// Opens the range editor on [initialText]. Returns the edited range as text,
/// or null if the user cancelled.
Future<String?> showRangeEditor(
  BuildContext context, {
  required String title,
  required String initialText,
  required RangeStore store,
}) {
  return showDialog<String>(
    context: context,
    builder: (context) =>
        RangeEditor(title: title, initialText: initialText, store: store),
  );
}

/// Edits a range on the 13×13 grid or as text; both stay in step.
class RangeEditor extends StatefulWidget {
  const RangeEditor({
    super.key,
    required this.title,
    required this.initialText,
    required this.store,
  });

  final String title;
  final String initialText;
  final RangeStore store;

  @override
  State<RangeEditor> createState() => _RangeEditorState();
}

class _RangeEditorState extends State<RangeEditor> {
  late HandRange _range;
  late final TextEditingController _text;
  final _nameController = TextEditingController();

  /// Weight the brush paints, 0.05 to 1.
  double _brush = 1;

  /// Whether the stroke in progress removes hands instead of adding them.
  bool _erasing = false;
  String? _textError;
  Map<String, String> _saved = const {};

  @override
  void initState() {
    super.initState();
    // Keep text we cannot read on screen, so the user can fix it.
    final parsed = HandRange.tryParse(widget.initialText);
    _range = parsed ?? HandRange();
    _text = TextEditingController(text: widget.initialText.trim());
    if (parsed == null) _textError = _problem(widget.initialText);
    _loadSaved();
  }

  @override
  void dispose() {
    _text.dispose();
    _nameController.dispose();
    super.dispose();
  }

  Future<void> _loadSaved() async {
    final saved = await widget.store.load();
    if (mounted) setState(() => _saved = saved);
  }

  static String? _problem(String text) {
    try {
      HandRange.parse(text);
      return null;
    } on RangeFormatException catch (e) {
      return e.message;
    }
  }

  void _textChanged(String text) {
    final parsed = HandRange.tryParse(text);
    setState(() {
      if (parsed != null) {
        _range = parsed;
        _textError = null;
      } else {
        _textError = _problem(text);
      }
    });
  }

  /// Applies an edit made on the grid and rewrites the text to match.
  void _edit(void Function() change) {
    setState(() {
      change();
      _text.text = _range.toString();
      _textError = null;
    });
  }

  void _strokeStart(HandClass hand) {
    // Starting on a cell that already has the brush weight removes it.
    _erasing =
        (_range.classWeight(hand) - _brush).abs() < 1e-6 &&
        !_range.isMixed(hand);
    _stroke(hand);
  }

  void _stroke(HandClass hand) {
    final weight = _erasing ? 0.0 : _brush;
    if (!_range.isMixed(hand) && _range.classWeight(hand) == weight) return;
    _edit(() => _range.setClassWeight(hand, weight));
  }

  Future<void> _save() async {
    final name = _nameController.text.trim();
    if (name.isEmpty || _textError != null) return;
    await widget.store.save(name, _range.toString());
    _nameController.clear();
    await _loadSaved();
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final size = MediaQuery.sizeOf(context);
    final wide = size.width >= 900;

    final grid = RangeGrid(
      key: const Key('range-grid'),
      range: _range,
      onStrokeStart: _strokeStart,
      onStroke: _stroke,
    );
    final panel = _panel(theme);

    final body = wide
        ? Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                child: Align(alignment: Alignment.topLeft, child: grid),
              ),
              const SizedBox(width: 16),
              SizedBox(width: 320, child: SingleChildScrollView(child: panel)),
            ],
          )
        : ListView(children: [grid, const SizedBox(height: 12), panel]);

    return Dialog(
      insetPadding: wide
          ? const EdgeInsets.all(32)
          : const EdgeInsets.symmetric(horizontal: 8, vertical: 16),
      child: ConstrainedBox(
        constraints: const BoxConstraints(maxWidth: 1040, maxHeight: 760),
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Text(widget.title, style: theme.textTheme.titleLarge),
              const SizedBox(height: 12),
              Expanded(child: body),
              const SizedBox(height: 12),
              // Stacks the buttons where a row would not fit.
              OverflowBar(
                alignment: MainAxisAlignment.end,
                spacing: 8,
                overflowSpacing: 8,
                children: [
                  TextButton(
                    onPressed: () => Navigator.pop(context),
                    child: const Text('Cancel'),
                  ),
                  FilledButton(
                    key: const Key('range-use'),
                    // Text that cannot be read must be fixed first.
                    onPressed: _textError != null
                        ? null
                        : () => Navigator.pop(context, _range.toString()),
                    child: const Text('Use this range'),
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _panel(ThemeData theme) {
    final combos = _range.totalCombos;
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Text(
          '${combos.toStringAsFixed(1)} combos · '
          '${(combos / HandRange.comboCount * 100).toStringAsFixed(1)}% of hands',
          key: const Key('range-summary'),
          style: theme.textTheme.titleMedium,
        ),
        const SizedBox(height: 12),
        Text('Paint with weight ${(_brush * 100).round()}%'),
        Slider(
          key: const Key('range-brush'),
          value: _brush,
          min: 0.05,
          max: 1,
          divisions: 19,
          label: '${(_brush * 100).round()}%',
          onChanged: (value) => setState(() => _brush = value),
        ),
        Text(
          'Tap or drag over hands to add them. Starting on a hand that '
          'already has this weight removes instead.',
          style: theme.textTheme.bodySmall?.copyWith(
            color: theme.colorScheme.onSurfaceVariant,
          ),
        ),
        const SizedBox(height: 8),
        Align(
          alignment: Alignment.centerLeft,
          child: OutlinedButton.icon(
            key: const Key('range-clear'),
            onPressed: _range.isEmpty ? null : () => _edit(_range.clear),
            icon: const Icon(Icons.clear, size: 18),
            label: const Text('Clear'),
          ),
        ),
        const SizedBox(height: 16),
        TextField(
          key: const Key('range-text'),
          controller: _text,
          minLines: 3,
          maxLines: 6,
          autocorrect: false,
          enableSuggestions: false,
          decoration: InputDecoration(
            labelText: 'As text',
            helperText: 'Paste a range from PioSOLVER or GTO+ here.',
            errorText: _textError,
            errorMaxLines: 3,
            border: const OutlineInputBorder(),
          ),
          onChanged: _textChanged,
        ),
        const SizedBox(height: 16),
        Text('Saved ranges', style: theme.textTheme.titleSmall),
        const SizedBox(height: 8),
        if (_saved.isEmpty)
          Text(
            'None yet.',
            style: theme.textTheme.bodySmall?.copyWith(
              color: theme.colorScheme.onSurfaceVariant,
            ),
          )
        else
          Wrap(
            spacing: 6,
            runSpacing: 6,
            children: [
              for (final entry in _saved.entries)
                InputChip(
                  key: Key('saved-${entry.key}'),
                  label: Text(entry.key),
                  onPressed: () {
                    _text.text = entry.value;
                    _textChanged(entry.value);
                  },
                  deleteButtonTooltipMessage: 'Delete',
                  onDeleted: () async {
                    await widget.store.delete(entry.key);
                    await _loadSaved();
                  },
                ),
            ],
          ),
        const SizedBox(height: 8),
        Row(
          children: [
            Expanded(
              child: TextField(
                key: const Key('range-save-name'),
                controller: _nameController,
                decoration: const InputDecoration(
                  labelText: 'Save as',
                  isDense: true,
                  border: OutlineInputBorder(),
                ),
                onChanged: (_) => setState(() {}),
                onSubmitted: (_) => _save(),
              ),
            ),
            const SizedBox(width: 8),
            FilledButton.tonal(
              key: const Key('range-save'),
              onPressed:
                  _nameController.text.trim().isEmpty || _textError != null
                  ? null
                  : _save,
              child: const Text('Save'),
            ),
          ],
        ),
      ],
    );
  }
}

/// The 13×13 grid of a range. Each cell is filled from the bottom as far as
/// its hands are in the range.
class RangeGrid extends StatelessWidget {
  const RangeGrid({
    super.key,
    required this.range,
    this.onStrokeStart,
    this.onStroke,
  });

  final HandRange range;

  /// Called for the hand a tap or drag starts on, then [onStroke] for every
  /// further hand the drag crosses.
  final ValueChanged<HandClass>? onStrokeStart;
  final ValueChanged<HandClass>? onStroke;

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;
    return AspectRatio(
      aspectRatio: 1,
      child: LayoutBuilder(
        builder: (context, constraints) {
          final side = constraints.biggest.shortestSide;
          HandClass? at(Offset position) {
            final cell = side / 13;
            final col = (position.dx / cell).floor();
            final row = (position.dy / cell).floor();
            if (row < 0 || row > 12 || col < 0 || col > 12) return null;
            return HandClass.all[row * 13 + col];
          }

          HandClass? last;
          return GestureDetector(
            onPanDown: (details) {
              final hand = at(details.localPosition);
              last = hand;
              if (hand != null) onStrokeStart?.call(hand);
            },
            onPanUpdate: (details) {
              final hand = at(details.localPosition);
              if (hand == null || hand == last) return;
              last = hand;
              onStroke?.call(hand);
            },
            child: CustomPaint(
              size: Size.square(side),
              painter: _RangeGridPainter(
                // The range object is edited in place; its text identifies
                // what is on screen.
                signature: range.toString(),
                range: range,
                fontFamily: DefaultTextStyle.of(context).style.fontFamily,
                emptyColor: scheme.surfaceContainerHighest,
                fillColor: const Color(0xFF45A36B),
              ),
            ),
          );
        },
      ),
    );
  }
}

class _RangeGridPainter extends CustomPainter {
  _RangeGridPainter({
    required this.signature,
    required this.range,
    required this.fontFamily,
    required this.emptyColor,
    required this.fillColor,
  });

  final String signature;
  final HandRange range;
  final String? fontFamily;
  final Color emptyColor;
  final Color fillColor;

  @override
  void paint(Canvas canvas, Size size) {
    final cell = size.width / 13;
    final gap = cell > 24 ? 1.0 : 0.5;
    final paint = Paint();
    final fontSize = (cell * 0.3).clamp(7.0, 15.0);

    for (final hand in HandClass.all) {
      final rect = Rect.fromLTWH(
        (hand.index % 13) * cell + gap,
        (hand.index ~/ 13) * cell + gap,
        cell - 2 * gap,
        cell - 2 * gap,
      );
      canvas.drawRect(rect, paint..color = emptyColor);

      final weight = range.classWeight(hand);
      if (weight > 0) {
        canvas.drawRect(
          Rect.fromLTRB(
            rect.left,
            rect.bottom - rect.height * weight,
            rect.right,
            rect.bottom,
          ),
          paint..color = fillColor,
        );
      }

      if (cell >= 18) {
        final text = TextPainter(
          text: TextSpan(
            text: hand.name,
            style: TextStyle(
              fontFamily: fontFamily,
              fontSize: fontSize,
              fontWeight: FontWeight.w600,
              color: weight > 0
                  ? const Color(0xFFFFFFFF)
                  : const Color(0x66FFFFFF),
              shadows: weight > 0
                  ? const [Shadow(blurRadius: 2, color: Color(0xCC000000))]
                  : null,
            ),
          ),
          textDirection: TextDirection.ltr,
        )..layout(maxWidth: rect.width);
        text.paint(canvas, rect.topLeft + const Offset(3, 2));
      }
    }
  }

  @override
  bool shouldRepaint(_RangeGridPainter old) =>
      old.signature != signature || old.emptyColor != emptyColor;
}
