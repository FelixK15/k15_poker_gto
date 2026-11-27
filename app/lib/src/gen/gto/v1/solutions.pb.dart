// This is a generated file - do not edit.
//
// Generated from gto/v1/solutions.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

import 'common.pb.dart' as $1;
import 'solutions.pbenum.dart';

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

export 'solutions.pbenum.dart';

class GetNodeRequest extends $pb.GeneratedMessage {
  factory GetNodeRequest({
    $core.String? jobId,
    $core.String? path,
    $core.int? maxHands,
    HandDetail? detail,
  }) {
    final result = GetNodeRequest._();
    if (jobId != null) result.jobId = jobId;
    if (path != null) result.path = path;
    if (maxHands != null) result.maxHands = maxHands;
    if (detail != null) result.detail = detail;
    return result;
  }

  GetNodeRequest._();

  factory GetNodeRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetNodeRequest()..mergeFromBuffer(data, registry);
  factory GetNodeRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetNodeRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetNodeRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetNodeRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'jobId')
    ..aOS(2, _omitFieldNames ? '' : 'path')
    ..aI(3, _omitFieldNames ? '' : 'maxHands', fieldType: $pb.PbFieldType.OU3)
    ..aE<HandDetail>(4, _omitFieldNames ? '' : 'detail',
        enumValues: HandDetail.values)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetNodeRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetNodeRequest copyWith(void Function(GetNodeRequest) updates) =>
      super.copyWith((message) => updates(message as GetNodeRequest))
          as GetNodeRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use GetNodeRequest() / GetNodeRequest.new instead')
  static GetNodeRequest create() => GetNodeRequest._();
  static $pb.GeneratedMessage $_createMessage() => GetNodeRequest._();
  @$core.override
  GetNodeRequest createEmptyInstance() => GetNodeRequest._();
  @$core.pragma('dart2js:noInline')
  static GetNodeRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GetNodeRequest>(
          GetNodeRequest.$_createMessage);
  static GetNodeRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get jobId => $_getSZ(0);
  @$pb.TagNumber(1)
  set jobId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasJobId() => $_has(0);
  @$pb.TagNumber(1)
  void clearJobId() => $_clearField(1);

  /// Line from the start of the spot: actions and dealt cards separated by
  /// '/', e.g. "check/bet50/call/Ts/check". Empty for the root. Actions are
  /// named as in NodeAction.label.
  ///
  /// Preflop solves: preflop actions are "fold", "check", "call", "raise2.5"
  /// (raise to 2.5 big blinds in total) and "allin". A segment may name the
  /// player as "POS:action", e.g. "BTN:raise2.5/BB:call": everyone whose turn
  /// comes before the named position folds. The flop is dealt as its three
  /// cards in one segment, e.g. ".../Ks7d2c".
  @$pb.TagNumber(2)
  $core.String get path => $_getSZ(1);
  @$pb.TagNumber(2)
  set path($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPath() => $_has(1);
  @$pb.TagNumber(2)
  void clearPath() => $_clearField(2);

  /// Most hands to list per player. 0: the server default, which is every
  /// hand for Hold'em and 2,000 for PLO4 (a PLO range can hold 270,725
  /// combos). The server never sends more than 20,000 per player. When a range
  /// holds more, the hands with the most reach at this node are sent, still in
  /// the range's own order; PlayerRange.total_hands tells how many there are.
  /// Action frequencies and range EVs always cover the whole range.
  @$pb.TagNumber(3)
  $core.int get maxHands => $_getIZ(2);
  @$pb.TagNumber(3)
  set maxHands($core.int value) => $_setUnsignedInt32(2, value);
  @$pb.TagNumber(3)
  $core.bool hasMaxHands() => $_has(2);
  @$pb.TagNumber(3)
  void clearMaxHands() => $_clearField(3);

  /// Individual hands, hand classes or both. Default: hands only.
  @$pb.TagNumber(4)
  HandDetail get detail => $_getN(3);
  @$pb.TagNumber(4)
  set detail(HandDetail value) => $_setField(4, value);
  @$pb.TagNumber(4)
  $core.bool hasDetail() => $_has(3);
  @$pb.TagNumber(4)
  void clearDetail() => $_clearField(4);
}

class NodeAction extends $pb.GeneratedMessage {
  factory NodeAction({
    ActionType? type,
    $core.double? amount,
    $core.String? label,
    $core.double? frequency,
  }) {
    final result = NodeAction._();
    if (type != null) result.type = type;
    if (amount != null) result.amount = amount;
    if (label != null) result.label = label;
    if (frequency != null) result.frequency = frequency;
    return result;
  }

  NodeAction._();

