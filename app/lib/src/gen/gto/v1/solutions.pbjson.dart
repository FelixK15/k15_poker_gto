// This is a generated file - do not edit.
//
// Generated from gto/v1/solutions.proto.

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

@$core.Deprecated('Use nodeTypeDescriptor instead')
const NodeType$json = {
  '1': 'NodeType',
  '2': [
    {'1': 'NODE_TYPE_UNSPECIFIED', '2': 0},
    {'1': 'NODE_TYPE_ACTION', '2': 1},
    {'1': 'NODE_TYPE_CHANCE', '2': 2},
    {'1': 'NODE_TYPE_FOLD', '2': 3},
    {'1': 'NODE_TYPE_SHOWDOWN', '2': 4},
  ],
};

/// Descriptor for `NodeType`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List nodeTypeDescriptor = $convert.base64Decode(
    'CghOb2RlVHlwZRIZChVOT0RFX1RZUEVfVU5TUEVDSUZJRUQQABIUChBOT0RFX1RZUEVfQUNUSU'
    '9OEAESFAoQTk9ERV9UWVBFX0NIQU5DRRACEhIKDk5PREVfVFlQRV9GT0xEEAMSFgoSTk9ERV9U'
    'WVBFX1NIT1dET1dOEAQ=');

@$core.Deprecated('Use playerDescriptor instead')
const Player$json = {
  '1': 'Player',
  '2': [
    {'1': 'PLAYER_OOP', '2': 0},
    {'1': 'PLAYER_IP', '2': 1},
  ],
};

/// Descriptor for `Player`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List playerDescriptor = $convert
    .base64Decode('CgZQbGF5ZXISDgoKUExBWUVSX09PUBAAEg0KCVBMQVlFUl9JUBAB');

@$core.Deprecated('Use actionTypeDescriptor instead')
const ActionType$json = {
  '1': 'ActionType',
  '2': [
    {'1': 'ACTION_TYPE_UNSPECIFIED', '2': 0},
    {'1': 'ACTION_TYPE_FOLD', '2': 1},
    {'1': 'ACTION_TYPE_CHECK', '2': 2},
    {'1': 'ACTION_TYPE_CALL', '2': 3},
    {'1': 'ACTION_TYPE_BET', '2': 4},
    {'1': 'ACTION_TYPE_RAISE', '2': 5},
    {'1': 'ACTION_TYPE_ALLIN', '2': 6},
  ],
};

/// Descriptor for `ActionType`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List actionTypeDescriptor = $convert.base64Decode(
    'CgpBY3Rpb25UeXBlEhsKF0FDVElPTl9UWVBFX1VOU1BFQ0lGSUVEEAASFAoQQUNUSU9OX1RZUE'
    'VfRk9MRBABEhUKEUFDVElPTl9UWVBFX0NIRUNLEAISFAoQQUNUSU9OX1RZUEVfQ0FMTBADEhMK'
    'D0FDVElPTl9UWVBFX0JFVBAEEhUKEUFDVElPTl9UWVBFX1JBSVNFEAUSFQoRQUNUSU9OX1RZUE'
    'VfQUxMSU4QBg==');

@$core.Deprecated('Use getNodeRequestDescriptor instead')
const GetNodeRequest$json = {
  '1': 'GetNodeRequest',
  '2': [
    {'1': 'job_id', '3': 1, '4': 1, '5': 9, '10': 'jobId'},
    {'1': 'path', '3': 2, '4': 1, '5': 9, '10': 'path'},
  ],
};

/// Descriptor for `GetNodeRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getNodeRequestDescriptor = $convert.base64Decode(
    'Cg5HZXROb2RlUmVxdWVzdBIVCgZqb2JfaWQYASABKAlSBWpvYklkEhIKBHBhdGgYAiABKAlSBH'
    'BhdGg=');

@$core.Deprecated('Use nodeActionDescriptor instead')
const NodeAction$json = {
  '1': 'NodeAction',
  '2': [
    {
      '1': 'type',
      '3': 1,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.ActionType',
      '10': 'type'
    },
    {'1': 'amount', '3': 2, '4': 1, '5': 1, '10': 'amount'},
    {'1': 'label', '3': 3, '4': 1, '5': 9, '10': 'label'},
    {'1': 'frequency', '3': 4, '4': 1, '5': 1, '10': 'frequency'},
  ],
};

