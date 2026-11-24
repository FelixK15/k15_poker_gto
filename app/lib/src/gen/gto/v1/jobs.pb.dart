// This is a generated file - do not edit.
//
// Generated from gto/v1/jobs.proto.

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
import 'jobs.pbenum.dart';

export 'package:protobuf/protobuf.dart' show GeneratedMessageGenericExtensions;

export 'jobs.pbenum.dart';

class Job extends $pb.GeneratedMessage {
  factory Job({
    $core.String? id,
    $core.String? name,
    JobState? state,
    $1.Spot? spot,
    $1.SolveSettings? settings,
    $core.int? iteration,
    $core.bool? measured,
    $core.double? exploitabilityPct,
    $core.double? oopEv,
    $core.double? ipEv,
    $core.String? error,
    $fixnum.Int64? createdMs,
    $fixnum.Int64? finishedMs,
    $core.double? solveSeconds,
    $fixnum.Int64? memoryBytes,
    $core.bool? hasSolution,
    $core.String? parentId,
    $core.Iterable<$1.NodeLock>? locks,
    $core.bool? warmStartPending,
  }) {
    final result = Job._();
    if (id != null) result.id = id;
    if (name != null) result.name = name;
    if (state != null) result.state = state;
    if (spot != null) result.spot = spot;
    if (settings != null) result.settings = settings;
    if (iteration != null) result.iteration = iteration;
    if (measured != null) result.measured = measured;
    if (exploitabilityPct != null) result.exploitabilityPct = exploitabilityPct;
    if (oopEv != null) result.oopEv = oopEv;
    if (ipEv != null) result.ipEv = ipEv;
    if (error != null) result.error = error;
    if (createdMs != null) result.createdMs = createdMs;
    if (finishedMs != null) result.finishedMs = finishedMs;
    if (solveSeconds != null) result.solveSeconds = solveSeconds;
    if (memoryBytes != null) result.memoryBytes = memoryBytes;
    if (hasSolution != null) result.hasSolution = hasSolution;
    if (parentId != null) result.parentId = parentId;
    if (locks != null) result.locks.addAll(locks);
    if (warmStartPending != null) result.warmStartPending = warmStartPending;
    return result;
  }

  Job._();

