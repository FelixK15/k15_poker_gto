import 'dart:async';
import 'dart:ui' show Size;

import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/app.dart';
import 'package:k15_gto/src/connection/local_server.dart';
import 'package:k15_gto/src/connection/server_connection.dart';

import 'fake_api.dart';

Future<void> pumpApp(
  WidgetTester tester, {
  RemoteConnector? remote,
  LocalConnector? local,
}) {
  // Tall enough that the connect button stays on screen with every field shown.
  tester.view.physicalSize = const Size(900, 1300);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);
  return tester.pumpWidget(
    ProviderScope(
      overrides: [
        remoteConnectorProvider.overrideWithValue(
          remote ?? (_) async => fail('remote connector not expected'),
        ),
        localConnectorProvider.overrideWithValue(
          local ?? () async => fail('local connector not expected'),
        ),
      ],
      child: const GtoApp(),
    ),
  );
}

void main() {
  testWidgets('connects to a server with host, port and token', (tester) async {
    RemoteTarget? target;
    final api = FakeApi();
    await pumpApp(
      tester,
      remote: (t) async {
        target = t;
        return api;
      },
    );

    await tester.enterText(
      find.byKey(const Key('host-field')),
      'solver.example',
    );
    await tester.enterText(find.byKey(const Key('port-field')), '6000');
    await tester.enterText(find.byKey(const Key('token-field')), ' s3cret ');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(target!.host, 'solver.example');
    expect(target!.port, 6000);
    expect(target!.token, 's3cret');
    // The app moved on to the home screen and names the server.
    expect(find.byKey(const Key('server-label')), findsOneWidget);
    expect(find.textContaining('solver.example:50051'), findsOneWidget);
    expect(find.textContaining('16 threads'), findsOneWidget);
    expect(find.textContaining('32.0 GB'), findsOneWidget);
  });

  testWidgets('disconnecting returns to the connect screen', (tester) async {
    final api = FakeApi();
    await pumpApp(tester, remote: (_) async => api);
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(api.closed, isFalse);

    await tester.tap(find.byKey(const Key('disconnect-button')));
    await tester.pumpAndSettle();

    expect(api.closed, isTrue);
    expect(find.byKey(const Key('connect-button')), findsOneWidget);
  });

  testWidgets('explains why connecting failed', (tester) async {
    Object error = const GrpcError.unavailable();
    await pumpApp(tester, remote: (_) async => throw error);

    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.text('No server reachable at that address.'), findsOneWidget);

    error = const GrpcError.unauthenticated(
      'this server needs an access token',
    );
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.text('This server needs an access token.'), findsOneWidget);

    error = const GrpcError.unauthenticated('the access token is wrong');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.text('The access token is wrong.'), findsOneWidget);

    error = IncompatibleServerException(2);
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(
      find.text('Server speaks API version 2, this app needs version 1.'),
      findsOneWidget,
    );
    // Still on the connect screen, ready for another try.
    expect(find.byKey(const Key('host-field')), findsOneWidget);
  });

  testWidgets('rejects an invalid port without calling the server', (
    tester,
  ) async {
    await pumpApp(tester);
    await tester.enterText(find.byKey(const Key('port-field')), '70000');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.text('1 to 65535'), findsOneWidget);
  });

  testWidgets('offers only a server where local solving is unavailable', (
    tester,
  ) async {
    // Widget tests run as Android by default.
    await pumpApp(tester);
    expect(find.byKey(const Key('host-field')), findsOneWidget);
    expect(find.text('Connect'), findsOneWidget);
  });

  group('on Windows', () {
    setUp(() => debugDefaultTargetPlatformOverride = TargetPlatform.windows);
    tearDown(() => debugDefaultTargetPlatformOverride = null);

    testWidgets('starts the solver on this computer by default', (
      tester,
    ) async {
      var started = 0;
      final api = FakeApi(label: 'This computer', isLocal: true);
      await pumpApp(
        tester,
        local: () async {
          ++started;
          return api;
        },
      );
      expect(find.byKey(const Key('host-field')), findsNothing);

      await tester.tap(find.text('Start'));
      await tester.pumpAndSettle();

      expect(started, 1);
      expect(find.textContaining('This computer'), findsOneWidget);
      debugDefaultTargetPlatformOverride = null;
    });

    testWidgets('says so when the solver cannot be started', (tester) async {
      await pumpApp(
        tester,
        local: () async => throw LocalServerException(
          'The solver (gto-server) was not found next to the app.',
        ),
      );
      await tester.tap(find.text('Start'));
      await tester.pumpAndSettle();

      expect(
        find.text('The solver (gto-server) was not found next to the app.'),
        findsOneWidget,
      );
      // The other option is still there.
      await tester.tap(find.text('A server'));
      await tester.pumpAndSettle();
      expect(find.byKey(const Key('host-field')), findsOneWidget);
      expect(find.byKey(const Key('connect-error')), findsNothing);
      debugDefaultTargetPlatformOverride = null;
    });
  });

  testWidgets('asks for an encrypted connection with a certificate file', (
    tester,
  ) async {
    RemoteTarget? target;
    await pumpApp(
      tester,
      remote: (t) async {
        target = t;
        return FakeApi();
      },
    );
    expect(find.byKey(const Key('certificate-field')), findsNothing);

    await tester.tap(find.byKey(const Key('tls-switch')));
    await tester.pumpAndSettle();
    await tester.enterText(
      find.byKey(const Key('certificate-field')),
      ' C:/certs/server.pem ',
    );
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(target!.tls, isTrue);
    expect(target!.certificatePath, 'C:/certs/server.pem');
  });

  testWidgets('connects in the clear unless asked otherwise', (tester) async {
    RemoteTarget? target;
    await pumpApp(
      tester,
      remote: (t) async {
        target = t;
        return FakeApi();
      },
    );
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(target!.tls, isFalse);
    expect(target!.certificatePath, isEmpty);
  });

  testWidgets('explains certificate problems', (tester) async {
    Object error = CertificateFileException('C:/nope.pem');
    await pumpApp(tester, remote: (_) async => throw error);
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(
      find.text('The certificate file "C:/nope.pem" cannot be read.'),
      findsOneWidget,
    );

    error = const GrpcError.unavailable('Error connecting: HandshakeException');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(
      find.text('The server\'s certificate was not accepted.'),
      findsOneWidget,
    );
  });

  testWidgets('returns to the connect screen when the solver stops', (
    tester,
  ) async {
    final stopped = Completer<String>();
    final api = FakeApi(label: 'This computer', isLocal: true)
      ..lost = stopped.future;
    await pumpApp(tester, remote: (_) async => api);
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('server-label')), findsOneWidget);

    stopped.complete('The solver on this computer stopped unexpectedly.');
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('connect-button')), findsOneWidget);
    expect(
      find.text('The solver on this computer stopped unexpectedly.'),
      findsOneWidget,
    );

    // Connecting again clears the notice.
    final again = FakeApi();
    await pumpApp(tester, remote: (_) async => again);
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();
    expect(find.byKey(const Key('connection-lost')), findsNothing);
  });

  test('describes server-worded errors as sentences', () {
    expect(
      describeError(
        const GrpcError.invalidArgument('ip_range: cannot read "x"'),
      ),
      'Ip_range: cannot read "x".',
    );
    expect(
      describeError(const GrpcError.resourceExhausted('the solve needs 9 GB.')),
      'The solve needs 9 GB.',
    );
  });
}
