// This is a generated file - do not edit.
//
// Generated from gto/v1/server.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:core' as $core;

import 'package:fixnum/fixnum.dart' as $fixnum;
import 'package:protobuf/protobuf.dart' as $pb;

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

class GetInfoRequest extends $pb.GeneratedMessage {
  factory GetInfoRequest() => GetInfoRequest._();

  GetInfoRequest._();

  factory GetInfoRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetInfoRequest()..mergeFromBuffer(data, registry);
  factory GetInfoRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetInfoRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetInfoRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetInfoRequest.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetInfoRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetInfoRequest copyWith(void Function(GetInfoRequest) updates) =>
      super.copyWith((message) => updates(message as GetInfoRequest))
          as GetInfoRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use GetInfoRequest() / GetInfoRequest.new instead')
  static GetInfoRequest create() => GetInfoRequest._();
  static $pb.GeneratedMessage $_createMessage() => GetInfoRequest._();
  @$core.override
  GetInfoRequest createEmptyInstance() => GetInfoRequest._();
  @$core.pragma('dart2js:noInline')
  static GetInfoRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GetInfoRequest>(
          GetInfoRequest.$_createMessage);
  static GetInfoRequest? _defaultInstance;
}

class GetInfoResponse extends $pb.GeneratedMessage {
  factory GetInfoResponse({
    $core.String? serverVersion,
    $core.String? engineVersion,
    $core.int? apiMajor,
    $core.int? apiMinor,
    $core.String? os,
    $core.int? logicalCores,
    $fixnum.Int64? totalMemoryBytes,
    $fixnum.Int64? availableMemoryBytes,
  }) {
    final result = GetInfoResponse._();
    if (serverVersion != null) result.serverVersion = serverVersion;
    if (engineVersion != null) result.engineVersion = engineVersion;
    if (apiMajor != null) result.apiMajor = apiMajor;
    if (apiMinor != null) result.apiMinor = apiMinor;
    if (os != null) result.os = os;
    if (logicalCores != null) result.logicalCores = logicalCores;
    if (totalMemoryBytes != null) result.totalMemoryBytes = totalMemoryBytes;
    if (availableMemoryBytes != null)
      result.availableMemoryBytes = availableMemoryBytes;
    return result;
  }

  GetInfoResponse._();