  factory Job.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Job()..mergeFromBuffer(data, registry);
  factory Job.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      Job()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'Job',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: Job.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..aOS(2, _omitFieldNames ? '' : 'name')
    ..aE<JobState>(3, _omitFieldNames ? '' : 'state',
        enumValues: JobState.values)
    ..aOM<$1.Spot>(4, _omitFieldNames ? '' : 'spot',
        subBuilder: $1.Spot.$_createMessage)
    ..aOM<$1.SolveSettings>(5, _omitFieldNames ? '' : 'settings',
        subBuilder: $1.SolveSettings.$_createMessage)
    ..aI(6, _omitFieldNames ? '' : 'iteration', fieldType: $pb.PbFieldType.OU3)
    ..aOB(7, _omitFieldNames ? '' : 'measured')
    ..aD(8, _omitFieldNames ? '' : 'exploitabilityPct')
    ..aD(9, _omitFieldNames ? '' : 'oopEv')
    ..aD(10, _omitFieldNames ? '' : 'ipEv')
    ..aOS(11, _omitFieldNames ? '' : 'error')
    ..aInt64(12, _omitFieldNames ? '' : 'createdMs')
    ..aInt64(13, _omitFieldNames ? '' : 'finishedMs')
    ..aD(14, _omitFieldNames ? '' : 'solveSeconds')
    ..a<$fixnum.Int64>(
        15, _omitFieldNames ? '' : 'memoryBytes', $pb.PbFieldType.OU6,
        defaultOrMaker: $fixnum.Int64.ZERO)
    ..aOB(16, _omitFieldNames ? '' : 'hasSolution')
    ..aOS(17, _omitFieldNames ? '' : 'parentId')
    ..pPM<$1.NodeLock>(18, _omitFieldNames ? '' : 'locks',
        subBuilder: $1.NodeLock.$_createMessage)
    ..aOB(19, _omitFieldNames ? '' : 'warmStartPending')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Job clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  Job copyWith(void Function(Job) updates) =>
      super.copyWith((message) => updates(message as Job)) as Job;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use Job() / Job.new instead')
  static Job create() => Job._();
  static $pb.GeneratedMessage $_createMessage() => Job._();
  @$core.override
  Job createEmptyInstance() => Job._();
  @$core.pragma('dart2js:noInline')
  static Job getDefault() => _defaultInstance ??=
      $pb.GeneratedMessage.$_defaultFor<Job>(Job.$_createMessage);
  static Job? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);

  /// Free-text label chosen by the client.
  @$pb.TagNumber(2)
  $core.String get name => $_getSZ(1);
  @$pb.TagNumber(2)
  set name($core.String value) => $_setString(1, value);
  @$pb.TagNumber(2)
  $core.bool hasName() => $_has(1);
  @$pb.TagNumber(2)
  void clearName() => $_clearField(2);

  @$pb.TagNumber(3)
  JobState get state => $_getN(2);
  @$pb.TagNumber(3)
  set state(JobState value) => $_setField(3, value);
  @$pb.TagNumber(3)
  $core.bool hasState() => $_has(2);
  @$pb.TagNumber(3)
  void clearState() => $_clearField(3);

  @$pb.TagNumber(4)
  $1.Spot get spot => $_getN(3);
  @$pb.TagNumber(4)
  set spot($1.Spot value) => $_setField(4, value);
  @$pb.TagNumber(4)
  $core.bool hasSpot() => $_has(3);
  @$pb.TagNumber(4)
  void clearSpot() => $_clearField(4);
  @$pb.TagNumber(4)
  $1.Spot ensureSpot() => $_ensure(3);

  @$pb.TagNumber(5)
  $1.SolveSettings get settings => $_getN(4);
  @$pb.TagNumber(5)
  set settings($1.SolveSettings value) => $_setField(5, value);
  @$pb.TagNumber(5)
  $core.bool hasSettings() => $_has(4);
  @$pb.TagNumber(5)
  void clearSettings() => $_clearField(5);
  @$pb.TagNumber(5)
  $1.SolveSettings ensureSettings() => $_ensure(4);

  /// Progress. Exploitability and EVs are from the latest measurement, which
  /// happens every few iterations.
  @$pb.TagNumber(6)
  $core.int get iteration => $_getIZ(5);
  @$pb.TagNumber(6)
  set iteration($core.int value) => $_setUnsignedInt32(5, value);
  @$pb.TagNumber(6)
  $core.bool hasIteration() => $_has(5);
  @$pb.TagNumber(6)
  void clearIteration() => $_clearField(6);

  @$pb.TagNumber(7)
  $core.bool get measured => $_getBF(6);
  @$pb.TagNumber(7)
  set measured($core.bool value) => $_setBool(6, value);
  @$pb.TagNumber(7)
  $core.bool hasMeasured() => $_has(6);
  @$pb.TagNumber(7)
  void clearMeasured() => $_clearField(7);

  @$pb.TagNumber(8)
  $core.double get exploitabilityPct => $_getN(7);
  @$pb.TagNumber(8)
  set exploitabilityPct($core.double value) => $_setDouble(7, value);
  @$pb.TagNumber(8)
  $core.bool hasExploitabilityPct() => $_has(7);
  @$pb.TagNumber(8)
  void clearExploitabilityPct() => $_clearField(8);

  @$pb.TagNumber(9)
  $core.double get oopEv => $_getN(8);
  @$pb.TagNumber(9)
  set oopEv($core.double value) => $_setDouble(8, value);
  @$pb.TagNumber(9)
  $core.bool hasOopEv() => $_has(8);
  @$pb.TagNumber(9)
  void clearOopEv() => $_clearField(9);

  @$pb.TagNumber(10)
  $core.double get ipEv => $_getN(9);
  @$pb.TagNumber(10)
  set ipEv($core.double value) => $_setDouble(9, value);
  @$pb.TagNumber(10)
  $core.bool hasIpEv() => $_has(9);
  @$pb.TagNumber(10)
  void clearIpEv() => $_clearField(10);

  /// Why the job failed, if it did.
  @$pb.TagNumber(11)
  $core.String get error => $_getSZ(10);
  @$pb.TagNumber(11)
  set error($core.String value) => $_setString(10, value);
  @$pb.TagNumber(11)
  $core.bool hasError() => $_has(10);
  @$pb.TagNumber(11)
  void clearError() => $_clearField(11);

  /// Milliseconds since the Unix epoch; 0 if it has not happened.
  @$pb.TagNumber(12)
  $fixnum.Int64 get createdMs => $_getI64(11);
  @$pb.TagNumber(12)
  set createdMs($fixnum.Int64 value) => $_setInt64(11, value);
  @$pb.TagNumber(12)
  $core.bool hasCreatedMs() => $_has(11);
  @$pb.TagNumber(12)
  void clearCreatedMs() => $_clearField(12);

  @$pb.TagNumber(13)
  $fixnum.Int64 get finishedMs => $_getI64(12);
  @$pb.TagNumber(13)
  set finishedMs($fixnum.Int64 value) => $_setInt64(12, value);
  @$pb.TagNumber(13)
  $core.bool hasFinishedMs() => $_has(12);
  @$pb.TagNumber(13)
  void clearFinishedMs() => $_clearField(13);

  /// Time spent solving, not counting time queued or paused.
  @$pb.TagNumber(14)
  $core.double get solveSeconds => $_getN(13);
  @$pb.TagNumber(14)
  set solveSeconds($core.double value) => $_setDouble(13, value);
  @$pb.TagNumber(14)
  $core.bool hasSolveSeconds() => $_has(13);
  @$pb.TagNumber(14)
  void clearSolveSeconds() => $_clearField(14);

  /// Memory the solve needs.
  @$pb.TagNumber(15)
  $fixnum.Int64 get memoryBytes => $_getI64(14);
  @$pb.TagNumber(15)
  set memoryBytes($fixnum.Int64 value) => $_setInt64(14, value);
  @$pb.TagNumber(15)
  $core.bool hasMemoryBytes() => $_has(14);
  @$pb.TagNumber(15)
  void clearMemoryBytes() => $_clearField(15);

  /// Whether the solution can be browsed: the job has run at least once.
  @$pb.TagNumber(16)
  $core.bool get hasSolution => $_getBF(15);
  @$pb.TagNumber(16)
  set hasSolution($core.bool value) => $_setBool(15, value);
  @$pb.TagNumber(16)
  $core.bool hasHasSolution() => $_has(15);
  @$pb.TagNumber(16)
  void clearHasSolution() => $_clearField(16);

  /// Set for a re-solve under locks: the job it started from, and the locks
  /// in force. See locks.proto.
  @$pb.TagNumber(17)
  $core.String get parentId => $_getSZ(16);
  @$pb.TagNumber(17)
  set parentId($core.String value) => $_setString(16, value);
  @$pb.TagNumber(17)
  $core.bool hasParentId() => $_has(16);
  @$pb.TagNumber(17)
  void clearParentId() => $_clearField(17);

  @$pb.TagNumber(18)
  $pb.PbList<$1.NodeLock> get locks => $_getList(17);

  /// Server bookkeeping: the job still has to start from its parent's state.
  @$pb.TagNumber(19)
  $core.bool get warmStartPending => $_getBF(18);
  @$pb.TagNumber(19)
  set warmStartPending($core.bool value) => $_setBool(18, value);
  @$pb.TagNumber(19)
  $core.bool hasWarmStartPending() => $_has(18);
  @$pb.TagNumber(19)
  void clearWarmStartPending() => $_clearField(19);
}

