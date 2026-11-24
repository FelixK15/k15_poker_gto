import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../gen/gto/v1/server.pb.dart';
import 'server_connection.dart';

enum ConnectionMode { local, remote }

/// Local solving needs the bundled server, which only exists on desktop.
bool get localSolvingSupported =>
    defaultTargetPlatform == TargetPlatform.windows;

/// First screen: choose between solving locally and connecting to a server.
class ConnectScreen extends ConsumerStatefulWidget {
  const ConnectScreen({super.key});

  @override
  ConsumerState<ConnectScreen> createState() => _ConnectScreenState();
}

class _ConnectScreenState extends ConsumerState<ConnectScreen> {
  final _formKey = GlobalKey<FormState>();
  final _hostController = TextEditingController(text: '127.0.0.1');
  final _portController = TextEditingController(text: '50051');

  ConnectionMode _mode = ConnectionMode.remote;
  bool _connecting = false;
  String? _error;
  GetInfoResponse? _info;

  @override
  void dispose() {
    _hostController.dispose();
    _portController.dispose();
    super.dispose();
  }

  Future<void> _connect() async {
    if (!_formKey.currentState!.validate()) return;
    final host = _hostController.text.trim();
    final port = int.parse(_portController.text.trim());

    setState(() {
      _connecting = true;
      _error = null;
      _info = null;
    });
    try {
      final info = await ref.read(serverInfoFetcherProvider)(host, port);
      if (!mounted) return;
      setState(() => _info = info);
    } catch (error) {
      if (!mounted) return;
      setState(() => _error = describeConnectionError(error));
    } finally {
      if (mounted) setState(() => _connecting = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    return Scaffold(
      body: SafeArea(
        child: Center(
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: ConstrainedBox(
              constraints: const BoxConstraints(maxWidth: 440),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  Text('K15 GTO', style: theme.textTheme.headlineMedium),
                  const SizedBox(height: 4),
                  Text(
                    'Where should solves run?',
                    style: theme.textTheme.bodyLarge?.copyWith(
                      color: theme.colorScheme.onSurfaceVariant,
                    ),
                  ),
                  const SizedBox(height: 24),
                  SegmentedButton<ConnectionMode>(
                    segments: [
                      ButtonSegment(
                        value: ConnectionMode.local,
                        icon: const Icon(Icons.computer),
                        label: const Text('This computer'),
                        enabled: localSolvingSupported,
                      ),
                      const ButtonSegment(
                        value: ConnectionMode.remote,
                        icon: Icon(Icons.dns_outlined),
                        label: Text('A server'),
                      ),
                    ],
                    selected: {_mode},
                    onSelectionChanged: _connecting
                        ? null
                        : (selection) => setState(() {
                            _mode = selection.first;
                            _error = null;
                            _info = null;
                          }),
                  ),
                  const SizedBox(height: 24),
                  if (_mode == ConnectionMode.local)
                    const _LocalPanel()
                  else
                    _buildRemotePanel(),
                  if (_error != null) ...[
                    const SizedBox(height: 16),
                    Text(
                      _error!,
                      key: const Key('connect-error'),
                      style: TextStyle(color: theme.colorScheme.error),
                    ),
                  ],
                  if (_info != null) ...[
                    const SizedBox(height: 16),
                    ServerInfoCard(info: _info!),
                  ],
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildRemotePanel() {
    return Form(
      key: _formKey,
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.stretch,
        children: [
          Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Expanded(
                flex: 3,
                child: TextFormField(
                  key: const Key('host-field'),
                  controller: _hostController,
                  enabled: !_connecting,
                  autocorrect: false,
                  keyboardType: TextInputType.url,
                  decoration: const InputDecoration(
                    labelText: 'Host',
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) => (value == null || value.trim().isEmpty)
                      ? 'Enter a host name or IP'
                      : null,
                ),
              ),
              const SizedBox(width: 12),
              Expanded(
                flex: 2,
                child: TextFormField(
                  key: const Key('port-field'),
                  controller: _portController,
                  enabled: !_connecting,
                  keyboardType: TextInputType.number,
                  inputFormatters: [FilteringTextInputFormatter.digitsOnly],
                  decoration: const InputDecoration(
                    labelText: 'Port',
                    border: OutlineInputBorder(),
                  ),
                  validator: (value) {
                    final port = int.tryParse(value?.trim() ?? '');
                    return (port == null || port < 1 || port > 65535)
                        ? '1 to 65535'
                        : null;
                  },
                  onFieldSubmitted: (_) => _connect(),
                ),
              ),
            ],
          ),
          const SizedBox(height: 16),
          FilledButton(
            key: const Key('connect-button'),
            onPressed: _connecting ? null : _connect,
            child: _connecting
                ? const SizedBox.square(
                    dimension: 18,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Text('Connect'),
          ),
        ],
      ),
    );
  }
}

class _LocalPanel extends StatelessWidget {
  const _LocalPanel();

  @override
  Widget build(BuildContext context) {
    return const Card(
      child: Padding(
        padding: EdgeInsets.all(16),
        child: Text(
          'Solving on this computer is not available yet. '
          'Until then, start gto-server yourself and connect to it under "A server".',
        ),
      ),
    );
  }
}

/// What a server reported about itself.
class ServerInfoCard extends StatelessWidget {
  const ServerInfoCard({super.key, required this.info});

  final GetInfoResponse info;

  static String formatGigabytes(int bytes) =>
      '${(bytes / (1 << 30)).toStringAsFixed(1)} GB';

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final rows = <(String, String)>[
      ('Server', info.serverVersion),
      ('Engine', info.engineVersion),
      ('System', info.os),
      ('CPU threads', '${info.logicalCores}'),
      (
        'Memory',
        '${formatGigabytes(info.availableMemoryBytes.toInt())} free of '
            '${formatGigabytes(info.totalMemoryBytes.toInt())}',
      ),
    ];
    return Card(
      key: const Key('server-info'),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  Icons.check_circle,
                  color: theme.colorScheme.primary,
                  size: 20,
                ),
                const SizedBox(width: 8),
                Text('Connected', style: theme.textTheme.titleMedium),
              ],
            ),
            const SizedBox(height: 12),
            for (final (label, value) in rows)
              Padding(
                padding: const EdgeInsets.symmetric(vertical: 2),
                child: Row(
                  children: [
                    SizedBox(
                      width: 110,
                      child: Text(
                        label,
                        style: TextStyle(
                          color: theme.colorScheme.onSurfaceVariant,
                        ),
                      ),
                    ),
                    Expanded(child: Text(value)),
                  ],
                ),
              ),
          ],
        ),
      ),
    );
  }
}
