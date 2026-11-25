import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';

import '../api/gto_api.dart';
import '../common/format.dart';
import '../common/info_button.dart';
import '../common/playing_card.dart';
import '../range/preflop_library.dart';
import '../range/range_editor.dart';
import '../range/range_preview.dart';
import '../range/range_store.dart';
import 'board_picker.dart';
import 'spot_form.dart';
import 'tree_presets.dart';

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
    return _section(info: _preflopInfo, 'Preflop', [
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
        style: theme.textTheme.bodySmall?.copyWith(
          color: theme.colorScheme.onSurfaceVariant,
        ),
      ),
    ]);
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
            hint: oop ? 'QQ+, AKs, AQo:0.5, A5s-A2s' : null,
            info:
                'The hands the player who acts '
                '${oop ? 'first' : 'last'} can hold here, separated by '
                'commas. QQ+ is queens and better, A5s-A2s a run of suited '
                'hands, s and o mean suited and offsuit, and AQo:0.5 is a '
                'hand held half of the time. The grid next to it shows the '
                'range; tap it to edit the range by painting.',
          ),
        ),
        const SizedBox(width: 8),
        // The range at a glance; tapping it edits it on the full grid.
        RangePreview(
          key: Key(oop ? 'edit-oop-range' : 'edit-ip-range'),
          text: oop ? _input.oopRange : _input.ipRange,
          tooltip: 'Edit on the grid',
          onTap: () => _editRange(oop: oop),
        ),
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
                      _scenarioSection(),
                      _section('Spot', [
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
                              info:
                                  'The chips each player still has behind '
                                  'when the spot starts, in the same unit as '
                                  'the pot. Use the smaller stack if they '
                                  'differ.',
                            ),
                          ],
                        ),
                        const SizedBox(height: 12),
                        _rangeField(oop: true),
                        const SizedBox(height: 12),
                        _rangeField(oop: false),
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
                                    onPressed: () =>
                                        _filledIn(() => preset.applyTo(_input)),
                                    child: Text(preset.name),
                                  ),
                                ),
                            ],
                          ),
                          const SizedBox(height: 12),
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
