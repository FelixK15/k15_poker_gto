import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'connection/connect_screen.dart';
import 'connection/server_connection.dart';
import 'home/home_screen.dart';
import 'settings/theme_picker.dart';
import 'settings/theme_settings.dart';

class GtoApp extends ConsumerStatefulWidget {
  const GtoApp({super.key});

  @override
  ConsumerState<GtoApp> createState() => _GtoAppState();
}

class _GtoAppState extends ConsumerState<GtoApp> {
  late final AppLifecycleListener _lifecycle;

  @override
  void initState() {
    super.initState();
    // Give a server we started the chance to save before the window closes.
    // If the app dies without getting here, the server notices by itself.
    _lifecycle = AppLifecycleListener(
      onExitRequested: () async {
        await ref.read(connectionProvider.notifier).disconnect();
        return AppExitResponse.exit;
      },
    );
  }

  @override
  void dispose() {
    _lifecycle.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final look = ref.watch(themeSettingsProvider);
    final api = ref.watch(connectionProvider);
    return MaterialApp(
      title: 'K15 GTO',
      debugShowCheckedModeBanner: false,
      theme: look.theme(Brightness.light),
      darkTheme: look.theme(Brightness.dark),
      themeMode: look.mode,
      // A new key per connection, so nothing from the last server lingers.
      home: api == null
          ? const Stack(
              children: [
                ConnectScreen(),
                // The connect screen has no app bar to put this in.
                SafeArea(
                  child: Align(
                    alignment: Alignment.topRight,
                    child: Padding(
                      padding: EdgeInsets.all(8),
                      child: Material(
                        type: MaterialType.transparency,
                        child: ThemeButton(),
                      ),
                    ),
                  ),
                ),
              ],
            )
          : HomeScreen(key: ObjectKey(api), api: api),
    );
  }
}
