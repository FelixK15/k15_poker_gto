import 'dart:async';

import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import 'spot_form.dart';

/// Describe a spot, see what solving it takes, and send it off.
class SpotSetupScreen extends StatefulWidget {
  const SpotSetupScreen({
    super.key,
    required this.api,
    required this.onSubmitted,
    this.estimateDelay = const Duration(milliseconds: 400),
  });

  final GtoApi api;
  final ValueChanged<Job> onSubmitted;

  /// How long to wait after the last edit before asking the server for the
  /// size of the tree.
  final Duration estimateDelay;

  @override
  State<SpotSetupScreen> createState() => _SpotSetupScreenState();
}

class _SpotSetupScreenState extends State<SpotSetupScreen> {
  final _input = SpotInput();

  Timer? _debounce;
  int _estimateRequest = 0;
  bool _estimating = false;
  EstimateTreeResponse? _estimate;
  String? _problem;
  bool _submitting = false;

  @override
  void initState() {
    super.initState();
    _scheduleEstimate(Duration.zero);
  }

  @override
  void dispose() {
    _debounce?.cancel();
    super.dispose();
  }

  void _changed() {
    setState(() {
      // What is shown no longer describes what is typed.
      _estimate = null;
      _problem = null;
    });
    _scheduleEstimate(widget.estimateDelay);
  }

  void _scheduleEstimate(Duration delay) {
    _debounce?.cancel();
    _debounce = Timer(delay, _runEstimate);
  }

  Future<void> _runEstimate() async {
    final request = ++_estimateRequest;
    final parsed = _input.toSpot();
    if (parsed.spot == null) {
      setState(() {
        _problem = parsed.error;
        _estimating = false;
      });
      return;
    }
    setState(() => _estimating = true);
    try {
      final estimate = await widget.api.estimate(parsed.spot!);
      // An answer to an older version of the form is of no use.
      if (!mounted || request != _estimateRequest) return;
      setState(() {
        _estimate = estimate;
        _problem = null;
        _estimating = false;
      });
    } catch (error) {
      if (!mounted || request != _estimateRequest) return;
      setState(() {
        _estimate = null;
        _problem = describeError(error);
        _estimating = false;
      });
    }
  }

  Future<void> _submit() async {
    final spot = _input.toSpot();
    final settings = _input.toSettings();
    final problem = spot.error ?? settings.error;
    if (problem != null) {
      setState(() => _problem = problem);
      return;
    }
    setState(() => _submitting = true);
    try {
      final job = await widget.api.submit(
        spot.spot!,
        settings.settings!,
        '${spot.spot!.board} · pot ${_input.pot.trim()}',
      );
      if (!mounted) return;
      widget.onSubmitted(job);
    } catch (error) {
      if (!mounted) return;
      setState(() => _problem = describeError(error));
    } finally {
      if (mounted) setState(() => _submitting = false);
    }
  }

