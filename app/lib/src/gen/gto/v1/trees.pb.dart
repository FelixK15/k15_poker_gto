// This is a generated file - do not edit.
//
// Generated from gto/v1/trees.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

import 'common.pb.dart' as $1;

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

class EstimateTreeRequest extends $pb.GeneratedMessage {
  factory EstimateTreeRequest({
    $1.Spot? spot,
  }) {
    final result = EstimateTreeRequest._();
    if (spot != null) result.spot = spot;
    return result;
  }

  EstimateTreeRequest._();

  factory EstimateTreeRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      EstimateTreeRequest()..mergeFromBuffer(data, registry);
  factory EstimateTreeRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      EstimateTreeRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'EstimateTreeRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: EstimateTreeRequest.$_createMessage)
    ..aOM<$1.Spot>(1, _omitFieldNames ? '' : 'spot',
        subBuilder: $1.Spot.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  EstimateTreeRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  EstimateTreeRequest copyWith(void Function(EstimateTreeRequest) updates) =>
      super.copyWith((message) => updates(message as EstimateTreeRequest))
          as EstimateTreeRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core
      .Deprecated('Use EstimateTreeRequest() / EstimateTreeRequest.new instead')
  static EstimateTreeRequest create() => EstimateTreeRequest._();
  static $pb.GeneratedMessage $_createMessage() => EstimateTreeRequest._();
  @$core.override
  EstimateTreeRequest createEmptyInstance() => EstimateTreeRequest._();
  @$core.pragma('dart2js:noInline')
  static EstimateTreeRequest getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<EstimateTreeRequest>(
          EstimateTreeRequest.$_createMessage);
  static EstimateTreeRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $1.Spot get spot => $_getN(0);
  @$pb.TagNumber(1)
  set spot($1.Spot value) => $_setField(1, value);
  @$pb.TagNumber(1)
  $core.bool hasSpot() => $_has(0);
  @$pb.TagNumber(1)
  void clearSpot() => $_clearField(1);
  @$pb.TagNumber(1)
  $1.Spot ensureSpot() => $_ensure(0);
}

class EstimateTreeResponse extends $pb.GeneratedMessage {
  factory EstimateTreeResponse({
    $fixnum.Int64? nodes,
    $fixnum.Int64? decisionNodes,
    $core.int? oopHands,
    $core.int? ipHands,
    $fixnum.Int64? strategyBytes,
    $fixnum.Int64? treeBytes,
    $core.bool? tooLarge,
    $core.bool? canSolve,
    $fixnum.Int64? memoryBudgetBytes,
  }) {
    final result = EstimateTreeResponse._();
    if (nodes != null) result.nodes = nodes;
    if (decisionNodes != null) result.decisionNodes = decisionNodes;
    if (oopHands != null) result.oopHands = oopHands;
    if (ipHands != null) result.ipHands = ipHands;
    if (strategyBytes != null) result.strategyBytes = strategyBytes;
    if (treeBytes != null) result.treeBytes = treeBytes;
    if (tooLarge != null) result.tooLarge = tooLarge;
    if (canSolve != null) result.canSolve = canSolve;
    if (memoryBudgetBytes != null) result.memoryBudgetBytes = memoryBudgetBytes;
    return result;
  }

  EstimateTreeResponse._();

