import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:path_provider/path_provider.dart';

import 'src/app.dart';
import 'src/settings/theme_settings.dart';
import 'src/settings/theme_store.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();
  // Read the saved look before the first frame, so the app opens in it.
  final themeStore = FileThemeStore(getApplicationSupportDirectory);
  final themeSettings = await themeStore.load();
  runApp(
    ProviderScope(
      overrides: [
        themeStoreProvider.overrideWithValue(themeStore),
        initialThemeSettingsProvider.overrideWithValue(themeSettings),
      ],
      child: const GtoApp(),
    ),
  );
}