class SubmitJobRequest extends $pb.GeneratedMessage {
  factory SubmitJobRequest({
    $1.Spot? spot,
    $1.SolveSettings? settings,
    $core.String? name,
  }) {
    final result = SubmitJobRequest._();
    if (spot != null) result.spot = spot;
    if (settings != null) result.settings = settings;
    if (name != null) result.name = name;
    return result;
  }

  SubmitJobRequest._();

  factory SubmitJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      SubmitJobRequest()..mergeFromBuffer(data, registry);
  factory SubmitJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      SubmitJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'SubmitJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: SubmitJobRequest.$_createMessage)
    ..aOM<$1.Spot>(1, _omitFieldNames ? '' : 'spot',
        subBuilder: $1.Spot.$_createMessage)
    ..aOM<$1.SolveSettings>(2, _omitFieldNames ? '' : 'settings',
        subBuilder: $1.SolveSettings.$_createMessage)
    ..aOS(3, _omitFieldNames ? '' : 'name')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  SubmitJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  SubmitJobRequest copyWith(void Function(SubmitJobRequest) updates) =>
      super.copyWith((message) => updates(message as SubmitJobRequest))
          as SubmitJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use SubmitJobRequest() / SubmitJobRequest.new instead')
  static SubmitJobRequest create() => SubmitJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => SubmitJobRequest._();
  @$core.override
  SubmitJobRequest createEmptyInstance() => SubmitJobRequest._();
  @$core.pragma('dart2js:noInline')
  static SubmitJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<SubmitJobRequest>(
          SubmitJobRequest.$_createMessage);
  static SubmitJobRequest? _defaultInstance;

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

