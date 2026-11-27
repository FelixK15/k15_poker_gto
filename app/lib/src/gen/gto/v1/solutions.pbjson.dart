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

@$core.Deprecated('Use handDetailDescriptor instead')
const HandDetail$json = {
  '1': 'HandDetail',
  '2': [
    {'1': 'HAND_DETAIL_HANDS', '2': 0},
    {'1': 'HAND_DETAIL_HANDS_AND_CLASSES', '2': 1},
    {'1': 'HAND_DETAIL_CLASSES', '2': 2},
  ],
};

/// Descriptor for `HandDetail`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List handDetailDescriptor = $convert.base64Decode(
    'CgpIYW5kRGV0YWlsEhUKEUhBTkRfREVUQUlMX0hBTkRTEAASIQodSEFORF9ERVRBSUxfSEFORF'
    'NfQU5EX0NMQVNTRVMQARIXChNIQU5EX0RFVEFJTF9DTEFTU0VTEAI=');

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

@$core.Deprecated('Use preflopScenarioKindDescriptor instead')
const PreflopScenarioKind$json = {
  '1': 'PreflopScenarioKind',
  '2': [
    {'1': 'PREFLOP_SCENARIO_KIND_UNSPECIFIED', '2': 0},
    {'1': 'PREFLOP_SCENARIO_KIND_LIMPED', '2': 1},
    {'1': 'PREFLOP_SCENARIO_KIND_SINGLE_RAISED', '2': 2},
    {'1': 'PREFLOP_SCENARIO_KIND_THREE_BET', '2': 3},
    {'1': 'PREFLOP_SCENARIO_KIND_FOUR_BET', '2': 4},
    {'1': 'PREFLOP_SCENARIO_KIND_FIVE_BET_PLUS', '2': 5},
  ],
};

/// Descriptor for `PreflopScenarioKind`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List preflopScenarioKindDescriptor = $convert.base64Decode(
    'ChNQcmVmbG9wU2NlbmFyaW9LaW5kEiUKIVBSRUZMT1BfU0NFTkFSSU9fS0lORF9VTlNQRUNJRk'
    'lFRBAAEiAKHFBSRUZMT1BfU0NFTkFSSU9fS0lORF9MSU1QRUQQARInCiNQUkVGTE9QX1NDRU5B'
    'UklPX0tJTkRfU0lOR0xFX1JBSVNFRBACEiMKH1BSRUZMT1BfU0NFTkFSSU9fS0lORF9USFJFRV'
    '9CRVQQAxIiCh5QUkVGTE9QX1NDRU5BUklPX0tJTkRfRk9VUl9CRVQQBBInCiNQUkVGTE9QX1ND'
    'RU5BUklPX0tJTkRfRklWRV9CRVRfUExVUxAF');

@$core.Deprecated('Use getNodeRequestDescriptor instead')
const GetNodeRequest$json = {
  '1': 'GetNodeRequest',
  '2': [
    {'1': 'job_id', '3': 1, '4': 1, '5': 9, '10': 'jobId'},
    {'1': 'path', '3': 2, '4': 1, '5': 9, '10': 'path'},
    {'1': 'max_hands', '3': 3, '4': 1, '5': 13, '10': 'maxHands'},
    {
      '1': 'detail',
      '3': 4,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.HandDetail',
      '10': 'detail'
    },
  ],
};

/// Descriptor for `GetNodeRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getNodeRequestDescriptor = $convert.base64Decode(
    'Cg5HZXROb2RlUmVxdWVzdBIVCgZqb2JfaWQYASABKAlSBWpvYklkEhIKBHBhdGgYAiABKAlSBH'
    'BhdGgSGwoJbWF4X2hhbmRzGAMgASgNUghtYXhIYW5kcxIqCgZkZXRhaWwYBCABKA4yEi5ndG8u'
    'djEuSGFuZERldGFpbFIGZGV0YWls');

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

