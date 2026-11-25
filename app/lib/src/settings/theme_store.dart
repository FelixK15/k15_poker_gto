import 'dart:convert';
import 'dart:io';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:path_provider/path_provider.dart';

import 'theme_settings.dart';

/// Where the chosen look is kept between runs.
abstract class ThemeStore {
  /// The saved settings, or the defaults if nothing usable was saved.
  Future<ThemeSettings> load();

  Future<void> save(ThemeSettings settings);
}

/// Keeps the settings in memory only. For tests.
class MemoryThemeStore implements ThemeStore {
  MemoryThemeStore([this.settings = const ThemeSettings()]);

  ThemeSettings settings;

  @override
  Future<ThemeSettings> load() async => settings;

  @override
  Future<void> save(ThemeSettings settings) async => this.settings = settings;
}

/// Keeps the settings under `theme` in `settings.json`, readable and editable
/// by hand. Other keys in the file are left as they are.
class FileThemeStore implements ThemeStore {
  FileThemeStore(this._directory);

  /// Resolved on each use; the app's support directory by default.
  final Future<Directory> Function() _directory;

  Future<File> _file() async {
    final directory = await _directory();
    return File('${directory.path}${Platform.pathSeparator}settings.json');
  }

  Future<Map<String, Object?>> _read(File file) async {
    try {
      if (!await file.exists()) return {};
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return {};
      return {for (final entry in decoded.entries) '${entry.key}': entry.value};
    } on FormatException {
      // A file someone broke by hand should not keep the app from starting.
      return {};
    } on FileSystemException {
      return {};
    }
  }

  @override
  Future<ThemeSettings> load() async {
    try {
      return ThemeSettings.fromJson((await _read(await _file()))['theme']);
    } on Exception {
      // No directory to look in. The look is not worth failing to start over.
      return const ThemeSettings();
    }
  }

  @override
  Future<void> save(ThemeSettings settings) async {
    try {
      final file = await _file();
      final all = await _read(file);
      all['theme'] = settings.toJson();
      await file.parent.create(recursive: true);
      // Write next to the file and rename, so a crash cannot leave it half
      // written.
      final temporary = File('${file.path}.tmp');
      await temporary.writeAsString(
        const JsonEncoder.withIndent('  ').convert(all),
      );
      await temporary.rename(file.path);
    } on Exception {
      // The choice still holds for this run; it just will not be remembered.
    }
  }
}

/// Where the app keeps the chosen look. Overridden in tests.
final themeStoreProvider = Provider<ThemeStore>(
  (ref) => FileThemeStore(getApplicationSupportDirectory),
);
