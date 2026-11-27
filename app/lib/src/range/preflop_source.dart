import '../api/gto_api.dart';
import '../common/format.dart';
import '../spot/spot_form.dart';

/// Where the preflop line picker takes its ranges from.
enum PreflopSource {
  /// The client's rule-based library: always there, rough.
  library('Built-in rules'),

  /// The solved library shipped with the app (assets/ranges).
  bundled('Solved library (bundled)'),

  /// A preflop solve on the server.
  solved('Server preflop job');

  const PreflopSource(this.label);

  final String label;
}

/// Jobs whose preflop solution can fill in ranges: preflop solves that have
/// something to browse, finished ones first.
List<Job> preflopJobs(List<Job> jobs) {
  final usable = [
    for (final job in jobs)
      if (job.spot.hasPreflop() &&
          job.hasSolution &&
          job.state != JobState.JOB_STATE_FAILED)
        job,
  ];
  usable.sort((a, b) {
    final aDone = a.state == JobState.JOB_STATE_DONE ? 0 : 1;
    final bDone = b.state == JobState.JOB_STATE_DONE ? 0 : 1;
    return aDone.compareTo(bDone);
  });
  return usable;
}

/// "BTN open, BB call · pot 5.5 · 12% of deals".
String scenarioLabel(PreflopScenario scenario) {
  final parts = [
    scenario.name.isEmpty ? scenario.path : scenario.name,
    'pot ${formatChips(scenario.pot)}',
    if (scenario.frequency > 0)
      '${formatPercent(scenario.frequency, digits: scenario.frequency < 0.1 ? 1 : 0)} of deals',
  ];
  return parts.join(' · ');
}

/// Fills [input] with the ranges, pot and stacks of a solved preflop line.
/// Prefers the ready spot the server builds; falls back to the per-player
/// ranges. Returns a sentence on what was filled in.
String applyPreflopRanges(SpotInput input, GetPreflopRangesResponse result) {
  // Preflop solves are Hold'em.
  input.setVariant(Variant.VARIANT_HOLDEM);
  final spot = result.hasSpot() ? result.spot : null;
  final pot = spot != null && spot.pot > 0 ? spot.pot : result.pot;
  final stack = spot != null && spot.effectiveStack > 0
      ? spot.effectiveStack
      : result.effectiveStack;
  input
    ..pot = formatChips(pot)
    ..stack = formatChips(stack);

  final seats = <({String position, String range, double stack})>[];
  if (spot != null && spot.players.isNotEmpty) {
    for (final p in spot.players) {
      seats.add((position: p.position, range: p.range, stack: p.stack));
    }
  } else if (spot != null &&
      spot.oopRange.isNotEmpty &&
      spot.ipRange.isNotEmpty) {
    final names = [for (final p in result.players) p.position];
    seats
      ..add((
        position: names.isNotEmpty ? names.first : 'OOP',
        range: spot.oopRange,
        stack: 0,
      ))
      ..add((
        position: names.length > 1 ? names[1] : 'IP',
        range: spot.ipRange,
        stack: 0,
      ));
  } else {
    for (final p in result.players) {
      seats.add((position: p.position, range: p.range, stack: p.stack));
    }
  }

  if (seats.length <= 2) {
    input.setPlayerCount(2);
    if (seats.isNotEmpty) input.oopRange = seats.first.range;
    if (seats.length > 1) input.ipRange = seats[1].range;
  } else {
    input.setPlayerCount(seats.length);
    for (var i = 0; i < seats.length; ++i) {
      input.players[i]
        ..position = seats[i].position
        ..range = seats[i].range
        // Equal to the effective stack means "the same as everyone".
        ..stack = seats[i].stack <= 0 || seats[i].stack == stack
            ? ''
            : formatChips(seats[i].stack);
    }
  }
  final names = [for (final s in seats) s.position].join(', ');
  return seats.length == 2
      ? 'OOP is ${seats[0].position}, IP is ${seats[1].position}.'
      : '$names see the flop.';
}