@$core.Deprecated('Use handClassDescriptor instead')
const HandClass$json = {
  '1': 'HandClass',
  '2': [
    {'1': 'name', '3': 1, '4': 1, '5': 9, '10': 'name'},
    {'1': 'weight', '3': 2, '4': 1, '5': 2, '10': 'weight'},
    {'1': 'combos', '3': 3, '4': 1, '5': 2, '10': 'combos'},
    {'1': 'ev', '3': 4, '4': 1, '5': 2, '10': 'ev'},
    {'1': 'equity', '3': 5, '4': 1, '5': 2, '10': 'equity'},
    {'1': 'strategy', '3': 6, '4': 3, '5': 2, '10': 'strategy'},
  ],
};

/// Descriptor for `HandClass`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List handClassDescriptor = $convert.base64Decode(
    'CglIYW5kQ2xhc3MSEgoEbmFtZRgBIAEoCVIEbmFtZRIWCgZ3ZWlnaHQYAiABKAJSBndlaWdodB'
    'IWCgZjb21ib3MYAyABKAJSBmNvbWJvcxIOCgJldhgEIAEoAlICZXYSFgoGZXF1aXR5GAUgASgC'
    'UgZlcXVpdHkSGgoIc3RyYXRlZ3kYBiADKAJSCHN0cmF0ZWd5');

@$core.Deprecated('Use playerRangeDescriptor instead')
const PlayerRange$json = {
  '1': 'PlayerRange',
  '2': [
    {'1': 'hands', '3': 1, '4': 3, '5': 9, '10': 'hands'},
    {'1': 'reach', '3': 2, '4': 3, '5': 2, '10': 'reach'},
    {'1': 'ev', '3': 3, '4': 3, '5': 2, '10': 'ev'},
    {'1': 'range_ev', '3': 4, '4': 1, '5': 1, '10': 'rangeEv'},
    {'1': 'equity', '3': 5, '4': 3, '5': 2, '10': 'equity'},
    {'1': 'range_equity', '3': 6, '4': 1, '5': 1, '10': 'rangeEquity'},
    {'1': 'total_hands', '3': 7, '4': 1, '5': 13, '10': 'totalHands'},
    {
      '1': 'shown_reach_fraction',
      '3': 8,
      '4': 1,
      '5': 1,
      '10': 'shownReachFraction'
    },
    {
      '1': 'classes',
      '3': 9,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.HandClass',
      '10': 'classes'
    },
    {'1': 'position', '3': 10, '4': 1, '5': 9, '10': 'position'},
  ],
};

/// Descriptor for `PlayerRange`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List playerRangeDescriptor = $convert.base64Decode(
    'CgtQbGF5ZXJSYW5nZRIUCgVoYW5kcxgBIAMoCVIFaGFuZHMSFAoFcmVhY2gYAiADKAJSBXJlYW'
    'NoEg4KAmV2GAMgAygCUgJldhIZCghyYW5nZV9ldhgEIAEoAVIHcmFuZ2VFdhIWCgZlcXVpdHkY'
    'BSADKAJSBmVxdWl0eRIhCgxyYW5nZV9lcXVpdHkYBiABKAFSC3JhbmdlRXF1aXR5Eh8KC3RvdG'
    'FsX2hhbmRzGAcgASgNUgp0b3RhbEhhbmRzEjAKFHNob3duX3JlYWNoX2ZyYWN0aW9uGAggASgB'
    'UhJzaG93blJlYWNoRnJhY3Rpb24SKwoHY2xhc3NlcxgJIAMoCzIRLmd0by52MS5IYW5kQ2xhc3'
    'NSB2NsYXNzZXMSGgoIcG9zaXRpb24YCiABKAlSCHBvc2l0aW9u');

@$core.Deprecated('Use getRunoutReportRequestDescriptor instead')
const GetRunoutReportRequest$json = {
  '1': 'GetRunoutReportRequest',
  '2': [
    {'1': 'job_id', '3': 1, '4': 1, '5': 9, '10': 'jobId'},
    {'1': 'path', '3': 2, '4': 1, '5': 9, '10': 'path'},
  ],
};

/// Descriptor for `GetRunoutReportRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getRunoutReportRequestDescriptor =
    $convert.base64Decode(
        'ChZHZXRSdW5vdXRSZXBvcnRSZXF1ZXN0EhUKBmpvYl9pZBgBIAEoCVIFam9iSWQSEgoEcGF0aB'
        'gCIAEoCVIEcGF0aA==');

