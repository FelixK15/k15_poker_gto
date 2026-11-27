import '../api/gto_api.dart';
import 'format.dart';

/// How a job's or an estimate's exploitability number reads, by what it
/// means. Exact numbers keep the plain wording; Monte Carlo numbers are
/// marked as estimates, and with three or more players also as carrying no
/// equilibrium guarantee.
String exploitabilityLabel(double pct, ExploitabilityKind kind) {
  final value = '${pct.toStringAsFixed(2)}%';
  switch (kind) {
    case ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY:
      return '≈ $value of pot '
          '(estimate; no equilibrium guarantee with 3+ players)';
    case ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER:
      return '≈ $value of pot exploitable (estimate)';
    default:
      // Exact, locked game, and jobs from servers that did not say.
      return '$value of pot exploitable';
  }
}

/// A sentence on what the exploitability of [kind] means.
String exploitabilityMeaning(ExploitabilityKind kind) {
  switch (kind) {
    case ExploitabilityKind.EXPLOITABILITY_KIND_EXACT:
      return 'Exact: what a perfect opponent could still win. 0 is an '
          'equilibrium.';
    case ExploitabilityKind.EXPLOITABILITY_KIND_LOCKED_GAME:
      return 'Exact, with the locks held fixed: the distance from the best '
          'play given the locks.';
    case ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER:
      return 'Estimated on sampled deals of the bucketed game. 0 is an '
          'equilibrium, up to sampling noise.';
    case ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY:
      return 'Estimated sum of what each player could gain alone. With 3+ '
          'players there is no equilibrium guarantee: a low number means '
          'hard to exploit, not solved.';
    default:
      return '';
  }
}

/// Short name of an exploitability kind for the estimate line.
String exploitabilityKindLabel(ExploitabilityKind kind) => switch (kind) {
  ExploitabilityKind.EXPLOITABILITY_KIND_EXACT => 'exact exploitability',
  ExploitabilityKind.EXPLOITABILITY_KIND_LOCKED_GAME =>
    'exact exploitability (locked game)',
  ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER =>
    'estimated exploitability',
  ExploitabilityKind.EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY =>
    'estimated exploitability, no equilibrium guarantee',
  _ => 'exploitability',
};

/// "Exact", "Monte Carlo", "Auto".
String engineLabel(Engine engine) => switch (engine) {
  Engine.ENGINE_EXACT => 'Exact',
  Engine.ENGINE_MONTE_CARLO => 'Monte Carlo',
  _ => 'Auto',
};

/// "NLH", "PLO4".
String variantLabel(Variant variant) =>
    variant == Variant.VARIANT_OMAHA4 ? 'PLO4' : 'NLH';

/// Position names of a job's players in seat order. Jobs from older servers
/// carry none and are heads-up.
List<String> jobPositions(Job job) {
  if (job.positions.isNotEmpty) return job.positions;
  final named = [for (final p in job.spot.players) p.position];
  if (named.isNotEmpty && named.every((p) => p.isNotEmpty)) return named;
  final count = job.playerCount > 0
      ? job.playerCount
      : job.spot.players.isNotEmpty
      ? job.spot.players.length
      : 2;
  return count == 2
      ? const ['OOP', 'IP']
      : [for (var i = 1; i <= count; ++i) 'P$i'];
}

/// Every player's EV, e.g. "EV: OOP 4.21, IP 5.79" or
/// "EV ≈ SB -0.31, BB -0.62, BTN 1.10" for sampled values.
String evLine(Job job) {
  final positions = jobPositions(job);
  final evs = job.evs.isNotEmpty ? job.evs : [job.oopEv, job.ipEv];
  final parts = [
    for (var i = 0; i < evs.length; ++i)
      '${i < positions.length ? positions[i] : 'P${i + 1}'} '
          '${evs[i].toStringAsFixed(2)}',
  ];
  return '${job.evEstimated ? 'EV ≈ ' : 'EV: '}${parts.join(', ')}';
}

/// Whether the job runs on the Monte Carlo engine.
bool isMonteCarlo(Job job) => job.engine == Engine.ENGINE_MONTE_CARLO;

/// The Monte Carlo budget the server uses when a job does not set one.
const int defaultMcIterations = 100000000;

/// "12.5M", "250M", "800k", "950".
String formatSamples(num count) {
  if (count >= 1e6) {
    final millions = count / 1e6;
    return '${formatChips(millions < 100 ? (millions * 10).round() / 10 : millions.roundToDouble())}M';
  }
  if (count >= 1e3) return '${(count / 1e3).round()}k';
  return count.toStringAsFixed(0);
}

/// "Heads-up", "6-max", "9-max", or "5 players".
String tableLabel(int players) => switch (players) {
  2 => 'Heads-up',
  6 => '6-max',
  9 => '9-max',
  _ => '$players players',
};

/// "NLH · Exact", "PLO4 · 3 players · Monte Carlo", "Preflop · 6-max".
String jobKind(Job job) {
  final spot = job.spot;
  final parts = <String>[];
  if (spot.hasPreflop()) {
    parts
      ..add('Preflop')
      ..add(tableLabel(spot.preflop.players));
  } else {
    parts.add(variantLabel(spot.variant));
    final players = jobPositions(job).length;
    if (players > 2) parts.add('$players players');
  }
  if (job.engine != Engine.ENGINE_AUTO) parts.add(engineLabel(job.engine));
  return parts.join(' · ');
}
