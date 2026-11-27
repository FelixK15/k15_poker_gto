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

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

import 'common.pbenum.dart';

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

export 'common.pbenum.dart';

/// Bet and raise options for one player on one street.
class BetSizing extends $pb.GeneratedMessage {
  factory BetSizing({
    $core.Iterable<$core.double>? betPct,
    $core.Iterable<$core.double>? raisePct,
    $core.bool? allin,
  }) {
    final result = BetSizing._();
    if (betPct != null) result.betPct.addAll(betPct);
    if (raisePct != null) result.raisePct.addAll(raisePct);
    if (allin != null) result.allin = allin;
    return result;
  }

  BetSizing._();

  factory BetSizing.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      BetSizing()..mergeFromBuffer(data, registry);
  factory BetSizing.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      BetSizing()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'BetSizing',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: BetSizing.$_createMessage)
    ..p<$core.double>(1, _omitFieldNames ? '' : 'betPct', $pb.PbFieldType.KD)
    ..p<$core.double>(2, _omitFieldNames ? '' : 'raisePct', $pb.PbFieldType.KD)
    ..aOB(3, _omitFieldNames ? '' : 'allin')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  BetSizing clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  BetSizing copyWith(void Function(BetSizing) updates) =>
      super.copyWith((message) => updates(message as BetSizing)) as BetSizing;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use BetSizing() / BetSizing.new instead')
  static BetSizing create() => BetSizing._();
  static $pb.GeneratedMessage $_createMessage() => BetSizing._();
  @$core.override
  BetSizing createEmptyInstance() => BetSizing._();
  @$core.pragma('dart2js:noInline')
  static BetSizing getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<BetSizing>(BetSizing.$_createMessage);
  static BetSizing? _defaultInstance;

  /// Bet sizes in percent of the pot (50 = half pot).
  @$pb.TagNumber(1)
  $pb.PbList<$core.double> get betPct => $_getList(0);

  /// Raise sizes in percent of the pot after calling (100 = pot-size raise).
  @$pb.TagNumber(2)
  $pb.PbList<$core.double> get raisePct => $_getList(1);

  /// Also offer all-in as a bet and as a raise.
  @$pb.TagNumber(3)
  $core.bool get allin => $_getBF(2);
  @$pb.TagNumber(3)
  set allin($core.bool value) => $_setBool(2, value);
  @$pb.TagNumber(3)
  $core.bool hasAllin() => $_has(2);
  @$pb.TagNumber(3)
  void clearAllin() => $_clearField(3);
}

class StreetSizing extends $pb.GeneratedMessage {
  factory StreetSizing({
    BetSizing? oop,
    BetSizing? ip,
    BetSizing? all,
    $core.Iterable<BetSizing>? players,
  }) {
    final result = StreetSizing._();
    if (oop != null) result.oop = oop;
    if (ip != null) result.ip = ip;
    if (all != null) result.all = all;
    if (players != null) result.players.addAll(players);
    return result;
  }

  StreetSizing._();

