import 'package:fixnum/fixnum.dart';
import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:grpc/grpc.dart';
import 'package:k15_gto/src/app.dart';
import 'package:k15_gto/src/connection/connect_screen.dart';
import 'package:k15_gto/src/connection/server_connection.dart';
import 'package:k15_gto/src/gen/gto/v1/server.pb.dart';

Future<void> pumpApp(WidgetTester tester, ServerInfoFetcher fetcher) {
  return tester.pumpWidget(
    ProviderScope(
      overrides: [serverInfoFetcherProvider.overrideWithValue(fetcher)],
      child: const GtoApp(),
    ),
  );
}

void main() {
  testWidgets('shows server details after a successful connect', (
    tester,
  ) async {
    String? calledHost;
    int? calledPort;
    await pumpApp(tester, (host, port) async {
      calledHost = host;
      calledPort = port;
      return GetInfoResponse(
        serverVersion: '0.1.0',
        engineVersion: '0.1.0',
        apiMajor: 1,
        os: 'linux',
        logicalCores: 32,
        totalMemoryBytes: Int64(64) << 30,
        availableMemoryBytes: Int64(48) << 30,
      );
    });

    await tester.enterText(
      find.byKey(const Key('host-field')),
      'solver.example',
    );
    await tester.enterText(find.byKey(const Key('port-field')), '6000');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(calledHost, 'solver.example');
    expect(calledPort, 6000);
    expect(find.byKey(const Key('server-info')), findsOneWidget);
    expect(find.text('linux'), findsOneWidget);
    expect(find.text('32'), findsOneWidget);
    expect(find.text('48.0 GB free of 64.0 GB'), findsOneWidget);
    expect(find.byKey(const Key('connect-error')), findsNothing);
  });

  testWidgets('explains an unreachable server', (tester) async {
    await pumpApp(
      tester,
      (host, port) async => throw const GrpcError.unavailable(),
    );

    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(find.text('No server reachable at that address.'), findsOneWidget);
    expect(find.byKey(const Key('server-info')), findsNothing);
  });

  testWidgets('rejects an invalid port without calling the server', (
    tester,
  ) async {
    var calls = 0;
    await pumpApp(tester, (host, port) async {
      calls++;
      return GetInfoResponse();
    });

    await tester.enterText(find.byKey(const Key('port-field')), '70000');
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(calls, 0);
    expect(find.text('1 to 65535'), findsOneWidget);
  });

  test('describes an API version mismatch', () {
    expect(
      describeConnectionError(IncompatibleServerException(2)),
      'Server speaks API version 2, this app needs version 1.',
    );
  });

  test('formats memory in gigabytes', () {
    expect(ServerInfoCard.formatGigabytes(3 * (1 << 29)), '1.5 GB');
  });
}
