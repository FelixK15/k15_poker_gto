import 'dart:ui';

import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import 'connection/connect_screen.dart';
import 'connection/server_connection.dart';
import 'home/home_screen.dart';

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
    const seed = Color(0xFF2E7D5B);
    final api = ref.watch(connectionProvider);
    return MaterialApp(
      title: 'K15 GTO',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.light),
      darkTheme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.dark),
      themeMode: ThemeMode.dark,
      // A new key per connection, so nothing from the last server lingers.
      home: api == null
          ? const ConnectScreen()
          : HomeScreen(key: ObjectKey(api), api: api),
    );
  }
}
