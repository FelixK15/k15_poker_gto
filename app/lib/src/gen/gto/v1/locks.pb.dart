// This is a generated file - do not edit.
//
// Generated from gto/v1/locks.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:protobuf/protobuf.dart' as $pb;

import 'common.pb.dart' as $2;

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

class ResolveRequest extends $pb.GeneratedMessage {
  factory ResolveRequest({
    $core.String? parentJobId,
    $core.Iterable<$2.NodeLock>? locks,
    $2.SolveSettings? settings,
    $core.String? name,
  }) {
    final result = ResolveRequest._();
    if (parentJobId != null) result.parentJobId = parentJobId;
    if (locks != null) result.locks.addAll(locks);
    if (settings != null) result.settings = settings;
    if (name != null) result.name = name;
    return result;
  }

  ResolveRequest._();

  factory ResolveRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ResolveRequest()..mergeFromBuffer(data, registry);
  factory ResolveRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ResolveRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ResolveRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ResolveRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'parentJobId')
    ..pPM<$2.NodeLock>(2, _omitFieldNames ? '' : 'locks',
        subBuilder: $2.NodeLock.$_createMessage)
    ..aOM<$2.SolveSettings>(3, _omitFieldNames ? '' : 'settings',
        subBuilder: $2.SolveSettings.$_createMessage)
    ..aOS(4, _omitFieldNames ? '' : 'name')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ResolveRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ResolveRequest copyWith(void Function(ResolveRequest) updates) =>
      super.copyWith((message) => updates(message as ResolveRequest))
          as ResolveRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use ResolveRequest() / ResolveRequest.new instead')
  static ResolveRequest create() => ResolveRequest._();
  static $pb.GeneratedMessage $_createMessage() => ResolveRequest._();
  @$core.override
  ResolveRequest createEmptyInstance() => ResolveRequest._();
  @$core.pragma('dart2js:noInline')
  static ResolveRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ResolveRequest>(
          ResolveRequest.$_createMessage);
  static ResolveRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get parentJobId => $_getSZ(0);
  @$pb.TagNumber(1)
  set parentJobId($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasParentJobId() => $_has(0);
  @$pb.TagNumber(1)
  void clearParentJobId() => $_clearField(1);

  /// All locks the child is to have. To keep the parent's own locks, send
  /// them again together with the new ones.
  @$pb.TagNumber(2)
  $pb.PbList<$2.NodeLock> get locks => $_getList(1);

  /// max_iterations counts iterations on top of the parent's.
  @$pb.TagNumber(3)
  $2.SolveSettings get settings => $_getN(2);
  @$pb.TagNumber(3)
  set settings($2.SolveSettings value) => $_setField(3, value);
  @$pb.TagNumber(3)
  $core.bool hasSettings() => $_has(2);
  @$pb.TagNumber(3)
  void clearSettings() => $_clearField(3);
  @$pb.TagNumber(3)
  $2.SolveSettings ensureSettings() => $_ensure(2);

  /// Label for the new job; defaults to the parent's name with " (locked)".
  @$pb.TagNumber(4)
  $core.String get name => $_getSZ(3);
  @$pb.TagNumber(4)
  set name($core.String value) => $_setString(3, value);
  @$pb.TagNumber(4)
  $core.bool hasName() => $_has(3);
  @$pb.TagNumber(4)
  void clearName() => $_clearField(4);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