  factory GetInfoResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetInfoResponse()..mergeFromBuffer(data, registry);
  factory GetInfoResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetInfoResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetInfoResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetInfoResponse.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'serverVersion')
    ..aOS(2, _omitFieldNames ? '' : 'engineVersion')
    ..aI(3, _omitFieldNames ? '' : 'apiMajor', fieldType: $pb.PbFieldType.OU3)
    ..aI(4, _omitFieldNames ? '' : 'apiMinor', fieldType: $pb.PbFieldType.OU3)
    ..aOS(5, _omitFieldNames ? '' : 'os')
    ..aI(6, _omitFieldNames ? '' : 'logicalCores',
        fieldType: $pb.PbFieldType.OU3)
    ..a<$fixnum.Int64>(
        7, _omitFieldNames ? '' : 'totalMemoryBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..a<$fixnum.Int64>(
        8, _omitFieldNames ? '' : 'availableMemoryBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetInfoResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetInfoResponse copyWith(void Function(GetInfoResponse) updates) =>
      super.copyWith((message) => updates(message as GetInfoResponse))
          as GetInfoResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use GetInfoResponse() / GetInfoResponse.new instead')
  static GetInfoResponse create() => GetInfoResponse._();
  static $pb.GeneratedMessage $_createMessage() => GetInfoResponse._();
  @$core.override
  GetInfoResponse createEmptyInstance() => GetInfoResponse._();
  @$core.pragma('dart2js:noInline')
  static GetInfoResponse getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GetInfoResponse>(
          GetInfoResponse.$_createMessage);
  static GetInfoResponse? _defaultInstance;

  /// Server release, e.g. "0.1.0".
  @$pb.TagNumber(1)
  $core.String get serverVersion => $_getSZ(0);
  @$pb.TagNumber(1)
  set serverVersion($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasServerVersion() => $_has(0);
  @$pb.TagNumber(1)
  void clearServerVersion() => $_clearField(1);

  /// Version of the solver engine library linked into the server.
  @$pb.TagNumber(2)
  $core.String get engineVersion => $_getSZ(1);
  @$pb.TagNumber(2)
  set engineVersion($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasEngineVersion() => $_has(1);
  @$pb.TagNumber(2)
  void clearEngineVersion() => $_clearField(2);

  /// API contract version. Clients refuse to talk to a different major version.
  @$pb.TagNumber(3)
  $core.int get apiMajor => $_getIZ(2);
  @$pb.TagNumber(3)
  set apiMajor($core.int value) => $_setUnsignedInt32(2, value);
  @$pb.TagNumber(3)
  $core.bool hasApiMajor() => $_has(2);
  @$pb.TagNumber(3)
  void clearApiMajor() => $_clearField(3);

  @$pb.TagNumber(4)
  $core.int get apiMinor => $_getIZ(3);
  @$pb.TagNumber(4)
  set apiMinor($core.int value) => $_setUnsignedInt32(3, value);
  @$pb.TagNumber(4)
  $core.bool hasApiMinor() => $_has(3);
  @$pb.TagNumber(4)
  void clearApiMinor() => $_clearField(4);

  /// "windows" or "linux".
  @$pb.TagNumber(5)
  $core.String get os => $_getSZ(4);
  @$pb.TagNumber(5)
  set os($core.String value) => $_setString(4, value);
  @$pb.TagNumber(5)
  $core.bool hasOs() => $_has(4);
  @$pb.TagNumber(5)
  void clearOs() => $_clearField(5);

  /// Logical CPU cores available to the solver.
  @$pb.TagNumber(6)
  $core.int get logicalCores => $_getIZ(5);
  @$pb.TagNumber(6)
  set logicalCores($core.int value) => $_setUnsignedInt32(5, value);
  @$pb.TagNumber(6)
  $core.bool hasLogicalCores() => $_has(5);
  @$pb.TagNumber(6)
  void clearLogicalCores() => $_clearField(6);

  /// Physical RAM of the machine, in bytes.
  @$pb.TagNumber(7)
  $fixnum.Int64 get totalMemoryBytes => $_getI64(6);
  @$pb.TagNumber(7)
  set totalMemoryBytes($fixnum.Int64 value) => $_setInt64(6, value);
  @$pb.TagNumber(7)
  $core.bool hasTotalMemoryBytes() => $_has(6);
  @$pb.TagNumber(7)
  void clearTotalMemoryBytes() => $_clearField(7);

  /// RAM currently available, in bytes.
  @$pb.TagNumber(8)
  $fixnum.Int64 get availableMemoryBytes => $_getI64(7);
  @$pb.TagNumber(8)
  set availableMemoryBytes($fixnum.Int64 value) => $_setInt64(7, value);
  @$pb.TagNumber(8)
  $core.bool hasAvailableMemoryBytes() => $_has(7);
  @$pb.TagNumber(8)
  void clearAvailableMemoryBytes() => $_clearField(8);
}

class PingRequest extends $pb.GeneratedMessage {
  factory PingRequest({
    $core.String? payload,
  }) {
    final result = PingRequest._();
    if (payload != null) result.payload = payload;
    return result;
  }

  PingRequest._();

  factory PingRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PingRequest()..mergeFromBuffer(data, registry);
  factory PingRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PingRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PingRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PingRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'payload')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PingRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PingRequest copyWith(void Function(PingRequest) updates) =>
      super.copyWith((message) => updates(message as PingRequest))
          as PingRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PingRequest() / PingRequest.new instead')
  static PingRequest create() => PingRequest._();
  static $pb.GeneratedMessage $_createMessage() => PingRequest._();
  @$core.override
  PingRequest createEmptyInstance() => PingRequest._();
  @$core.pragma('dart2js:noInline')
  static PingRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PingRequest>(
          PingRequest.$_createMessage);
  static PingRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get payload => $_getSZ(0);
  @$pb.TagNumber(1)
  set payload($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPayload() => $_has(0);
  @$pb.TagNumber(1)
  void clearPayload() => $_clearField(1);
}

class PingResponse extends $pb.GeneratedMessage {
  factory PingResponse({
    $core.String? payload,
  }) {
    final result = PingResponse._();
    if (payload != null) result.payload = payload;
    return result;
  }

  PingResponse._();

  factory PingResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PingResponse()..mergeFromBuffer(data, registry);
  factory PingResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PingResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PingResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PingResponse.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'payload')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PingResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PingResponse copyWith(void Function(PingResponse) updates) =>
      super.copyWith((message) => updates(message as PingResponse))
          as PingResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PingResponse() / PingResponse.new instead')
  static PingResponse create() => PingResponse._();
  static $pb.GeneratedMessage $_createMessage() => PingResponse._();
  @$core.override
  PingResponse createEmptyInstance() => PingResponse._();
  @$core.pragma('dart2js:noInline')
  static PingResponse getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PingResponse>(
          PingResponse.$_createMessage);
  static PingResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get payload => $_getSZ(0);
  @$pb.TagNumber(1)
  set payload($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasPayload() => $_has(0);
  @$pb.TagNumber(1)
  void clearPayload() => $_clearField(1);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
