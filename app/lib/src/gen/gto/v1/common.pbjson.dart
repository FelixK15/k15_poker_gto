// This is a generated file - do not edit.
//
// Generated from gto/v1/common.proto.

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

@$core.Deprecated('Use betSizingDescriptor instead')
const BetSizing$json = {
  '1': 'BetSizing',
  '2': [
    {'1': 'bet_pct', '3': 1, '4': 3, '5': 1, '10': 'betPct'},
    {'1': 'raise_pct', '3': 2, '4': 3, '5': 1, '10': 'raisePct'},
    {'1': 'allin', '3': 3, '4': 1, '5': 8, '10': 'allin'},
  ],
};

/// Descriptor for `BetSizing`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List betSizingDescriptor = $convert.base64Decode(
    'CglCZXRTaXppbmcSFwoHYmV0X3BjdBgBIAMoAVIGYmV0UGN0EhsKCXJhaXNlX3BjdBgCIAMoAV'
    'IIcmFpc2VQY3QSFAoFYWxsaW4YAyABKAhSBWFsbGlu');

@$core.Deprecated('Use streetSizingDescriptor instead')
const StreetSizing$json = {
  '1': 'StreetSizing',
  '2': [
    {
      '1': 'oop',
      '3': 1,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.BetSizing',
      '10': 'oop'
    },
    {'1': 'ip', '3': 2, '4': 1, '5': 11, '6': '.gto.v1.BetSizing', '10': 'ip'},
  ],
};

/// Descriptor for `StreetSizing`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List streetSizingDescriptor = $convert.base64Decode(
    'CgxTdHJlZXRTaXppbmcSIwoDb29wGAEgASgLMhEuZ3RvLnYxLkJldFNpemluZ1IDb29wEiEKAm'
    'lwGAIgASgLMhEuZ3RvLnYxLkJldFNpemluZ1ICaXA=');

@$core.Deprecated('Use treeSettingsDescriptor instead')
const TreeSettings$json = {
  '1': 'TreeSettings',
  '2': [
    {
      '1': 'flop',
      '3': 1,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.StreetSizing',
      '10': 'flop'
    },
    {
      '1': 'turn',
      '3': 2,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.StreetSizing',
      '10': 'turn'
    },
    {
      '1': 'river',
      '3': 3,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.StreetSizing',
      '10': 'river'
    },
    {
      '1': 'raise_cap',
      '3': 4,
      '4': 1,
      '5': 13,
      '9': 0,
      '10': 'raiseCap',
      '17': true
    },
    {
      '1': 'allin_threshold_pct',
      '3': 5,
      '4': 1,
      '5': 1,
      '9': 1,
      '10': 'allinThresholdPct',
      '17': true
    },
  ],
  '8': [
    {'1': '_raise_cap'},
    {'1': '_allin_threshold_pct'},
  ],
};

/// Descriptor for `TreeSettings`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List treeSettingsDescriptor = $convert.base64Decode(
    'CgxUcmVlU2V0dGluZ3MSKAoEZmxvcBgBIAEoCzIULmd0by52MS5TdHJlZXRTaXppbmdSBGZsb3'
    'ASKAoEdHVybhgCIAEoCzIULmd0by52MS5TdHJlZXRTaXppbmdSBHR1cm4SKgoFcml2ZXIYAyAB'
    'KAsyFC5ndG8udjEuU3RyZWV0U2l6aW5nUgVyaXZlchIgCglyYWlzZV9jYXAYBCABKA1IAFIIcm'
    'Fpc2VDYXCIAQESMwoTYWxsaW5fdGhyZXNob2xkX3BjdBgFIAEoAUgBUhFhbGxpblRocmVzaG9s'
    'ZFBjdIgBAUIMCgpfcmFpc2VfY2FwQhYKFF9hbGxpbl90aHJlc2hvbGRfcGN0');

