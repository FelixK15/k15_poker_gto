import 'package:flutter/material.dart';

/// A small "i" that explains the control next to it. The text pops up on a
/// tap or click and stays open until a tap elsewhere or on the button again.
class InfoButton extends StatefulWidget {
  const InfoButton(this.text, {super.key});

  final String text;

  /// Room for the button at the end of a dense text field, which the usual
  /// 48 pixels would make taller.
  static const BoxConstraints fieldConstraints = BoxConstraints(
    minWidth: 32,
    minHeight: 32,
  );

  /// Puts the button at the end of a text field or a dropdown.
  static InputDecoration decorate(InputDecoration decoration, String? text) {
    if (text == null) return decoration;
    return decoration.copyWith(
      suffixIcon: InfoButton(text),
      suffixIconConstraints: fieldConstraints,
    );
  }

  @override
  State<InfoButton> createState() => _InfoButtonState();
}

class _InfoButtonState extends State<InfoButton> {
  final _menu = MenuController();

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    // A pop-over rather than a tooltip: it stays open while it is read or
    // clicked, and closes on a tap outside it or on the button again.
    return MenuAnchor(
      controller: _menu,
      alignmentOffset: const Offset(-8, 4),
      style: MenuStyle(
        backgroundColor: WidgetStatePropertyAll(
          theme.colorScheme.surfaceContainerHighest,
        ),
        padding: const WidgetStatePropertyAll(EdgeInsets.zero),
        shape: WidgetStatePropertyAll(
          RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(8),
            side: BorderSide(color: theme.colorScheme.outlineVariant),
          ),
        ),
      ),
      menuChildren: [
        ConstrainedBox(
          constraints: const BoxConstraints(maxWidth: 320),
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 14, vertical: 10),
            child: Text(
              widget.text,
              key: const Key('info-text'),
              style: theme.textTheme.bodyMedium?.copyWith(
                color: theme.colorScheme.onSurface,
              ),
            ),
          ),
        ),
      ],
      child: Tooltip(
        message: 'More about this',
        child: InkResponse(
          radius: 16,
          canRequestFocus: false,
          // Also keeps the tap from reaching the text field around it.
          onTap: () => _menu.isOpen ? _menu.close() : _menu.open(),
          child: Padding(
            padding: const EdgeInsets.all(6),
            child: Icon(
              Icons.info_outline,
              size: 18,
              color: theme.colorScheme.onSurfaceVariant,
            ),
          ),
        ),
      ),
    );
  }
}
