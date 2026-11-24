import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/jobs/jobs_screen.dart';

import 'fake_api.dart';

Future<List<Job>> pumpJobs(WidgetTester tester, FakeApi api) async {
  tester.view.physicalSize = const Size(1000, 1400);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);

  final opened = <Job>[];
  await tester.pumpWidget(
    MaterialApp(
      home: Scaffold(
        body: JobsScreen(api: api, onOpen: opened.add),
      ),
    ),
  );
  await tester.pump();
  await tester.pump();
  return opened;
}

void main() {
  testWidgets('says so when there is nothing yet', (tester) async {
    await pumpJobs(tester, FakeApi());
    expect(find.textContaining('No solves yet'), findsOneWidget);
  });

  testWidgets('shows each job with its progress', (tester) async {
    final api = FakeApi()
      ..jobs.addAll([
        sampleJob(id: 'run', state: JobState.JOB_STATE_RUNNING, iteration: 120),
        sampleJob(
          id: 'wait',
          state: JobState.JOB_STATE_QUEUED,
          iteration: 0,
          hasSolution: false,
        ),
        sampleJob(id: 'done'),
        sampleJob(
          id: 'bad',
          state: JobState.JOB_STATE_FAILED,
          iteration: 0,
          hasSolution: false,
        )..error = 'cannot read the saved solution: the file is missing',
      ]);
    await pumpJobs(tester, api);

    expect(find.text('Solving'), findsOneWidget);
    expect(find.text('Queued'), findsOneWidget);
    expect(find.text('Done'), findsOneWidget);
    expect(find.text('Failed'), findsOneWidget);
    expect(
      find.text('Iteration 120 of 500 · 0.42% of pot exploitable · 0.8 s'),
      findsOneWidget,
    );
    expect(find.text('Iteration 0 of 500 · 0.8 s'), findsOneWidget);
    expect(
      find.text('cannot read the saved solution: the file is missing'),
      findsOneWidget,
    );
    expect(find.byType(LinearProgressIndicator), findsOneWidget);

    // Controls fit the state.
    expect(find.byKey(const Key('pause-run')), findsOneWidget);
    expect(find.byKey(const Key('cancel-run')), findsOneWidget);
    expect(find.byKey(const Key('resume-run')), findsNothing);
    expect(find.byKey(const Key('pause-done')), findsNothing);
    expect(find.byKey(const Key('open-run')), findsOneWidget);
    expect(find.byKey(const Key('open-done')), findsOneWidget);
    expect(find.byKey(const Key('open-wait')), findsNothing);
    expect(find.text('Browse so far'), findsOneWidget);
  });

  testWidgets('pauses, resumes and cancels', (tester) async {
    final api = FakeApi()
      ..jobs.add(sampleJob(id: 'run', state: JobState.JOB_STATE_RUNNING));
    await pumpJobs(tester, api);

    await tester.tap(find.byKey(const Key('pause-run')));
    await tester.pump();
    await tester.pump();
    expect(api.calls, ['pause run']);
    expect(find.text('Paused'), findsOneWidget);

    await tester.tap(find.byKey(const Key('resume-run')));
    await tester.pump();
    await tester.pump();
    expect(find.text('Queued'), findsOneWidget);

    await tester.tap(find.byKey(const Key('cancel-run')));
    await tester.pump();
    await tester.pump();
    expect(api.calls, ['pause run', 'resume run', 'cancel run']);
    expect(find.text('Cancelled'), findsOneWidget);
  });

  testWidgets('deletes only after confirmation', (tester) async {
    final api = FakeApi()..jobs.add(sampleJob(id: 'done'));
    await pumpJobs(tester, api);

    await tester.tap(find.byKey(const Key('delete-done')));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    expect(find.text('Delete this solve?'), findsOneWidget);
    await tester.tap(find.text('Keep'));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    expect(api.calls, isEmpty);
    expect(api.jobs, hasLength(1));

    await tester.tap(find.byKey(const Key('delete-done')));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    await tester.tap(find.byKey(const Key('confirm-delete')));
    await tester.pumpAndSettle(const Duration(milliseconds: 100));
    expect(api.calls, ['delete done']);
    expect(find.textContaining('No solves yet'), findsOneWidget);
  });

  testWidgets('opens a solution', (tester) async {
    final api = FakeApi()..jobs.add(sampleJob(id: 'done'));
    final opened = await pumpJobs(tester, api);

    await tester.tap(find.byKey(const Key('open-done')));
    await tester.pump();
    expect(opened.single.id, 'done');
  });

  testWidgets('picks up changes on its own', (tester) async {
    final api = FakeApi()
      ..jobs.add(
        sampleJob(id: 'run', state: JobState.JOB_STATE_RUNNING, iteration: 10),
      );
    await pumpJobs(tester, api);
    expect(find.textContaining('Iteration 10 of 500'), findsOneWidget);

    api.jobs.single
      ..iteration = 500
      ..state = JobState.JOB_STATE_DONE;
    await tester.pump(const Duration(seconds: 1));
    await tester.pump();
    expect(find.textContaining('Iteration 500 of 500'), findsOneWidget);
    expect(find.text('Done'), findsOneWidget);
  });

  testWidgets('shows when the server cannot be reached', (tester) async {
    final api = FakeApi()..listError = const GrpcError.unavailable();
    await pumpJobs(tester, api);
    expect(find.text('No server reachable at that address.'), findsOneWidget);

    // …and recovers when it is back.
    api
      ..listError = null
      ..jobs.add(sampleJob(id: 'done'));
    await tester.pump(const Duration(seconds: 1));
    await tester.pump();
    expect(find.byKey(const Key('jobs-error')), findsNothing);
    expect(find.text('Done'), findsOneWidget);
  });
}
