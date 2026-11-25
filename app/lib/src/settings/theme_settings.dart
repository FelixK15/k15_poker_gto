import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'app_theme.dart';
import 'theme_store.dart';

export 'app_theme.dart';

/// The colour families the app can be themed in. Each one seeds a full
/// Material colour scheme, for light and for dark.
enum AppPalette {
  felt('Felt green', Color(0xFF2E7D5B)),
  ocean('Ocean blue', Color(0xFF1F6FB2)),
  royal('Royal purple', Color(0xFF6750A4)),
  ember('Ember', Color(0xFFC2532D)),
  gold('Gold', Color(0xFFB08A1E)),
  slate('Slate', Color(0xFF546E7A));

  const AppPalette(this.label, this.seed);

  final String label;
  final Color seed;
}

/// How the app looks: which colours, and light or dark.
@immutable
class ThemeSettings {
  const ThemeSettings({
    this.palette = AppPalette.felt,
    this.mode = ThemeMode.dark,
  });

  /// Reads what [toJson] wrote. Anything missing or unknown, such as a palette
  /// from a newer version, falls back to the default.
  factory ThemeSettings.fromJson(Object? json) {
    const fallback = ThemeSettings();
    if (json is! Map) return fallback;
    return ThemeSettings(
      palette:
          AppPalette.values.asNameMap()[json['palette']] ?? fallback.palette,
      mode: ThemeMode.values.asNameMap()[json['mode']] ?? fallback.mode,
    );
  }

  final AppPalette palette;

  /// Light, dark, or whatever the system is set to.
  final ThemeMode mode;

  ThemeSettings copyWith({AppPalette? palette, ThemeMode? mode}) =>
      ThemeSettings(palette: palette ?? this.palette, mode: mode ?? this.mode);

  ThemeData theme(Brightness brightness) =>
      buildAppTheme(palette.seed, brightness);

  Map<String, Object?> toJson() => {'palette': palette.name, 'mode': mode.name};

  @override
  bool operator ==(Object other) =>
      other is ThemeSettings && other.palette == palette && other.mode == mode;

  @override
  int get hashCode => Object.hash(palette, mode);

  @override
  String toString() => 'ThemeSettings(${palette.name}, ${mode.name})';
}

/// What the app starts with. `main` overrides it with what was saved, read
/// before the first frame so the app does not open in the wrong colours.
final initialThemeSettingsProvider = Provider<ThemeSettings>(
  (ref) => const ThemeSettings(),
);

/// The current look. Every change is saved.
class ThemeController extends Notifier<ThemeSettings> {
  Future<void> _saved = Future.value();

  @override
  ThemeSettings build() => ref.watch(initialThemeSettingsProvider);

  /// Completes once every change so far is written.
  Future<void> get saved => _saved;

  void setPalette(AppPalette palette) => _set(state.copyWith(palette: palette));

  void setMode(ThemeMode mode) => _set(state.copyWith(mode: mode));

  void _set(ThemeSettings next) {
    if (next == state) return;
    state = next;
    final store = ref.read(themeStoreProvider);
    // One write at a time, in order, so the last choice is what ends up saved.
    _saved = _saved.then((_) => store.save(next));
  }
}

final themeSettingsProvider = NotifierProvider<ThemeController, ThemeSettings>(
  ThemeController.new,
);