/// Descriptor for `NodeAction`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List nodeActionDescriptor = $convert.base64Decode(
    'CgpOb2RlQWN0aW9uEiYKBHR5cGUYASABKA4yEi5ndG8udjEuQWN0aW9uVHlwZVIEdHlwZRIWCg'
    'ZhbW91bnQYAiABKAFSBmFtb3VudBIUCgVsYWJlbBgDIAEoCVIFbGFiZWwSHAoJZnJlcXVlbmN5'
    'GAQgASgBUglmcmVxdWVuY3k=');

@$core.Deprecated('Use playerRangeDescriptor instead')
const PlayerRange$json = {
  '1': 'PlayerRange',
  '2': [
    {'1': 'hands', '3': 1, '4': 3, '5': 9, '10': 'hands'},
    {'1': 'reach', '3': 2, '4': 3, '5': 2, '10': 'reach'},
    {'1': 'ev', '3': 3, '4': 3, '5': 2, '10': 'ev'},
    {'1': 'range_ev', '3': 4, '4': 1, '5': 1, '10': 'rangeEv'},
  ],
};

/// Descriptor for `PlayerRange`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List playerRangeDescriptor = $convert.base64Decode(
    'CgtQbGF5ZXJSYW5nZRIUCgVoYW5kcxgBIAMoCVIFaGFuZHMSFAoFcmVhY2gYAiADKAJSBXJlYW'
    'NoEg4KAmV2GAMgAygCUgJldhIZCghyYW5nZV9ldhgEIAEoAVIHcmFuZ2VFdg==');

@$core.Deprecated('Use getNodeResponseDescriptor instead')
const GetNodeResponse$json = {
  '1': 'GetNodeResponse',
  '2': [
    {
      '1': 'type',
      '3': 1,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.NodeType',
      '10': 'type'
    },
    {
      '1': 'player',
      '3': 2,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.Player',
      '10': 'player'
    },
    {'1': 'path', '3': 3, '4': 1, '5': 9, '10': 'path'},
    {
      '1': 'actions',
      '3': 4,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.NodeAction',
      '10': 'actions'
    },
    {'1': 'strategy', '3': 5, '4': 3, '5': 2, '10': 'strategy'},
    {'1': 'next_cards', '3': 6, '4': 3, '5': 9, '10': 'nextCards'},
    {
      '1': 'oop',
      '3': 7,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.PlayerRange',
      '10': 'oop'
    },
    {
      '1': 'ip',
      '3': 8,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.PlayerRange',
      '10': 'ip'
    },
    {'1': 'pot', '3': 9, '4': 1, '5': 1, '10': 'pot'},
    {'1': 'oop_committed', '3': 10, '4': 1, '5': 1, '10': 'oopCommitted'},
    {'1': 'ip_committed', '3': 11, '4': 1, '5': 1, '10': 'ipCommitted'},
    {'1': 'iteration', '3': 12, '4': 1, '5': 13, '10': 'iteration'},
    {'1': 'locked', '3': 13, '4': 3, '5': 8, '10': 'locked'},
  ],
};

/// Descriptor for `GetNodeResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getNodeResponseDescriptor = $convert.base64Decode(
    'Cg9HZXROb2RlUmVzcG9uc2USJAoEdHlwZRgBIAEoDjIQLmd0by52MS5Ob2RlVHlwZVIEdHlwZR'
    'ImCgZwbGF5ZXIYAiABKA4yDi5ndG8udjEuUGxheWVyUgZwbGF5ZXISEgoEcGF0aBgDIAEoCVIE'
    'cGF0aBIsCgdhY3Rpb25zGAQgAygLMhIuZ3RvLnYxLk5vZGVBY3Rpb25SB2FjdGlvbnMSGgoIc3'
    'RyYXRlZ3kYBSADKAJSCHN0cmF0ZWd5Eh0KCm5leHRfY2FyZHMYBiADKAlSCW5leHRDYXJkcxIl'
    'CgNvb3AYByABKAsyEy5ndG8udjEuUGxheWVyUmFuZ2VSA29vcBIjCgJpcBgIIAEoCzITLmd0by'
    '52MS5QbGF5ZXJSYW5nZVICaXASEAoDcG90GAkgASgBUgNwb3QSIwoNb29wX2NvbW1pdHRlZBgK'
    'IAEoAVIMb29wQ29tbWl0dGVkEiEKDGlwX2NvbW1pdHRlZBgLIAEoAVILaXBDb21taXR0ZWQSHA'
    'oJaXRlcmF0aW9uGAwgASgNUglpdGVyYXRpb24SFgoGbG9ja2VkGA0gAygIUgZsb2NrZWQ=');
