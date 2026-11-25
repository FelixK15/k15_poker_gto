import 'package:flutter/material.dart';

/// A small "i" that explains the control next to it. The text pops up on a
/// tap or click, and under the mouse where there is one.
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
  final _tooltip = GlobalKey<TooltipState>();

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Tooltip(
      key: _tooltip,
      message: widget.text,
      // Opened by the tap below, which also keeps the tap from reaching
      // the text field around the button.
      triggerMode: TooltipTriggerMode.manual,
      // Long enough to read; a tap anywhere else closes it sooner.
      showDuration: const Duration(seconds: 20),
      constraints: const BoxConstraints(maxWidth: 320),
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      textStyle: theme.textTheme.bodyMedium?.copyWith(
        color: theme.colorScheme.onInverseSurface,
      ),
      child: InkResponse(
        radius: 16,
        canRequestFocus: false,
        onTap: () => _tooltip.currentState?.ensureTooltipVisible(),
        child: Padding(
          padding: const EdgeInsets.all(6),
          child: Icon(
            Icons.info_outline,
            size: 18,
            color: theme.colorScheme.onSurfaceVariant,
          ),
        ),
      ),
    );
  }
}
