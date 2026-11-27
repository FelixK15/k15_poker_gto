// This is a generated file - do not edit.
//
// Generated from gto/v1/common.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

/// The poker game being played.
class Variant extends $pb.ProtobufEnum {
  /// No-Limit Hold'em, two hole cards.
  static const Variant VARIANT_HOLDEM =
      Variant._(0, _omitEnumNames ? '' : 'VARIANT_HOLDEM');

  /// Omaha with four hole cards (PLO4): exactly two hole cards and three board
  /// cards make a hand. Pot-limit unless Spot.pot_limit says otherwise.
  static const Variant VARIANT_OMAHA4 =
      Variant._(1, _omitEnumNames ? '' : 'VARIANT_OMAHA4');

  static const $core.List<Variant> values = <Variant>[
    VARIANT_HOLDEM,
    VARIANT_OMAHA4,
  ];

  static final $core.List<Variant?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 1);
  static Variant? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const Variant._(super.value, super.name);
}

/// Which solver works on a spot.
class Engine extends $pb.ProtobufEnum {
  /// The server chooses: EXACT for a heads-up postflop spot whose full tree
  /// fits the server's limits, MONTE_CARLO for everything else (multiway,
  /// preflop, heads-up spots too large to solve exactly).
  static const Engine ENGINE_AUTO =
      Engine._(0, _omitEnumNames ? '' : 'ENGINE_AUTO');

  /// Vector CFR over every hand, no abstraction. Heads-up postflop only.
  /// Fails with RESOURCE_EXHAUSTED when the tree does not fit.
  static const Engine ENGINE_EXACT =
      Engine._(1, _omitEnumNames ? '' : 'ENGINE_EXACT');

  /// External-sampling Monte Carlo CFR for 2 to 9 players, with hands grouped
  /// into buckets (see Abstraction). Exploitability is an estimate.
  static const Engine ENGINE_MONTE_CARLO =
      Engine._(2, _omitEnumNames ? '' : 'ENGINE_MONTE_CARLO');

  static const $core.List<Engine> values = <Engine>[
    ENGINE_AUTO,
    ENGINE_EXACT,
    ENGINE_MONTE_CARLO,
  ];

  static final $core.List<Engine?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 2);
  static Engine? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const Engine._(super.value, super.name);
}

/// How to read Job.exploitability_pct.
class ExploitabilityKind extends $pb.ProtobufEnum {
  static const ExploitabilityKind EXPLOITABILITY_KIND_UNSPECIFIED =
      ExploitabilityKind._(
          0, _omitEnumNames ? '' : 'EXPLOITABILITY_KIND_UNSPECIFIED');

  /// Exact: the average gain of a best response against the current
  /// strategies, computed over the full tree. 0 means Nash equilibrium.
  static const ExploitabilityKind EXPLOITABILITY_KIND_EXACT =
      ExploitabilityKind._(
          1, _omitEnumNames ? '' : 'EXPLOITABILITY_KIND_EXACT');

  /// Exact, but in the game with the job's locks held fixed: the distance
  /// from the best play given the locks, not from the unlocked equilibrium.
  static const ExploitabilityKind EXPLOITABILITY_KIND_LOCKED_GAME =
      ExploitabilityKind._(
          2, _omitEnumNames ? '' : 'EXPLOITABILITY_KIND_LOCKED_GAME');

  /// Heads-up Monte Carlo: an estimate of the best-response gain, measured
  /// on sampled deals and on the abstracted game. Has the usual two-player
  /// meaning (0 = equilibrium) up to sampling noise and abstraction error.
  static const ExploitabilityKind EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER =
      ExploitabilityKind._(
          3, _omitEnumNames ? '' : 'EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER');

  /// Three or more players: the estimated sum of what each player could gain
  /// by best-responding while the others keep their strategies. CFR has no
  /// Nash guarantee with more than two players, so a low number means "hard
  /// to exploit unilaterally", not "equilibrium". Label it as such.
  static const ExploitabilityKind EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY =
      ExploitabilityKind._(
          4, _omitEnumNames ? '' : 'EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY');

  static const $core.List<ExploitabilityKind> values = <ExploitabilityKind>[
    EXPLOITABILITY_KIND_UNSPECIFIED,
    EXPLOITABILITY_KIND_EXACT,
    EXPLOITABILITY_KIND_LOCKED_GAME,
    EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER,
    EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY,
  ];

  static final $core.List<ExploitabilityKind?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 4);
  static ExploitabilityKind? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const ExploitabilityKind._(super.value, super.name);
}

const $core.bool _omitEnumNames =
    $core.bool.fromEnvironment('protobuf.omit_enum_names');