  @$pb.TagNumber(2)
  $1.SolveSettings get settings => $_getN(1);
  @$pb.TagNumber(2)
  set settings($1.SolveSettings value) => $_setField(2, value);
  @$pb.TagNumber(2)
  $core.bool hasSettings() => $_has(1);
  @$pb.TagNumber(2)
  void clearSettings() => $_clearField(2);
  @$pb.TagNumber(2)
  $1.SolveSettings ensureSettings() => $_ensure(1);

  @$pb.TagNumber(3)
  $core.String get name => $_getSZ(2);
  @$pb.TagNumber(3)
  set name($core.String value) => $_setString(2, value);
  @$pb.TagNumber(3)
  $core.bool hasName() => $_has(2);
  @$pb.TagNumber(3)
  void clearName() => $_clearField(3);
}

class ListJobsRequest extends $pb.GeneratedMessage {
  factory ListJobsRequest() => ListJobsRequest._();

  ListJobsRequest._();

  factory ListJobsRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListJobsRequest()..mergeFromBuffer(data, registry);
  factory ListJobsRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListJobsRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ListJobsRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ListJobsRequest.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListJobsRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListJobsRequest copyWith(void Function(ListJobsRequest) updates) =>
      super.copyWith((message) => updates(message as ListJobsRequest))
          as ListJobsRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use ListJobsRequest() / ListJobsRequest.new instead')
  static ListJobsRequest create() => ListJobsRequest._();
  static $pb.GeneratedMessage $_createMessage() => ListJobsRequest._();
  @$core.override
  ListJobsRequest createEmptyInstance() => ListJobsRequest._();
  @$core.pragma('dart2js:noInline')
  static ListJobsRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ListJobsRequest>(
          ListJobsRequest.$_createMessage);
  static ListJobsRequest? _defaultInstance;
}

class ListJobsResponse extends $pb.GeneratedMessage {
  factory ListJobsResponse({
    $core.Iterable<Job>? jobs,
  }) {
    final result = ListJobsResponse._();
    if (jobs != null) result.jobs.addAll(jobs);
    return result;
  }

  ListJobsResponse._();

  factory ListJobsResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListJobsResponse()..mergeFromBuffer(data, registry);
  factory ListJobsResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ListJobsResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ListJobsResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ListJobsResponse.$_createMessage)
    ..pPM<Job>(1, _omitFieldNames ? '' : 'jobs',
        subBuilder: Job.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListJobsResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ListJobsResponse copyWith(void Function(ListJobsResponse) updates) =>
      super.copyWith((message) => updates(message as ListJobsResponse))
          as ListJobsResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use ListJobsResponse() / ListJobsResponse.new instead')
  static ListJobsResponse create() => ListJobsResponse._();
  static $pb.GeneratedMessage $_createMessage() => ListJobsResponse._();
  @$core.override
  ListJobsResponse createEmptyInstance() => ListJobsResponse._();
  @$core.pragma('dart2js:noInline')
  static ListJobsResponse getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ListJobsResponse>(
          ListJobsResponse.$_createMessage);
  static ListJobsResponse? _defaultInstance;

  @$pb.TagNumber(1)
  $pb.PbList<Job> get jobs => $_getList(0);
}

