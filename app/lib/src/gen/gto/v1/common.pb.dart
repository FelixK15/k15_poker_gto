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

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

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
  }) {
    final result = StreetSizing._();
    if (oop != null) result.oop = oop;
    if (ip != null) result.ip = ip;
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

/// A heads-up postflop spot.
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

  /// Ranges in the usual text form, e.g. "QQ+, AKs, AQo:0.5, A5s-A2s".
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

  /// The hand as GetNode names it, e.g. "AhKd".
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
  }) {
    final result = SolveSettings._();
    if (maxIterations != null) result.maxIterations = maxIterations;
    if (targetExploitabilityPct != null)
      result.targetExploitabilityPct = targetExploitabilityPct;
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
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
