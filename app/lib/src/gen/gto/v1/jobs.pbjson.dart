// This is a generated file - do not edit.
//
// Generated from gto/v1/jobs.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports
// ignore_for_file: unused_import

import 'dart:convert' as $convert;
import 'dart:core' as $core;
import 'dart:typed_data' as $typed_data;

@$core.Deprecated('Use jobStateDescriptor instead')
const JobState$json = {
  '1': 'JobState',
  '2': [
    {'1': 'JOB_STATE_UNSPECIFIED', '2': 0},
    {'1': 'JOB_STATE_QUEUED', '2': 1},
    {'1': 'JOB_STATE_RUNNING', '2': 2},
    {'1': 'JOB_STATE_PAUSED', '2': 3},
    {'1': 'JOB_STATE_DONE', '2': 4},
    {'1': 'JOB_STATE_CANCELLED', '2': 5},
    {'1': 'JOB_STATE_FAILED', '2': 6},
  ],
};

/// Descriptor for `JobState`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List jobStateDescriptor = $convert.base64Decode(
    'CghKb2JTdGF0ZRIZChVKT0JfU1RBVEVfVU5TUEVDSUZJRUQQABIUChBKT0JfU1RBVEVfUVVFVU'
    'VEEAESFQoRSk9CX1NUQVRFX1JVTk5JTkcQAhIUChBKT0JfU1RBVEVfUEFVU0VEEAMSEgoOSk9C'
    'X1NUQVRFX0RPTkUQBBIXChNKT0JfU1RBVEVfQ0FOQ0VMTEVEEAUSFAoQSk9CX1NUQVRFX0ZBSU'
    'xFRBAG');

@$core.Deprecated('Use jobDescriptor instead')
const Job$json = {
  '1': 'Job',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
    {'1': 'name', '3': 2, '4': 1, '5': 9, '10': 'name'},
    {
      '1': 'state',
      '3': 3,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.JobState',
      '10': 'state'
    },
    {'1': 'spot', '3': 4, '4': 1, '5': 11, '6': '.gto.v1.Spot', '10': 'spot'},
    {
      '1': 'settings',
      '3': 5,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.SolveSettings',
      '10': 'settings'
    },
    {'1': 'iteration', '3': 6, '4': 1, '5': 13, '10': 'iteration'},
    {'1': 'measured', '3': 7, '4': 1, '5': 8, '10': 'measured'},
    {
      '1': 'exploitability_pct',
      '3': 8,
      '4': 1,
      '5': 1,
      '10': 'exploitabilityPct'
    },
    {'1': 'oop_ev', '3': 9, '4': 1, '5': 1, '10': 'oopEv'},
    {'1': 'ip_ev', '3': 10, '4': 1, '5': 1, '10': 'ipEv'},
    {'1': 'error', '3': 11, '4': 1, '5': 9, '10': 'error'},
    {'1': 'created_ms', '3': 12, '4': 1, '5': 3, '10': 'createdMs'},
    {'1': 'finished_ms', '3': 13, '4': 1, '5': 3, '10': 'finishedMs'},
    {'1': 'solve_seconds', '3': 14, '4': 1, '5': 1, '10': 'solveSeconds'},
    {'1': 'memory_bytes', '3': 15, '4': 1, '5': 4, '10': 'memoryBytes'},
    {'1': 'has_solution', '3': 16, '4': 1, '5': 8, '10': 'hasSolution'},
    {'1': 'parent_id', '3': 17, '4': 1, '5': 9, '10': 'parentId'},
    {
      '1': 'locks',
      '3': 18,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.NodeLock',
      '10': 'locks'
    },
    {
      '1': 'warm_start_pending',
      '3': 19,
      '4': 1,
      '5': 8,
      '10': 'warmStartPending'
    },
  ],
};

/// Descriptor for `Job`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List jobDescriptor = $convert.base64Decode(
    'CgNKb2ISDgoCaWQYASABKAlSAmlkEhIKBG5hbWUYAiABKAlSBG5hbWUSJgoFc3RhdGUYAyABKA'
    '4yEC5ndG8udjEuSm9iU3RhdGVSBXN0YXRlEiAKBHNwb3QYBCABKAsyDC5ndG8udjEuU3BvdFIE'
    'c3BvdBIxCghzZXR0aW5ncxgFIAEoCzIVLmd0by52MS5Tb2x2ZVNldHRpbmdzUghzZXR0aW5ncx'
    'IcCglpdGVyYXRpb24YBiABKA1SCWl0ZXJhdGlvbhIaCghtZWFzdXJlZBgHIAEoCFIIbWVhc3Vy'
    'ZWQSLQoSZXhwbG9pdGFiaWxpdHlfcGN0GAggASgBUhFleHBsb2l0YWJpbGl0eVBjdBIVCgZvb3'
    'BfZXYYCSABKAFSBW9vcEV2EhMKBWlwX2V2GAogASgBUgRpcEV2EhQKBWVycm9yGAsgASgJUgVl'
    'cnJvchIdCgpjcmVhdGVkX21zGAwgASgDUgljcmVhdGVkTXMSHwoLZmluaXNoZWRfbXMYDSABKA'
    'NSCmZpbmlzaGVkTXMSIwoNc29sdmVfc2Vjb25kcxgOIAEoAVIMc29sdmVTZWNvbmRzEiEKDG1l'
    'bW9yeV9ieXRlcxgPIAEoBFILbWVtb3J5Qnl0ZXMSIQoMaGFzX3NvbHV0aW9uGBAgASgIUgtoYX'
    'NTb2x1dGlvbhIbCglwYXJlbnRfaWQYESABKAlSCHBhcmVudElkEiYKBWxvY2tzGBIgAygLMhAu'
    'Z3RvLnYxLk5vZGVMb2NrUgVsb2NrcxIsChJ3YXJtX3N0YXJ0X3BlbmRpbmcYEyABKAhSEHdhcm'
    '1TdGFydFBlbmRpbmc=');

