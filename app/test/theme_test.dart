import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/app.dart';
import 'package:k15_gto/src/connection/server_connection.dart';
import 'package:k15_gto/src/settings/theme_settings.dart';
import 'package:k15_gto/src/settings/theme_store.dart';

void main() {
  group('ThemeSettings', () {
    test('round-trips through JSON', () {
      const settings = ThemeSettings(
        palette: AppPalette.ember,
        mode: ThemeMode.light,
      );
      expect(ThemeSettings.fromJson(settings.toJson()), settings);
    });

    test('falls back to the defaults for missing or unknown values', () {
      expect(ThemeSettings.fromJson(null), const ThemeSettings());
      expect(
        ThemeSettings.fromJson({'palette': 'neon', 'mode': 'light'}),
        const ThemeSettings(mode: ThemeMode.light),
      );
    });
  });

  group('FileThemeStore', () {
    late Directory directory;

    setUp(() => directory = Directory.systemTemp.createTempSync('gto_theme'));
    tearDown(() => directory.deleteSync(recursive: true));

    File settingsFile() =>
        File('${directory.path}${Platform.pathSeparator}settings.json');

    test('loads the defaults when nothing was saved', () async {
      final store = FileThemeStore(() async => directory);
      expect(await store.load(), const ThemeSettings());
    });

    test('saves and loads, keeping other keys in the file', () async {
      settingsFile().writeAsStringSync(jsonEncode({'other': 1}));
      final store = FileThemeStore(() async => directory);
      const settings = ThemeSettings(
        palette: AppPalette.ocean,
        mode: ThemeMode.system,
      );

      await store.save(settings);

      expect(await FileThemeStore(() async => directory).load(), settings);
      expect(jsonDecode(settingsFile().readAsStringSync())['other'], 1);
    });

    test('loads the defaults from a broken file', () async {
      settingsFile().writeAsStringSync('{not json');
      final store = FileThemeStore(() async => directory);
      expect(await store.load(), const ThemeSettings());
    });
  });

  group('theme picker', () {
    Future<MemoryThemeStore> pumpApp(
      WidgetTester tester, {
      ThemeSettings initial = const ThemeSettings(),
    }) async {
      tester.view.physicalSize = const Size(900, 1300);
      tester.view.devicePixelRatio = 1;
      addTearDown(tester.view.reset);
      final store = MemoryThemeStore(initial);
      await tester.pumpWidget(
        ProviderScope(
          overrides: [
            themeStoreProvider.overrideWithValue(store),
            initialThemeSettingsProvider.overrideWithValue(initial),
            remoteConnectorProvider.overrideWithValue(
              (_) async => fail('remote connector not expected'),
            ),
            localConnectorProvider.overrideWithValue(
              () async => fail('local connector not expected'),
            ),
          ],
          child: const GtoApp(),
        ),
      );
      return store;
    }

    MaterialApp app(WidgetTester tester) =>
        tester.widget<MaterialApp>(find.byType(MaterialApp));

    testWidgets('starts in the saved look', (tester) async {
      await pumpApp(
        tester,
        initial: const ThemeSettings(
          palette: AppPalette.royal,
          mode: ThemeMode.light,
        ),
      );

      expect(app(tester).themeMode, ThemeMode.light);
      expect(
        app(tester).theme!.colorScheme,
        ColorScheme.fromSeed(seedColor: AppPalette.royal.seed),
      );
    });

    testWidgets('changes and saves the palette and mode', (tester) async {
      final store = await pumpApp(tester);

      await tester.tap(find.byKey(const Key('theme-button')));
      await tester.pumpAndSettle();
      await tester.tap(find.byKey(const Key('palette-ocean')));
      await tester.tap(find.text('Light'));
      await tester.pumpAndSettle();

      expect(app(tester).themeMode, ThemeMode.light);
      expect(
        app(tester).theme!.colorScheme,
        ColorScheme.fromSeed(seedColor: AppPalette.ocean.seed),
      );
      expect(
        store.settings,
        const ThemeSettings(palette: AppPalette.ocean, mode: ThemeMode.light),
      );

      await tester.tap(find.text('Done'));
      await tester.pumpAndSettle();
      expect(find.byType(AlertDialog), findsNothing);
    });
  });
}
