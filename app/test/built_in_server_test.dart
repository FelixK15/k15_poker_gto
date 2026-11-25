import 'dart:ui' show Size;

import 'package:flutter/foundation.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/app.dart';
import 'package:k15_gto/src/connection/built_in_server.dart';
import 'package:k15_gto/src/connection/server_connection.dart';

import 'fake_api.dart';

const _builtIn = BuiltInServer(
  host: 'mine.example',
  port: 6000,
  token: 'baked-in',
  tls: true,
  certificate: [1, 2, 3],
);

Future<List<RemoteTarget>> pumpApp(
  WidgetTester tester,
  BuiltInServer? builtIn,
) async {
  tester.view.physicalSize = const Size(900, 1300);
  tester.view.devicePixelRatio = 1;
  addTearDown(tester.view.reset);
  final targets = <RemoteTarget>[];
  await tester.pumpWidget(
    ProviderScope(
      overrides: [
        builtInServerProvider.overrideWithValue(builtIn),
        remoteConnectorProvider.overrideWithValue((target) async {
          targets.add(target);
          return FakeApi();
        }),
        localConnectorProvider.overrideWithValue(
          () async => fail('local connector not expected'),
        ),
      ],
      child: const GtoApp(),
    ),
  );
  return targets;
}

void main() {
  test('an ordinary build has no server compiled in', () {
    expect(BuiltInServer.fromEnvironment(), isNull);
  });

  testWidgets('a build with a server connects with one tap', (tester) async {
    final targets = await pumpApp(tester, _builtIn);

    // The form opens on the server, already filled in.
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    final target = targets.single;
    expect(target.host, 'mine.example');
    expect(target.port, 6000);
    expect(target.token, 'baked-in');
    expect(target.tls, isTrue);
    expect(target.certificatePath, isEmpty);
    expect(target.certificate, [1, 2, 3]);
  });

  testWidgets('the built-in certificate is not offered to another host', (
    tester,
  ) async {
    final targets = await pumpApp(tester, _builtIn);

    await tester.enterText(
      find.byKey(const Key('host-field')),
      'other.example',
    );
    await tester.tap(find.byKey(const Key('connect-button')));
    await tester.pumpAndSettle();

    expect(targets.single.host, 'other.example');
    expect(targets.single.certificate, isNull);
  });
}
