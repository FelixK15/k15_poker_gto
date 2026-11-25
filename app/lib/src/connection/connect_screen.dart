import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';

import '../api/gto_api.dart';
import '../common/info_button.dart';
import 'built_in_server.dart';
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
  // A server compiled into this build fills in the form.
  late final BuiltInServer? _builtIn = ref.read(builtInServerProvider);
  late final _hostController = TextEditingController(
    text: _builtIn?.host ?? '127.0.0.1',
  );
  late final _portController = TextEditingController(
    text: '${_builtIn?.port ?? 50051}',
  );
  late final _tokenController = TextEditingController(text: _builtIn?.token);
  final _certificateController = TextEditingController();
  late bool _tls = _builtIn?.tls ?? false;

  late final bool _localSupported =
      widget.localSupported ?? localSolvingSupported;
  late ConnectionMode _mode = _localSupported && _builtIn == null
      ? ConnectionMode.local
      : ConnectionMode.remote;

  /// The built-in certificate belongs to the built-in server only.
  bool get _usesBuiltInCertificate =>
      _builtIn?.certificate != null &&
      _hostController.text.trim() == _builtIn!.host;
  bool _connecting = false;
  String? _error;

  @override
  void dispose() {
    _hostController.dispose();
    _portController.dispose();
    _tokenController.dispose();
    _certificateController.dispose();
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
            tls: _tls,
            certificatePath: _certificateController.text.trim(),
            certificate: _usesBuiltInCertificate ? _builtIn!.certificate : null,
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
            : error is CertificateFileException
            ? error.toString()
            : describeError(error);
        _connecting = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    // Shown until the next attempt: why we are back on this screen.
    final lost = ref.watch(connectionLostProvider);
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
                  if (_error == null && lost != null) ...[
                    const SizedBox(height: 16),
                    Text(
                      lost,
                      key: const Key('connection-lost'),
                      style: TextStyle(color: theme.colorScheme.error),
                    ),
                  ],
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
                  decoration: InfoButton.decorate(
                    const InputDecoration(
                      labelText: 'Host',
                      border: OutlineInputBorder(),
                    ),
                    'The name or IP address of the computer the solver '
                    'server runs on, such as 192.168.1.20 or '
                    'solver.example.com.',
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
                  decoration: InfoButton.decorate(
                    const InputDecoration(
                      labelText: 'Port',
                      border: OutlineInputBorder(),
                    ),
                    'The port the server listens on, 1 to 65535. Whoever '
                    'started the server chose it.',
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
            decoration: InfoButton.decorate(
              const InputDecoration(
                labelText: 'Access token',
                helperText: 'Leave empty if the server has none.',
                border: OutlineInputBorder(),
              ),
              'The password the server was started with. It keeps others '
              'from using the server. Without TLS it is sent unencrypted.',
            ),
            onFieldSubmitted: (_) => _connect(),
          ),
          SwitchListTile(
            key: const Key('tls-switch'),
            contentPadding: EdgeInsets.zero,
            title: const Text('Encrypted connection (TLS)'),
            subtitle: const Text(
              'For servers started with a certificate. Use it whenever the '
              'server is not on your own network.',
            ),
            value: _tls,
            onChanged: _connecting ? null : (v) => setState(() => _tls = v),
          ),
          if (_tls)
            TextFormField(
              key: const Key('certificate-field'),
              controller: _certificateController,
              enabled: !_connecting,
              autocorrect: false,
              enableSuggestions: false,
              decoration: InputDecoration(
                labelText: 'Server certificate file',
                helperText: _usesBuiltInCertificate
                    ? 'Leave empty to use the certificate that comes with '
                          'this build.'
                    : 'Only for a self-signed certificate: the path of its '
                          '.pem file on this device.',
                helperMaxLines: 2,
                border: const OutlineInputBorder(),
              ),
            ),
        ],
      ),
    );
  }
}
