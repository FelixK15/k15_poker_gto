import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'theme_settings.dart';

/// Opens the dialog to choose the app's colours and light or dark.
class ThemeButton extends StatelessWidget {
  const ThemeButton({super.key});

  @override
  Widget build(BuildContext context) {
    return IconButton(
      key: const Key('theme-button'),
      tooltip: 'Appearance',
      icon: const Icon(Icons.palette_outlined),
      onPressed: () => showDialog<void>(
        context: context,
        builder: (_) => const ThemeDialog(),
      ),
    );
  }
}

/// Chooses the look. Changes apply, and are saved, as they are made.
class ThemeDialog extends ConsumerWidget {
  const ThemeDialog({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settings = ref.watch(themeSettingsProvider);
    final controller = ref.read(themeSettingsProvider.notifier);
    final theme = Theme.of(context);

    return AlertDialog(
      title: const Text('Appearance'),
      contentPadding: const EdgeInsets.fromLTRB(0, 16, 0, 0),
      content: SizedBox(
        width: 340,
        child: SingleChildScrollView(
          child: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Padding(
                padding: const EdgeInsets.symmetric(horizontal: 24),
                child: SegmentedButton<ThemeMode>(
                  key: const Key('theme-mode'),
                  showSelectedIcon: false,
                  segments: const [
                    ButtonSegment(
                      value: ThemeMode.system,
                      icon: Icon(Icons.brightness_auto_outlined),
                      label: Text('System'),
                    ),
                    ButtonSegment(
                      value: ThemeMode.light,
                      icon: Icon(Icons.light_mode_outlined),
                      label: Text('Light'),
                    ),
                    ButtonSegment(
                      value: ThemeMode.dark,
                      icon: Icon(Icons.dark_mode_outlined),
                      label: Text('Dark'),
                    ),
                  ],
                  selected: {settings.mode},
                  onSelectionChanged: (modes) =>
                      controller.setMode(modes.single),
                ),
              ),
              const SizedBox(height: 8),
              for (final palette in AppPalette.values)
                ListTile(
                  key: Key('palette-${palette.name}'),
                  contentPadding: const EdgeInsets.symmetric(horizontal: 24),
                  leading: CircleAvatar(
                    radius: 14,
                    backgroundColor: palette.seed,
                  ),
                  title: Text(palette.label),
                  selected: palette == settings.palette,
                  trailing: palette == settings.palette
                      ? Icon(Icons.check, color: theme.colorScheme.primary)
                      : null,
                  onTap: () => controller.setPalette(palette),
                ),
            ],
          ),
        ),
      ),
      actions: [
        TextButton(
          onPressed: () => Navigator.of(context).pop(),
          child: const Text('Done'),
        ),
      ],
    );
  }
}
