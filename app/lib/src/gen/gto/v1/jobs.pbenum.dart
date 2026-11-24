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

import 'package:protobuf/protobuf.dart' as $pb;

class JobState extends $pb.ProtobufEnum {
  static const JobState JOB_STATE_UNSPECIFIED =
      JobState._(0, _omitEnumNames ? '' : 'JOB_STATE_UNSPECIFIED');
  static const JobState JOB_STATE_QUEUED =
      JobState._(1, _omitEnumNames ? '' : 'JOB_STATE_QUEUED');
  static const JobState JOB_STATE_RUNNING =
      JobState._(2, _omitEnumNames ? '' : 'JOB_STATE_RUNNING');
  static const JobState JOB_STATE_PAUSED =
      JobState._(3, _omitEnumNames ? '' : 'JOB_STATE_PAUSED');
  static const JobState JOB_STATE_DONE =
      JobState._(4, _omitEnumNames ? '' : 'JOB_STATE_DONE');
  static const JobState JOB_STATE_CANCELLED =
      JobState._(5, _omitEnumNames ? '' : 'JOB_STATE_CANCELLED');
  static const JobState JOB_STATE_FAILED =
      JobState._(6, _omitEnumNames ? '' : 'JOB_STATE_FAILED');

  static const $core.List<JobState> values = <JobState>[
    JOB_STATE_UNSPECIFIED,
    JOB_STATE_QUEUED,
    JOB_STATE_RUNNING,
    JOB_STATE_PAUSED,
    JOB_STATE_DONE,
    JOB_STATE_CANCELLED,
    JOB_STATE_FAILED,
  ];

  static final $core.List<JobState?> _byValue =
      $pb.ProtobufEnum.$_initByValueList(values, 6);
  static JobState? valueOf($core.int value) =>
      value < 0 || value >= _byValue.length ? null : _byValue[value];

  const JobState._(super.value, super.name);
}

const $core.bool _omitEnumNames =
    $core.bool.fromEnvironment('protobuf.omit_enum_names');
