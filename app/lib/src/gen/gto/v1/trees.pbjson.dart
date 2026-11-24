// This is a generated file - do not edit.
//
// Generated from gto/v1/trees.proto.

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

@$core.Deprecated('Use estimateTreeRequestDescriptor instead')
const EstimateTreeRequest$json = {
  '1': 'EstimateTreeRequest',
  '2': [
    {'1': 'spot', '3': 1, '4': 1, '5': 11, '6': '.gto.v1.Spot', '10': 'spot'},
  ],
};

/// Descriptor for `EstimateTreeRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List estimateTreeRequestDescriptor = $convert.base64Decode(
    'ChNFc3RpbWF0ZVRyZWVSZXF1ZXN0EiAKBHNwb3QYASABKAsyDC5ndG8udjEuU3BvdFIEc3BvdA'
    '==');

@$core.Deprecated('Use estimateTreeResponseDescriptor instead')
const EstimateTreeResponse$json = {
  '1': 'EstimateTreeResponse',
  '2': [
    {'1': 'nodes', '3': 1, '4': 1, '5': 4, '10': 'nodes'},
    {'1': 'decision_nodes', '3': 2, '4': 1, '5': 4, '10': 'decisionNodes'},
    {'1': 'oop_hands', '3': 3, '4': 1, '5': 13, '10': 'oopHands'},
    {'1': 'ip_hands', '3': 4, '4': 1, '5': 13, '10': 'ipHands'},
    {'1': 'strategy_bytes', '3': 5, '4': 1, '5': 4, '10': 'strategyBytes'},
    {'1': 'tree_bytes', '3': 6, '4': 1, '5': 4, '10': 'treeBytes'},
    {'1': 'too_large', '3': 7, '4': 1, '5': 8, '10': 'tooLarge'},
    {'1': 'can_solve', '3': 8, '4': 1, '5': 8, '10': 'canSolve'},
    {
      '1': 'memory_budget_bytes',
      '3': 9,
      '4': 1,
      '5': 4,
      '10': 'memoryBudgetBytes'
    },
  ],
};

/// Descriptor for `EstimateTreeResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List estimateTreeResponseDescriptor = $convert.base64Decode(
    'ChRFc3RpbWF0ZVRyZWVSZXNwb25zZRIUCgVub2RlcxgBIAEoBFIFbm9kZXMSJQoOZGVjaXNpb2'
    '5fbm9kZXMYAiABKARSDWRlY2lzaW9uTm9kZXMSGwoJb29wX2hhbmRzGAMgASgNUghvb3BIYW5k'
    'cxIZCghpcF9oYW5kcxgEIAEoDVIHaXBIYW5kcxIlCg5zdHJhdGVneV9ieXRlcxgFIAEoBFINc3'
    'RyYXRlZ3lCeXRlcxIdCgp0cmVlX2J5dGVzGAYgASgEUgl0cmVlQnl0ZXMSGwoJdG9vX2xhcmdl'
    'GAcgASgIUgh0b29MYXJnZRIbCgljYW5fc29sdmUYCCABKAhSCGNhblNvbHZlEi4KE21lbW9yeV'
    '9idWRnZXRfYnl0ZXMYCSABKARSEW1lbW9yeUJ1ZGdldEJ5dGVz');