@$core.Deprecated('Use runoutDescriptor instead')
const Runout$json = {
  '1': 'Runout',
  '2': [
    {'1': 'card', '3': 1, '4': 1, '5': 9, '10': 'card'},
    {'1': 'oop_ev', '3': 2, '4': 1, '5': 1, '10': 'oopEv'},
    {'1': 'ip_ev', '3': 3, '4': 1, '5': 1, '10': 'ipEv'},
    {'1': 'oop_equity', '3': 4, '4': 1, '5': 1, '10': 'oopEquity'},
    {'1': 'ip_equity', '3': 5, '4': 1, '5': 1, '10': 'ipEquity'},
    {
      '1': 'type',
      '3': 6,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.NodeType',
      '10': 'type'
    },
    {
      '1': 'player',
      '3': 7,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.Player',
      '10': 'player'
    },
    {
      '1': 'actions',
      '3': 8,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.NodeAction',
      '10': 'actions'
    },
    {'1': 'evs', '3': 9, '4': 3, '5': 1, '10': 'evs'},
    {'1': 'equities', '3': 10, '4': 3, '5': 1, '10': 'equities'},
    {'1': 'acting_player', '3': 11, '4': 1, '5': 13, '10': 'actingPlayer'},
  ],
};

/// Descriptor for `Runout`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List runoutDescriptor = $convert.base64Decode(
    'CgZSdW5vdXQSEgoEY2FyZBgBIAEoCVIEY2FyZBIVCgZvb3BfZXYYAiABKAFSBW9vcEV2EhMKBW'
    'lwX2V2GAMgASgBUgRpcEV2Eh0KCm9vcF9lcXVpdHkYBCABKAFSCW9vcEVxdWl0eRIbCglpcF9l'
    'cXVpdHkYBSABKAFSCGlwRXF1aXR5EiQKBHR5cGUYBiABKA4yEC5ndG8udjEuTm9kZVR5cGVSBH'
    'R5cGUSJgoGcGxheWVyGAcgASgOMg4uZ3RvLnYxLlBsYXllclIGcGxheWVyEiwKB2FjdGlvbnMY'
    'CCADKAsyEi5ndG8udjEuTm9kZUFjdGlvblIHYWN0aW9ucxIQCgNldnMYCSADKAFSA2V2cxIaCg'
    'hlcXVpdGllcxgKIAMoAVIIZXF1aXRpZXMSIwoNYWN0aW5nX3BsYXllchgLIAEoDVIMYWN0aW5n'
    'UGxheWVy');

@$core.Deprecated('Use getRunoutReportResponseDescriptor instead')
const GetRunoutReportResponse$json = {
  '1': 'GetRunoutReportResponse',
  '2': [
    {
      '1': 'runouts',
      '3': 1,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.Runout',
      '10': 'runouts'
    },
  ],
};

/// Descriptor for `GetRunoutReportResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getRunoutReportResponseDescriptor =
    $convert.base64Decode(
        'ChdHZXRSdW5vdXRSZXBvcnRSZXNwb25zZRIoCgdydW5vdXRzGAEgAygLMg4uZ3RvLnYxLlJ1bm'
        '91dFIHcnVub3V0cw==');

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
    {
      '1': 'players',
      '3': 14,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.PlayerRange',
      '10': 'players'
    },
    {'1': 'acting_player', '3': 15, '4': 1, '5': 13, '10': 'actingPlayer'},
    {'1': 'committed', '3': 16, '4': 3, '5': 1, '10': 'committed'},
    {'1': 'folded', '3': 17, '4': 3, '5': 8, '10': 'folded'},
    {'1': 'all_in', '3': 18, '4': 3, '5': 8, '10': 'allIn'},
    {'1': 'board', '3': 19, '4': 1, '5': 9, '10': 'board'},
    {'1': 'ev_estimated', '3': 20, '4': 1, '5': 8, '10': 'evEstimated'},
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
    'oJaXRlcmF0aW9uGAwgASgNUglpdGVyYXRpb24SFgoGbG9ja2VkGA0gAygIUgZsb2NrZWQSLQoH'
    'cGxheWVycxgOIAMoCzITLmd0by52MS5QbGF5ZXJSYW5nZVIHcGxheWVycxIjCg1hY3RpbmdfcG'
    'xheWVyGA8gASgNUgxhY3RpbmdQbGF5ZXISHAoJY29tbWl0dGVkGBAgAygBUgljb21taXR0ZWQS'
    'FgoGZm9sZGVkGBEgAygIUgZmb2xkZWQSFQoGYWxsX2luGBIgAygIUgVhbGxJbhIUCgVib2FyZB'
    'gTIAEoCVIFYm9hcmQSIQoMZXZfZXN0aW1hdGVkGBQgASgIUgtldkVzdGltYXRlZA==');

