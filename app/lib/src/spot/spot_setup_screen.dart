import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/info_button.dart';
import '../common/playing_card.dart';
import '../common/solve_labels.dart';
import '../jobs/jobs_screen.dart' show jobStateLabel;
import '../range/omaha_syntax.dart';
import '../range/preflop_library.dart';
import '../range/preflop_source.dart';
import '../range/range_editor.dart';
import '../range/range_preview.dart';
import '../range/range_store.dart';
import 'board_picker.dart';
import 'preflop_form.dart';
import 'spot_form.dart';
import 'tree_presets.dart';

/// What the setup screen describes: a postflop spot, or a preflop solve.
enum SetupMode {
  postflop('Postflop spot'),
  preflop('Preflop solve');

  const SetupMode(this.label);

  final String label;
}

/// Describe a spot, see what solving it takes, and send it off.
class SpotSetupScreen extends StatefulWidget {
  const SpotSetupScreen({
    super.key,
    required this.api,
    required this.onSubmitted,
    this.store,
    this.estimateDelay = const Duration(milliseconds: 400),
  });

  final GtoApi api;
  final ValueChanged<Job> onSubmitted;

  /// How long to wait after the last edit before asking the server for the
  /// size of the tree.
  final Duration estimateDelay;

  /// Where the range editor keeps saved ranges; memory only if not given.
  final RangeStore? store;

  @override
  State<SpotSetupScreen> createState() => _SpotSetupScreenState();
}

class _SpotSetupScreenState extends State<SpotSetupScreen> {
  final _input = SpotInput();
  final _preflop = PreflopInput();
  SetupMode _mode = SetupMode.postflop;
  late final RangeStore _store = widget.store ?? MemoryRangeStore();

  /// Bumped whenever the form is filled in from code, so that the text
  /// fields start over with the new values.
  int _formVersion = 0;

  // The preflop line the ranges are taken from.
  TableFormat _format = TableFormat.sixMax;
  int _stack = 100;
  PotType _potType = PotType.singleRaised;
  String _opener = 'BTN';
  String _defender = 'BB';
  String? _scenarioDescription;

  /// The ranges, pot and stack as the defaults or the last preflop line left
  /// them. Anything else in those fields is the user's own.
  late List<String> _filledSpot;

  /// The chosen line was not filled in, to keep what the user entered.
  bool _keptOwnSpot = false;

  // Ranges from a preflop solve on the server instead of the library.
  PreflopSource _source = PreflopSource.library;
  List<Job>? _preflopJobs;
  Job? _sourceJob;
  List<PreflopScenario>? _scenarios;
  String? _scenarioPath;
  String? _sourceProblem;
  bool _loadingSource = false;

  Timer? _debounce;
  int _estimateRequest = 0;
  bool _estimating = false;
  EstimateTreeResponse? _estimate;
  String? _problem;
  bool _submitting = false;

  /// The board is typed rather than shown as cards.
  bool _typingBoard = false;

