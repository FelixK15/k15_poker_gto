// Renders the main screens to PNG files, with real fonts, for a look at the
// design without starting the app. Skipped unless GTO_SCREENSHOT_DIR is set:
//
//   $env:GTO_SCREENSHOT_DIR = "C:\temp\shots"
//   flutter test test/screenshots_test.dart --update-goldens
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/jobs/jobs_screen.dart';
import 'package:k15_gto/src/range/preflop_library.dart';
import 'package:k15_gto/src/range/range_editor.dart';
import 'package:k15_gto/src/range/range_store.dart';
import 'package:k15_gto/src/solution/matrix_model.dart';
import 'package:k15_gto/src/settings/theme_settings.dart';
import 'package:k15_gto/src/solution/solution_screen.dart';
import 'package:k15_gto/src/spot/spot_setup_screen.dart';

import 'fake_api.dart';

Future<void> loadFont(String family, List<String> paths) async {
  final loader = FontLoader(family);
  var found = false;
  for (final path in paths) {
    final file = File(path);
    if (!file.existsSync()) continue;
    final bytes = file.readAsBytesSync();
    loader.addFont(Future.value(ByteData.sublistView(bytes)));
    found = true;
  }
  if (found) await loader.load();
}

void main() {
  final directory = Platform.environment['GTO_SCREENSHOT_DIR'];
  final skip = directory == null || directory.isEmpty;

  setUpAll(() async {
    if (skip) return;
    final windows = Platform.environment['WINDIR'] ?? r'C:\Windows';
    final text = [
      '$windows\\Fonts\\segoeui.ttf',
      '$windows\\Fonts\\segoeuib.ttf',
    ];
    // Material asks for these family names depending on the platform.
    await loadFont('Roboto', text);
    await loadFont('Segoe UI', text);
    final flutterBin = File(Platform.resolvedExecutable).parent.path;
    await loadFont('MaterialIcons', [
      // flutter_tester lives in bin/cache/artifacts/engine/<platform>.
      '$flutterBin\\..\\..\\material_fonts\\materialicons-regular.otf',
    ]);
  });

  Future<void> shoot(
    WidgetTester tester,
    String name,
    Widget home, {
    Size size = const Size(1400, 900),
    Future<void> Function()? then,
  }) async {
    tester.view.physicalSize = size;
    tester.view.devicePixelRatio = 1;
    addTearDown(tester.view.reset);
    await tester.pumpWidget(
      MaterialApp(
        debugShowCheckedModeBanner: false,
        theme: buildAppTheme(AppPalette.felt.seed, Brightness.dark),
        home: home,
      ),
    );
    await tester.pump();
    await tester.pump(const Duration(milliseconds: 50));
    await tester.pump(const Duration(milliseconds: 300));
    if (then != null) await then();
    await expectLater(
      find.byType(MaterialApp),
      // A file URI: a plain Windows path would lose its drive letter.
      matchesGoldenFile(
        Uri.file('$directory${Platform.pathSeparator}$name.png'),
      ),
    );
  }

  FakeApi sample() {
    final api = FakeApi();
    addSampleSolution(api);
    api.jobs.addAll([
      sampleJob(id: 'run', state: JobState.JOB_STATE_RUNNING, iteration: 120),
      sampleJob(id: 'done'),
      sampleJob(
        id: 'wait',
        state: JobState.JOB_STATE_QUEUED,
        iteration: 0,
        hasSolution: false,
      ),
    ]);
    return api;
  }

  testWidgets('solution, desktop', (tester) async {
    final api = sample();
    await shoot(
      tester,
      'solution_desktop',
      SolutionScreen(api: api, job: sampleJob()),
      then: () async {
        final index = MatrixModel.cellIndex('AhKh')!;
        final box = tester.getRect(find.byKey(const Key('matrix')));
        final cell = box.width / 13;
        await tester.tapAt(
          box.topLeft +
              Offset((index % 13 + 0.5) * cell, (index ~/ 13 + 0.5) * cell),
        );
        await tester.pump();
        await tester.pump(const Duration(milliseconds: 300));
      },
    );
  }, skip: skip);

  testWidgets('solution, EV view with locks', (tester) async {
    final api = sample();
    final root = api.nodes['']!;
    root.locked.addAll([
      for (final hand in root.oop.hands) hand.startsWith('A') && hand[2] == 'K',
    ]);
    await shoot(
      tester,
      'solution_ev',
      SolutionScreen(
        api: api,
        job: sampleJob()
          ..locks.add(
            NodeLock(
              path: '',
              hands: [
                HandLock(hand: 'AhKh', frequencies: [1, 0, 0]),
              ],
            ),
          ),
      ),
      then: () async {
        await tester.tap(
          find.descendant(
            of: find.byKey(const Key('matrix-view')),
            matching: find.text('EV'),
          ),
        );
        await tester.pump();
        await tester.pump(const Duration(milliseconds: 300));
      },
    );
  }, skip: skip);

  testWidgets('solution, phone', (tester) async {
    await shoot(
      tester,
      'solution_phone',
      SolutionScreen(api: sample(), job: sampleJob()),
      size: const Size(390, 844),
    );
  }, skip: skip);

  testWidgets('card picker', (tester) async {
    final api = sample();
    api.nodes[''] = api.nodes['check/check']!;
    await shoot(
      tester,
      'card_picker',
      SolutionScreen(api: api, job: sampleJob()),
    );
  }, skip: skip);

  testWidgets('spot setup', (tester) async {
    await shoot(
      tester,
      'spot_setup',
      Scaffold(
        body: SpotSetupScreen(
          api: sample(),
          onSubmitted: (_) {},
          estimateDelay: Duration.zero,
        ),
      ),
      size: const Size(1100, 1000),
    );
  }, skip: skip);

  testWidgets('range editor', (tester) async {
    final scenario = PreflopLibrary.scenario(
      format: TableFormat.sixMax,
      stack: 100,
      potType: PotType.singleRaised,
      opener: 'BTN',
      defender: 'BB',
    );
    await shoot(
      tester,
      'range_editor',
      Scaffold(
        body: RangeEditor(
          title: 'BB calls a BTN open (baseline)',
          initialText: scenario.oopRange.toString(),
          store: MemoryRangeStore({
            'My BB defend': 'AA',
            '3-bet bluffs': 'A5s',
          }),
        ),
      ),
      size: const Size(1200, 820),
    );
  }, skip: skip);

  testWidgets('jobs', (tester) async {
    await shoot(
      tester,
      'jobs',
      Scaffold(
        body: JobsScreen(api: sample(), onOpen: (_) {}),
      ),
      size: const Size(1000, 700),
    );
  }, skip: skip);
}