class GetJobRequest extends $pb.GeneratedMessage {
  factory GetJobRequest({
    $core.String? id,
  }) {
    final result = GetJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  GetJobRequest._();

  factory GetJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetJobRequest()..mergeFromBuffer(data, registry);
  factory GetJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      GetJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'GetJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: GetJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  GetJobRequest copyWith(void Function(GetJobRequest) updates) =>
      super.copyWith((message) => updates(message as GetJobRequest))
          as GetJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use GetJobRequest() / GetJobRequest.new instead')
  static GetJobRequest create() => GetJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => GetJobRequest._();
  @$core.override
  GetJobRequest createEmptyInstance() => GetJobRequest._();
  @$core.pragma('dart2js:noInline')
  static GetJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<GetJobRequest>(
          GetJobRequest.$_createMessage);
  static GetJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

class PauseJobRequest extends $pb.GeneratedMessage {
  factory PauseJobRequest({
    $core.String? id,
  }) {
    final result = PauseJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  PauseJobRequest._();

  factory PauseJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PauseJobRequest()..mergeFromBuffer(data, registry);
  factory PauseJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      PauseJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'PauseJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: PauseJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PauseJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  PauseJobRequest copyWith(void Function(PauseJobRequest) updates) =>
      super.copyWith((message) => updates(message as PauseJobRequest))
          as PauseJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use PauseJobRequest() / PauseJobRequest.new instead')
  static PauseJobRequest create() => PauseJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => PauseJobRequest._();
  @$core.override
  PauseJobRequest createEmptyInstance() => PauseJobRequest._();
  @$core.pragma('dart2js:noInline')
  static PauseJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<PauseJobRequest>(
          PauseJobRequest.$_createMessage);
  static PauseJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

class ResumeJobRequest extends $pb.GeneratedMessage {
  factory ResumeJobRequest({
    $core.String? id,
  }) {
    final result = ResumeJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  ResumeJobRequest._();

  factory ResumeJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ResumeJobRequest()..mergeFromBuffer(data, registry);
  factory ResumeJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      ResumeJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'ResumeJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: ResumeJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ResumeJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  ResumeJobRequest copyWith(void Function(ResumeJobRequest) updates) =>
      super.copyWith((message) => updates(message as ResumeJobRequest))
          as ResumeJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use ResumeJobRequest() / ResumeJobRequest.new instead')
  static ResumeJobRequest create() => ResumeJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => ResumeJobRequest._();
  @$core.override
  ResumeJobRequest createEmptyInstance() => ResumeJobRequest._();
  @$core.pragma('dart2js:noInline')
  static ResumeJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<ResumeJobRequest>(
          ResumeJobRequest.$_createMessage);
  static ResumeJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

class CancelJobRequest extends $pb.GeneratedMessage {
  factory CancelJobRequest({
    $core.String? id,
  }) {
    final result = CancelJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  CancelJobRequest._();

  factory CancelJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      CancelJobRequest()..mergeFromBuffer(data, registry);
  factory CancelJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      CancelJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'CancelJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: CancelJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  CancelJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  CancelJobRequest copyWith(void Function(CancelJobRequest) updates) =>
      super.copyWith((message) => updates(message as CancelJobRequest))
          as CancelJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use CancelJobRequest() / CancelJobRequest.new instead')
  static CancelJobRequest create() => CancelJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => CancelJobRequest._();
  @$core.override
  CancelJobRequest createEmptyInstance() => CancelJobRequest._();
  @$core.pragma('dart2js:noInline')
  static CancelJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<CancelJobRequest>(
          CancelJobRequest.$_createMessage);
  static CancelJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

class DeleteJobRequest extends $pb.GeneratedMessage {
  factory DeleteJobRequest({
    $core.String? id,
  }) {
    final result = DeleteJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  DeleteJobRequest._();

  factory DeleteJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      DeleteJobRequest()..mergeFromBuffer(data, registry);
  factory DeleteJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      DeleteJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'DeleteJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: DeleteJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  DeleteJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  DeleteJobRequest copyWith(void Function(DeleteJobRequest) updates) =>
      super.copyWith((message) => updates(message as DeleteJobRequest))
          as DeleteJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use DeleteJobRequest() / DeleteJobRequest.new instead')
  static DeleteJobRequest create() => DeleteJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => DeleteJobRequest._();
  @$core.override
  DeleteJobRequest createEmptyInstance() => DeleteJobRequest._();
  @$core.pragma('dart2js:noInline')
  static DeleteJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<DeleteJobRequest>(
          DeleteJobRequest.$_createMessage);
  static DeleteJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

class DeleteJobResponse extends $pb.GeneratedMessage {
  factory DeleteJobResponse() => DeleteJobResponse._();

  DeleteJobResponse._();

  factory DeleteJobResponse.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      DeleteJobResponse()..mergeFromBuffer(data, registry);
  factory DeleteJobResponse.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      DeleteJobResponse()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'DeleteJobResponse',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: DeleteJobResponse.$_createMessage)
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  DeleteJobResponse clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  DeleteJobResponse copyWith(void Function(DeleteJobResponse) updates) =>
      super.copyWith((message) => updates(message as DeleteJobResponse))
          as DeleteJobResponse;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use DeleteJobResponse() / DeleteJobResponse.new instead')
  static DeleteJobResponse create() => DeleteJobResponse._();
  static $pb.GeneratedMessage $_createMessage() => DeleteJobResponse._();
  @$core.override
  DeleteJobResponse createEmptyInstance() => DeleteJobResponse._();
  @$core.pragma('dart2js:noInline')
  static DeleteJobResponse getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<DeleteJobResponse>(
          DeleteJobResponse.$_createMessage);
  static DeleteJobResponse? _defaultInstance;
}

class WatchJobRequest extends $pb.GeneratedMessage {
  factory WatchJobRequest({
    $core.String? id,
  }) {
    final result = WatchJobRequest._();
    if (id != null) result.id = id;
    return result;
  }

  WatchJobRequest._();

  factory WatchJobRequest.fromBuffer($core.List<$core.int> data,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      WatchJobRequest()..mergeFromBuffer(data, registry);
  factory WatchJobRequest.fromJson($core.String json,
          [$pb.ExtensionRegistry registry = $pb.ExtensionRegistry.EMPTY]) =>
      WatchJobRequest()..mergeFromJson(json, registry);

  static final $pb.BuilderInfo _i = $pb.BuilderInfo(
      _omitMessageNames ? '' : 'WatchJobRequest',
      package: const $pb.PackageName(_omitMessageNames ? '' : 'gto.v1'),
      createEmptyInstance: WatchJobRequest.$_createMessage)
    ..aOS(1, _omitFieldNames ? '' : 'id')
    ..hasRequiredFields = false;

  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  WatchJobRequest clone() => deepCopy();
  @$core.Deprecated('See https://github.com/google/protobuf.dart/issues/998.')
  WatchJobRequest copyWith(void Function(WatchJobRequest) updates) =>
      super.copyWith((message) => updates(message as WatchJobRequest))
          as WatchJobRequest;

  @$core.override
  $pb.BuilderInfo get info_ => _i;

  @$core.pragma('dart2js:noInline')
  @$core.Deprecated('Use WatchJobRequest() / WatchJobRequest.new instead')
  static WatchJobRequest create() => WatchJobRequest._();
  static $pb.GeneratedMessage $_createMessage() => WatchJobRequest._();
  @$core.override
  WatchJobRequest createEmptyInstance() => WatchJobRequest._();
  @$core.pragma('dart2js:noInline')
  static WatchJobRequest getDefault() =>
      _defaultInstance ??= $pb.GeneratedMessage.$_defaultFor<WatchJobRequest>(
          WatchJobRequest.$_createMessage);
  static WatchJobRequest? _defaultInstance;

  @$pb.TagNumber(1)
  $core.String get id => $_getSZ(0);
  @$pb.TagNumber(1)
  set id($core.String value) => $_setString(0, value);
  @$pb.TagNumber(1)
  $core.bool hasId() => $_has(0);
  @$pb.TagNumber(1)
  void clearId() => $_clearField(1);
}

const $core.bool _omitFieldNames =
    $core.bool.fromEnvironment('protobuf.omit_field_names');
const $core.bool _omitMessageNames =
    $core.bool.fromEnvironment('protobuf.omit_message_names');