  @override
  Widget build(BuildContext context) {
    // Streets before the board's own are not played in this spot.
    final cards = countBoardCards(_input.board);
    final firstStreet = cards != null && cards >= 3 && cards <= 5
        ? cards - 3
        : 0;

    return Column(
      children: [
        Expanded(
          child: ListView(
            padding: const EdgeInsets.all(16),
            children: [
              Center(
                child: ConstrainedBox(
                  constraints: const BoxConstraints(maxWidth: 900),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _section('Spot', [
                        Wrap(
                          spacing: 12,
                          runSpacing: 12,
                          children: [
                            _field(
                              'board',
                              'Board',
                              _input.board,
                              (v) => _input.board = v,
                              width: 220,
                              hint: 'Ks Td 7h',
                            ),
                            _field(
                              'pot',
                              'Pot',
                              _input.pot,
                              (v) => _input.pot = v,
                              width: 120,
                            ),
                            _field(
                              'stack',
                              'Effective stack',
                              _input.stack,
                              (v) => _input.stack = v,
                              width: 160,
                            ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        _field(
                          'oop-range',
                          'Out of position range',
                          _input.oopRange,
                          (v) => _input.oopRange = v,
                          lines: 3,
                          hint: 'QQ+, AKs, AQo:0.5, A5s-A2s',
                        ),
                        const SizedBox(height: 12),
                        _field(
                          'ip-range',
                          'In position range',
                          _input.ipRange,
                          (v) => _input.ipRange = v,
                          lines: 3,
                        ),
                      ]),
                      _section('Bet sizes, in % of the pot', [
                        for (var street = firstStreet; street < 3; ++street)
                          _streetSizing(street),
                        const SizedBox(height: 4),
                        Wrap(
                          spacing: 12,
                          runSpacing: 12,
                          children: [
                            _field(
                              'raise-cap',
                              'Raises per street',
                              _input.raiseCap,
                              (v) => _input.raiseCap = v,
                              width: 160,
                            ),
                            _field(
                              'allin-threshold',
                              'All-in from % of stack',
                              _input.allinThreshold,
                              (v) => _input.allinThreshold = v,
                              width: 200,
                            ),
                          ],
                        ),
                      ]),
                      _section('Solve', [
                        Wrap(
                          spacing: 12,
                          runSpacing: 12,
                          children: [
                            _field(
                              'iterations',
                              'Iterations at most',
                              _input.maxIterations,
                              (v) => _input.maxIterations = v,
                              width: 180,
                            ),
                            _field(
                              'target',
                              'Stop at % of pot exploitable',
                              _input.targetExploitability,
                              (v) => _input.targetExploitability = v,
                              width: 240,
                            ),
                          ],
                        ),
                      ]),
                    ],
                  ),
                ),
              ),
            ],
          ),
        ),
        const Divider(height: 1),
        _footer(context),
      ],
    );
  }

  Widget _footer(BuildContext context) {
    final theme = Theme.of(context);
    final estimate = _estimate;

    Widget status;
    if (_problem != null) {
      status = Text(
        _problem!,
        key: const Key('spot-problem'),
        style: TextStyle(color: theme.colorScheme.error),
      );
    } else if (estimate == null) {
      status = Text(
        _estimating ? 'Measuring the tree…' : '',
        style: theme.textTheme.bodyMedium,
      );
    } else {
      final memory =
          estimate.strategyBytes.toInt() + estimate.treeBytes.toInt();
      final size =
          '${estimate.tooLarge ? 'More than ' : ''}'
          '${formatCount(estimate.nodes.toInt())} nodes · '
          '${estimate.oopHands} vs ${estimate.ipHands} hands · '
          '${estimate.tooLarge ? 'at least ' : ''}${formatBytes(memory)}';
      final String? verdict = estimate.canSolve
          ? null
          : estimate.tooLarge
          ? 'Too large for this server. Use fewer bet sizes or a lower raise cap.'
          : 'Needs more memory than this server allows '
                '(${formatBytes(estimate.memoryBudgetBytes.toInt())}).';
      status = Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisSize: MainAxisSize.min,
        children: [
          Text(size, key: const Key('spot-estimate')),
          if (verdict != null)
            Text(
              verdict,
              key: const Key('spot-verdict'),
              style: TextStyle(color: theme.colorScheme.error),
            ),
        ],
      );
    }

    final canSolve = estimate != null && estimate.canSolve && !_submitting;
    return Padding(
      padding: const EdgeInsets.fromLTRB(16, 12, 16, 12),
      child: Row(
        children: [
          Expanded(child: status),
          const SizedBox(width: 16),
          FilledButton.icon(
            key: const Key('solve-button'),
            onPressed: canSolve ? _submit : null,
            icon: _submitting
                ? const SizedBox.square(
                    dimension: 16,
                    child: CircularProgressIndicator(strokeWidth: 2),
                  )
                : const Icon(Icons.play_arrow),
            label: const Text('Solve'),
          ),
        ],
      ),
    );
  }

  Widget _section(String title, List<Widget> children) {
    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(title, style: Theme.of(context).textTheme.titleMedium),
            const SizedBox(height: 12),
            ...children,
          ],
        ),
      ),
    );
  }

  Widget _streetSizing(int street) {
    const streets = ['Flop', 'Turn', 'River'];
    const players = ['OOP', 'IP'];
    final key = streets[street].toLowerCase();
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(streets[street], style: Theme.of(context).textTheme.labelLarge),
          const SizedBox(height: 8),
          for (var player = 0; player < 2; ++player)
            Padding(
              padding: const EdgeInsets.only(bottom: 8),
              child: Wrap(
                spacing: 12,
                runSpacing: 8,
                crossAxisAlignment: WrapCrossAlignment.center,
                children: [
                  SizedBox(width: 36, child: Text(players[player])),
                  _field(
                    '$key-${players[player].toLowerCase()}-bet',
                    'Bet',
                    _input.sizing[street][player].bet,
                    (v) => _input.sizing[street][player].bet = v,
                    width: 170,
                    hint: '33, 75',
                  ),
                  _field(
                    '$key-${players[player].toLowerCase()}-raise',
                    'Raise',
                    _input.sizing[street][player].raise,
                    (v) => _input.sizing[street][player].raise = v,
                    width: 170,
                    hint: '60',
                  ),
                  Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Checkbox(
                        value: _input.sizing[street][player].allin,
                        onChanged: (v) {
                          _input.sizing[street][player].allin = v ?? false;
                          _changed();
                        },
                      ),
                      const Text('All-in'),
                    ],
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _field(
    String key,
    String label,
    String value,
    ValueChanged<String> onChanged, {
    double? width,
    int lines = 1,
    String? hint,
  }) {
    final field = TextFormField(
      key: Key('field-$key'),
      initialValue: value,
      minLines: lines,
      maxLines: lines,
      autocorrect: false,
      enableSuggestions: false,
      decoration: InputDecoration(
        labelText: label,
        hintText: hint,
        border: const OutlineInputBorder(),
        isDense: true,
      ),
      onChanged: (text) {
        onChanged(text);
        _changed();
      },
    );
    return width == null ? field : SizedBox(width: width, child: field);
  }
}
