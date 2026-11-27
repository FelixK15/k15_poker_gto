import 'package:fixnum/fixnum.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:k15_gto/src/api/gto_api.dart';
import 'package:k15_gto/src/common/solve_labels.dart';
import 'package:k15_gto/src/range/omaha_syntax.dart';
import 'package:k15_gto/src/range/preflop_source.dart';
import 'package:k15_gto/src/solution/matrix_model.dart';
import 'package:k15_gto/src/solution/node_view.dart';
import 'package:k15_gto/src/solution/plo_hands.dart';
import 'package:k15_gto/src/spot/preflop_form.dart';
import 'package:k15_gto/src/spot/spot_form.dart';

void main() {
  group('exploitabilityLabel', () {
    test('keeps the plain wording for exact and locked games', () {
      expect(
        exploitabilityLabel(0.42, ExploitabilityKind.EXPLOITABILITY_KIND_EXACT),
        '0.42% of pot exploitable',
      );
      expect(
        exploitabilityLabel(
          0.42,
          ExploitabilityKind.EXPLOITABILITY_KIND_LOCKED_GAME,
        ),
        '0.42% of pot exploitable',
      );
      // Jobs from servers before API 1.4 say nothing; they were exact.
      expect(
        exploitabilityLabel(
          0.42,
          ExploitabilityKind.EXPLOITABILITY_KIND_UNSPECIFIED,
        ),
        '0.42% of pot exploitable',
      );
    });

    test('marks Monte Carlo numbers as estimates', () {
      expect(
        exploitabilityLabel(
          1.234,
          ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER,
        ),
        '≈ 1.23% of pot exploitable (estimate)',
      );
      expect(
        exploitabilityLabel(
          0.8,
          ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY,
        ),
        '≈ 0.80% of pot (estimate; no equilibrium guarantee with 3+ players)',
      );
    });
  });

  group('job labels', () {
    test('lists every player\'s EV by position', () {
      final job = Job(
        positions: ['SB', 'BB', 'BTN'],
        evs: [-0.31, -0.625, 1.1],
        evEstimated: true,
      );
      expect(evLine(job), 'EV ≈ SB -0.31, BB -0.63, BTN 1.10');
    });

    test('falls back to OOP and IP for older jobs', () {
      final job = Job(oopEv: 4.21, ipEv: 5.79);
      expect(evLine(job), 'EV: OOP 4.21, IP 5.79');
      expect(jobPositions(job), ['OOP', 'IP']);
    });

    test('names the kind of job', () {
      expect(
        jobKind(
          Job(
            spot: Spot(variant: Variant.VARIANT_OMAHA4),
            engine: Engine.ENGINE_EXACT,
          ),
        ),
        'PLO4 · Exact',
      );
      expect(
        jobKind(
          Job(
            spot: Spot(preflop: PreflopSettings(players: 6)),
            engine: Engine.ENGINE_MONTE_CARLO,
          ),
        ),
        'Preflop · 6-max · Monte Carlo',
      );
      expect(jobKind(Job(positions: ['BB', 'CO', 'BTN'])), 'NLH · 3 players');
    });

    test('formats sample counts', () {
      expect(formatSamples(950), '950');
      expect(formatSamples(812000), '812k');
      expect(formatSamples(12345678), '12.3M');
      expect(formatSamples(250000000), '250M');
    });
  });

  group('checkOmahaRange', () {
    test('accepts the documented forms', () {
      for (final text in [
        '*',
        'all',
        'AsKsQhJh',
        'AAKK, AKQJ',
        'AAxx, KQJx',
        'AAxxds, KQJTss, 9876r',
        'top15%',
        'top12.5%:0.5, AAxx:1',
        'AAxx, KKxxds:0.75, top10%',
      ]) {
        expect(checkOmahaRange(text), isNull, reason: text);
      }
    });

    test('points at the item that cannot be read', () {
      expect(checkOmahaRange(''), contains('empty'));
      expect(checkOmahaRange('AAxx, AKs'), contains('"AKs"'));
      expect(checkOmahaRange('AAxx:2'), contains('weight'));
      expect(checkOmahaRange('top0%'), contains('top'));
      expect(checkOmahaRange('AsAsKhKd'), contains('same card'));
      expect(checkOmahaRange('AAxxqs'), isNotNull);
    });
  });

  group('SpotInput with variants and players', () {
    test('a heads-up Hold\'em spot stays a v1 request', () {
      final spot = SpotInput().toSpot().spot!;
      expect(spot.hasVariant(), isFalse);
      expect(spot.hasPotLimit(), isFalse);
      expect(spot.hasEngine(), isFalse);
      expect(spot.players, isEmpty);
    });

    test('PLO keeps its own ranges and turns pot-limit on', () {
      final input = SpotInput();
      final holdem = input.oopRange;
      input.setVariant(Variant.VARIANT_OMAHA4);
      expect(input.potLimit, isTrue);
      expect(input.oopRange, 'top5%');
      input.oopRange = 'AAxx, KKxxds';
      final spot = input.toSpot().spot!;
      expect(spot.variant, Variant.VARIANT_OMAHA4);
      // Pot-limit is PLO's default, so it is left to the server.
      expect(spot.hasPotLimit(), isFalse);
      expect(spot.oopRange, 'AAxx, KKxxds');

      input.setVariant(Variant.VARIANT_HOLDEM);
      expect(input.oopRange, holdem);
      expect(input.potLimit, isFalse);
      input.setVariant(Variant.VARIANT_OMAHA4);
      expect(input.oopRange, 'AAxx, KKxxds');
    });

    test('PLO range typos are caught before the server', () {
      final input = SpotInput()..setVariant(Variant.VARIANT_OMAHA4);
      input.ipRange = 'AAxx, AKs';
      expect(input.toSpot().error, contains('In position range'));
    });

    test('multiway spots list the players and share the sizes', () {
      final input = SpotInput()..setPlayerCount(3);
      expect([for (final p in input.players) p.position], ['BB', 'CO', 'BTN']);
      input.players[2].stack = '150';
      input.engine = Engine.ENGINE_MONTE_CARLO;
      input.mcIterations = '20';
      input.maxSeconds = '60';
      final spot = input.toSpot().spot!;
      expect(spot.players, hasLength(3));
      expect(spot.players[0].position, 'BB');
      expect(spot.players[0].stack, 0);
      expect(spot.players[2].stack, 150);
      expect(spot.oopRange, isEmpty);
      expect(spot.tree.flop.all.betPct, [33]);
      expect(spot.tree.flop.hasOop(), isFalse);
      expect(spot.engine, Engine.ENGINE_MONTE_CARLO);
      final settings = input.toSettings().settings!;
      expect(settings.mcIterations, Int64(20000000));
      expect(settings.maxSeconds, 60);
    });

    test('player count changes keep names the user chose', () {
      final input = SpotInput()..setPlayerCount(3);
      input.players[1].position = 'HERO';
      input.setPlayerCount(4);
      expect(
        [for (final p in input.players) p.position],
        ['BB', 'HERO', 'CO', 'BTN'],
      );
      input.players[0].position = 'BTN';
      expect(input.toSpot().error, contains('Two players are both BTN'));
    });

    test('seat positions follow the server\'s table order', () {
      expect(seatPositions(2), ['BB', 'BTN']);
      expect(seatPositions(3), ['SB', 'BB', 'BTN']);
      expect(seatPositions(6), ['SB', 'BB', 'UTG', 'HJ', 'CO', 'BTN']);
      expect(seatPositions(7), ['SB', 'BB', 'UTG', 'LJ', 'HJ', 'CO', 'BTN']);
      expect(seatPositions(9), hasLength(9));
      expect(multiwayPositions(3), ['BB', 'CO', 'BTN']);
    });
  });

  group('PreflopInput', () {
    test('builds a preflop spot', () {
      final input = PreflopInput()
        ..players = 9
        ..stackBb = 200
        ..openSizes = '2.2, 3'
        ..allowLimp = true
        ..rakePct = '5'
        ..rakeCap = '3';
      final spot = input.toSpot().spot!;
      expect(spot.hasPreflop(), isTrue);
      expect(spot.preflop.players, 9);
      expect(spot.preflop.stackBb, 200);
      expect(spot.preflop.openSizesBb, [2.2, 3]);
      expect(spot.preflop.sbOpenSizesBb, [3]);
      expect(spot.preflop.threeBetIpMultiplier, 3);
      expect(spot.preflop.threeBetOopMultiplier, 4);
      expect(spot.preflop.fourBetMultiplier, 2.3);
      expect(spot.preflop.allowLimp, isTrue);
      expect(spot.preflop.allowColdCalls, isTrue);
      expect(spot.rakePct, 5);
      expect(spot.rakeCap, 3);
      expect(input.jobName, 'Preflop · 9-max · 200 BB');
    });

    test('names what cannot be read', () {
      expect(
        (PreflopInput()..openSizes = '1').toSpot().error,
        contains('Open sizes'),
      );
      expect(
        (PreflopInput()..threeBetOop = 'x').toSpot().error,
        contains('3-bet'),
      );
      expect((PreflopInput()..rakePct = '-1').toSpot().error, contains('rake'));
      expect(
        (PreflopInput()..maxSeconds = 'soon').toSettings().error,
        contains('time limit'),
      );
    });
  });

  group('preflop source', () {
    Job preflopJob(String id, JobState state, {bool solution = true}) => Job(
      id: id,
      state: state,
      hasSolution: solution,
      spot: Spot(preflop: PreflopSettings(players: 6, stackBb: 100)),
    );

    test('offers preflop solves that can be browsed, finished first', () {
      final jobs = [
        preflopJob('running', JobState.JOB_STATE_RUNNING),
        Job(id: 'postflop', hasSolution: true, state: JobState.JOB_STATE_DONE),
        preflopJob('queued', JobState.JOB_STATE_QUEUED, solution: false),
        preflopJob('failed', JobState.JOB_STATE_FAILED),
        preflopJob('done', JobState.JOB_STATE_DONE),
      ];
      expect([for (final j in preflopJobs(jobs)) j.id], ['done', 'running']);
    });

    test('labels a scenario', () {
      expect(
        scenarioLabel(
          PreflopScenario(name: 'BTN open, BB call', pot: 5.5, frequency: 0.12),
        ),
        'BTN open, BB call · pot 5.5 · 12% of deals',
      );
    });

    test('fills a heads-up line from the ready spot', () {
      final input = SpotInput()..setVariant(Variant.VARIANT_OMAHA4);
      final message = applyPreflopRanges(
        input,
        GetPreflopRangesResponse(
          pot: 5.5,
          effectiveStack: 97.5,
          players: [
            PreflopRange(position: 'BB', range: 'x'),
            PreflopRange(position: 'BTN', range: 'y'),
          ],
          spot: Spot(
            oopRange: 'QQ-22, AKo:0.5',
            ipRange: 'AA-22, AKs',
            pot: 5.5,
            effectiveStack: 97.5,
          ),
        ),
      );
      expect(input.variant, Variant.VARIANT_HOLDEM);
      expect(input.playerCount, 2);
      expect(input.oopRange, 'QQ-22, AKo:0.5');
      expect(input.ipRange, 'AA-22, AKs');
      expect(input.pot, '5.5');
      expect(input.stack, '97.5');
      expect(message, 'OOP is BB, IP is BTN.');
    });

    test('fills a multiway line seat by seat', () {
      final input = SpotInput();
      final message = applyPreflopRanges(
        input,
        GetPreflopRangesResponse(
          pot: 7.5,
          effectiveStack: 97.5,
          spot: Spot(
            pot: 7.5,
            effectiveStack: 97.5,
            players: [
              PlayerSpot(position: 'BB', range: 'a', stack: 97.5),
              PlayerSpot(position: 'CO', range: 'b', stack: 97.5),
              PlayerSpot(position: 'BTN', range: 'c', stack: 150),
            ],
          ),
        ),
      );
      expect(input.playerCount, 3);
      expect([for (final p in input.players) p.range], ['a', 'b', 'c']);
      expect([for (final p in input.players) p.stack], ['', '', '150']);
      expect(message, 'BB, CO, BTN see the flop.');
    });
  });

  group('PloHandList', () {
    PlayerRange range() => PlayerRange(
      hands: ['AsKsQhJh', 'AhAdKcQc', '9s8s7h6h'],
      reach: [0.5, 1.0, 0.0],
      ev: [3, 7, 1],
      equity: [0.4, 0.7, 0.2],
      totalHands: 270725,
      shownReachFraction: 0.931,
    );

    test('sorts the hands still in the range', () {
      final list = PloHandList.fromRange(
        range(),
        strategy: [0.9, 0.2, 1, 0.1, 0.8, 0],
        actionCount: 2,
      );
      expect(list.actionCount, 2);
      final byWeight = list.sorted(PloSort.weight);
      // The hand without reach is left out.
      expect([for (final h in byWeight) h.name], ['AhAdKcQc', 'AsKsQhJh']);
      expect(byWeight.first.frequencies, [0.2, 0.8]);
      expect(byWeight.first.cards, ['Ah', 'Ad', 'Kc', 'Qc']);
      final byCheck = list.sorted(PloSort.action, action: 0);
      expect(byCheck.first.name, 'AsKsQhJh');
      final byEvLow = list.sorted(PloSort.ev, ascending: true);
      expect(byEvLow.first.name, 'AsKsQhJh');
    });

    test('says how much of the range is listed', () {
      expect(
        PloHandList.fromRange(range()).coverage,
        'Top 3 of 270,725 hands · 93.1% of the range\'s weight',
      );
      expect(
        PloHandList.fromRange(PlayerRange(hands: ['AsKsQhJh'], reach: [1]))
            .coverage,
        'All 1 hands',
      );
    });
  });

  group('NodeSeats', () {
    test('reads multiway nodes by seat', () {
      final node = GetNodeResponse(
        type: NodeType.NODE_TYPE_ACTION,
        players: [
          PlayerRange(position: 'BB', hands: ['AhKd'], reach: [0.5]),
          PlayerRange(position: 'CO', hands: ['QsQd'], reach: [1]),
          PlayerRange(position: 'BTN'),
        ],
        actingPlayer: 2,
        committed: [0, 3.3, 3.3],
        folded: [true, false, false],
        allIn: [false, false, true],
      );
      final seats = NodeSeats(node, Job(spot: Spot(effectiveStack: 100)));
      expect(seats.count, 3);
      expect(seats.positions, ['BB', 'CO', 'BTN']);
      expect(seats.acting, 2);
      expect(seats.behind(1), closeTo(96.7, 1e-9));
      expect(seats.folded(0), isTrue);
      expect(seats.allIn(2), isTrue);
      expect(seats.combos(0), 0.5);
    });

    test('reads older heads-up nodes', () {
      final node = GetNodeResponse(
        type: NodeType.NODE_TYPE_ACTION,
        player: Player.PLAYER_IP,
        oop: PlayerRange(hands: ['AhKd'], reach: [1]),
        ip: PlayerRange(hands: ['QsQd'], reach: [1]),
        oopCommitted: 5,
      );
      final seats = NodeSeats(node, Job(spot: Spot(effectiveStack: 95)));
      expect(seats.positions, ['OOP', 'IP']);
      expect(seats.acting, 1);
      expect(seats.behind(0), 90);
    });
  });

  test('MatrixModel.fromClasses reads preflop classes', () {
    final classes = [
      for (final cell in MatrixModel.fromRange(PlayerRange()).cells)
        HandClass(name: cell.label),
    ];
    classes[0]
      ..weight = 1
      ..strategy.addAll([0.25, 0.75]); // AA
    classes[1]
      ..weight = 0.5
      ..strategy.addAll([1, 0]); // AKs
    final model = MatrixModel.fromClasses(classes, actionCount: 2);
    expect(model.actionCount, 2);
    expect(model.cells[0].fraction, 1);
    expect(model.cells[0].frequency(1), closeTo(0.75, 1e-9));
    expect(model.cells[1].reach, 2);
    expect(model.cells[1].fraction, 0.5);
    expect(model.totalReach, 8);
    expect(model.cells[2].isEmpty, isTrue);
  });
}
