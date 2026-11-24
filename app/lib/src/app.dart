import 'package:flutter/material.dart';

import 'connection/connect_screen.dart';

class GtoApp extends StatelessWidget {
  const GtoApp({super.key});

  @override
  Widget build(BuildContext context) {
    const seed = Color(0xFF2E7D5B);
    return MaterialApp(
      title: 'K15 GTO',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.light),
      darkTheme: ThemeData(colorSchemeSeed: seed, brightness: Brightness.dark),
      themeMode: ThemeMode.dark,
      home: const ConnectScreen(),
    );
  }
}