  factory EstimateTreeResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      EstimateTreeResponse()..mergeFromBuffer(data, registry);
  factory EstimateTreeResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      EstimateTreeResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'EstimateTreeResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: EstimateTreeResponse.$_createMessage)
    ..a<$fixnum.Int64>(1, _omitFieldNames ? '' : 'nodes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        2, _omitFieldNames ? '' : 'decisionNodes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..aI(3, _omitFieldNames ? '' : 'oopHands', fieldType: $pb.PbFieldType.OU3)
    ..aI(4, _omitFieldNames ? '' : 'ipHands', fieldType: $pb.PbFieldType.OU3)
    ..a<$fixnum.Int64>(
        5, _omitFieldNames ? '' : 'strategyBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        6, _omitFieldNames ? '' : 'treeBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..aOB(7, _omitFieldNames ? '' : 'tooLarge')
    ..aOB(8, _omitFieldNames ? '' : 'canSolve')
    ..a<$fixnum.Int64>(
        9, _omitFieldNames ? '' : 'memoryBudgetBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  EstimateTreeResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  EstimateTreeResponse copyWith(void Function(EstimateTreeResponse) updates) =>
      super.copyWith((message) => updates(message as EstimateTreeResponse))
          as EstimateTreeResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated(
      'Use EstimateTreeResponse() / EstimateTreeResponse.new instead')
  static EstimateTreeResponse create() => EstimateTreeResponse._();
  static $pb.GeneratedMessage $_createMessage() => EstimateTreeResponse._();
  @$core.override
  EstimateTreeResponse createEmptyInstance() => EstimateTreeResponse._();
  @$core.pragma('dart2js:noInline')
  static EstimateTreeResponse getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<EstimateTreeResponse>(
          EstimateTreeResponse.$_createMessage);
  static EstimateTreeResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $fixnum.Int64 get nodes => $_getI64(0);
  @$pb.TagNumber(1)
  set nodes($fixnum.Int64 value) => $_setInt64(0, value);
  @$pb.TagNumber(1)
  $core.bool hasNodes() => $_has(0);
  @$pb.TagNumber(1)
  void clearNodes() => $_clearField(1);

  @$pb.TagNumber(2)
  $fixnum.Int64 get decisionNodes => $_getI64(1);
  @$pb.TagNumber(2)
  set decisionNodes($fixnum.Int64 value) => $_setInt64(1, value);
  @$pb.TagNumber(2)
  $core.bool hasDecisionNodes() => $_has(1);
  @$pb.TagNumber(2)
  void clearDecisionNodes() => $_clearField(2);

  /// Hands in each range that are possible on the board.
  @$pb.TagNumber(3)
  $core.int get oopHands => $_getIZ(2);
  @$pb.TagNumber(3)
  set oopHands($core.int value) => $_setUnsignedInt32(2, value);
  @$pb.TagNumber(3)
  $core.bool hasOopHands() => $_has(2);
  @$pb.TagNumber(3)
  void clearOopHands() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.int get ipHands => $_getIZ(3);
  @$pb.TagNumber(4)
  set ipHands($core.int value) => $_setUnsignedInt32(3, value);
  @$pb.TagNumber(4)
  $core.bool hasIpHands() => $_has(3);
  @$pb.TagNumber(4)
  void clearIpHands() => $_clearField(4);

  /// Memory for regrets and strategies, and for the tree itself.
  @$pb.TagNumber(5)
  $fixnum.Int64 get strategyBytes => $_getI64(4);
  @$pb.TagNumber(5)
  set strategyBytes($fixnum.Int64 value) => $_setInt64(4, value);
  @$pb.TagNumber(5)
  $core.bool hasStrategyBytes() => $_has(4);
  @$pb.TagNumber(5)
  void clearStrategyBytes() => $_clearField(5);

  @$pb.TagNumber(6)
  $fixnum.Int64 get treeBytes => $_getI64(5);
  @$pb.TagNumber(6)
  set treeBytes($fixnum.Int64 value) => $_setInt64(5, value);
  @$pb.TagNumber(6)
  $core.bool hasTreeBytes() => $_has(5);
  @$pb.TagNumber(6)
  void clearTreeBytes() => $_clearField(6);

  /// The tree exceeds the server's node limit. The numbers above are then
  /// lower bounds, counted up to the limit.
  @$pb.TagNumber(7)
  $core.bool get tooLarge => $_getBF(6);
  @$pb.TagNumber(7)
  set tooLarge($core.bool value) => $_setBool(6, value);
  @$pb.TagNumber(7)
  $core.bool hasTooLarge() => $_has(6);
  @$pb.TagNumber(7)
  void clearTooLarge() => $_clearField(7);

  /// Whether the server would accept the spot: within the node limit and
  /// within its memory budget.
  @$pb.TagNumber(8)
  $core.bool get canSolve => $_getBF(7);
  @$pb.TagNumber(8)
  set canSolve($core.bool value) => $_setBool(7, value);
  @$pb.TagNumber(8)
  $core.bool hasCanSolve() => $_has(7);
  @$pb.TagNumber(8)
  void clearCanSolve() => $_clearField(8);

  /// The server's memory budget for one solve.
  @$pb.TagNumber(9)
  $fixnum.Int64 get memoryBudgetBytes => $_getI64(8);
  @$pb.TagNumber(9)
  set memoryBudgetBytes($fixnum.Int64 value) => $_setInt64(8, value);
  @$pb.TagNumber(9)
  $core.bool hasMemoryBudgetBytes() => $_has(8);
  @$pb.TagNumber(9)
  void clearMemoryBudgetBytes() => $_clearField(9);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