  factory StreetSizing.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      StreetSizing()..mergeFromBuffer(data, registry);
  factory StreetSizing.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      StreetSizing()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'StreetSizing',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: StreetSizing.$_createMessage)
    ..aOM<BetSizing>(1, _omitFieldNames ? '' : 'oop',
        subBuilder: BetSizing.$_createMessage)
    ..aOM<BetSizing>(2, _omitFieldNames ? '' : 'ip',
        subBuilder: BetSizing.$_createMessage)
    ..aOM<BetSizing>(3, _omitFieldNames ? '' : 'all',
        subBuilder: BetSizing.$_createMessage)
    ..pPM<BetSizing>(4, _omitFieldNames ? '' : 'players',
        subBuilder: BetSizing.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  StreetSizing clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  StreetSizing copyWith(void Function(StreetSizing) updates) =>
      super.copyWith((message) => updates(message as StreetSizing))
          as StreetSizing;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use StreetSizing() / StreetSizing.new instead')
  static StreetSizing create() => StreetSizing._();
  static $pb.GeneratedMessage $_createMessage() => StreetSizing._();
  @$core.override
  StreetSizing createEmptyInstance() => StreetSizing._();
  @$core.pragma('dart2js:noInline')
  static StreetSizing getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<StreetSizing>(
          StreetSizing.$_createMessage);
  static StreetSizing? _defaultInstance;

  /// Heads-up: the out-of-position and the in-position player.
  @$pb.TagNumber(1)
  BetSizing get oop => $_getN(0);
  @$pb.TagNumber(1)
  set oop(BetSizing value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasOop() => $_has(0);
  @$pb.TagNumber(1)
  void clearOop() => $_clearField(1);
  @$pb.TagNumber(1)
  BetSizing ensureOop() => $_ensure(0);

  @$pb.TagNumber(2)
  BetSizing get ip => $_getN(1);
  @$pb.TagNumber(2)
  set ip(BetSizing value) => $_setField(2, value);
  @$pb.TagNumber(2)
  $core.bool hasIp() => $_has(1);
  @$pb.TagNumber(2)
  void clearIp() => $_clearField(2);
  @$pb.TagNumber(2)
  BetSizing ensureIp() => $_ensure(1);

  /// Sizes for every player who has none of their own: in a multiway spot
  /// all players without an entry in `players`; heads-up a player whose
  /// `oop` / `ip` field is not set at all.
  @$pb.TagNumber(3)
  BetSizing get all => $_getN(2);
  @$pb.TagNumber(3)
  set all(BetSizing value) => $_setField(3, value);
  @$pb.TagNumber(3)
  $core.bool hasAll() => $_has(2);
  @$pb.TagNumber(3)
  void clearAll() => $_clearField(3);
  @$pb.TagNumber(3)
  BetSizing ensureAll() => $_ensure(2);

  /// Multiway, optional: sizes per player, in the order of Spot.players.
  /// Shorter than the player list (or empty) means `all` for the rest.
  @$pb.TagNumber(4)
  $pb.PbList<BetSizing> get players => $_getList(3);
}

class TreeSettings extends $pb.GeneratedMessage {
  factory TreeSettings({
    StreetSizing? flop,
    StreetSizing? turn,
    StreetSizing? river,
    $core.int? raiseCap,
    $core.double? allinThresholdPct,
  }) {
    final result = TreeSettings._();
    if (flop != null) result.flop = flop;
    if (turn != null) result.turn = turn;
    if (river != null) result.river = river;
    if (raiseCap != null) result.raiseCap = raiseCap;
    if (allinThresholdPct != null) result.allinThresholdPct = allinThresholdPct;
    return result;
  }

  TreeSettings._();

  factory TreeSettings.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      TreeSettings()..mergeFromBuffer(data, registry);
  factory TreeSettings.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      TreeSettings()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'TreeSettings',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: TreeSettings.$_createMessage)
    ..aOM<StreetSizing>(1, _omitFieldNames ? '' : 'flop',
        subBuilder: StreetSizing.$_createMessage)
    ..aOM<StreetSizing>(2, _omitFieldNames ? '' : 'turn',
        subBuilder: StreetSizing.$_createMessage)
    ..aOM<StreetSizing>(3, _omitFieldNames ? '' : 'river',
        subBuilder: StreetSizing.$_createMessage)
    ..aI(4, _omitFieldNames ? '' : 'raiseCap', fieldType: $pb.PbFieldType.OU3)
    ..aD(5, _omitFieldNames ? '' : 'allinThresholdPct')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  TreeSettings clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  TreeSettings copyWith(void Function(TreeSettings) updates) =>
      super.copyWith((message) => updates(message as TreeSettings))
          as TreeSettings;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use TreeSettings() / TreeSettings.new instead')
  static TreeSettings create() => TreeSettings._();
  static $pb.GeneratedMessage $_createMessage() => TreeSettings._();
  @$core.override
  TreeSettings createEmptyInstance() => TreeSettings._();
  @$core.pragma('dart2js:noInline')
  static TreeSettings getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<TreeSettings>(
          TreeSettings.$_createMessage);
  static TreeSettings? _defaultInstance;

  @$pb.TagNumber(1)
  StreetSizing get flop => $_getN(0);
  @$pb.TagNumber(1)
  set flop(StreetSizing value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasFlop() => $_has(0);
  @$pb.TagNumber(1)
  void clearFlop() => $_clearField(1);
  @$pb.TagNumber(1)
  StreetSizing ensureFlop() => $_ensure(0);

  @$pb.TagNumber(2)
  StreetSizing get turn => $_getN(1);
  @$pb.TagNumber(2)
  set turn(StreetSizing value) => $_setField(2, value);
  @$pb.TagNumber(2)
  $core.bool hasTurn() => $_has(1);
  @$pb.TagNumber(2)
  void clearTurn() => $_clearField(2);
  @$pb.TagNumber(2)
  StreetSizing ensureTurn() => $_ensure(1);

  @$pb.TagNumber(3)
  StreetSizing get river => $_getN(2);
  @$pb.TagNumber(3)
  set river(StreetSizing value) => $_setField(3, value);
  @$pb.TagNumber(3)
  $core.bool hasRiver() => $_has(2);
  @$pb.TagNumber(3)
  void clearRiver() => $_clearField(3);
  @$pb.TagNumber(3)
  StreetSizing ensureRiver() => $_ensure(2);

  /// Most raises allowed on a street after the opening bet. Default 3.
  @$pb.TagNumber(4)
  $core.int get raiseCap => $_getIZ(3);
  @$pb.TagNumber(4)
  set raiseCap($core.int value) => $_setUnsignedInt32(3, value);
  @$pb.TagNumber(4)
  $core.bool hasRaiseCap() => $_has(3);
  @$pb.TagNumber(4)
  void clearRaiseCap() => $_clearField(4);

  /// A bet or raise that puts in at least this percentage of the remaining
  /// stack becomes all-in. 0 disables the rule. Default 67.
  @$pb.TagNumber(5)
  $core.double get allinThresholdPct => $_getN(4);
  @$pb.TagNumber(5)
  set allinThresholdPct($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasAllinThresholdPct() => $_has(4);
  @$pb.TagNumber(5)
  void clearAllinThresholdPct() => $_clearField(5);
}

/// One player of a spot with more than two players (or, optionally, of a
/// heads-up spot instead of oop_range / ip_range / effective_stack).
class PlayerSpot extends $pb.GeneratedMessage {
  factory PlayerSpot({
    $core.String? position,
    $core.String? range,
    $core.double? stack,
  }) {
    final result = PlayerSpot._();
    if (position != null) result.position = position;
    if (range != null) result.range = range;
    if (stack != null) result.stack = stack;
    return result;
  }

  PlayerSpot._();

  factory PlayerSpot.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PlayerSpot()..mergeFromBuffer(data, registry);
  factory PlayerSpot.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PlayerSpot()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PlayerSpot',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PlayerSpot.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'position')
    ..aOS(2, _omitFieldNames ? '' : 'range')
    ..aD(3, _omitFieldNames ? '' : 'stack')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PlayerSpot clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PlayerSpot copyWith(void Function(PlayerSpot) updates) =>
      super.copyWith((message) => updates(message as PlayerSpot)) as PlayerSpot;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PlayerSpot() / PlayerSpot.new instead')
  static PlayerSpot create() => PlayerSpot._();
  static $pb.GeneratedMessage $_createMessage() => PlayerSpot._();
  @$core.override
  PlayerSpot createEmptyInstance() => PlayerSpot._();
  @$core.pragma('dart2js:noInline')
  static PlayerSpot getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<PlayerSpot>(PlayerSpot.$_createMessage);
  static PlayerSpot? _defaultInstance;

  /// Position name shown to the user and used in preflop paths, e.g. "BTN".
  /// Must be unique within the spot. Empty: "P1", "P2", … by seat.
  @$pb.TagNumber(1)
  $core.String get position => $_getSZ(0);
  @$pb.TagNumber(1)
  set position($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPosition() => $_has(0);
  @$pb.TagNumber(1)
  void clearPosition() => $_clearField(1);

  /// The player's range in text form: Hold'em as in Spot.oop_range, PLO4 as
  /// the Omaha syntax (e.g. "AAxx, KQJTds, top15%").
  @$pb.TagNumber(2)
  $core.String get range => $_getSZ(1);
  @$pb.TagNumber(2)
  set range($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasRange() => $_has(1);
  @$pb.TagNumber(2)
  void clearRange() => $_clearField(2);

  /// Chips behind at the start of the spot. 0: Spot.effective_stack.
  @$pb.TagNumber(3)
  $core.double get stack => $_getN(2);
  @$pb.TagNumber(3)
  set stack($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasStack() => $_has(2);
  @$pb.TagNumber(3)
  void clearStack() => $_clearField(3);
}

/// How the Monte Carlo engine groups hands. Ignored by the exact engine, which
/// never abstracts.
class Abstraction extends $pb.GeneratedMessage {
  factory Abstraction({
    $core.bool? lossless,
    $core.int? preflopBuckets,
    $core.int? flopBuckets,
    $core.int? turnBuckets,
    $core.int? riverBuckets,
  }) {
    final result = Abstraction._();
    if (lossless != null) result.lossless = lossless;
    if (preflopBuckets != null) result.preflopBuckets = preflopBuckets;
    if (flopBuckets != null) result.flopBuckets = flopBuckets;
    if (turnBuckets != null) result.turnBuckets = turnBuckets;
    if (riverBuckets != null) result.riverBuckets = riverBuckets;
    return result;
  }

  Abstraction._();

  factory Abstraction.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Abstraction()..mergeFromBuffer(data, registry);
  factory Abstraction.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Abstraction()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Abstraction',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: Abstraction.$_createMessage)
    ..aOB(1, _omitFieldNames ? '' : 'lossless')
    ..aI(2, _omitFieldNames ? '' : 'preflopBuckets',
        fieldType: $pb.PbFieldType.OU3)
    ..aI(3, _omitFieldNames ? '' : 'flopBuckets',
        fieldType: $pb.PbFieldType.OU3)
    ..aI(4, _omitFieldNames ? '' : 'turnBuckets',
        fieldType: $pb.PbFieldType.OU3)
    ..aI(5, _omitFieldNames ? '' : 'riverBuckets',
        fieldType: $pb.PbFieldType.OU3)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Abstraction clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Abstraction copyWith(void Function(Abstraction) updates) =>
      super.copyWith((message) => updates(message as Abstraction))
          as Abstraction;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use Abstraction() / Abstraction.new instead')
  static Abstraction create() => Abstraction._();
  static $pb.GeneratedMessage $_createMessage() => Abstraction._();
  @$core.override
  Abstraction createEmptyInstance() => Abstraction._();
  @$core.pragma('dart2js:noInline')
  static Abstraction getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<Abstraction>(
          Abstraction.$_createMessage);
  static Abstraction? _defaultInstance;

  /// Keep every hand apart (no bucketing). Only feasible for small spots.
  @$pb.TagNumber(1)
  $core.bool get lossless => $_getBF(0);
  @$pb.TagNumber(1)
  set lossless($core.bool value) => $_setBool(0, value);
  @$pb.TagNumber(1)
  $core.bool hasLossless() => $_has(0);
  @$pb.TagNumber(1)
  void clearLossless() => $_clearField(1);

  /// Buckets per street. 0 or unset: the engine's default for the street.
  /// Preflop Hold'em is always lossless (169 classes).
  @$pb.TagNumber(2)
  $core.int get preflopBuckets => $_getIZ(1);
  @$pb.TagNumber(2)
  set preflopBuckets($core.int value) => $_setUnsignedInt32(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPreflopBuckets() => $_has(1);
  @$pb.TagNumber(2)
  void clearPreflopBuckets() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.int get flopBuckets => $_getIZ(2);
  @$pb.TagNumber(3)
  set flopBuckets($core.int value) => $_setUnsignedInt32(2, value);
  @$pb.TagNumber(3)
  $core.bool hasFlopBuckets() => $_has(2);
  @$pb.TagNumber(3)
  void clearFlopBuckets() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.int get turnBuckets => $_getIZ(3);
  @$pb.TagNumber(4)
  set turnBuckets($core.int value) => $_setUnsignedInt32(3, value);
  @$pb.TagNumber(4)
  $core.bool hasTurnBuckets() => $_has(3);
  @$pb.TagNumber(4)
  void clearTurnBuckets() => $_clearField(4);

  @$pb.TagNumber(5)
  $core.int get riverBuckets => $_getIZ(4);
  @$pb.TagNumber(5)
  set riverBuckets($core.int value) => $_setUnsignedInt32(4, value);
  @$pb.TagNumber(5)
  $core.bool hasRiverBuckets() => $_has(4);
  @$pb.TagNumber(5)
  void clearRiverBuckets() => $_clearField(5);
}

/// A preflop solve: the betting starts with the blinds and the job's solution
/// covers the preflop tree (see PreflopSettings.solve_postflop). Spot.board,
/// the ranges, pot and stacks are then ignored: everyone starts with every
/// hand and `stack_bb` behind.
class PreflopSettings extends $pb.GeneratedMessage {
  factory PreflopSettings({
    $core.int? players,
    $core.double? stackBb,
    $core.double? smallBlind,
    $core.double? bigBlind,
    $core.double? ante,
    $core.bool? bbAnte,
    $core.Iterable<$core.double>? openSizesBb,
    $core.Iterable<$core.double>? sbOpenSizesBb,
    $core.bool? allowLimp,
    $core.double? threeBetIpMultiplier,
    $core.double? threeBetOopMultiplier,
    $core.double? fourBetMultiplier,
    $core.bool? allowColdCalls,
    $core.int? raiseCap,
    $core.bool? solvePostflop,
    $core.bool? allowSbComplete,
    $core.bool? allowColdCall3bet,
    $core.bool? allowColdCall4bet,
    $core.double? squeezePerCaller,
    $core.Iterable<PositionSizes>? openSizesByPosition,
    $core.String? preset,
  }) {
    final result = PreflopSettings._();
    if (players != null) result.players = players;
    if (stackBb != null) result.stackBb = stackBb;
    if (smallBlind != null) result.smallBlind = smallBlind;
    if (bigBlind != null) result.bigBlind = bigBlind;
    if (ante != null) result.ante = ante;
    if (bbAnte != null) result.bbAnte = bbAnte;
    if (openSizesBb != null) result.openSizesBb.addAll(openSizesBb);
    if (sbOpenSizesBb != null) result.sbOpenSizesBb.addAll(sbOpenSizesBb);
    if (allowLimp != null) result.allowLimp = allowLimp;
    if (threeBetIpMultiplier != null)
      result.threeBetIpMultiplier = threeBetIpMultiplier;
    if (threeBetOopMultiplier != null)
      result.threeBetOopMultiplier = threeBetOopMultiplier;
    if (fourBetMultiplier != null) result.fourBetMultiplier = fourBetMultiplier;
    if (allowColdCalls != null) result.allowColdCalls = allowColdCalls;
    if (raiseCap != null) result.raiseCap = raiseCap;
    if (solvePostflop != null) result.solvePostflop = solvePostflop;
    if (allowSbComplete != null) result.allowSbComplete = allowSbComplete;
    if (allowColdCall3bet != null) result.allowColdCall3bet = allowColdCall3bet;
    if (allowColdCall4bet != null) result.allowColdCall4bet = allowColdCall4bet;
    if (squeezePerCaller != null) result.squeezePerCaller = squeezePerCaller;
    if (openSizesByPosition != null)
      result.openSizesByPosition.addAll(openSizesByPosition);
    if (preset != null) result.preset = preset;
    return result;
  }

  PreflopSettings._();

  factory PreflopSettings.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopSettings()..mergeFromBuffer(data, registry);
  factory PreflopSettings.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopSettings()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PreflopSettings',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PreflopSettings.$_createMessage)
    ..aI(1, _omitFieldNames ? '' : 'players', fieldType: $pb.PbFieldType.OU3)
    ..aD(2, _omitFieldNames ? '' : 'stackBb')
    ..aD(3, _omitFieldNames ? '' : 'smallBlind')
    ..aD(4, _omitFieldNames ? '' : 'bigBlind')
    ..aD(5, _omitFieldNames ? '' : 'ante')
    ..aOB(6, _omitFieldNames ? '' : 'bbAnte')
    ..p<$core.double>(
        7, _omitFieldNames ? '' : 'openSizesBb', $pb.PbFieldType.KD)
    ..p<$core.double>(
        8, _omitFieldNames ? '' : 'sbOpenSizesBb', $pb.PbFieldType.KD)
    ..aOB(9, _omitFieldNames ? '' : 'allowLimp')
    ..aD(10, _omitFieldNames ? '' : 'threeBetIpMultiplier')
    ..aD(11, _omitFieldNames ? '' : 'threeBetOopMultiplier')
    ..aD(12, _omitFieldNames ? '' : 'fourBetMultiplier')
    ..aOB(13, _omitFieldNames ? '' : 'allowColdCalls')
    ..aI(14, _omitFieldNames ? '' : 'raiseCap', fieldType: $pb.PbFieldType.OU3)
    ..aOB(15, _omitFieldNames ? '' : 'solvePostflop')
    ..aOB(16, _omitFieldNames ? '' : 'allowSbComplete')
    ..aOB(17, _omitFieldNames ? '' : 'allowColdCall3bet',
        protoName: 'allow_cold_call_3bet')
    ..aOB(18, _omitFieldNames ? '' : 'allowColdCall4bet',
        protoName: 'allow_cold_call_4bet')
    ..aD(19, _omitFieldNames ? '' : 'squeezePerCaller')
    ..pPM<PositionSizes>(20, _omitFieldNames ? '' : 'openSizesByPosition',
        subBuilder: PositionSizes.$_createMessage)
    ..aOS(21, _omitFieldNames ? '' : 'preset')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopSettings clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopSettings copyWith(void Function(PreflopSettings) updates) =>
      super.copyWith((message) => updates(message as PreflopSettings))
          as PreflopSettings;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PreflopSettings() / PreflopSettings.new instead')
  static PreflopSettings create() => PreflopSettings._();
  static $pb.GeneratedMessage $_createMessage() => PreflopSettings._();
  @$core.override
  PreflopSettings createEmptyInstance() => PreflopSettings._();
  @$core.pragma('dart2js:noInline')
  static PreflopSettings getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PreflopSettings>(
          PreflopSettings.$_createMessage);
  static PreflopSettings? _defaultInstance;

  /// Players at the table, 2 to 9 (6 for 6-max, 9 for 9-max). Seats are
  /// numbered in the order of preflop action: UTG, UTG1, UTG2, LJ, HJ, CO,
  /// BTN, SB, BB as the table size needs them (6-max: UTG, HJ, CO, BTN, SB,
  /// BB; 3-handed: BTN, SB, BB; heads-up: BTN, BB, the button posting the
  /// small blind). Job.positions and every per-player array of a preflop job
  /// use this order; postflop the SB acts first (heads-up the BB).
  @$pb.TagNumber(1)
  $core.int get players => $_getIZ(0);
  @$pb.TagNumber(1)
  set players($core.int value) => $_setUnsignedInt32(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPlayers() => $_has(0);
  @$pb.TagNumber(1)
  void clearPlayers() => $_clearField(1);

  /// Stack of every player before posting, in big blinds.
  @$pb.TagNumber(2)
  $core.double get stackBb => $_getN(1);
  @$pb.TagNumber(2)
  set stackBb($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasStackBb() => $_has(1);
  @$pb.TagNumber(2)
  void clearStackBb() => $_clearField(2);

  /// Blinds and ante in big blinds. Defaults: small blind 0.5, big blind 1,
  /// no ante.
  @$pb.TagNumber(3)
  $core.double get smallBlind => $_getN(2);
  @$pb.TagNumber(3)
  set smallBlind($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasSmallBlind() => $_has(2);
  @$pb.TagNumber(3)
  void clearSmallBlind() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.double get bigBlind => $_getN(3);
  @$pb.TagNumber(4)
  set bigBlind($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasBigBlind() => $_has(3);
  @$pb.TagNumber(4)
  void clearBigBlind() => $_clearField(4);

  @$pb.TagNumber(5)
  $core.double get ante => $_getN(4);
  @$pb.TagNumber(5)
  set ante($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasAnte() => $_has(4);
  @$pb.TagNumber(5)
  void clearAnte() => $_clearField(5);

  /// The ante is a single big-blind ante paid by the big blind (else every
  /// player pays `ante`).
  @$pb.TagNumber(6)
  $core.bool get bbAnte => $_getBF(5);
  @$pb.TagNumber(6)
  set bbAnte($core.bool value) => $_setBool(5, value);
  @$pb.TagNumber(6)
  $core.bool hasBbAnte() => $_has(5);
  @$pb.TagNumber(6)
  void clearBbAnte() => $_clearField(6);

  /// Opening raise sizes ("raise to", in big blinds) for every position but
  /// the small blind. Default: 2.5.
  @$pb.TagNumber(7)
  $pb.PbList<$core.double> get openSizesBb => $_getList(6);

  /// Small blind opening sizes ("raise to", in big blinds). Default: 3.
  @$pb.TagNumber(8)
  $pb.PbList<$core.double> get sbOpenSizesBb => $_getList(7);

  /// Whether a player may limp in (call the big blind first in). The small
  /// blind completing counts as a limp.
  @$pb.TagNumber(9)
  $core.bool get allowLimp => $_getBF(8);
  @$pb.TagNumber(9)
  set allowLimp($core.bool value) => $_setBool(8, value);
  @$pb.TagNumber(9)
  $core.bool hasAllowLimp() => $_has(8);
  @$pb.TagNumber(9)
  void clearAllowLimp() => $_clearField(9);

  /// 3-bet sizes as multiples of the open, in position and out of position.
  /// Defaults 3.0 and 4.0.
  @$pb.TagNumber(10)
  $core.double get threeBetIpMultiplier => $_getN(9);
  @$pb.TagNumber(10)
  set threeBetIpMultiplier($core.double value) => $_setDouble(9, value);
  @$pb.TagNumber(10)
  $core.bool hasThreeBetIpMultiplier() => $_has(9);
  @$pb.TagNumber(10)
  void clearThreeBetIpMultiplier() => $_clearField(10);

  @$pb.TagNumber(11)
  $core.double get threeBetOopMultiplier => $_getN(10);
  @$pb.TagNumber(11)
  set threeBetOopMultiplier($core.double value) => $_setDouble(10, value);
  @$pb.TagNumber(11)
  $core.bool hasThreeBetOopMultiplier() => $_has(10);
  @$pb.TagNumber(11)
  void clearThreeBetOopMultiplier() => $_clearField(11);

  /// 4-bet size as a multiple of the 3-bet; later raises use it too.
  /// Default 2.3.
  @$pb.TagNumber(12)
  $core.double get fourBetMultiplier => $_getN(11);
  @$pb.TagNumber(12)
  set fourBetMultiplier($core.double value) => $_setDouble(11, value);
  @$pb.TagNumber(12)
  $core.bool hasFourBetMultiplier() => $_has(11);
  @$pb.TagNumber(12)
  void clearFourBetMultiplier() => $_clearField(12);

  /// Players who have not put money in yet may call an open (cold call). The
  /// blinds may always call an open; whoever has raised or called may always
  /// call a re-raise.
  @$pb.TagNumber(13)
  $core.bool get allowColdCalls => $_getBF(12);
  @$pb.TagNumber(13)
  set allowColdCalls($core.bool value) => $_setBool(12, value);
  @$pb.TagNumber(13)
  $core.bool hasAllowColdCalls() => $_has(12);
  @$pb.TagNumber(13)
  void clearAllowColdCalls() => $_clearField(13);

  /// Most raises preflop, the open counting as the first. Default 4 (open,
  /// 3-bet, 4-bet, 5-bet all-in).
  @$pb.TagNumber(14)
  $core.int get raiseCap => $_getIZ(13);
  @$pb.TagNumber(14)
  set raiseCap($core.int value) => $_setUnsignedInt32(13, value);
  @$pb.TagNumber(14)
  $core.bool hasRaiseCap() => $_has(13);
  @$pb.TagNumber(14)
  void clearRaiseCap() => $_clearField(14);

  /// Also play the flop (no turn or river betting) with Spot.tree.flop sizes
  /// and strength buckets; Spot.tree.raise_cap applies (default 1). Off: the
  /// hand is valued at the flop by equity, without further betting (every
  /// player realises all of their equity: the main approximation).
  @$pb.TagNumber(15)
  $core.bool get solvePostflop => $_getBF(14);
  @$pb.TagNumber(15)
  set solvePostflop($core.bool value) => $_setBool(14, value);
  @$pb.TagNumber(15)
  $core.bool hasSolvePostflop() => $_has(14);
  @$pb.TagNumber(15)
  void clearSolvePostflop() => $_clearField(15);

  /// The small blind may complete even when allow_limp is off.
  @$pb.TagNumber(16)
  $core.bool get allowSbComplete => $_getBF(15);
  @$pb.TagNumber(16)
  set allowSbComplete($core.bool value) => $_setBool(15, value);
  @$pb.TagNumber(16)
  $core.bool hasAllowSbComplete() => $_has(15);
  @$pb.TagNumber(16)
  void clearAllowSbComplete() => $_clearField(16);

  /// Cold calls of 3-bets, and of 4-bets and more.
  @$pb.TagNumber(17)
  $core.bool get allowColdCall3bet => $_getBF(16);
  @$pb.TagNumber(17)
  set allowColdCall3bet($core.bool value) => $_setBool(16, value);
  @$pb.TagNumber(17)
  $core.bool hasAllowColdCall3bet() => $_has(16);
  @$pb.TagNumber(17)
  void clearAllowColdCall3bet() => $_clearField(17);

  @$pb.TagNumber(18)
  $core.bool get allowColdCall4bet => $_getBF(17);
  @$pb.TagNumber(18)
  set allowColdCall4bet($core.bool value) => $_setBool(17, value);
  @$pb.TagNumber(18)
  $core.bool hasAllowColdCall4bet() => $_has(17);
  @$pb.TagNumber(18)
  void clearAllowColdCall4bet() => $_clearField(18);

  /// Added to a 3-bet or 4-bet per caller of the raise being re-raised, in
  /// multiples of that raise (squeeze sizing). Default 1.
  @$pb.TagNumber(19)
  $core.double get squeezePerCaller => $_getN(18);
  @$pb.TagNumber(19)
  set squeezePerCaller($core.double value) => $_setDouble(18, value);
  @$pb.TagNumber(19)
  $core.bool hasSqueezePerCaller() => $_has(18);
  @$pb.TagNumber(19)
  void clearSqueezePerCaller() => $_clearField(19);

  /// Open sizes for single positions, replacing open_sizes_bb there.
  @$pb.TagNumber(20)
  $pb.PbList<PositionSizes> get openSizesByPosition => $_getList(19);

  /// A tuned preset: "hu", "6max" or "9max". When set, everything above but
  /// stack_bb and the rake comes from the preset (sizes follow the nearest of
  /// 100, 200 and 300 BB): the settings the shipped preflop library uses.
  @$pb.TagNumber(21)
  $core.String get preset => $_getSZ(20);
  @$pb.TagNumber(21)
  set preset($core.String value) => $_setString(20, value);
  @$pb.TagNumber(21)
  $core.bool hasPreset() => $_has(20);
  @$pb.TagNumber(21)
  void clearPreset() => $_clearField(21);
}

class PositionSizes extends $pb.GeneratedMessage {
  factory PositionSizes({
    $core.String? position,
    $core.Iterable<$core.double>? sizesBb,
  }) {
    final result = PositionSizes._();
    if (position != null) result.position = position;
    if (sizesBb != null) result.sizesBb.addAll(sizesBb);
    return result;
  }

  PositionSizes._();

  factory PositionSizes.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PositionSizes()..mergeFromBuffer(data, registry);
  factory PositionSizes.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PositionSizes()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PositionSizes',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PositionSizes.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'position')
    ..p<$core.double>(2, _omitFieldNames ? '' : 'sizesBb', $pb.PbFieldType.KD)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PositionSizes clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PositionSizes copyWith(void Function(PositionSizes) updates) =>
      super.copyWith((message) => updates(message as PositionSizes))
          as PositionSizes;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PositionSizes() / PositionSizes.new instead')
  static PositionSizes create() => PositionSizes._();
  static $pb.GeneratedMessage $_createMessage() => PositionSizes._();
  @$core.override
  PositionSizes createEmptyInstance() => PositionSizes._();
  @$core.pragma('dart2js:noInline')
  static PositionSizes getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PositionSizes>(
          PositionSizes.$_createMessage);
  static PositionSizes? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get position => $_getSZ(0);
  @$pb.TagNumber(1)
  set position($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPosition() => $_has(0);
  @$pb.TagNumber(1)
  void clearPosition() => $_clearField(1);

  @$pb.TagNumber(2)
  $pb.PbList<$core.double> get sizesBb => $_getList(1);
}

/// A spot to solve: heads-up or multiway postflop, or a preflop solve.
class Spot extends $pb.GeneratedMessage {
  factory Spot({
    $core.String? board,
    $core.String? oopRange,
    $core.String? ipRange,
    $core.double? pot,
    $core.double? effectiveStack,
    $core.double? rakePct,
    $core.double? rakeCap,
    TreeSettings? tree,
    Variant? variant,
    $core.bool? potLimit,
    $core.Iterable<PlayerSpot>? players,
    PreflopSettings? preflop,
    Abstraction? abstraction,
    Engine? engine,
  }) {
    final result = Spot._();
    if (board != null) result.board = board;
    if (oopRange != null) result.oopRange = oopRange;
    if (ipRange != null) result.ipRange = ipRange;
    if (pot != null) result.pot = pot;
    if (effectiveStack != null) result.effectiveStack = effectiveStack;
    if (rakePct != null) result.rakePct = rakePct;
    if (rakeCap != null) result.rakeCap = rakeCap;
    if (tree != null) result.tree = tree;
    if (variant != null) result.variant = variant;
    if (potLimit != null) result.potLimit = potLimit;
    if (players != null) result.players.addAll(players);
    if (preflop != null) result.preflop = preflop;
    if (abstraction != null) result.abstraction = abstraction;
    if (engine != null) result.engine = engine;
    return result;
  }

  Spot._();

  factory Spot.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Spot()..mergeFromBuffer(data, registry);
  factory Spot.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Spot()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Spot',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: Spot.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'board')
    ..aOS(2, _omitFieldNames ? '' : 'oopRange')
    ..aOS(3, _omitFieldNames ? '' : 'ipRange')
    ..aD(4, _omitFieldNames ? '' : 'pot')
    ..aD(5, _omitFieldNames ? '' : 'effectiveStack')
    ..aD(6, _omitFieldNames ? '' : 'rakePct')
    ..aD(7, _omitFieldNames ? '' : 'rakeCap')
    ..aOM<TreeSettings>(8, _omitFieldNames ? '' : 'tree',
        subBuilder: TreeSettings.$_createMessage)
    ..aE<Variant>(9, _omitFieldNames ? '' : 'variant',
        enumValues: Variant.values)
    ..aOB(10, _omitFieldNames ? '' : 'potLimit')
    ..pPM<PlayerSpot>(11, _omitFieldNames ? '' : 'players',
        subBuilder: PlayerSpot.$_createMessage)
    ..aOM<PreflopSettings>(12, _omitFieldNames ? '' : 'preflop',
        subBuilder: PreflopSettings.$_createMessage)
    ..aOM<Abstraction>(13, _omitFieldNames ? '' : 'abstraction',
        subBuilder: Abstraction.$_createMessage)
    ..aE<Engine>(14, _omitFieldNames ? '' : 'engine', enumValues: Engine.values)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Spot clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Spot copyWith(void Function(Spot) updates) =>
      super.copyWith((message) => updates(message as Spot)) as Spot;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use Spot() / Spot.new instead')
  static Spot create() => Spot._();
  static $pb.GeneratedMessage $_createMessage() => Spot._();
  @$core.override
  Spot createEmptyInstance() => Spot._();
  @$core.pragma('dart2js:noInline')
  static Spot getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<Spot>(Spot.$_createMessage);
  static Spot? _defaultInstance;

  /// Three, four or five cards, e.g. "Ks Td 7h".
  @$pb.TagNumber(1)
  $core.String get board => $_getSZ(0);
  @$pb.TagNumber(1)
  set board($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasBoard() => $_has(0);
  @$pb.TagNumber(1)
  void clearBoard() => $_clearField(1);

  /// Ranges in the usual text form, e.g. "QQ+, AKs, AQo:0.5, A5s-A2s". PLO4:
  /// the Omaha syntax, e.g. "AAxx, KQJTds, top15%" (see OmahaRange::parse).
  /// Heads-up only; ignored when `players` is set.
  @$pb.TagNumber(2)
  $core.String get oopRange => $_getSZ(1);
  @$pb.TagNumber(2)
  set oopRange($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasOopRange() => $_has(1);
  @$pb.TagNumber(2)
  void clearOopRange() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.String get ipRange => $_getSZ(2);
  @$pb.TagNumber(3)
  set ipRange($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasIpRange() => $_has(2);
  @$pb.TagNumber(3)
  void clearIpRange() => $_clearField(3);

  /// Pot at the start of the spot, in any chip unit (typically big blinds).
  @$pb.TagNumber(4)
  $core.double get pot => $_getN(3);
  @$pb.TagNumber(4)
  set pot($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasPot() => $_has(3);
  @$pb.TagNumber(4)
  void clearPot() => $_clearField(4);

  /// Chips each player has behind at the start of the spot.
  @$pb.TagNumber(5)
  $core.double get effectiveStack => $_getN(4);
  @$pb.TagNumber(5)
  set effectiveStack($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasEffectiveStack() => $_has(4);
  @$pb.TagNumber(5)
  void clearEffectiveStack() => $_clearField(5);

  /// Rake in percent of the final pot, and its cap in chips (0 = no cap).
  @$pb.TagNumber(6)
  $core.double get rakePct => $_getN(5);
  @$pb.TagNumber(6)
  set rakePct($core.double value) => $_setDouble(5, value);
  @$pb.TagNumber(6)
  $core.bool hasRakePct() => $_has(5);
  @$pb.TagNumber(6)
  void clearRakePct() => $_clearField(6);

  @$pb.TagNumber(7)
  $core.double get rakeCap => $_getN(6);
  @$pb.TagNumber(7)
  set rakeCap($core.double value) => $_setDouble(6, value);
  @$pb.TagNumber(7)
  $core.bool hasRakeCap() => $_has(6);
  @$pb.TagNumber(7)
  void clearRakeCap() => $_clearField(7);

  @$pb.TagNumber(8)
  TreeSettings get tree => $_getN(7);
  @$pb.TagNumber(8)
  set tree(TreeSettings value) => $_setField(8, value);
  @$pb.TagNumber(8)
  $core.bool hasTree() => $_has(7);
  @$pb.TagNumber(8)
  void clearTree() => $_clearField(8);
  @$pb.TagNumber(8)
  TreeSettings ensureTree() => $_ensure(7);

  /// Default: Hold'em. For PLO4 the ranges use the Omaha syntax.
  @$pb.TagNumber(9)
  Variant get variant => $_getN(8);
  @$pb.TagNumber(9)
  set variant(Variant value) => $_setField(9, value);
  @$pb.TagNumber(9)
  $core.bool hasVariant() => $_has(8);
  @$pb.TagNumber(9)
  void clearVariant() => $_clearField(9);

  /// Pot-limit betting: no bet larger than the pot, no raise larger than a
  /// pot-size raise (larger sizes are cut down to that). Default: on for PLO4,
  /// off for Hold'em.
  @$pb.TagNumber(10)
  $core.bool get potLimit => $_getBF(9);
  @$pb.TagNumber(10)
  set potLimit($core.bool value) => $_setBool(9, value);
  @$pb.TagNumber(10)
  $core.bool hasPotLimit() => $_has(9);
  @$pb.TagNumber(10)
  void clearPotLimit() => $_clearField(10);

  /// Multiway: the players in postflop order of action, first to act first
  /// (e.g. SB, BB, BTN). Needed for more than two players; for two players it
  /// may replace oop_range / ip_range / effective_stack (players[0] is OOP).
  /// When set, oop_range and ip_range are ignored.
  @$pb.TagNumber(11)
  $pb.PbList<PlayerSpot> get players => $_getList(10);

  /// Set for a preflop solve; see PreflopSettings.
  @$pb.TagNumber(12)
  PreflopSettings get preflop => $_getN(11);
  @$pb.TagNumber(12)
  set preflop(PreflopSettings value) => $_setField(12, value);
  @$pb.TagNumber(12)
  $core.bool hasPreflop() => $_has(11);
  @$pb.TagNumber(12)
  void clearPreflop() => $_clearField(12);
  @$pb.TagNumber(12)
  PreflopSettings ensurePreflop() => $_ensure(11);

  /// Monte Carlo hand abstraction.
  @$pb.TagNumber(13)
  Abstraction get abstraction => $_getN(12);
  @$pb.TagNumber(13)
  set abstraction(Abstraction value) => $_setField(13, value);
  @$pb.TagNumber(13)
  $core.bool hasAbstraction() => $_has(12);
  @$pb.TagNumber(13)
  void clearAbstraction() => $_clearField(13);
  @$pb.TagNumber(13)
  Abstraction ensureAbstraction() => $_ensure(12);

  /// Which solver to use. Default: AUTO.
  @$pb.TagNumber(14)
  Engine get engine => $_getN(13);
  @$pb.TagNumber(14)
  set engine(Engine value) => $_setField(14, value);
  @$pb.TagNumber(14)
  $core.bool hasEngine() => $_has(13);
  @$pb.TagNumber(14)
  void clearEngine() => $_clearField(14);
}

/// A player's strategy fixed for some hands at one decision.
class NodeLock extends $pb.GeneratedMessage {
  factory NodeLock({
    $core.String? path,
    $core.Iterable<HandLock>? hands,
  }) {
    final result = NodeLock._();
    if (path != null) result.path = path;
    if (hands != null) result.hands.addAll(hands);
    return result;
  }

  NodeLock._();

  factory NodeLock.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      NodeLock()..mergeFromBuffer(data, registry);
  factory NodeLock.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      NodeLock()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'NodeLock',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: NodeLock.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'path')
    ..pPM<HandLock>(2, _omitFieldNames ? '' : 'hands',
        subBuilder: HandLock.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  NodeLock clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  NodeLock copyWith(void Function(NodeLock) updates) =>
      super.copyWith((message) => updates(message as NodeLock)) as NodeLock;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use NodeLock() / NodeLock.new instead')
  static NodeLock create() => NodeLock._();
  static $pb.GeneratedMessage $_createMessage() => NodeLock._();
  @$core.override
  NodeLock createEmptyInstance() => NodeLock._();
  @$core.pragma('dart2js:noInline')
  static NodeLock getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<NodeLock>(NodeLock.$_createMessage);
  static NodeLock? _defaultInstance;

  /// Line to the decision, as in GetNodeRequest.path.
  @$pb.TagNumber(1)
  $core.String get path => $_getSZ(0);
  @$pb.TagNumber(1)
  set path($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPath() => $_has(0);
  @$pb.TagNumber(1)
  void clearPath() => $_clearField(1);

  /// The hands to lock. Other hands at the node stay free.
  @$pb.TagNumber(2)
  $pb.PbList<HandLock> get hands => $_getList(1);
}

class HandLock extends $pb.GeneratedMessage {
  factory HandLock({
    $core.String? hand,
    $core.Iterable<$core.double>? frequencies,
  }) {
    final result = HandLock._();
    if (hand != null) result.hand = hand;
    if (frequencies != null) result.frequencies.addAll(frequencies);
    return result;
  }

  HandLock._();

  factory HandLock.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      HandLock()..mergeFromBuffer(data, registry);
  factory HandLock.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      HandLock()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'HandLock',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: HandLock.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'hand')
    ..p<$core.double>(
        2, _omitFieldNames ? '' : 'frequencies', $pb.PbFieldType.KF)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  HandLock clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  HandLock copyWith(void Function(HandLock) updates) =>
      super.copyWith((message) => updates(message as HandLock)) as HandLock;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use HandLock() / HandLock.new instead')
  static HandLock create() => HandLock._();
  static $pb.GeneratedMessage $_createMessage() => HandLock._();
  @$core.override
  HandLock createEmptyInstance() => HandLock._();
  @$core.pragma('dart2js:noInline')
  static HandLock getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<HandLock>(HandLock.$_createMessage);
  static HandLock? _defaultInstance;

  /// The hand as GetNode names it, e.g. "AhKd", or for PLO4 "AsKsQhJh".
  @$pb.TagNumber(1)
  $core.String get hand => $_getSZ(0);
  @$pb.TagNumber(1)
  set hand($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasHand() => $_has(0);
  @$pb.TagNumber(1)
  void clearHand() => $_clearField(1);

  /// How often it takes each action, in the order of the node's actions.
  /// Scaled to add up to one.
  @$pb.TagNumber(2)
  $pb.PbList<$core.double> get frequencies => $_getList(1);
}

class SolveSettings extends $pb.GeneratedMessage {
  factory SolveSettings({
    $core.int? maxIterations,
    $core.double? targetExploitabilityPct,
    $core.double? maxSeconds,
    $fixnum.Int64? mcIterations,
  }) {
    final result = SolveSettings._();
    if (maxIterations != null) result.maxIterations = maxIterations;
    if (targetExploitabilityPct != null)
      result.targetExploitabilityPct = targetExploitabilityPct;
    if (maxSeconds != null) result.maxSeconds = maxSeconds;
    if (mcIterations != null) result.mcIterations = mcIterations;
    return result;
  }

  SolveSettings._();

  factory SolveSettings.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      SolveSettings()..mergeFromBuffer(data, registry);
  factory SolveSettings.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      SolveSettings()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'SolveSettings',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: SolveSettings.$_createMessage)
    ..aI(1, _omitFieldNames ? '' : 'maxIterations',
        fieldType: $pb.PbFieldType.OU3)
    ..aD(2, _omitFieldNames ? '' : 'targetExploitabilityPct')
    ..aD(3, _omitFieldNames ? '' : 'maxSeconds')
    ..a<$fixnum.Int64>(
        4, _omitFieldNames ? '' : 'mcIterations', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  SolveSettings clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  SolveSettings copyWith(void Function(SolveSettings) updates) =>
      super.copyWith((message) => updates(message as SolveSettings))
          as SolveSettings;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use SolveSettings() / SolveSettings.new instead')
  static SolveSettings create() => SolveSettings._();
  static $pb.GeneratedMessage $_createMessage() => SolveSettings._();
  @$core.override
  SolveSettings createEmptyInstance() => SolveSettings._();
  @$core.pragma('dart2js:noInline')
  static SolveSettings getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<SolveSettings>(
          SolveSettings.$_createMessage);
  static SolveSettings? _defaultInstance;

  /// Stop after this many iterations. Default 1000.
  @$pb.TagNumber(1)
  $core.int get maxIterations => $_getIZ(0);
  @$pb.TagNumber(1)
  set maxIterations($core.int value) => $_setUnsignedInt32(0, value);
  @$pb.TagNumber(1)
  $core.bool hasMaxIterations() => $_has(0);
  @$pb.TagNumber(1)
  void clearMaxIterations() => $_clearField(1);

  /// Stop once exploitability is at or below this percentage of the pot.
  /// Default 0.3.
  @$pb.TagNumber(2)
  $core.double get targetExploitabilityPct => $_getN(1);
  @$pb.TagNumber(2)
  set targetExploitabilityPct($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasTargetExploitabilityPct() => $_has(1);
  @$pb.TagNumber(2)
  void clearTargetExploitabilityPct() => $_clearField(2);

  /// Stop after this many seconds of solving (time paused or queued does not
  /// count). 0: no time limit. Applies to both engines.
  @$pb.TagNumber(3)
  $core.double get maxSeconds => $_getN(2);
  @$pb.TagNumber(3)
  set maxSeconds($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasMaxSeconds() => $_has(2);
  @$pb.TagNumber(3)
  void clearMaxSeconds() => $_clearField(3);

  /// Monte Carlo engine: stop after this many sampled iterations (one
  /// iteration samples one deal and walks the tree once per player).
  /// max_iterations does not apply to it. 0: the server's default budget.
  @$pb.TagNumber(4)
  $fixnum.Int64 get mcIterations => $_getI64(3);
  @$pb.TagNumber(4)
  set mcIterations($fixnum.Int64 value) => $_setInt64(3, value);
  @$pb.TagNumber(4)
  $core.bool hasMcIterations() => $_has(3);
  @$pb.TagNumber(4)
  void clearMcIterations() => $_clearField(4);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