@$core.Deprecated('Use submitJobRequestDescriptor instead')
const SubmitJobRequest$json = {
  '1': 'SubmitJobRequest',
  '2': [
    {'1': 'spot', '3': 1, '4': 1, '5': 11, '6': '.gto.v1.Spot', '10': 'spot'},
    {
      '1': 'settings',
      '3': 2,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.SolveSettings',
      '10': 'settings'
    },
    {'1': 'name', '3': 3, '4': 1, '5': 9, '10': 'name'},
  ],
};

/// Descriptor for `SubmitJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List submitJobRequestDescriptor = $convert.base64Decode(
    'ChBTdWJtaXRKb2JSZXF1ZXN0EiAKBHNwb3QYASABKAsyDC5ndG8udjEuU3BvdFIEc3BvdBIxCg'
    'hzZXR0aW5ncxgCIAEoCzIVLmd0by52MS5Tb2x2ZVNldHRpbmdzUghzZXR0aW5ncxISCgRuYW1l'
    'GAMgASgJUgRuYW1l');

@$core.Deprecated('Use listJobsRequestDescriptor instead')
const ListJobsRequest$json = {
  '1': 'ListJobsRequest',
};

/// Descriptor for `ListJobsRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List listJobsRequestDescriptor =
    $convert.base64Decode('Cg9MaXN0Sm9ic1JlcXVlc3Q=');

@$core.Deprecated('Use listJobsResponseDescriptor instead')
const ListJobsResponse$json = {
  '1': 'ListJobsResponse',
  '2': [
    {'1': 'jobs', '3': 1, '4': 3, '5': 11, '6': '.gto.v1.Job', '10': 'jobs'},
  ],
};

/// Descriptor for `ListJobsResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List listJobsResponseDescriptor = $convert.base64Decode(
    'ChBMaXN0Sm9ic1Jlc3BvbnNlEh8KBGpvYnMYASADKAsyCy5ndG8udjEuSm9iUgRqb2Jz');

@$core.Deprecated('Use getJobRequestDescriptor instead')
const GetJobRequest$json = {
  '1': 'GetJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `GetJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getJobRequestDescriptor =
    $convert.base64Decode('Cg1HZXRKb2JSZXF1ZXN0Eg4KAmlkGAEgASgJUgJpZA==');

@$core.Deprecated('Use pauseJobRequestDescriptor instead')
const PauseJobRequest$json = {
  '1': 'PauseJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `PauseJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List pauseJobRequestDescriptor =
    $convert.base64Decode('Cg9QYXVzZUpvYlJlcXVlc3QSDgoCaWQYASABKAlSAmlk');

@$core.Deprecated('Use resumeJobRequestDescriptor instead')
const ResumeJobRequest$json = {
  '1': 'ResumeJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `ResumeJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List resumeJobRequestDescriptor =
    $convert.base64Decode('ChBSZXN1bWVKb2JSZXF1ZXN0Eg4KAmlkGAEgASgJUgJpZA==');

@$core.Deprecated('Use cancelJobRequestDescriptor instead')
const CancelJobRequest$json = {
  '1': 'CancelJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `CancelJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List cancelJobRequestDescriptor =
    $convert.base64Decode('ChBDYW5jZWxKb2JSZXF1ZXN0Eg4KAmlkGAEgASgJUgJpZA==');

@$core.Deprecated('Use deleteJobRequestDescriptor instead')
const DeleteJobRequest$json = {
  '1': 'DeleteJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `DeleteJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List deleteJobRequestDescriptor =
    $convert.base64Decode('ChBEZWxldGVKb2JSZXF1ZXN0Eg4KAmlkGAEgASgJUgJpZA==');

@$core.Deprecated('Use deleteJobResponseDescriptor instead')
const DeleteJobResponse$json = {
  '1': 'DeleteJobResponse',
};

/// Descriptor for `DeleteJobResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List deleteJobResponseDescriptor =
    $convert.base64Decode('ChFEZWxldGVKb2JSZXNwb25zZQ==');

@$core.Deprecated('Use watchJobRequestDescriptor instead')
const WatchJobRequest$json = {
  '1': 'WatchJobRequest',
  '2': [
    {'1': 'id', '3': 1, '4': 1, '5': 9, '10': 'id'},
  ],
};

/// Descriptor for `WatchJobRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List watchJobRequestDescriptor =
    $convert.base64Decode('Cg9XYXRjaEpvYlJlcXVlc3QSDgoCaWQYASABKAlSAmlk');