@$core.Deprecated('Use spotDescriptor instead')
const Spot$json = {
  '1': 'Spot',
  '2': [
    {'1': 'board', '3': 1, '4': 1, '5': 9, '10': 'board'},
    {'1': 'oop_range', '3': 2, '4': 1, '5': 9, '10': 'oopRange'},
    {'1': 'ip_range', '3': 3, '4': 1, '5': 9, '10': 'ipRange'},
    {'1': 'pot', '3': 4, '4': 1, '5': 1, '10': 'pot'},
    {'1': 'effective_stack', '3': 5, '4': 1, '5': 1, '10': 'effectiveStack'},
    {'1': 'rake_pct', '3': 6, '4': 1, '5': 1, '10': 'rakePct'},
    {'1': 'rake_cap', '3': 7, '4': 1, '5': 1, '10': 'rakeCap'},
    {
      '1': 'tree',
      '3': 8,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.TreeSettings',
      '10': 'tree'
    },
  ],
};

/// Descriptor for `Spot`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List spotDescriptor = $convert.base64Decode(
    'CgRTcG90EhQKBWJvYXJkGAEgASgJUgVib2FyZBIbCglvb3BfcmFuZ2UYAiABKAlSCG9vcFJhbm'
    'dlEhkKCGlwX3JhbmdlGAMgASgJUgdpcFJhbmdlEhAKA3BvdBgEIAEoAVIDcG90EicKD2VmZmVj'
    'dGl2ZV9zdGFjaxgFIAEoAVIOZWZmZWN0aXZlU3RhY2sSGQoIcmFrZV9wY3QYBiABKAFSB3Jha2'
    'VQY3QSGQoIcmFrZV9jYXAYByABKAFSB3Jha2VDYXASKAoEdHJlZRgIIAEoCzIULmd0by52MS5U'
    'cmVlU2V0dGluZ3NSBHRyZWU=');

@$core.Deprecated('Use nodeLockDescriptor instead')
const NodeLock$json = {
  '1': 'NodeLock',
  '2': [
    {'1': 'path', '3': 1, '4': 1, '5': 9, '10': 'path'},
    {
      '1': 'hands',
      '3': 2,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.HandLock',
      '10': 'hands'
    },
  ],
};

/// Descriptor for `NodeLock`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List nodeLockDescriptor = $convert.base64Decode(
    'CghOb2RlTG9jaxISCgRwYXRoGAEgASgJUgRwYXRoEiYKBWhhbmRzGAIgAygLMhAuZ3RvLnYxLk'
    'hhbmRMb2NrUgVoYW5kcw==');

@$core.Deprecated('Use handLockDescriptor instead')
const HandLock$json = {
  '1': 'HandLock',
  '2': [
    {'1': 'hand', '3': 1, '4': 1, '5': 9, '10': 'hand'},
    {'1': 'frequencies', '3': 2, '4': 3, '5': 2, '10': 'frequencies'},
  ],
};

/// Descriptor for `HandLock`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List handLockDescriptor = $convert.base64Decode(
    'CghIYW5kTG9jaxISCgRoYW5kGAEgASgJUgRoYW5kEiAKC2ZyZXF1ZW5jaWVzGAIgAygCUgtmcm'
    'VxdWVuY2llcw==');

@$core.Deprecated('Use solveSettingsDescriptor instead')
const SolveSettings$json = {
  '1': 'SolveSettings',
  '2': [
    {'1': 'max_iterations', '3': 1, '4': 1, '5': 13, '10': 'maxIterations'},
    {
      '1': 'target_exploitability_pct',
      '3': 2,
      '4': 1,
      '5': 1,
      '9': 0,
      '10': 'targetExploitabilityPct',
      '17': true
    },
  ],
  '8': [
    {'1': '_target_exploitability_pct'},
  ],
};

/// Descriptor for `SolveSettings`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List solveSettingsDescriptor = $convert.base64Decode(
    'Cg1Tb2x2ZVNldHRpbmdzEiUKDm1heF9pdGVyYXRpb25zGAEgASgNUg1tYXhJdGVyYXRpb25zEj'
    '8KGXRhcmdldF9leHBsb2l0YWJpbGl0eV9wY3QYAiABKAFIAFIXdGFyZ2V0RXhwbG9pdGFiaWxp'
    'dHlQY3SIAQFCHAoaX3RhcmdldF9leHBsb2l0YWJpbGl0eV9wY3Q=');