  @override
  void initState() {
    super.initState();
    _filledSpot = _spotFields();
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

  /// The spot the form describes in its current mode, or what is wrong.
  ({Spot? spot, String? error}) _parsedSpot() {
    final parsed = _mode == SetupMode.preflop
        ? _preflop.toSpot()
        : _input.toSpot();
    final spot = parsed.spot;
    if (spot == null) return parsed;
    final needsNew =
        spot.hasPreflop() ||
        spot.variant != Variant.VARIANT_HOLDEM ||
        spot.players.isNotEmpty ||
        spot.engine != Engine.ENGINE_AUTO;
    if (needsNew && !supportsVariants(widget.api.info)) {
      return (
        spot: null,
        error:
            'This server (API 1.${widget.api.info.apiMinor}) cannot solve '
            'PLO, multiway or preflop spots. Update it to use them.',
      );
    }
    return parsed;
  }

  ({SolveSettings? settings, String? error}) _parsedSettings() =>
      _mode == SetupMode.preflop ? _preflop.toSettings() : _input.toSettings();

  String _jobName(Spot spot) {
    if (_mode == SetupMode.preflop) return _preflop.jobName;
    final prefix = [
      if (_input.isOmaha) 'PLO',
      if (_input.isMultiway) '${_input.playerCount}-way',
    ].join(' ');
    return '${prefix.isEmpty ? '' : '$prefix · '}${spot.board} · pot ${_input.pot.trim()}';
  }

  Future<void> _runEstimate() async {
    final request = ++_estimateRequest;
    final parsed = _parsedSpot();
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
    final spot = _parsedSpot();
    final settings = _parsedSettings();
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
        _jobName(spot.spot!),
      );
      if (!mounted) return;
      if (job.reused) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              'Job "${job.name}" already solves this spot, so no new one was queued.',
            ),
          ),
        );
      }
      widget.onSubmitted(job);
    } catch (error) {
      if (!mounted) return;
      setState(() => _problem = describeError(error));
    } finally {
      if (mounted) setState(() => _submitting = false);
    }
  }

  /// Something the form shows was set from code rather than typed.
  void _filledIn(void Function() change) {
    setState(() {
      change();
      ++_formVersion;
    });
    _changed();
  }

  List<String> _spotFields() => [
    _input.oopRange.trim(),
    _input.ipRange.trim(),
    _input.pot.trim(),
    _input.stack.trim(),
  ];

  /// Fills in the chosen preflop line. Ranges, a pot or a stack the user
  /// entered are only replaced when [overwrite] is set.
  void _applyScenario({bool overwrite = false}) {
    // Changing the opener or the table can leave the other seat impossible.
    final openers = PreflopLibrary.openers(_format);
    if (!openers.contains(_opener)) _opener = openers.last;
    final defenders = PreflopLibrary.defenders(_format, _opener);
    if (!defenders.contains(_defender)) _defender = defenders.last;

    final scenario = PreflopLibrary.scenario(
      format: _format,
      stack: _stack,
      potType: _potType,
      opener: _opener,
      defender: _defender,
    );
    if (!overwrite && !listEquals(_spotFields(), _filledSpot)) {
      setState(() {
        _keptOwnSpot = true;
        // The seats on offer may have changed.
        ++_formVersion;
      });
      return;
    }
    _filledIn(() {
      _input.setPlayerCount(2);
      _input
        ..oopRange = scenario.oopRange.toString()
        ..ipRange = scenario.ipRange.toString()
        ..pot = formatChips(scenario.pot)
        ..stack = formatChips(scenario.effectiveStack);
      _scenarioDescription =
          '${scenario.description}. '
          'OOP is ${scenario.oopPosition}, IP is ${scenario.ipPosition}.';
      _filledSpot = _spotFields();
      _keptOwnSpot = false;
    });
  }

  /// Switches where the line picker takes its ranges from. The solved
  /// source lists the server's preflop solves.
  Future<void> _setSource(PreflopSource source) async {
    setState(() {
      _source = source;
      _sourceProblem = null;
    });
    if (source == PreflopSource.solved) await _loadPreflopJobs();
  }

  Future<void> _loadPreflopJobs() async {
    setState(() => _loadingSource = true);
    try {
      final jobs = preflopJobs(await widget.api.listJobs());
      if (!mounted) return;
      setState(() {
        _preflopJobs = jobs;
        if (_sourceJob != null &&
            !jobs.any((job) => job.id == _sourceJob!.id)) {
          _sourceJob = null;
          _scenarios = null;
          _scenarioPath = null;
        }
        _loadingSource = false;
      });
      if (_sourceJob == null && jobs.isNotEmpty) {
        await _pickSourceJob(jobs.first);
      }
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _sourceProblem = describeError(error);
        _loadingSource = false;
      });
    }
  }

  Future<void> _pickSourceJob(Job job) async {
    setState(() {
      _sourceJob = job;
      _scenarios = null;
      _scenarioPath = null;
      _sourceProblem = null;
      _loadingSource = true;
    });
    try {
      final scenarios = await widget.api.listPreflopScenarios(job.id);
      if (!mounted || _sourceJob?.id != job.id) return;
      setState(() {
        _scenarios = scenarios;
        _loadingSource = false;
      });
    } catch (error) {
      if (!mounted || _sourceJob?.id != job.id) return;
      setState(() {
        _sourceProblem = describeError(error);
        _loadingSource = false;
      });
    }
  }

  /// Fills in both ranges, the pot and the stacks from a solved line.
  Future<void> _pickSolvedScenario(PreflopScenario scenario) async {
    final job = _sourceJob;
    if (job == null) return;
    setState(() {
      _scenarioPath = scenario.path;
      _sourceProblem = null;
      _loadingSource = true;
    });
    try {
      final ranges = await widget.api.getPreflopRanges(job.id, scenario.path);
      if (!mounted || _scenarioPath != scenario.path) return;
      _filledIn(() {
        final seats = applyPreflopRanges(_input, ranges);
        _scenarioDescription =
            '${scenario.name.isEmpty ? scenario.path : scenario.name}. $seats '
            'From the solve "${jobTitle(job)}" at iteration '
            '${ranges.iteration}${ranges.complete ? '' : ' (the line does not reach the flop)'}.';
        _filledSpot = _spotFields();
        _keptOwnSpot = false;
        _loadingSource = false;
      });
    } catch (error) {
      if (!mounted) return;
      setState(() {
        _sourceProblem = describeError(error);
        _loadingSource = false;
      });
    }
  }

  Future<void> _editRange({required bool oop}) async {
    final text = await showRangeEditor(
      context,
      title: oop ? 'Out of position range' : 'In position range',
      initialText: oop ? _input.oopRange : _input.ipRange,
      store: _store,
    );
    if (text == null || !mounted) return;
    _filledIn(() => oop ? _input.oopRange = text : _input.ipRange = text);
  }

  Future<void> _pickBoard() async {
    final board = await showBoardPicker(context, _input.board);
    if (board == null || !mounted) return;
    _filledIn(() => _input.board = board);
  }

  Widget _scenarioSection() {
    final openers = PreflopLibrary.openers(_format);
    final defenders = PreflopLibrary.defenders(_format, _opener);

    Widget choice<T>(
      String key,
      String label,
      T value,
      List<T> values,
      String Function(T) text,
      void Function(T) set, {
      double width = 150,
    }) {
      return SizedBox(
        width: width,
        child: DropdownButtonFormField<T>(
          key: Key('scenario-$key'),
          initialValue: values.contains(value) ? value : values.last,
          isDense: true,
          // Let the text shrink to the box instead of overflowing it.
          isExpanded: true,
          decoration: InputDecoration(
            labelText: label,
            border: const OutlineInputBorder(),
            isDense: true,
          ),
          items: [
            for (final v in values)
              DropdownMenuItem(value: v, child: Text(text(v))),
          ],
          onChanged: (v) {
            if (v == null) return;
            set(v);
            _applyScenario();
          },
        ),
      );
    }

    final theme = Theme.of(context);
    final sourcePicker = SegmentedButton<PreflopSource>(
      key: const Key('preflop-source'),
      showSelectedIcon: false,
      segments: [
        for (final source in PreflopSource.values)
          ButtonSegment(value: source, label: Text(source.label)),
      ],
      selected: {_source},
      onSelectionChanged: (selection) => _setSource(selection.first),
    );
    if (_source == PreflopSource.solved) {
      return _section(info: _preflopInfo, 'Preflop', [
        sourcePicker,
        const SizedBox(height: 12),
        ..._solvedSourceChildren(theme),
      ]);
    }
    return _section(info: _preflopInfo, 'Preflop', [
      sourcePicker,
      const SizedBox(height: 12),
      Wrap(
        spacing: 12,
        runSpacing: 12,
        children: [
          choice<TableFormat>(
            'format',
            'Table',
            _format,
            TableFormat.values,
            (f) => f.label,
            (f) => _format = f,
            width: 120,
          ),
          choice<int>(
            'stack',
            'Stacks',
            _stack,
            stackDepths,
            (s) => '$s BB',
            (s) => _stack = s,
            width: 120,
          ),
          choice<PotType>(
            'pot-type',
            'Pot',
            _potType,
            PotType.values,
            (t) => t.label,
            (t) => _potType = t,
            width: 200,
          ),
          choice<String>(
            'opener',
            'Opens',
            _opener,
            openers,
            (p) => p,
            (p) => _opener = p,
            width: 120,
          ),
          choice<String>(
            'defender',
            'Against',
            _defender,
            defenders,
            (p) => p,
            (p) => _defender = p,
            width: 120,
          ),
        ],
      ),
      const SizedBox(height: 8),
      if (_keptOwnSpot)
        Wrap(
          spacing: 12,
          runSpacing: 4,
          crossAxisAlignment: WrapCrossAlignment.center,
          children: [
            Text(
              'The ranges, pot and stack you entered were kept.',
              key: const Key('scenario-description'),
              style: theme.textTheme.bodyMedium,
            ),
            OutlinedButton(
              key: const Key('scenario-overwrite'),
              onPressed: () => _applyScenario(overwrite: true),
              child: const Text('Replace them with this line'),
            ),
          ],
        )
      else
        Text(
          _scenarioDescription ??
              'Pick a line to fill in both ranges, the pot and the stacks.',
          key: const Key('scenario-description'),
          style: theme.textTheme.bodyMedium,
        ),
      Text(
        'These ranges are rough starting points built from simple rules, '
        'not solver output. Adjust them to your game.',
        key: const Key('preflop-source-label'),
        style: theme.textTheme.bodySmall?.copyWith(
          color: theme.colorScheme.onSurfaceVariant,
        ),
      ),
    ]);
  }

  /// The solved source: a preflop job, then one of its lines.
  List<Widget> _solvedSourceChildren(ThemeData theme) {
    final jobs = _preflopJobs;
    final muted = theme.textTheme.bodySmall?.copyWith(
      color: theme.colorScheme.onSurfaceVariant,
    );
    final scenarios = _scenarios;
    return [
      if (jobs != null && jobs.isEmpty)
        Text(
          'No preflop solve on this server yet. Start one with "Preflop '
          'solve" above, or use the built-in rules.',
          key: const Key('no-preflop-jobs'),
          style: theme.textTheme.bodyMedium,
        )
      else
        Wrap(
          spacing: 12,
          runSpacing: 12,
          crossAxisAlignment: WrapCrossAlignment.center,
          children: [
            SizedBox(
              width: 280,
              child: DropdownButtonFormField<String>(
                key: const Key('preflop-job'),
                initialValue: _sourceJob?.id,
                isDense: true,
                isExpanded: true,
                decoration: const InputDecoration(
                  labelText: 'Preflop solve',
                  border: OutlineInputBorder(),
                  isDense: true,
                ),
                items: [
                  for (final job in jobs ?? const <Job>[])
                    DropdownMenuItem(
                      value: job.id,
                      child: Text(
                        '${jobTitle(job)}'
                        '${job.state == JobState.JOB_STATE_DONE ? '' : ' (${jobStateLabel(job.state).toLowerCase()})'}',
                        overflow: TextOverflow.ellipsis,
                      ),
                    ),
                ],
                onChanged: (id) {
                  final job = jobs?.firstWhere((j) => j.id == id);
                  if (job != null) _pickSourceJob(job);
                },
              ),
            ),
            if (scenarios != null)
              SizedBox(
                width: 380,
                child: DropdownButtonFormField<String>(
                  key: const Key('preflop-line'),
                  initialValue: _scenarioPath,
                  isDense: true,
                  isExpanded: true,
                  decoration: const InputDecoration(
                    labelText: 'Line',
                    border: OutlineInputBorder(),
                    isDense: true,
                  ),
                  items: [
                    for (final scenario in scenarios)
                      DropdownMenuItem(
                        value: scenario.path,
                        child: Text(
                          scenarioLabel(scenario),
                          overflow: TextOverflow.ellipsis,
                        ),
                      ),
                  ],
                  onChanged: (path) {
                    final scenario = scenarios.firstWhere(
                      (s) => s.path == path,
                    );
                    _pickSolvedScenario(scenario);
                  },
                ),
              ),
            IconButton(
              tooltip: 'Look for preflop solves again',
              icon: const Icon(Icons.refresh),
              onPressed: _loadingSource ? null : _loadPreflopJobs,
            ),
            if (_loadingSource)
              const SizedBox.square(
                dimension: 16,
                child: CircularProgressIndicator(strokeWidth: 2),
              ),
          ],
        ),
      const SizedBox(height: 8),
      if (_sourceProblem != null)
        Text(
          _sourceProblem!,
          key: const Key('preflop-source-problem'),
          style: TextStyle(color: theme.colorScheme.error),
        )
      else if (_scenarioPath != null && _scenarioDescription != null)
        Text(
          _scenarioDescription!,
          key: const Key('scenario-description'),
          style: theme.textTheme.bodyMedium,
        )
      else if (scenarios != null)
        Text(
          'Pick a line to fill in the ranges, the pot and the stacks.',
          style: theme.textTheme.bodyMedium,
        ),
      Text(
        'Ranges from the server\'s preflop solver. With 3+ players at the '
        'table they are an estimate without an equilibrium guarantee.',
        key: const Key('preflop-source-label'),
        style: muted,
      ),
    ];
  }

  static const _boardInfo =
      'The cards on the table. Three cards solve from the flop on, four from '
      'the turn, five the river alone. Tap the cards to pick them, or the '
      'keyboard to type them, each as a rank (2-9, T, J, Q, K, A) and a suit '
      '(c, d, h, s).';

  /// The board as cards; tapping them opens the picker. The keyboard button
  /// switches to typing them instead.
  Widget _boardField() {
    if (_typingBoard) {
      return Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          _field(
            'board',
            'Board',
            _input.board,
            (v) => _input.board = v,
            width: 220,
            hint: 'Ks Td 7h',
            info: _boardInfo,
          ),
          IconButton(
            key: const Key('board-show-cards'),
            tooltip: 'Show as cards',
            icon: const Icon(Icons.style_outlined),
            onPressed: () => setState(() => _typingBoard = false),
          ),
        ],
      );
    }

    final theme = Theme.of(context);
    final cards = parseBoard(_input.board);
    return SizedBox(
      width: 260,
      child: InkWell(
        key: const Key('pick-board'),
        borderRadius: BorderRadius.circular(6),
        onTap: _pickBoard,
        child: InputDecorator(
          decoration: InputDecoration(
            labelText: 'Board',
            border: const OutlineInputBorder(),
            isDense: true,
            contentPadding: const EdgeInsets.fromLTRB(12, 10, 4, 8),
            suffixIcon: Row(
              mainAxisSize: MainAxisSize.min,
              children: [
                InfoButton(_boardInfo),
                IconButton(
                  key: const Key('board-type'),
                  tooltip: 'Type the cards',
                  icon: const Icon(Icons.keyboard_outlined, size: 20),
                  onPressed: () => setState(() => _typingBoard = true),
                ),
              ],
            ),
          ),
          child: cards.isEmpty
              ? Text(
                  _input.board.trim().isEmpty
                      ? 'Pick the cards'
                      : '${_input.board.trim()} · tap to fix',
                  style: theme.textTheme.bodyMedium?.copyWith(
                    color: _input.board.trim().isEmpty
                        ? theme.colorScheme.onSurfaceVariant
                        : theme.colorScheme.error,
                  ),
                )
              : Align(
                  alignment: Alignment.centerLeft,
                  child: CardRow(
                    key: const Key('board-cards'),
                    cards,
                    width: 22,
                  ),
                ),
        ),
      ),
    );
  }

  Widget _rangeField({required bool oop}) {
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Expanded(
          child: _field(
            oop ? 'oop-range' : 'ip-range',
            oop ? 'Out of position range' : 'In position range',
            oop ? _input.oopRange : _input.ipRange,
            (v) => oop ? _input.oopRange = v : _input.ipRange = v,
            lines: 3,
            hint: _input.isOmaha
                ? 'AAxx, KQJTds, top15%'
                : oop
                ? 'QQ+, AKs, AQo:0.5, A5s-A2s'
                : null,
            info: _input.isOmaha
                ? omahaRangeHelp
                : 'The hands the player who acts '
                      '${oop ? 'first' : 'last'} can hold here, separated by '
                      'commas. QQ+ is queens and better, A5s-A2s a run of suited '
                      'hands, s and o mean suited and offsuit, and AQo:0.5 is a '
                      'hand held half of the time. The grid next to it shows the '
                      'range; tap it to edit the range by painting.',
          ),
        ),
        // The range at a glance; tapping it edits it on the full grid.
        // PLO has no grid.
        if (!_input.isOmaha) ...[
          const SizedBox(width: 8),
          RangePreview(
            key: Key(oop ? 'edit-oop-range' : 'edit-ip-range'),
            text: oop ? _input.oopRange : _input.ipRange,
            tooltip: 'Edit on the grid',
            onTap: () => _editRange(oop: oop),
          ),
        ],
      ],
    );
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
                    // A new key makes every field take its value afresh.
                    key: ValueKey(_formVersion),
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      _modePicker(),
                      if (_mode == SetupMode.preflop)
                        ..._preflopSections()
                      else ...[
                        if (!_input.isOmaha) _scenarioSection(),
                        _section('Spot', info: _gameInfo, [
                          _gameRow(),
                          const SizedBox(height: 12),
                          Wrap(
                            spacing: 12,
                            runSpacing: 12,
                            children: [
                              _boardField(),
                              _field(
                                'pot',
                                'Pot',
                                _input.pot,
                                (v) => _input.pot = v,
                                width: 120,
                                info:
                                    'The chips in the middle when the spot '
                                    'starts. Any unit works as long as the '
                                    'stack uses the same one; big blinds are '
                                    'usual.',
                              ),
                              _field(
                                'stack',
                                'Effective stack',
                                _input.stack,
                                (v) => _input.stack = v,
                                width: 160,
                                info: _input.isMultiway
                                    ? 'The chips each player has behind when '
                                          'the spot starts, unless the player\'s '
                                          'own stack below says otherwise.'
                                    : 'The chips each player still has behind '
                                          'when the spot starts, in the same unit '
                                          'as the pot. Use the smaller stack if '
                                          'they differ.',
                              ),
                            ],
                          ),
                          const SizedBox(height: 12),
                          if (_input.isMultiway)
                            for (var i = 0; i < _input.playerCount; ++i) ...[
                              if (i > 0) const SizedBox(height: 12),
                              _playerRow(i),
                            ]
                          else ...[
                            _rangeField(oop: true),
                            const SizedBox(height: 12),
                            _rangeField(oop: false),
                          ],
                        ]),
                        _section(
                          info: _sizingInfo,
                          'Bet sizes, in % of the pot',
                          [
                            Wrap(
                              spacing: 8,
                              runSpacing: 8,
                              crossAxisAlignment: WrapCrossAlignment.center,
                              children: [
                                const Text('Fill in:'),
                                for (final preset in treePresets)
                                  Tooltip(
                                    message: preset.description,
                                    child: OutlinedButton(
                                      key: Key('preset-${preset.name}'),
                                      onPressed: () => _filledIn(
                                        () => preset.applyTo(_input),
                                      ),
                                      child: Text(preset.name),
                                    ),
                                  ),
                              ],
                            ),
                            const SizedBox(height: 12),
                            for (var street = firstStreet; street < 3; ++street)
                              _input.isMultiway
                                  ? _multiwayStreetSizing(street)
                                  : _streetSizing(street),
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
                                  width: 180,
                                  info:
                                      'The most raises allowed on one street '
                                      'after the first bet, 0 to 20. A lower '
                                      'number makes a smaller tree that solves '
                                      'faster.',
                                ),
                                _field(
                                  'allin-threshold',
                                  'All-in from % of stack',
                                  _input.allinThreshold,
                                  (v) => _input.allinThreshold = v,
                                  width: 220,
                                  info:
                                      'A bet or raise that would put in at least '
                                      'this share of the remaining stack is made '
                                      'all-in instead. 0 turns the rule off.',
                                ),
                              ],
                            ),
                          ],
                        ),
                        _section('Solve', info: _engineInfo, [
                          Wrap(
                            spacing: 12,
                            runSpacing: 12,
                            children: [
                              _engineChoice(),
                              if (_input.engine != Engine.ENGINE_MONTE_CARLO)
                                _field(
                                  'iterations',
                                  'Iterations at most',
                                  _input.maxIterations,
                                  (v) => _input.maxIterations = v,
                                  width: 200,
                                  info:
                                      'The solver stops after this many rounds '
                                      'even if the target next to it is not '
                                      'reached. More rounds come closer to the '
                                      'optimal strategy and take longer.',
                                ),
                              _field(
                                'target',
                                'Stop at % of pot exploitable',
                                _input.targetExploitability,
                                (v) => _input.targetExploitability = v,
                                width: 260,
                                info:
                                    'How much a perfect opponent could still '
                                    'win against the solution, in % of the '
                                    'pot. Solving stops once it is this low. '
                                    'Smaller is more exact and takes longer.',
                              ),
                              if (_monteCarloLikely)
                                _mcIterationsField(
                                  _input.mcIterations,
                                  (v) => _input.mcIterations = v,
                                ),
                              _maxSecondsField(
                                _input.maxSeconds,
                                (v) => _input.maxSeconds = v,
                              ),
                            ],
                          ),
                        ]),
                      ],
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

  /// Postflop spot or preflop solve.
  Widget _modePicker() {
    return Padding(
      padding: const EdgeInsets.only(bottom: 16),
      child: Align(
        alignment: Alignment.centerLeft,
        child: SegmentedButton<SetupMode>(
          key: const Key('setup-mode'),
          showSelectedIcon: false,
          segments: [
            for (final mode in SetupMode.values)
              ButtonSegment(value: mode, label: Text(mode.label)),
          ],
          selected: {_mode},
          onSelectionChanged: (selection) {
            setState(() => _mode = selection.first);
            _changed();
          },
        ),
      ),
    );
  }

  static const _gameInfo =
      'Game: No-Limit Hold\'em or Pot-Limit Omaha with four cards. PLO '
      'ranges are typed in their own syntax; the help next to the range '
      'field explains it.\n\n'
      'Pot-limit: no bet larger than the pot. On by default for PLO.\n\n'
      'Players: two for a heads-up spot, up to nine for a multiway pot. '
      'Spots with three or more players are solved by the Monte Carlo '
      'engine, which gives no equilibrium guarantee.';

  /// Game, pot-limit and number of players.
  Widget _gameRow() {
    return Wrap(
      spacing: 16,
      runSpacing: 12,
      crossAxisAlignment: WrapCrossAlignment.center,
      children: [
        SegmentedButton<Variant>(
          key: const Key('game'),
          showSelectedIcon: false,
          segments: const [
            ButtonSegment(value: Variant.VARIANT_HOLDEM, label: Text('NLH')),
            ButtonSegment(value: Variant.VARIANT_OMAHA4, label: Text('PLO4')),
          ],
          selected: {_input.variant},
          onSelectionChanged: (selection) =>
              _filledIn(() => _input.setVariant(selection.first)),
        ),
        Row(
          mainAxisSize: MainAxisSize.min,
          children: [
            Checkbox(
              key: const Key('pot-limit'),
              value: _input.potLimit,
              onChanged: (v) {
                _input.potLimit = v ?? false;
                _changed();
              },
            ),
            const Text('Pot-limit'),
          ],
        ),
        SizedBox(
          width: 150,
          child: DropdownButtonFormField<int>(
            key: const Key('player-count'),
            initialValue: _input.playerCount,
            isDense: true,
            isExpanded: true,
            decoration: const InputDecoration(
              labelText: 'Players',
              border: OutlineInputBorder(),
              isDense: true,
            ),
            items: [
              for (var n = 2; n <= 9; ++n)
                DropdownMenuItem(
                  value: n,
                  child: Text(n == 2 ? '2 (heads-up)' : '$n'),
                ),
            ],
            onChanged: (n) {
              if (n == null) return;
              _filledIn(() => _input.setPlayerCount(n));
            },
          ),
        ),
      ],
    );
  }

  /// One seat of a multiway spot: position, own stack and range.
  Widget _playerRow(int index) {
    final player = _input.players[index];
    final omaha = _input.isOmaha;
    return Row(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        SizedBox(
          width: 110,
          child: Column(
            children: [
              _field(
                'player-$index-position',
                'Position',
                player.position,
                (v) => player.position = v,
                hint: 'P${index + 1}',
              ),
              const SizedBox(height: 8),
              _field(
                'player-$index-stack',
                'Stack',
                player.stack,
                (v) => player.stack = v,
                hint: _input.stack.trim(),
              ),
            ],
          ),
        ),
        const SizedBox(width: 8),
        Expanded(
          child: _field(
            'player-$index-range',
            index == 0
                ? 'Range · acts first'
                : index == _input.playerCount - 1
                ? 'Range · acts last'
                : 'Range',
            player.range,
            (v) => player.range = v,
            lines: 3,
            hint: omaha ? 'AAxx, KQJTds, top15%' : 'QQ+, AKs, AQo:0.5',
            info: omaha
                ? omahaRangeHelp
                : 'The hands this player can hold here, in the usual '
                      'text form. The grid next to it shows the range; tap it '
                      'to edit the range by painting.',
          ),
        ),
        if (!omaha) ...[
          const SizedBox(width: 8),
          RangePreview(
            key: Key('edit-player-$index-range'),
            text: player.range,
            tooltip: 'Edit on the grid',
            onTap: () => _editPlayerRange(index),
          ),
        ],
      ],
    );
  }

  Future<void> _editPlayerRange(int index) async {
    final player = _input.players[index];
    final name = player.position.trim().isEmpty
        ? 'Player ${index + 1}'
        : player.position.trim();
    final text = await showRangeEditor(
      context,
      title: '$name range',
      initialText: player.range,
      store: _store,
    );
    if (text == null || !mounted) return;
    _filledIn(() => player.range = text);
  }

  /// One row of sizes per street for every player of a multiway spot.
  Widget _multiwayStreetSizing(int street) {
    const streets = ['Flop', 'Turn', 'River'];
    final key = streets[street].toLowerCase();
    final sizing = _input.multiwaySizing[street];
    return Padding(
      padding: const EdgeInsets.only(bottom: 12),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Text(streets[street], style: Theme.of(context).textTheme.labelLarge),
          const SizedBox(height: 8),
          Wrap(
            spacing: 12,
            runSpacing: 8,
            crossAxisAlignment: WrapCrossAlignment.center,
            children: [
              const SizedBox(width: 36, child: Text('All')),
              _field(
                '$key-all-bet',
                'Bet',
                sizing.bet,
                (v) => sizing.bet = v,
                width: 170,
                hint: '33, 75',
              ),
              _field(
                '$key-all-raise',
                'Raise',
                sizing.raise,
                (v) => sizing.raise = v,
                width: 170,
                hint: '60',
              ),
              Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  Checkbox(
                    value: sizing.allin,
                    onChanged: (v) {
                      sizing.allin = v ?? false;
                      _changed();
                    },
                  ),
                  const Text('All-in'),
                ],
              ),
            ],
          ),
        ],
      ),
    );
  }

  static const _engineInfo =
      'Engine: Auto lets the server choose: the exact solver for heads-up '
      'spots that fit, Monte Carlo for everything else.\n\n'
      'Exact solves every hand of a heads-up spot and measures how far the '
      'strategy is from an equilibrium exactly.\n\n'
      'Monte Carlo samples deals and groups similar hands. It handles '
      'multiway pots, preflop and large PLO spots; its exploitability is an '
      'estimate, and with three or more players there is no equilibrium '
      'guarantee.\n\n'
      'Monte Carlo iterations: how many deals to sample, in millions. Empty '
      'uses the server\'s default (100 million).\n\n'
      'Time limit: stop after this many seconds of solving. Empty for none.';

  /// Whether the spot will likely run on the Monte Carlo engine, so its
  /// budget is worth asking for.
  bool get _monteCarloLikely =>
      _input.engine == Engine.ENGINE_MONTE_CARLO ||
      (_input.engine == Engine.ENGINE_AUTO &&
          (_input.isMultiway ||
              _estimate?.engine == Engine.ENGINE_MONTE_CARLO));

  Widget _engineChoice() {
    return SizedBox(
      width: 180,
      child: DropdownButtonFormField<Engine>(
        key: const Key('engine'),
        initialValue: _input.engine,
        isDense: true,
        isExpanded: true,
        decoration: const InputDecoration(
          labelText: 'Engine',
          border: OutlineInputBorder(),
          isDense: true,
        ),
        items: [
          for (final engine in const [
            Engine.ENGINE_AUTO,
            Engine.ENGINE_EXACT,
            Engine.ENGINE_MONTE_CARLO,
          ])
            DropdownMenuItem(value: engine, child: Text(engineLabel(engine))),
        ],
        onChanged: (engine) {
          if (engine == null) return;
          _filledIn(() => _input.engine = engine);
        },
      ),
    );
  }

  Widget _mcIterationsField(String value, ValueChanged<String> set) => _field(
    'mc-iterations',
    'Monte Carlo iterations (M)',
    value,
    set,
    width: 240,
    hint: '100',
    info:
        'Deals the Monte Carlo engine samples, in millions. More come '
        'closer to a stable strategy and take longer. Empty: the server\'s '
        'default of 100 million.',
  );

  Widget _maxSecondsField(String value, ValueChanged<String> set) => _field(
    'max-seconds',
    'Time limit (s)',
    value,
    set,
    width: 170,
    hint: 'none',
    info:
        'Stop after this many seconds of solving, whatever else is left. '
        'Empty for no limit.',
  );

  static const _preflopSolveInfo =
      'A preflop solve plays the hand from the blinds with every hand dealt. '
      'Its lines can then fill in the ranges of a postflop spot: pick '
      '"Solved preflop job" in the Preflop section of a postflop spot.\n\n'
      'Table: heads-up, 6-max or 9-max. Stacks: what everyone starts with, '
      'in big blinds.\n\n'
      'Preflop solves run on the Monte Carlo engine. With three or more '
      'players its result is an estimate without an equilibrium guarantee.';

  /// The form of a preflop solve.
  List<Widget> _preflopSections() {
    final p = _preflop;
    final theme = Theme.of(context);
    Widget choice<T>(
      String key,
      String label,
      T value,
      List<T> values,
      String Function(T) text,
      void Function(T) set,
    ) {
      return SizedBox(
        width: 140,
        child: DropdownButtonFormField<T>(
          key: Key('preflop-$key'),
          initialValue: value,
          isDense: true,
          isExpanded: true,
          decoration: InputDecoration(
            labelText: label,
            border: const OutlineInputBorder(),
            isDense: true,
          ),
          items: [
            for (final v in values)
              DropdownMenuItem(value: v, child: Text(text(v))),
          ],
          onChanged: (v) {
            if (v == null) return;
            setState(() => set(v));
            _changed();
          },
        ),
      );
    }

    Widget toggle(
      String key,
      String label,
      bool value,
      void Function(bool) set,
    ) {
      return Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Checkbox(
            key: Key('preflop-$key'),
            value: value,
            onChanged: (v) {
              setState(() => set(v ?? false));
              _changed();
            },
          ),
          Text(label),
        ],
      );
    }

    return [
      _section('Table', info: _preflopSolveInfo, [
        Wrap(
          spacing: 12,
          runSpacing: 12,
          children: [
            choice<int>(
              'table',
              'Table',
              p.players,
              preflopTables,
              tableLabel,
              (v) => p.players = v,
            ),
            choice<int>(
              'stack',
              'Stacks',
              p.stackBb,
              preflopStacks,
              (v) => '$v BB',
              (v) => p.stackBb = v,
            ),
          ],
        ),
        const SizedBox(height: 8),
        Text(
          'Seats: ${p.positions.join(', ')}',
          key: const Key('preflop-positions'),
          style: theme.textTheme.bodyMedium,
        ),
      ]),
      _section('Sizes', [
        Wrap(
          spacing: 12,
          runSpacing: 12,
          children: [
            _field(
              'preflop-open',
              'Open to (BB)',
              p.openSizes,
              (v) => p.openSizes = v,
              width: 150,
              hint: '2.5',
              info:
                  'Raise-to amount of a first raise, in big blinds, for every '
                  'seat but the small blind. Several sizes separated by '
                  'commas make a larger tree.',
            ),
            _field(
              'preflop-sb-open',
              'SB open to (BB)',
              p.sbOpenSizes,
              (v) => p.sbOpenSizes = v,
              width: 160,
              hint: '3',
            ),
            _field(
              'preflop-3bet-ip',
              '3-bet IP (× open)',
              p.threeBetIp,
              (v) => p.threeBetIp = v,
              width: 170,
              hint: '3',
            ),
            _field(
              'preflop-3bet-oop',
              '3-bet OOP (× open)',
              p.threeBetOop,
              (v) => p.threeBetOop = v,
              width: 170,
              hint: '4',
            ),
            _field(
              'preflop-4bet',
              '4-bet (× 3-bet)',
              p.fourBet,
              (v) => p.fourBet = v,
              width: 160,
              hint: '2.3',
            ),
          ],
        ),
        const SizedBox(height: 8),
        Wrap(
          spacing: 16,
          children: [
            toggle(
              'limp',
              'Allow limping',
              p.allowLimp,
              (v) => p.allowLimp = v,
            ),
            toggle(
              'cold-call',
              'Allow cold calls',
              p.allowColdCalls,
              (v) => p.allowColdCalls = v,
            ),
          ],
        ),
      ]),
      _section('Rake and solve', [
        Wrap(
          spacing: 12,
          runSpacing: 12,
          children: [
            _field(
              'preflop-rake',
              'Rake %',
              p.rakePct,
              (v) => p.rakePct = v,
              width: 110,
            ),
            _field(
              'preflop-rake-cap',
              'Rake cap (BB)',
              p.rakeCap,
              (v) => p.rakeCap = v,
              width: 140,
              info:
                  'The most rake taken from one pot, in big blinds. 0: no '
                  'cap.',
            ),
            _mcIterationsField(p.mcIterations, (v) => p.mcIterations = v),
            _maxSecondsField(p.maxSeconds, (v) => p.maxSeconds = v),
          ],
        ),
      ]),
    ];
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
      final hands = estimate.playerHands.length > 2
          ? estimate.playerHands.join(' vs ')
          : '${estimate.oopHands} vs ${estimate.ipHands}';
      final size =
          '${estimate.tooLarge ? 'More than ' : ''}'
          '${formatCount(estimate.nodes.toInt())} nodes · '
          '$hands hands · '
          '${estimate.tooLarge ? 'at least ' : ''}${formatBytes(memory)}';
      final engine = estimate.engine == Engine.ENGINE_AUTO
          ? null
          : [
              '${engineLabel(estimate.engine)} engine',
              if (estimate.positions.isNotEmpty)
                estimate.positions.join(', ')
              else if (estimate.playerCount > 2)
                '${estimate.playerCount} players',
              if (estimate.engine == Engine.ENGINE_MONTE_CARLO)
                '${formatCount(estimate.infoSets.toInt())} info sets'
              else if (estimate.strategyEntries > 0)
                '${formatCount(estimate.strategyEntries.toInt())} strategy entries',
              exploitabilityKindLabel(estimate.exploitabilityKind),
            ].join(' · ');
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
          if (engine != null)
            Tooltip(
              message: exploitabilityMeaning(estimate.exploitabilityKind),
              child: Text(
                engine,
                key: const Key('spot-engine'),
                style: theme.textTheme.bodySmall?.copyWith(
                  color: theme.colorScheme.onSurfaceVariant,
                ),
              ),
            ),
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

  static const _preflopInfo =
      'Picks how the hand got to the flop and fills in both ranges, the pot '
      'and the effective stack from it.\n\n'
      'Table: how many seats, which decides the positions and how wide '
      'they play.\n\n'
      'Stacks: what both players started with, in big blinds.\n\n'
      'Pot: one raise and a call, a called 3-bet or a called 4-bet.\n\n'
      'Opens: the seat that raises first.\n\n'
      'Against: the seat that plays against the opener.\n\n'
      'Once you have typed your own ranges, pot or stack, a new choice here '
      'keeps them; a button then offers to replace them.';

  static const _sizingInfo =
      'The bets and raises each player may choose from on each street.\n\n'
      'Bet: sizes in % of the pot, separated by commas; 50 is half the '
      'pot.\n\n'
      'Raise: sizes in % of the pot after calling; 100 is a pot-size '
      'raise.\n\n'
      'All-in: also offer all-in as a bet and as a raise.\n\n'
      'Every size added makes the tree larger and the solve slower.';

  Widget _section(String title, List<Widget> children, {String? info}) {
    return Card(
      margin: const EdgeInsets.only(bottom: 16),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Flexible(
                  child: Text(
                    title,
                    style: Theme.of(context).textTheme.titleMedium,
                  ),
                ),
                if (info != null) InfoButton(info),
              ],
            ),
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
    String? info,
  }) {
    final field = TextFormField(
      key: Key('field-$key'),
      initialValue: value,
      minLines: lines,
      maxLines: lines,
      autocorrect: false,
      enableSuggestions: false,
      decoration: InfoButton.decorate(
        InputDecoration(
          labelText: label,
          hintText: hint,
          border: const OutlineInputBorder(),
          isDense: true,
        ),
        info,
      ),
      onChanged: (text) {
        onChanged(text);
        _changed();
      },
    );
    return width == null ? field : SizedBox(width: width, child: field);
  }
}