@$core.Deprecated('Use listPreflopScenariosRequestDescriptor instead')
const ListPreflopScenariosRequest$json = {
  '1': 'ListPreflopScenariosRequest',
  '2': [
    {'1': 'job_id', '3': 1, '4': 1, '5': 9, '10': 'jobId'},
    {'1': 'max_raises', '3': 2, '4': 1, '5': 13, '10': 'maxRaises'},
  ],
};

/// Descriptor for `ListPreflopScenariosRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List listPreflopScenariosRequestDescriptor =
    $convert.base64Decode(
        'ChtMaXN0UHJlZmxvcFNjZW5hcmlvc1JlcXVlc3QSFQoGam9iX2lkGAEgASgJUgVqb2JJZBIdCg'
        'ptYXhfcmFpc2VzGAIgASgNUgltYXhSYWlzZXM=');

@$core.Deprecated('Use preflopScenarioDescriptor instead')
const PreflopScenario$json = {
  '1': 'PreflopScenario',
  '2': [
    {'1': 'name', '3': 1, '4': 1, '5': 9, '10': 'name'},
    {'1': 'path', '3': 2, '4': 1, '5': 9, '10': 'path'},
    {
      '1': 'kind',
      '3': 3,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.PreflopScenarioKind',
      '10': 'kind'
    },
    {'1': 'positions', '3': 4, '4': 3, '5': 9, '10': 'positions'},
    {'1': 'aggressor', '3': 5, '4': 1, '5': 9, '10': 'aggressor'},
    {'1': 'pot', '3': 6, '4': 1, '5': 1, '10': 'pot'},
    {'1': 'effective_stack', '3': 7, '4': 1, '5': 1, '10': 'effectiveStack'},
    {'1': 'frequency', '3': 8, '4': 1, '5': 1, '10': 'frequency'},
  ],
};

/// Descriptor for `PreflopScenario`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List preflopScenarioDescriptor = $convert.base64Decode(
    'Cg9QcmVmbG9wU2NlbmFyaW8SEgoEbmFtZRgBIAEoCVIEbmFtZRISCgRwYXRoGAIgASgJUgRwYX'
    'RoEi8KBGtpbmQYAyABKA4yGy5ndG8udjEuUHJlZmxvcFNjZW5hcmlvS2luZFIEa2luZBIcCglw'
    'b3NpdGlvbnMYBCADKAlSCXBvc2l0aW9ucxIcCglhZ2dyZXNzb3IYBSABKAlSCWFnZ3Jlc3Nvch'
    'IQCgNwb3QYBiABKAFSA3BvdBInCg9lZmZlY3RpdmVfc3RhY2sYByABKAFSDmVmZmVjdGl2ZVN0'
    'YWNrEhwKCWZyZXF1ZW5jeRgIIAEoAVIJZnJlcXVlbmN5');

@$core.Deprecated('Use listPreflopScenariosResponseDescriptor instead')
const ListPreflopScenariosResponse$json = {
  '1': 'ListPreflopScenariosResponse',
  '2': [
    {
      '1': 'scenarios',
      '3': 1,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.PreflopScenario',
      '10': 'scenarios'
    },
  ],
};

/// Descriptor for `ListPreflopScenariosResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List listPreflopScenariosResponseDescriptor =
    $convert.base64Decode(
        'ChxMaXN0UHJlZmxvcFNjZW5hcmlvc1Jlc3BvbnNlEjUKCXNjZW5hcmlvcxgBIAMoCzIXLmd0by'
        '52MS5QcmVmbG9wU2NlbmFyaW9SCXNjZW5hcmlvcw==');

