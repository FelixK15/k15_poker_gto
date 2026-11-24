// This is a generated file - do not edit.
//
// Generated from gto/v1/locks.proto.

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

@$core.Deprecated('Use resolveRequestDescriptor instead')
const ResolveRequest$json = {
  '1': 'ResolveRequest',
  '2': [
    {'1': 'parent_job_id', '3': 1, '4': 1, '5': 9, '10': 'parentJobId'},
    {
      '1': 'locks',
      '3': 2,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.NodeLock',
      '10': 'locks'
    },
    {
      '1': 'settings',
      '3': 3,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.SolveSettings',
      '10': 'settings'
    },
    {'1': 'name', '3': 4, '4': 1, '5': 9, '10': 'name'},
  ],
};

/// Descriptor for `ResolveRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List resolveRequestDescriptor = $convert.base64Decode(
    'Cg5SZXNvbHZlUmVxdWVzdBIiCg1wYXJlbnRfam9iX2lkGAEgASgJUgtwYXJlbnRKb2JJZBImCg'
    'Vsb2NrcxgCIAMoCzIQLmd0by52MS5Ob2RlTG9ja1IFbG9ja3MSMQoIc2V0dGluZ3MYAyABKAsy'
    'FS5ndG8udjEuU29sdmVTZXR0aW5nc1IIc2V0dGluZ3MSEgoEbmFtZRgEIAEoCVIEbmFtZQ==');
