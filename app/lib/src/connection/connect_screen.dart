import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import 'local_server.dart';
import 'server_connection.dart';

enum ConnectionMode { local, remote }

/// First screen: choose between solving on this computer and connecting to a
/// server.
class ConnectScreen extends ConsumerStatefulWidget {
  const ConnectScreen({super.key, this.localSupported});

  /// Whether to offer solving on this computer. Defaults to what the
  /// platform allows; tests set it.
  final bool? localSupported;

  @override
  ConsumerState<ConnectScreen> createState() => _ConnectScreenState();
}

class _ConnectScreenState extends ConsumerState<ConnectScreen> {
  final _formKey = GlobalKey<FormState>();
  final _hostController = TextEditingController(text: '127.0.0.1');
  final _portController = TextEditingController(text: '50051');
  final _tokenController = TextEditingController();

  late final bool _localSupported =
      widget.localSupported ?? localSolvingSupported;
  late ConnectionMode _mode = _localSupported
      ? ConnectionMode.local
      : ConnectionMode.remote;
  bool _connecting = false;
  String? _error;

  @override
  void dispose() {
    _hostController.dispose();
    _portController.dispose();
    _tokenController.dispose();
    super.dispose();
  }

  Future<void> _connect() async {
    if (_mode == ConnectionMode.remote && !_formKey.currentState!.validate()) {
      return;
    }
    setState(() {
      _connecting = true;
      _error = null;
    });
    try {
      final GtoApi api;
      if (_mode == ConnectionMode.local) {
        api = await ref.read(localConnectorProvider)();
      } else {
        api = await ref.read(remoteConnectorProvider)(
          RemoteTarget(
            host: _hostController.text.trim(),
            port: int.parse(_portController.text.trim()),
            token: _tokenController.text.trim(),
          ),
        );
      }
      // The app switches to the home screen when this is set.
      ref.read(connectionProvider.notifier).connected(api);
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _error = error is LocalServerException
            ? error.message
            : describeError(error);
        _connecting = false;
      });
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
                        enabled: _localSupported,
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
                          }),
                  ),
                  const SizedBox(height: 24),
                  if (_mode == ConnectionMode.local)
                    Text(
                      'Solves use this computer\'s processor and memory. '
                      'They stop when you close the app and can be resumed later.',
                      style: theme.textTheme.bodyMedium?.copyWith(
                        color: theme.colorScheme.onSurfaceVariant,
                      ),
                    )
                  else
                    _buildRemoteFields(),
                  const SizedBox(height: 16),
                  FilledButton(
                    key: const Key('connect-button'),
                    onPressed: _connecting ? null : _connect,
                    child: _connecting
                        ? const SizedBox.square(
                            dimension: 18,
                            child: CircularProgressIndicator(strokeWidth: 2),
                          )
                        : Text(
                            _mode == ConnectionMode.local ? 'Start' : 'Connect',
                          ),
                  ),
                  if (_error != null) ...[
                    const SizedBox(height: 16),
                    Text(
                      _error!,
                      key: const Key('connect-error'),
                      style: TextStyle(color: theme.colorScheme.error),
                    ),
                  ],
                ],
              ),
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildRemoteFields() {
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
                ),
              ),
            ],
          ),
          const SizedBox(height: 12),
          TextFormField(
            key: const Key('token-field'),
            controller: _tokenController,
            enabled: !_connecting,
            obscureText: true,
            autocorrect: false,
            enableSuggestions: false,
            decoration: const InputDecoration(
              labelText: 'Access token',
              helperText: 'Leave empty if the server has none.',
              border: OutlineInputBorder(),
            ),
            onFieldSubmitted: (_) => _connect(),
          ),
        ],
      ),
    );
  }
}