@$core.Deprecated('Use getPreflopRangesRequestDescriptor instead')
const GetPreflopRangesRequest$json = {
  '1': 'GetPreflopRangesRequest',
  '2': [
    {'1': 'job_id', '3': 1, '4': 1, '5': 9, '10': 'jobId'},
    {'1': 'path', '3': 2, '4': 1, '5': 9, '10': 'path'},
  ],
};

/// Descriptor for `GetPreflopRangesRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getPreflopRangesRequestDescriptor =
    $convert.base64Decode(
        'ChdHZXRQcmVmbG9wUmFuZ2VzUmVxdWVzdBIVCgZqb2JfaWQYASABKAlSBWpvYklkEhIKBHBhdG'
        'gYAiABKAlSBHBhdGg=');

@$core.Deprecated('Use preflopRangeDescriptor instead')
const PreflopRange$json = {
  '1': 'PreflopRange',
  '2': [
    {'1': 'position', '3': 1, '4': 1, '5': 9, '10': 'position'},
    {
      '1': 'classes',
      '3': 2,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.HandClass',
      '10': 'classes'
    },
    {'1': 'range', '3': 3, '4': 1, '5': 9, '10': 'range'},
    {'1': 'committed', '3': 4, '4': 1, '5': 1, '10': 'committed'},
    {'1': 'stack', '3': 5, '4': 1, '5': 1, '10': 'stack'},
  ],
};

/// Descriptor for `PreflopRange`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List preflopRangeDescriptor = $convert.base64Decode(
    'CgxQcmVmbG9wUmFuZ2USGgoIcG9zaXRpb24YASABKAlSCHBvc2l0aW9uEisKB2NsYXNzZXMYAi'
    'ADKAsyES5ndG8udjEuSGFuZENsYXNzUgdjbGFzc2VzEhQKBXJhbmdlGAMgASgJUgVyYW5nZRIc'
    'Cgljb21taXR0ZWQYBCABKAFSCWNvbW1pdHRlZBIUCgVzdGFjaxgFIAEoAVIFc3RhY2s=');

@$core.Deprecated('Use getPreflopRangesResponseDescriptor instead')
const GetPreflopRangesResponse$json = {
  '1': 'GetPreflopRangesResponse',
  '2': [
    {'1': 'path', '3': 1, '4': 1, '5': 9, '10': 'path'},
    {'1': 'complete', '3': 2, '4': 1, '5': 8, '10': 'complete'},
    {
      '1': 'players',
      '3': 3,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.PreflopRange',
      '10': 'players'
    },
    {'1': 'pot', '3': 4, '4': 1, '5': 1, '10': 'pot'},
    {'1': 'effective_stack', '3': 5, '4': 1, '5': 1, '10': 'effectiveStack'},
    {'1': 'spot', '3': 6, '4': 1, '5': 11, '6': '.gto.v1.Spot', '10': 'spot'},
    {'1': 'iteration', '3': 7, '4': 1, '5': 13, '10': 'iteration'},
  ],
};

/// Descriptor for `GetPreflopRangesResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getPreflopRangesResponseDescriptor = $convert.base64Decode(
    'ChhHZXRQcmVmbG9wUmFuZ2VzUmVzcG9uc2USEgoEcGF0aBgBIAEoCVIEcGF0aBIaCghjb21wbG'
    'V0ZRgCIAEoCFIIY29tcGxldGUSLgoHcGxheWVycxgDIAMoCzIULmd0by52MS5QcmVmbG9wUmFu'
    'Z2VSB3BsYXllcnMSEAoDcG90GAQgASgBUgNwb3QSJwoPZWZmZWN0aXZlX3N0YWNrGAUgASgBUg'
    '5lZmZlY3RpdmVTdGFjaxIgCgRzcG90GAYgASgLMgwuZ3RvLnYxLlNwb3RSBHNwb3QSHAoJaXRl'
    'cmF0aW9uGAcgASgNUglpdGVyYXRpb24=');
