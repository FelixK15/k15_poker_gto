import 'dart:convert';
import 'dart:io';

import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:path_provider/path_provider.dart';

/// Ranges the user saved under a name.
abstract class RangeStore {
  /// Name to range text, sorted by name.
  Future<Map<String, String>> load();

  /// Saves [text] as [name], replacing a range of that name.
  Future<void> save(String name, String text);

  Future<void> delete(String name);
}

/// Keeps the ranges in memory only. For tests, and as a fallback where no
/// directory can be written.
class MemoryRangeStore implements RangeStore {
  MemoryRangeStore([Map<String, String>? initial]) : _ranges = {...?initial};

  final Map<String, String> _ranges;

  @override
  Future<Map<String, String>> load() async => _sorted(_ranges);

  @override
  Future<void> save(String name, String text) async => _ranges[name] = text;

  @override
  Future<void> delete(String name) async => _ranges.remove(name);
}

/// Keeps the ranges in one JSON file, readable and editable by hand.
class FileRangeStore implements RangeStore {
  FileRangeStore(this._directory);

  /// Resolved on first use; the app's support directory by default.
  final Future<Directory> Function() _directory;

  Future<File> _file() async {
    final directory = await _directory();
    return File('${directory.path}${Platform.pathSeparator}ranges.json');
  }

  @override
  Future<Map<String, String>> load() async {
    try {
      final file = await _file();
      if (!await file.exists()) return {};
      final decoded = jsonDecode(await file.readAsString());
      if (decoded is! Map) return {};
      return _sorted({
        for (final entry in decoded.entries)
          if (entry.value is String) '${entry.key}': entry.value as String,
      });
    } on FormatException {
      // A file someone broke by hand should not take the editor down with it.
      return {};
    } on FileSystemException {
      return {};
    }
  }

  Future<void> _write(Map<String, String> ranges) async {
    final file = await _file();
    await file.parent.create(recursive: true);
    // Write next to the file and rename, so a crash cannot leave it half
    // written.
    final temporary = File('${file.path}.tmp');
    await temporary.writeAsString(
      const JsonEncoder.withIndent('  ').convert(_sorted(ranges)),
    );
    await temporary.rename(file.path);
  }

  @override
  Future<void> save(String name, String text) async {
    final ranges = await load();
    ranges[name] = text;
    await _write(ranges);
  }

  @override
  Future<void> delete(String name) async {
    final ranges = await load();
    if (ranges.remove(name) != null) await _write(ranges);
  }
}

Map<String, String> _sorted(Map<String, String> ranges) {
  final names = ranges.keys.toList()
    ..sort((a, b) => a.toLowerCase().compareTo(b.toLowerCase()));
  return {for (final name in names) name: ranges[name]!};
}

/// Where the app keeps saved ranges. Overridden in tests.
final rangeStoreProvider = Provider<RangeStore>(
  (ref) => FileRangeStore(getApplicationSupportDirectory),
);