  factory NodeAction.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      NodeAction()..mergeFromBuffer(data, registry);
  factory NodeAction.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      NodeAction()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'NodeAction',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: NodeAction.$_createMessage)
    ..aE<ActionType>(1, _omitFieldNames ? '' : 'type',
        enumValues: ActionType.values)
    ..aD(2, _omitFieldNames ? '' : 'amount')
    ..aOS(3, _omitFieldNames ? '' : 'label')
    ..aD(4, _omitFieldNames ? '' : 'frequency')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  NodeAction clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  NodeAction copyWith(void Function(NodeAction) updates) =>
      super.copyWith((message) => updates(message as NodeAction)) as NodeAction;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use NodeAction() / NodeAction.new instead')
  static NodeAction create() => NodeAction._();
  static $pb.GeneratedMessage $_createMessage() => NodeAction._();
  @$core.override
  NodeAction createEmptyInstance() => NodeAction._();
  @$core.pragma('dart2js:noInline')
  static NodeAction getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<NodeAction>(NodeAction.$_createMessage);
  static NodeAction? _defaultInstance;

  @$pb.TagNumber(1)
  ActionType get type => $_getN(0);
  @$pb.TagNumber(1)
  set type(ActionType value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasType() => $_has(0);
  @$pb.TagNumber(1)
  void clearType() => $_clearField(1);

  /// Chips the player has put in on this street after the action. 0 for fold
  /// and check.
  @$pb.TagNumber(2)
  $core.double get amount => $_getN(1);
  @$pb.TagNumber(2)
  set amount($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasAmount() => $_has(1);
  @$pb.TagNumber(2)
  void clearAmount() => $_clearField(2);

  /// The name to use for this action in a path, e.g. "bet50".
  @$pb.TagNumber(3)
  $core.String get label => $_getSZ(2);
  @$pb.TagNumber(3)
  set label($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasLabel() => $_has(2);
  @$pb.TagNumber(3)
  void clearLabel() => $_clearField(3);

  /// How often the acting player takes this action over their whole range.
  @$pb.TagNumber(4)
  $core.double get frequency => $_getN(3);
  @$pb.TagNumber(4)
  set frequency($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasFrequency() => $_has(3);
  @$pb.TagNumber(4)
  void clearFrequency() => $_clearField(4);
}

/// One of the 169 Hold'em starting-hand classes ("AA", "AKs", "AKo") at a
/// node, summed over its combos.
class HandClass extends $pb.GeneratedMessage {
  factory HandClass({
    $core.String? name,
    $core.double? weight,
    $core.double? combos,
    $core.double? ev,
    $core.double? equity,
    $core.Iterable<$core.double>? strategy,
  }) {
    final result = HandClass._();
    if (name != null) result.name = name;
    if (weight != null) result.weight = weight;
    if (combos != null) result.combos = combos;
    if (ev != null) result.ev = ev;
    if (equity != null) result.equity = equity;
    if (strategy != null) result.strategy.addAll(strategy);
    return result;
  }

  HandClass._();

  factory HandClass.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      HandClass()..mergeFromBuffer(data, registry);
  factory HandClass.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      HandClass()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'HandClass',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: HandClass.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'name')
    ..aD(2, _omitFieldNames ? '' : 'weight', fieldType: $pb.PbFieldType.OF)
    ..aD(3, _omitFieldNames ? '' : 'combos', fieldType: $pb.PbFieldType.OF)
    ..aD(4, _omitFieldNames ? '' : 'ev', fieldType: $pb.PbFieldType.OF)
    ..aD(5, _omitFieldNames ? '' : 'equity', fieldType: $pb.PbFieldType.OF)
    ..p<$core.double>(6, _omitFieldNames ? '' : 'strategy', $pb.PbFieldType.KF)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  HandClass clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  HandClass copyWith(void Function(HandClass) updates) =>
      super.copyWith((message) => updates(message as HandClass)) as HandClass;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use HandClass() / HandClass.new instead')
  static HandClass create() => HandClass._();
  static $pb.GeneratedMessage $_createMessage() => HandClass._();
  @$core.override
  HandClass createEmptyInstance() => HandClass._();
  @$core.pragma('dart2js:noInline')
  static HandClass getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<HandClass>(HandClass.$_createMessage);
  static HandClass? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get name => $_getSZ(0);
  @$pb.TagNumber(1)
  set name($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasName() => $_has(0);
  @$pb.TagNumber(1)
  void clearName() => $_clearField(1);

  /// Average reach over the class's combos that the board allows: 1 when all
  /// of them get here with full weight. For a preflop range this is the
  /// class's weight in the range.
  @$pb.TagNumber(2)
  $core.double get weight => $_getN(1);
  @$pb.TagNumber(2)
  set weight($core.double value) => $_setFloat(1, value);
  @$pb.TagNumber(2)
  $core.bool hasWeight() => $_has(1);
  @$pb.TagNumber(2)
  void clearWeight() => $_clearField(2);

  /// Sum of the reach of its combos (how many combos of the class are here).
  @$pb.TagNumber(3)
  $core.double get combos => $_getN(2);
  @$pb.TagNumber(3)
  set combos($core.double value) => $_setFloat(2, value);
  @$pb.TagNumber(3)
  $core.bool hasCombos() => $_has(2);
  @$pb.TagNumber(3)
  void clearCombos() => $_clearField(3);

  /// Reach-weighted average EV and equity of its combos.
  @$pb.TagNumber(4)
  $core.double get ev => $_getN(3);
  @$pb.TagNumber(4)
  set ev($core.double value) => $_setFloat(3, value);
  @$pb.TagNumber(4)
  $core.bool hasEv() => $_has(3);
  @$pb.TagNumber(4)
  void clearEv() => $_clearField(4);

  @$pb.TagNumber(5)
  $core.double get equity => $_getN(4);
  @$pb.TagNumber(5)
  set equity($core.double value) => $_setFloat(4, value);
  @$pb.TagNumber(5)
  $core.bool hasEquity() => $_has(4);
  @$pb.TagNumber(5)
  void clearEquity() => $_clearField(5);

  /// Acting player at an action node: how often the class takes each action,
  /// reach-weighted, in the order of GetNodeResponse.actions. Empty
  /// otherwise.
  @$pb.TagNumber(6)
  $pb.PbList<$core.double> get strategy => $_getList(5);
}

/// One player's range at a node. The arrays are parallel.
class PlayerRange extends $pb.GeneratedMessage {
  factory PlayerRange({
    $core.Iterable<$core.String>? hands,
    $core.Iterable<$core.double>? reach,
    $core.Iterable<$core.double>? ev,
    $core.double? rangeEv,
    $core.Iterable<$core.double>? equity,
    $core.double? rangeEquity,
    $core.int? totalHands,
    $core.double? shownReachFraction,
    $core.Iterable<HandClass>? classes,
    $core.String? position,
  }) {
    final result = PlayerRange._();
    if (hands != null) result.hands.addAll(hands);
    if (reach != null) result.reach.addAll(reach);
    if (ev != null) result.ev.addAll(ev);
    if (rangeEv != null) result.rangeEv = rangeEv;
    if (equity != null) result.equity.addAll(equity);
    if (rangeEquity != null) result.rangeEquity = rangeEquity;
    if (totalHands != null) result.totalHands = totalHands;
    if (shownReachFraction != null)
      result.shownReachFraction = shownReachFraction;
    if (classes != null) result.classes.addAll(classes);
    if (position != null) result.position = position;
    return result;
  }

  PlayerRange._();

  factory PlayerRange.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PlayerRange()..mergeFromBuffer(data, registry);
  factory PlayerRange.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PlayerRange()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PlayerRange',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PlayerRange.$_createMessage)
    ..pPS(1, _omitFieldNames ? '' : 'hands')
    ..p<$core.double>(2, _omitFieldNames ? '' : 'reach', $pb.PbFieldType.KF)
    ..p<$core.double>(3, _omitFieldNames ? '' : 'ev', $pb.PbFieldType.KF)
    ..aD(4, _omitFieldNames ? '' : 'rangeEv')
    ..p<$core.double>(5, _omitFieldNames ? '' : 'equity', $pb.PbFieldType.KF)
    ..aD(6, _omitFieldNames ? '' : 'rangeEquity')
    ..aI(7, _omitFieldNames ? '' : 'totalHands', fieldType: $pb.PbFieldType.OU3)
    ..aD(8, _omitFieldNames ? '' : 'shownReachFraction')
    ..pPM<HandClass>(9, _omitFieldNames ? '' : 'classes',
        subBuilder: HandClass.$_createMessage)
    ..aOS(10, _omitFieldNames ? '' : 'position')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PlayerRange clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PlayerRange copyWith(void Function(PlayerRange) updates) =>
      super.copyWith((message) => updates(message as PlayerRange))
          as PlayerRange;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PlayerRange() / PlayerRange.new instead')
  static PlayerRange create() => PlayerRange._();
  static $pb.GeneratedMessage $_createMessage() => PlayerRange._();
  @$core.override
  PlayerRange createEmptyInstance() => PlayerRange._();
  @$core.pragma('dart2js:noInline')
  static PlayerRange getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PlayerRange>(
          PlayerRange.$_createMessage);
  static PlayerRange? _defaultInstance;

  /// Hands, higher card first: Hold'em as four characters, e.g. "AhKd"; PLO4
  /// as eight, cards ordered by rank, e.g. "AsKsQhJh".
  @$pb.TagNumber(1)
  $pb.PbList<$core.String> get hands => $_getList(0);

  /// How much of each hand gets here: its starting weight times the
  /// probability of the player's own actions on the way. 0 for hands that a
  /// dealt card makes impossible.
  @$pb.TagNumber(2)
  $pb.PbList<$core.double> get reach => $_getList(1);

  /// Expected value of each hand, in chips.
  @$pb.TagNumber(3)
  $pb.PbList<$core.double> get ev => $_getList(2);

  /// Reach-weighted average of `ev`.
  @$pb.TagNumber(4)
  $core.double get rangeEv => $_getN(3);
  @$pb.TagNumber(4)
  set rangeEv($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasRangeEv() => $_has(3);
  @$pb.TagNumber(4)
  void clearRangeEv() => $_clearField(4);

  /// Equity of each hand against the other player's range: its share of the
  /// pot, 0 to 1, if both checked or called to the end. Exact engine only:
  /// empty (and range_equity 0) for Monte Carlo solutions. Monte Carlo EVs
  /// are sampled (GetNodeResponse.ev_estimated); `ev` is empty for a player
  /// with more than 5,000 hands reaching the node (wide PLO ranges).
  @$pb.TagNumber(5)
  $pb.PbList<$core.double> get equity => $_getList(4);

  @$pb.TagNumber(6)
  $core.double get rangeEquity => $_getN(5);
  @$pb.TagNumber(6)
  set rangeEquity($core.double value) => $_setDouble(5, value);
  @$pb.TagNumber(6)
  $core.bool hasRangeEquity() => $_has(5);
  @$pb.TagNumber(6)
  void clearRangeEquity() => $_clearField(6);

  /// Hands in the whole range (possible on the starting board). More than
  /// hands_size() when the response was cut to GetNodeRequest.max_hands.
  @$pb.TagNumber(7)
  $core.int get totalHands => $_getIZ(6);
  @$pb.TagNumber(7)
  set totalHands($core.int value) => $_setUnsignedInt32(6, value);
  @$pb.TagNumber(7)
  $core.bool hasTotalHands() => $_has(6);
  @$pb.TagNumber(7)
  void clearTotalHands() => $_clearField(7);

  /// Share of the range's total reach at this node that the listed hands
  /// hold: 1 when nothing was left out.
  @$pb.TagNumber(8)
  $core.double get shownReachFraction => $_getN(7);
  @$pb.TagNumber(8)
  set shownReachFraction($core.double value) => $_setDouble(7, value);
  @$pb.TagNumber(8)
  $core.bool hasShownReachFraction() => $_has(7);
  @$pb.TagNumber(8)
  void clearShownReachFraction() => $_clearField(8);

  /// With HandDetail CLASSES or HANDS_AND_CLASSES (Hold'em): all 169 classes
  /// in 13x13 grid order: row by row from aces down, pairs on the diagonal,
  /// suited hands right of it, offsuit hands left of it ("AA", "AKs", "AQs",
  /// …, "AKo", "KK", "KQs", …).
  @$pb.TagNumber(9)
  $pb.PbList<HandClass> get classes => $_getList(8);

  /// The player's position name (Job.positions).
  @$pb.TagNumber(10)
  $core.String get position => $_getSZ(9);
  @$pb.TagNumber(10)
  set position($core.String value) => $_setString(9, value);
  @$pb.TagNumber(10)
  $core.bool hasPosition() => $_has(9);
  @$pb.TagNumber(10)
  void clearPosition() => $_clearField(10);
}

class GetRunoutReportRequest extends $pb.GeneratedMessage {
  factory GetRunoutReportRequest({
    $core.String? jobId,
    $core.String? path,
  }) {
    final result = GetRunoutReportRequest._();
    if (jobId != null) result.jobId = jobId;
    if (path != null) result.path = path;
    return result;
  }

  GetRunoutReportRequest._();

  factory GetRunoutReportRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetRunoutReportRequest()..mergeFromBuffer(data, registry);
  factory GetRunoutReportRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetRunoutReportRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetRunoutReportRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetRunoutReportRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'jobId')
    ..aOS(2, _omitFieldNames ? '' : 'path')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetRunoutReportRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetRunoutReportRequest copyWith(
          void Function(GetRunoutReportRequest) updates) =>
      super.copyWith((message) => updates(message as GetRunoutReportRequest))
          as GetRunoutReportRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use GetRunoutReportRequest() / GetRunoutReportRequest.new instead')
  static GetRunoutReportRequest create() => GetRunoutReportRequest._();
  static $pb.GeneratedMessage $_createMessage() => GetRunoutReportRequest._();
  @$core.override
  GetRunoutReportRequest createEmptyInstance() => GetRunoutReportRequest._();
  @$core.pragma('dart2js:noInline')
  static GetRunoutReportRequest getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<GetRunoutReportRequest>(
          GetRunoutReportRequest.$_createMessage);
  static GetRunoutReportRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get jobId => $_getSZ(0);
  @$pb.TagNumber(1)
  set jobId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasJobId() => $_has(0);
  @$pb.TagNumber(1)
  void clearJobId() => $_clearField(1);

  /// Path of the node where the card is dealt.
  @$pb.TagNumber(2)
  $core.String get path => $_getSZ(1);
  @$pb.TagNumber(2)
  set path($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPath() => $_has(1);
  @$pb.TagNumber(2)
  void clearPath() => $_clearField(2);
}

/// The spot right after one particular card.
class Runout extends $pb.GeneratedMessage {
  factory Runout({
    $core.String? card,
    $core.double? oopEv,
    $core.double? ipEv,
    $core.double? oopEquity,
    $core.double? ipEquity,
    NodeType? type,
    Player? player,
    $core.Iterable<NodeAction>? actions,
    $core.Iterable<$core.double>? evs,
    $core.Iterable<$core.double>? equities,
    $core.int? actingPlayer,
  }) {
    final result = Runout._();
    if (card != null) result.card = card;
    if (oopEv != null) result.oopEv = oopEv;
    if (ipEv != null) result.ipEv = ipEv;
    if (oopEquity != null) result.oopEquity = oopEquity;
    if (ipEquity != null) result.ipEquity = ipEquity;
    if (type != null) result.type = type;
    if (player != null) result.player = player;
    if (actions != null) result.actions.addAll(actions);
    if (evs != null) result.evs.addAll(evs);
    if (equities != null) result.equities.addAll(equities);
    if (actingPlayer != null) result.actingPlayer = actingPlayer;
    return result;
  }

  Runout._();

  factory Runout.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Runout()..mergeFromBuffer(data, registry);
  factory Runout.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Runout()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Runout',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: Runout.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'card')
    ..aD(2, _omitFieldNames ? '' : 'oopEv')
    ..aD(3, _omitFieldNames ? '' : 'ipEv')
    ..aD(4, _omitFieldNames ? '' : 'oopEquity')
    ..aD(5, _omitFieldNames ? '' : 'ipEquity')
    ..aE<NodeType>(6, _omitFieldNames ? '' : 'type',
        enumValues: NodeType.values)
    ..aE<Player>(7, _omitFieldNames ? '' : 'player', enumValues: Player.values)
    ..pPM<NodeAction>(8, _omitFieldNames ? '' : 'actions',
        subBuilder: NodeAction.$_createMessage)
    ..p<$core.double>(9, _omitFieldNames ? '' : 'evs', $pb.PbFieldType.KD)
    ..p<$core.double>(10, _omitFieldNames ? '' : 'equities', $pb.PbFieldType.KD)
    ..aI(11, _omitFieldNames ? '' : 'actingPlayer',
        fieldType: $pb.PbFieldType.OU3)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Runout clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Runout copyWith(void Function(Runout) updates) =>
      super.copyWith((message) => updates(message as Runout)) as Runout;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use Runout() / Runout.new instead')
  static Runout create() => Runout._();
  static $pb.GeneratedMessage $_createMessage() => Runout._();
  @$core.override
  Runout createEmptyInstance() => Runout._();
  @$core.pragma('dart2js:noInline')
  static Runout getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<Runout>(Runout.$_createMessage);
  static Runout? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get card => $_getSZ(0);
  @$pb.TagNumber(1)
  set card($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasCard() => $_has(0);
  @$pb.TagNumber(1)
  void clearCard() => $_clearField(1);

  @$pb.TagNumber(2)
  $core.double get oopEv => $_getN(1);
  @$pb.TagNumber(2)
  set oopEv($core.double value) => $_setDouble(1, value);
  @$pb.TagNumber(2)
  $core.bool hasOopEv() => $_has(1);
  @$pb.TagNumber(2)
  void clearOopEv() => $_clearField(2);

  @$pb.TagNumber(3)
  $core.double get ipEv => $_getN(2);
  @$pb.TagNumber(3)
  set ipEv($core.double value) => $_setDouble(2, value);
  @$pb.TagNumber(3)
  $core.bool hasIpEv() => $_has(2);
  @$pb.TagNumber(3)
  void clearIpEv() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.double get oopEquity => $_getN(3);
  @$pb.TagNumber(4)
  set oopEquity($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasOopEquity() => $_has(3);
  @$pb.TagNumber(4)
  void clearOopEquity() => $_clearField(4);

  @$pb.TagNumber(5)
  $core.double get ipEquity => $_getN(4);
  @$pb.TagNumber(5)
  set ipEquity($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasIpEquity() => $_has(4);
  @$pb.TagNumber(5)
  void clearIpEquity() => $_clearField(5);

  /// What follows the card: usually a decision, or a showdown or another
  /// card if the players are all-in.
  @$pb.TagNumber(6)
  NodeType get type => $_getN(5);
  @$pb.TagNumber(6)
  set type(NodeType value) => $_setField(6, value);
  @$pb.TagNumber(6)
  $core.bool hasType() => $_has(5);
  @$pb.TagNumber(6)
  void clearType() => $_clearField(6);

  /// If a decision follows: whose it is and what they do with their range.
  @$pb.TagNumber(7)
  Player get player => $_getN(6);
  @$pb.TagNumber(7)
  set player(Player value) => $_setField(7, value);
  @$pb.TagNumber(7)
  $core.bool hasPlayer() => $_has(6);
  @$pb.TagNumber(7)
  void clearPlayer() => $_clearField(7);

  @$pb.TagNumber(8)
  $pb.PbList<NodeAction> get actions => $_getList(7);

  /// Every player's range EV and equity in seat order (also filled heads-up,
  /// where they repeat the oop_/ip_ fields).
  @$pb.TagNumber(9)
  $pb.PbList<$core.double> get evs => $_getList(8);

  @$pb.TagNumber(10)
  $pb.PbList<$core.double> get equities => $_getList(9);

  /// If a decision follows: the acting player's index in seat order.
  @$pb.TagNumber(11)
  $core.int get actingPlayer => $_getIZ(10);
  @$pb.TagNumber(11)
  set actingPlayer($core.int value) => $_setUnsignedInt32(10, value);
  @$pb.TagNumber(11)
  $core.bool hasActingPlayer() => $_has(10);
  @$pb.TagNumber(11)
  void clearActingPlayer() => $_clearField(11);
}

class GetRunoutReportResponse extends $pb.GeneratedMessage {
  factory GetRunoutReportResponse({
    $core.Iterable<Runout>? runouts,
  }) {
    final result = GetRunoutReportResponse._();
    if (runouts != null) result.runouts.addAll(runouts);
    return result;
  }

  GetRunoutReportResponse._();

  factory GetRunoutReportResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetRunoutReportResponse()..mergeFromBuffer(data, registry);
  factory GetRunoutReportResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetRunoutReportResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetRunoutReportResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetRunoutReportResponse.$_createMessage)
    ..pPM<Runout>(1, _omitFieldNames ? '' : 'runouts',
        subBuilder: Runout.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetRunoutReportResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetRunoutReportResponse copyWith(
          void Function(GetRunoutReportResponse) updates) =>
      super.copyWith((message) => updates(message as GetRunoutReportResponse))
          as GetRunoutReportResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use GetRunoutReportResponse() / GetRunoutReportResponse.new instead')
  static GetRunoutReportResponse create() => GetRunoutReportResponse._();
  static $pb.GeneratedMessage $_createMessage() => GetRunoutReportResponse._();
  @$core.override
  GetRunoutReportResponse createEmptyInstance() => GetRunoutReportResponse._();
  @$core.pragma('dart2js:noInline')
  static GetRunoutReportResponse getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<GetRunoutReportResponse>(
          GetRunoutReportResponse.$_createMessage);
  static GetRunoutReportResponse? _defaultInstance;

  /// One entry per card that can come, in card order.
  @$pb.TagNumber(1)
  $pb.PbList<Runout> get runouts => $_getList(0);
}

class GetNodeResponse extends $pb.GeneratedMessage {
  factory GetNodeResponse({
    NodeType? type,
    Player? player,
    $core.String? path,
    $core.Iterable<NodeAction>? actions,
    $core.Iterable<$core.double>? strategy,
    $core.Iterable<$core.String>? nextCards,
    PlayerRange? oop,
    PlayerRange? ip,
    $core.double? pot,
    $core.double? oopCommitted,
    $core.double? ipCommitted,
    $core.int? iteration,
    $core.Iterable<$core.bool>? locked,
    $core.Iterable<PlayerRange>? players,
    $core.int? actingPlayer,
    $core.Iterable<$core.double>? committed,
    $core.Iterable<$core.bool>? folded,
    $core.Iterable<$core.bool>? allIn,
    $core.String? board,
    $core.bool? evEstimated,
  }) {
    final result = GetNodeResponse._();
    if (type != null) result.type = type;
    if (player != null) result.player = player;
    if (path != null) result.path = path;
    if (actions != null) result.actions.addAll(actions);
    if (strategy != null) result.strategy.addAll(strategy);
    if (nextCards != null) result.nextCards.addAll(nextCards);
    if (oop != null) result.oop = oop;
    if (ip != null) result.ip = ip;
    if (pot != null) result.pot = pot;
    if (oopCommitted != null) result.oopCommitted = oopCommitted;
    if (ipCommitted != null) result.ipCommitted = ipCommitted;
    if (iteration != null) result.iteration = iteration;
    if (locked != null) result.locked.addAll(locked);
    if (players != null) result.players.addAll(players);
    if (actingPlayer != null) result.actingPlayer = actingPlayer;
    if (committed != null) result.committed.addAll(committed);
    if (folded != null) result.folded.addAll(folded);
    if (allIn != null) result.allIn.addAll(allIn);
    if (board != null) result.board = board;
    if (evEstimated != null) result.evEstimated = evEstimated;
    return result;
  }

  GetNodeResponse._();

  factory GetNodeResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetNodeResponse()..mergeFromBuffer(data, registry);
  factory GetNodeResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetNodeResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetNodeResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetNodeResponse.$_createMessage)
    ..aE<NodeType>(1, _omitFieldNames ? '' : 'type',
        enumValues: NodeType.values)
    ..aE<Player>(2, _omitFieldNames ? '' : 'player', enumValues: Player.values)
    ..aOS(3, _omitFieldNames ? '' : 'path')
    ..pPM<NodeAction>(4, _omitFieldNames ? '' : 'actions',
        subBuilder: NodeAction.$_createMessage)
    ..p<$core.double>(5, _omitFieldNames ? '' : 'strategy', $pb.PbFieldType.KF)
    ..pPS(6, _omitFieldNames ? '' : 'nextCards')
    ..aOM<PlayerRange>(7, _omitFieldNames ? '' : 'oop',
        subBuilder: PlayerRange.$_createMessage)
    ..aOM<PlayerRange>(8, _omitFieldNames ? '' : 'ip',
        subBuilder: PlayerRange.$_createMessage)
    ..aD(9, _omitFieldNames ? '' : 'pot')
    ..aD(10, _omitFieldNames ? '' : 'oopCommitted')
    ..aD(11, _omitFieldNames ? '' : 'ipCommitted')
    ..aI(12, _omitFieldNames ? '' : 'iteration', fieldType: $pb.PbFieldType.OU3)
    ..p<$core.bool>(13, _omitFieldNames ? '' : 'locked', $pb.PbFieldType.KB)
    ..pPM<PlayerRange>(14, _omitFieldNames ? '' : 'players',
        subBuilder: PlayerRange.$_createMessage)
    ..aI(15, _omitFieldNames ? '' : 'actingPlayer',
        fieldType: $pb.PbFieldType.OU3)
    ..p<$core.double>(
        16, _omitFieldNames ? '' : 'committed', $pb.PbFieldType.KD)
    ..p<$core.bool>(17, _omitFieldNames ? '' : 'folded', $pb.PbFieldType.KB)
    ..p<$core.bool>(18, _omitFieldNames ? '' : 'allIn', $pb.PbFieldType.KB)
    ..aOS(19, _omitFieldNames ? '' : 'board')
    ..aOB(20, _omitFieldNames ? '' : 'evEstimated')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetNodeResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetNodeResponse copyWith(void Function(GetNodeResponse) updates) =>
      super.copyWith((message) => updates(message as GetNodeResponse))
          as GetNodeResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use GetNodeResponse() / GetNodeResponse.new instead')
  static GetNodeResponse create() => GetNodeResponse._();
  static $pb.GeneratedMessage $_createMessage() => GetNodeResponse._();
  @$core.override
  GetNodeResponse createEmptyInstance() => GetNodeResponse._();
  @$core.pragma('dart2js:noInline')
  static GetNodeResponse getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GetNodeResponse>(
          GetNodeResponse.$_createMessage);
  static GetNodeResponse? _defaultInstance;

  @$pb.TagNumber(1)
  NodeType get type => $_getN(0);
  @$pb.TagNumber(1)
  set type(NodeType value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasType() => $_has(0);
  @$pb.TagNumber(1)
  void clearType() => $_clearField(1);

  /// Action node: who acts. Fold node: who folded. Two players only; see
  /// acting_player.
  @$pb.TagNumber(2)
  Player get player => $_getN(1);
  @$pb.TagNumber(2)
  set player(Player value) => $_setField(2, value);
  @$pb.TagNumber(2)
  $core.bool hasPlayer() => $_has(1);
  @$pb.TagNumber(2)
  void clearPlayer() => $_clearField(2);

  /// The path of this node as the client gave it.
  @$pb.TagNumber(3)
  $core.String get path => $_getSZ(2);
  @$pb.TagNumber(3)
  set path($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasPath() => $_has(2);
  @$pb.TagNumber(3)
  void clearPath() => $_clearField(3);

  /// Action nodes: what the player can do.
  @$pb.TagNumber(4)
  $pb.PbList<NodeAction> get actions => $_getList(3);

  /// Action nodes: the acting player's strategy, one value per action and
  /// hand: strategy[action * hand_count + hand], with hands in the order of
  /// that player's PlayerRange (as sent, after any max_hands cut).
  @$pb.TagNumber(5)
  $pb.PbList<$core.double> get strategy => $_getList(4);

  /// Chance nodes: the cards that can come. Where a whole flop is dealt
  /// (preflop solves), the next path entry names three of them at once.
  @$pb.TagNumber(6)
  $pb.PbList<$core.String> get nextCards => $_getList(5);

  /// Two players: the same as players[0] and players[1].
  @$pb.TagNumber(7)
  PlayerRange get oop => $_getN(6);
  @$pb.TagNumber(7)
  set oop(PlayerRange value) => $_setField(7, value);
  @$pb.TagNumber(7)
  $core.bool hasOop() => $_has(6);
  @$pb.TagNumber(7)
  void clearOop() => $_clearField(7);
  @$pb.TagNumber(7)
  PlayerRange ensureOop() => $_ensure(6);

  @$pb.TagNumber(8)
  PlayerRange get ip => $_getN(7);
  @$pb.TagNumber(8)
  set ip(PlayerRange value) => $_setField(8, value);
  @$pb.TagNumber(8)
  $core.bool hasIp() => $_has(7);
  @$pb.TagNumber(8)
  void clearIp() => $_clearField(8);
  @$pb.TagNumber(8)
  PlayerRange ensureIp() => $_ensure(7);

  /// Pot at this node, including bets made since the start of the spot.
  @$pb.TagNumber(9)
  $core.double get pot => $_getN(8);
  @$pb.TagNumber(9)
  set pot($core.double value) => $_setDouble(8, value);
  @$pb.TagNumber(9)
  $core.bool hasPot() => $_has(8);
  @$pb.TagNumber(9)
  void clearPot() => $_clearField(9);

  /// Two players: chips each has put in since the start of the spot.
  @$pb.TagNumber(10)
  $core.double get oopCommitted => $_getN(9);
  @$pb.TagNumber(10)
  set oopCommitted($core.double value) => $_setDouble(9, value);
  @$pb.TagNumber(10)
  $core.bool hasOopCommitted() => $_has(9);
  @$pb.TagNumber(10)
  void clearOopCommitted() => $_clearField(10);

  @$pb.TagNumber(11)
  $core.double get ipCommitted => $_getN(10);
  @$pb.TagNumber(11)
  set ipCommitted($core.double value) => $_setDouble(10, value);
  @$pb.TagNumber(11)
  $core.bool hasIpCommitted() => $_has(10);
  @$pb.TagNumber(11)
  void clearIpCommitted() => $_clearField(11);

  /// The job's iteration count when this was read.
  @$pb.TagNumber(12)
  $core.int get iteration => $_getIZ(11);
  @$pb.TagNumber(12)
  set iteration($core.int value) => $_setUnsignedInt32(11, value);
  @$pb.TagNumber(12)
  $core.bool hasIteration() => $_has(11);
  @$pb.TagNumber(12)
  void clearIteration() => $_clearField(12);

  /// Action nodes: for each hand of the acting player (as sent), whether its
  /// strategy here is locked.
  @$pb.TagNumber(13)
  $pb.PbList<$core.bool> get locked => $_getList(12);

  /// Every player in seat order (Job.positions). Always filled.
  @$pb.TagNumber(14)
  $pb.PbList<PlayerRange> get players => $_getList(13);

  /// Action node: index of the acting player in `players`. Fold node: the
  /// player who folded.
  @$pb.TagNumber(15)
  $core.int get actingPlayer => $_getIZ(14);
  @$pb.TagNumber(15)
  set actingPlayer($core.int value) => $_setUnsignedInt32(14, value);
  @$pb.TagNumber(15)
  $core.bool hasActingPlayer() => $_has(14);
  @$pb.TagNumber(15)
  void clearActingPlayer() => $_clearField(15);

  /// Per player in seat order: chips put in since the start of the spot
  /// (preflop solves: including the blinds posted),
  /// whether they have folded, whether they are all-in.
  @$pb.TagNumber(16)
  $pb.PbList<$core.double> get committed => $_getList(15);

  @$pb.TagNumber(17)
  $pb.PbList<$core.bool> get folded => $_getList(16);

  @$pb.TagNumber(18)
  $pb.PbList<$core.bool> get allIn => $_getList(17);

  /// The board at this node: the spot's board plus the cards dealt on the
  /// way, e.g. "Ks Td 7h 2c". Empty preflop.
  @$pb.TagNumber(19)
  $core.String get board => $_getSZ(18);
  @$pb.TagNumber(19)
  set board($core.String value) => $_setString(18, value);
  @$pb.TagNumber(19)
  $core.bool hasBoard() => $_has(18);
  @$pb.TagNumber(19)
  void clearBoard() => $_clearField(19);

  /// EVs and equities are sampled estimates (Monte Carlo engine).
  @$pb.TagNumber(20)
  $core.bool get evEstimated => $_getBF(19);
  @$pb.TagNumber(20)
  set evEstimated($core.bool value) => $_setBool(19, value);
  @$pb.TagNumber(20)
  $core.bool hasEvEstimated() => $_has(19);
  @$pb.TagNumber(20)
  void clearEvEstimated() => $_clearField(20);
}

class ListPreflopScenariosRequest extends $pb.GeneratedMessage {
  factory ListPreflopScenariosRequest({
    $core.String? jobId,
    $core.int? maxRaises,
  }) {
    final result = ListPreflopScenariosRequest._();
    if (jobId != null) result.jobId = jobId;
    if (maxRaises != null) result.maxRaises = maxRaises;
    return result;
  }

  ListPreflopScenariosRequest._();

  factory ListPreflopScenariosRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListPreflopScenariosRequest()..mergeFromBuffer(data, registry);
  factory ListPreflopScenariosRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListPreflopScenariosRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ListPreflopScenariosRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ListPreflopScenariosRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'jobId')
    ..aI(2, _omitFieldNames ? '' : 'maxRaises', fieldType: $pb.PbFieldType.OU3)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListPreflopScenariosRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListPreflopScenariosRequest copyWith(
          void Function(ListPreflopScenariosRequest) updates) =>
      super.copyWith(
              (message) => updates(message as ListPreflopScenariosRequest))
          as ListPreflopScenariosRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use ListPreflopScenariosRequest() / ListPreflopScenariosRequest.new instead')
  static ListPreflopScenariosRequest create() =>
      ListPreflopScenariosRequest._();
  static $pb.GeneratedMessage $_createMessage() =>
      ListPreflopScenariosRequest._();
  @$core.override
  ListPreflopScenariosRequest createEmptyInstance() =>
      ListPreflopScenariosRequest._();
  @$core.pragma('dart2js:noInline')
  static ListPreflopScenariosRequest getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<ListPreflopScenariosRequest>(
          ListPreflopScenariosRequest.$_createMessage);
  static ListPreflopScenariosRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get jobId => $_getSZ(0);
  @$pb.TagNumber(1)
  set jobId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasJobId() => $_has(0);
  @$pb.TagNumber(1)
  void clearJobId() => $_clearField(1);

  /// Deepest pots to list: 2 = up to single-raised pots, 3 = up to 3-bet
  /// pots, 4 = up to 4-bet pots (default), 5 or more = everything.
  @$pb.TagNumber(2)
  $core.int get maxRaises => $_getIZ(1);
  @$pb.TagNumber(2)
  set maxRaises($core.int value) => $_setUnsignedInt32(1, value);
  @$pb.TagNumber(2)
  $core.bool hasMaxRaises() => $_has(1);
  @$pb.TagNumber(2)
  void clearMaxRaises() => $_clearField(2);
}

/// A preflop line that ends with the flop being dealt.
class PreflopScenario extends $pb.GeneratedMessage {
  factory PreflopScenario({
    $core.String? name,
    $core.String? path,
    PreflopScenarioKind? kind,
    $core.Iterable<$core.String>? positions,
    $core.String? aggressor,
    $core.double? pot,
    $core.double? effectiveStack,
    $core.double? frequency,
  }) {
    final result = PreflopScenario._();
    if (name != null) result.name = name;
    if (path != null) result.path = path;
    if (kind != null) result.kind = kind;
    if (positions != null) result.positions.addAll(positions);
    if (aggressor != null) result.aggressor = aggressor;
    if (pot != null) result.pot = pot;
    if (effectiveStack != null) result.effectiveStack = effectiveStack;
    if (frequency != null) result.frequency = frequency;
    return result;
  }

  PreflopScenario._();

  factory PreflopScenario.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopScenario()..mergeFromBuffer(data, registry);
  factory PreflopScenario.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopScenario()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PreflopScenario',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PreflopScenario.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'name')
    ..aOS(2, _omitFieldNames ? '' : 'path')
    ..aE<PreflopScenarioKind>(3, _omitFieldNames ? '' : 'kind',
        enumValues: PreflopScenarioKind.values)
    ..pPS(4, _omitFieldNames ? '' : 'positions')
    ..aOS(5, _omitFieldNames ? '' : 'aggressor')
    ..aD(6, _omitFieldNames ? '' : 'pot')
    ..aD(7, _omitFieldNames ? '' : 'effectiveStack')
    ..aD(8, _omitFieldNames ? '' : 'frequency')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopScenario clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopScenario copyWith(void Function(PreflopScenario) updates) =>
      super.copyWith((message) => updates(message as PreflopScenario))
          as PreflopScenario;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PreflopScenario() / PreflopScenario.new instead')
  static PreflopScenario create() => PreflopScenario._();
  static $pb.GeneratedMessage $_createMessage() => PreflopScenario._();
  @$core.override
  PreflopScenario createEmptyInstance() => PreflopScenario._();
  @$core.pragma('dart2js:noInline')
  static PreflopScenario getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PreflopScenario>(
          PreflopScenario.$_createMessage);
  static PreflopScenario? _defaultInstance;

  /// Readable name, e.g. "BTN open, BB call" or "CO open, BTN 3-bet, CO call".
  @$pb.TagNumber(1)
  $core.String get name => $_getSZ(0);
  @$pb.TagNumber(1)
  set name($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasName() => $_has(0);
  @$pb.TagNumber(1)
  void clearName() => $_clearField(1);

  /// The line in the positional form, e.g. "BTN:raise2.5/BB:call". Works as
  /// GetNodeRequest.path and GetPreflopRangesRequest.path.
  @$pb.TagNumber(2)
  $core.String get path => $_getSZ(1);
  @$pb.TagNumber(2)
  set path($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPath() => $_has(1);
  @$pb.TagNumber(2)
  void clearPath() => $_clearField(2);

  @$pb.TagNumber(3)
  PreflopScenarioKind get kind => $_getN(2);
  @$pb.TagNumber(3)
  set kind(PreflopScenarioKind value) => $_setField(3, value);
  @$pb.TagNumber(3)
  $core.bool hasKind() => $_has(2);
  @$pb.TagNumber(3)
  void clearKind() => $_clearField(3);

  /// Players who see the flop, in seat order.
  @$pb.TagNumber(4)
  $pb.PbList<$core.String> get positions => $_getList(3);

  /// Position of the last raiser; empty in a limped pot.
  @$pb.TagNumber(5)
  $core.String get aggressor => $_getSZ(4);
  @$pb.TagNumber(5)
  set aggressor($core.String value) => $_setString(4, value);
  @$pb.TagNumber(5)
  $core.bool hasAggressor() => $_has(4);
  @$pb.TagNumber(5)
  void clearAggressor() => $_clearField(5);

  /// Pot, and the smallest stack behind among the players left, in big
  /// blinds.
  @$pb.TagNumber(6)
  $core.double get pot => $_getN(5);
  @$pb.TagNumber(6)
  set pot($core.double value) => $_setDouble(5, value);
  @$pb.TagNumber(6)
  $core.bool hasPot() => $_has(5);
  @$pb.TagNumber(6)
  void clearPot() => $_clearField(6);

  @$pb.TagNumber(7)
  $core.double get effectiveStack => $_getN(6);
  @$pb.TagNumber(7)
  set effectiveStack($core.double value) => $_setDouble(6, value);
  @$pb.TagNumber(7)
  $core.bool hasEffectiveStack() => $_has(6);
  @$pb.TagNumber(7)
  void clearEffectiveStack() => $_clearField(7);

  /// How often the line is played under the solved strategies, over all
  /// deals (0 to 1).
  @$pb.TagNumber(8)
  $core.double get frequency => $_getN(7);
  @$pb.TagNumber(8)
  set frequency($core.double value) => $_setDouble(7, value);
  @$pb.TagNumber(8)
  $core.bool hasFrequency() => $_has(7);
  @$pb.TagNumber(8)
  void clearFrequency() => $_clearField(8);
}

class ListPreflopScenariosResponse extends $pb.GeneratedMessage {
  factory ListPreflopScenariosResponse({
    $core.Iterable<PreflopScenario>? scenarios,
  }) {
    final result = ListPreflopScenariosResponse._();
    if (scenarios != null) result.scenarios.addAll(scenarios);
    return result;
  }

  ListPreflopScenariosResponse._();

  factory ListPreflopScenariosResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListPreflopScenariosResponse()..mergeFromBuffer(data, registry);
  factory ListPreflopScenariosResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListPreflopScenariosResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ListPreflopScenariosResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ListPreflopScenariosResponse.$_createMessage)
    ..pPM<PreflopScenario>(1, _omitFieldNames ? '' : 'scenarios',
        subBuilder: PreflopScenario.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListPreflopScenariosResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListPreflopScenariosResponse copyWith(
          void Function(ListPreflopScenariosResponse) updates) =>
      super.copyWith(
              (message) => updates(message as ListPreflopScenariosResponse))
          as ListPreflopScenariosResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use ListPreflopScenariosResponse() / ListPreflopScenariosResponse.new instead')
  static ListPreflopScenariosResponse create() =>
      ListPreflopScenariosResponse._();
  static $pb.GeneratedMessage $_createMessage() =>
      ListPreflopScenariosResponse._();
  @$core.override
  ListPreflopScenariosResponse createEmptyInstance() =>
      ListPreflopScenariosResponse._();
  @$core.pragma('dart2js:noInline')
  static ListPreflopScenariosResponse getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<ListPreflopScenariosResponse>(
          ListPreflopScenariosResponse.$_createMessage);
  static ListPreflopScenariosResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $pb.PbList<PreflopScenario> get scenarios => $_getList(0);
}

class GetPreflopRangesRequest extends $pb.GeneratedMessage {
  factory GetPreflopRangesRequest({
    $core.String? jobId,
    $core.String? path,
  }) {
    final result = GetPreflopRangesRequest._();
    if (jobId != null) result.jobId = jobId;
    if (path != null) result.path = path;
    return result;
  }

  GetPreflopRangesRequest._();

  factory GetPreflopRangesRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetPreflopRangesRequest()..mergeFromBuffer(data, registry);
  factory GetPreflopRangesRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetPreflopRangesRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetPreflopRangesRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetPreflopRangesRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'jobId')
    ..aOS(2, _omitFieldNames ? '' : 'path')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetPreflopRangesRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetPreflopRangesRequest copyWith(
          void Function(GetPreflopRangesRequest) updates) =>
      super.copyWith((message) => updates(message as GetPreflopRangesRequest))
          as GetPreflopRangesRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use GetPreflopRangesRequest() / GetPreflopRangesRequest.new instead')
  static GetPreflopRangesRequest create() => GetPreflopRangesRequest._();
  static $pb.GeneratedMessage $_createMessage() => GetPreflopRangesRequest._();
  @$core.override
  GetPreflopRangesRequest createEmptyInstance() => GetPreflopRangesRequest._();
  @$core.pragma('dart2js:noInline')
  static GetPreflopRangesRequest getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<GetPreflopRangesRequest>(
          GetPreflopRangesRequest.$_createMessage);
  static GetPreflopRangesRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get jobId => $_getSZ(0);
  @$pb.TagNumber(1)
  set jobId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasJobId() => $_has(0);
  @$pb.TagNumber(1)
  void clearJobId() => $_clearField(1);

  /// A preflop line as in GetNodeRequest.path (either form), usually a
  /// PreflopScenario.path. Any preflop node works.
  @$pb.TagNumber(2)
  $core.String get path => $_getSZ(1);
  @$pb.TagNumber(2)
  set path($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPath() => $_has(1);
  @$pb.TagNumber(2)
  void clearPath() => $_clearField(2);
}

class PreflopRange extends $pb.GeneratedMessage {
  factory PreflopRange({
    $core.String? position,
    $core.Iterable<HandClass>? classes,
    $core.String? range,
    $core.double? committed,
    $core.double? stack,
  }) {
    final result = PreflopRange._();
    if (position != null) result.position = position;
    if (classes != null) result.classes.addAll(classes);
    if (range != null) result.range = range;
    if (committed != null) result.committed = committed;
    if (stack != null) result.stack = stack;
    return result;
  }

  PreflopRange._();

  factory PreflopRange.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopRange()..mergeFromBuffer(data, registry);
  factory PreflopRange.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PreflopRange()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PreflopRange',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PreflopRange.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'position')
    ..pPM<HandClass>(2, _omitFieldNames ? '' : 'classes',
        subBuilder: HandClass.$_createMessage)
    ..aOS(3, _omitFieldNames ? '' : 'range')
    ..aD(4, _omitFieldNames ? '' : 'committed')
    ..aD(5, _omitFieldNames ? '' : 'stack')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopRange clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PreflopRange copyWith(void Function(PreflopRange) updates) =>
      super.copyWith((message) => updates(message as PreflopRange))
          as PreflopRange;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PreflopRange() / PreflopRange.new instead')
  static PreflopRange create() => PreflopRange._();
  static $pb.GeneratedMessage $_createMessage() => PreflopRange._();
  @$core.override
  PreflopRange createEmptyInstance() => PreflopRange._();
  @$core.pragma('dart2js:noInline')
  static PreflopRange getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PreflopRange>(
          PreflopRange.$_createMessage);
  static PreflopRange? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get position => $_getSZ(0);
  @$pb.TagNumber(1)
  set position($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPosition() => $_has(0);
  @$pb.TagNumber(1)
  void clearPosition() => $_clearField(1);

  /// All 169 classes in grid order (see PlayerRange.classes). `weight` is the
  /// class's weight in the range: how often the player's own actions on the
  /// line are taken with it.
  @$pb.TagNumber(2)
  $pb.PbList<HandClass> get classes => $_getList(1);

  /// The same weights as range text ("AA, AKs:0.75, …"), ready for
  /// Spot.oop_range / Spot.players[].range.
  @$pb.TagNumber(3)
  $core.String get range => $_getSZ(2);
  @$pb.TagNumber(3)
  set range($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasRange() => $_has(2);
  @$pb.TagNumber(3)
  void clearRange() => $_clearField(3);

  /// Big blinds put in preflop, and behind.
  @$pb.TagNumber(4)
  $core.double get committed => $_getN(3);
  @$pb.TagNumber(4)
  set committed($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasCommitted() => $_has(3);
  @$pb.TagNumber(4)
  void clearCommitted() => $_clearField(4);

  @$pb.TagNumber(5)
  $core.double get stack => $_getN(4);
  @$pb.TagNumber(5)
  set stack($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasStack() => $_has(4);
  @$pb.TagNumber(5)
  void clearStack() => $_clearField(5);
}

class GetPreflopRangesResponse extends $pb.GeneratedMessage {
  factory GetPreflopRangesResponse({
    $core.String? path,
    $core.bool? complete,
    $core.Iterable<PreflopRange>? players,
    $core.double? pot,
    $core.double? effectiveStack,
    $1.Spot? spot,
    $core.int? iteration,
  }) {
    final result = GetPreflopRangesResponse._();
    if (path != null) result.path = path;
    if (complete != null) result.complete = complete;
    if (players != null) result.players.addAll(players);
    if (pot != null) result.pot = pot;
    if (effectiveStack != null) result.effectiveStack = effectiveStack;
    if (spot != null) result.spot = spot;
    if (iteration != null) result.iteration = iteration;
    return result;
  }

  GetPreflopRangesResponse._();

  factory GetPreflopRangesResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetPreflopRangesResponse()..mergeFromBuffer(data, registry);
  factory GetPreflopRangesResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetPreflopRangesResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetPreflopRangesResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetPreflopRangesResponse.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'path')
    ..aOB(2, _omitFieldNames ? '' : 'complete')
    ..pPM<PreflopRange>(3, _omitFieldNames ? '' : 'players',
        subBuilder: PreflopRange.$_createMessage)
    ..aD(4, _omitFieldNames ? '' : 'pot')
    ..aD(5, _omitFieldNames ? '' : 'effectiveStack')
    ..aOM<$1.Spot>(6, _omitFieldNames ? '' : 'spot',
        subBuilder: $1.Spot.$_createMessage)
    ..aI(7, _omitFieldNames ? '' : 'iteration', fieldType: $pb.PbFieldType.OU3)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetPreflopRangesResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetPreflopRangesResponse copyWith(
          void Function(GetPreflopRangesResponse) updates) =>
      super.copyWith((message) => updates(message as GetPreflopRangesResponse))
          as GetPreflopRangesResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use GetPreflopRangesResponse() / GetPreflopRangesResponse.new instead')
  static GetPreflopRangesResponse create() => GetPreflopRangesResponse._();
  static $pb.GeneratedMessage $_createMessage() => GetPreflopRangesResponse._();
  @$core.override
  GetPreflopRangesResponse createEmptyInstance() =>
      GetPreflopRangesResponse._();
  @$core.pragma('dart2js:noInline')
  static GetPreflopRangesResponse getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<GetPreflopRangesResponse>(
          GetPreflopRangesResponse.$_createMessage);
  static GetPreflopRangesResponse? _defaultInstance;

  /// The line in the full form, every action spelled out.
  @$pb.TagNumber(1)
  $core.String get path => $_getSZ(0);
  @$pb.TagNumber(1)
  set path($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPath() => $_has(0);
  @$pb.TagNumber(1)
  void clearPath() => $_clearField(1);

  /// The line ends where the flop is dealt: the preflop betting is over with
  /// two or more players left.
  @$pb.TagNumber(2)
  $core.bool get complete => $_getBF(1);
  @$pb.TagNumber(2)
  set complete($core.bool value) => $_setBool(1, value);
  @$pb.TagNumber(2)
  $core.bool hasComplete() => $_has(1);
  @$pb.TagNumber(2)
  void clearComplete() => $_clearField(2);

  /// Every player who has not folded, in seat order.
  @$pb.TagNumber(3)
  $pb.PbList<PreflopRange> get players => $_getList(2);

  @$pb.TagNumber(4)
  $core.double get pot => $_getN(3);
  @$pb.TagNumber(4)
  set pot($core.double value) => $_setDouble(3, value);
  @$pb.TagNumber(4)
  $core.bool hasPot() => $_has(3);
  @$pb.TagNumber(4)
  void clearPot() => $_clearField(4);

  /// Smallest stack behind among the players left.
  @$pb.TagNumber(5)
  $core.double get effectiveStack => $_getN(4);
  @$pb.TagNumber(5)
  set effectiveStack($core.double value) => $_setDouble(4, value);
  @$pb.TagNumber(5)
  $core.bool hasEffectiveStack() => $_has(4);
  @$pb.TagNumber(5)
  void clearEffectiveStack() => $_clearField(5);

  /// A postflop spot for this line, ready to submit once the client sets the
  /// board and the tree: variant, pot, stacks, ranges and positions filled in
  /// (oop_range / ip_range / effective_stack for two players, `players` for
  /// more), rake copied from the preflop spot.
  @$pb.TagNumber(6)
  $1.Spot get spot => $_getN(5);
  @$pb.TagNumber(6)
  set spot($1.Spot value) => $_setField(6, value);
  @$pb.TagNumber(6)
  $core.bool hasSpot() => $_has(5);
  @$pb.TagNumber(6)
  void clearSpot() => $_clearField(6);
  @$pb.TagNumber(6)
  $1.Spot ensureSpot() => $_ensure(5);

  /// The job's iteration count when this was read.
  @$pb.TagNumber(7)
  $core.int get iteration => $_getIZ(6);
  @$pb.TagNumber(7)
  set iteration($core.int value) => $_setUnsignedInt32(6, value);
  @$pb.TagNumber(7)
  $core.bool hasIteration() => $_has(6);
  @$pb.TagNumber(7)
  void clearIteration() => $_clearField(7);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
