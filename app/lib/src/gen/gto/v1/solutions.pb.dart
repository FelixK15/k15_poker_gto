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

import 'solutions.pbenum.dart';

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

export 'solutions.pbenum.dart';

class GetNodeRequest extends $pb.GeneratedMessage {
  factory GetNodeRequest({
    $core.String? jobId,
    $core.String? path,
  }) {
    final result = GetNodeRequest._();
    if (jobId != null) result.jobId = jobId;
    if (path != null) result.path = path;
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
  @$pb.TagNumber(2)
  $core.String get path => $_getSZ(1);
  @$pb.TagNumber(2)
  set path($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasPath() => $_has(1);
  @$pb.TagNumber(2)
  void clearPath() => $_clearField(2);
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

/// One player's range at a node. The three arrays are parallel.
class PlayerRange extends $pb.GeneratedMessage {
  factory PlayerRange({
    $core.Iterable<$core.String>? hands,
    $core.Iterable<$core.double>? reach,
    $core.Iterable<$core.double>? ev,
    $core.double? rangeEv,
    $core.Iterable<$core.double>? equity,
    $core.double? rangeEquity,
  }) {
    final result = PlayerRange._();
    if (hands != null) result.hands.addAll(hands);
    if (reach != null) result.reach.addAll(reach);
    if (ev != null) result.ev.addAll(ev);
    if (rangeEv != null) result.rangeEv = rangeEv;
    if (equity != null) result.equity.addAll(equity);
    if (rangeEquity != null) result.rangeEquity = rangeEquity;
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

  /// Hands as four characters, higher card first, e.g. "AhKd".
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
  /// pot, 0 to 1, if both checked or called to the end.
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

  /// Action node: who acts. Fold node: who folded.
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
  /// that player's PlayerRange.
  @$pb.TagNumber(5)
  $pb.PbList<$core.double> get strategy => $_getList(4);

  /// Chance nodes: the cards that can come.
  @$pb.TagNumber(6)
  $pb.PbList<$core.String> get nextCards => $_getList(5);

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

  /// Chips each player has put in since the start of the spot.
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

  /// Action nodes: for each hand of the acting player, whether its strategy
  /// here is locked.
  @$pb.TagNumber(13)
  $pb.PbList<$core.bool> get locked => $_getList(12);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
