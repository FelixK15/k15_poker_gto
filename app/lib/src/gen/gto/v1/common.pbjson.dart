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

@$core.Deprecated('Use variantDescriptor instead')
const Variant$json = {
  '1': 'Variant',
  '2': [
    {'1': 'VARIANT_HOLDEM', '2': 0},
    {'1': 'VARIANT_OMAHA4', '2': 1},
  ],
};

/// Descriptor for `Variant`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List variantDescriptor = $convert.base64Decode(
    'CgdWYXJpYW50EhIKDlZBUklBTlRfSE9MREVNEAASEgoOVkFSSUFOVF9PTUFIQTQQAQ==');

@$core.Deprecated('Use engineDescriptor instead')
const Engine$json = {
  '1': 'Engine',
  '2': [
    {'1': 'ENGINE_AUTO', '2': 0},
    {'1': 'ENGINE_EXACT', '2': 1},
    {'1': 'ENGINE_MONTE_CARLO', '2': 2},
  ],
};

/// Descriptor for `Engine`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List engineDescriptor = $convert.base64Decode(
    'CgZFbmdpbmUSDwoLRU5HSU5FX0FVVE8QABIQCgxFTkdJTkVfRVhBQ1QQARIWChJFTkdJTkVfTU'
    '9OVEVfQ0FSTE8QAg==');

@$core.Deprecated('Use exploitabilityKindDescriptor instead')
const ExploitabilityKind$json = {
  '1': 'ExploitabilityKind',
  '2': [
    {'1': 'EXPLOITABILITY_KIND_UNSPECIFIED', '2': 0},
    {'1': 'EXPLOITABILITY_KIND_EXACT', '2': 1},
    {'1': 'EXPLOITABILITY_KIND_LOCKED_GAME', '2': 2},
    {'1': 'EXPLOITABILITY_KIND_ESTIMATED_TWO_PLAYER', '2': 3},
    {'1': 'EXPLOITABILITY_KIND_ESTIMATED_MULTIWAY', '2': 4},
  ],
};

/// Descriptor for `ExploitabilityKind`. Decode as a `google.protobuf.EnumDescriptorProto`.
final $typed_data.Uint8List exploitabilityKindDescriptor = $convert.base64Decode(
    'ChJFeHBsb2l0YWJpbGl0eUtpbmQSIwofRVhQTE9JVEFCSUxJVFlfS0lORF9VTlNQRUNJRklFRB'
    'AAEh0KGUVYUExPSVRBQklMSVRZX0tJTkRfRVhBQ1QQARIjCh9FWFBMT0lUQUJJTElUWV9LSU5E'
    'X0xPQ0tFRF9HQU1FEAISLAooRVhQTE9JVEFCSUxJVFlfS0lORF9FU1RJTUFURURfVFdPX1BMQV'
    'lFUhADEioKJkVYUExPSVRBQklMSVRZX0tJTkRfRVNUSU1BVEVEX01VTFRJV0FZEAQ=');

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
    {
      '1': 'all',
      '3': 3,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.BetSizing',
      '10': 'all'
    },
    {
      '1': 'players',
      '3': 4,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.BetSizing',
      '10': 'players'
    },
  ],
};

/// Descriptor for `StreetSizing`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List streetSizingDescriptor = $convert.base64Decode(
    'CgxTdHJlZXRTaXppbmcSIwoDb29wGAEgASgLMhEuZ3RvLnYxLkJldFNpemluZ1IDb29wEiEKAm'
    'lwGAIgASgLMhEuZ3RvLnYxLkJldFNpemluZ1ICaXASIwoDYWxsGAMgASgLMhEuZ3RvLnYxLkJl'
    'dFNpemluZ1IDYWxsEisKB3BsYXllcnMYBCADKAsyES5ndG8udjEuQmV0U2l6aW5nUgdwbGF5ZX'
    'Jz');

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

@$core.Deprecated('Use playerSpotDescriptor instead')
const PlayerSpot$json = {
  '1': 'PlayerSpot',
  '2': [
    {'1': 'position', '3': 1, '4': 1, '5': 9, '10': 'position'},
    {'1': 'range', '3': 2, '4': 1, '5': 9, '10': 'range'},
    {'1': 'stack', '3': 3, '4': 1, '5': 1, '10': 'stack'},
  ],
};

/// Descriptor for `PlayerSpot`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List playerSpotDescriptor = $convert.base64Decode(
    'CgpQbGF5ZXJTcG90EhoKCHBvc2l0aW9uGAEgASgJUghwb3NpdGlvbhIUCgVyYW5nZRgCIAEoCV'
    'IFcmFuZ2USFAoFc3RhY2sYAyABKAFSBXN0YWNr');

@$core.Deprecated('Use abstractionDescriptor instead')
const Abstraction$json = {
  '1': 'Abstraction',
  '2': [
    {'1': 'lossless', '3': 1, '4': 1, '5': 8, '10': 'lossless'},
    {
      '1': 'preflop_buckets',
      '3': 2,
      '4': 1,
      '5': 13,
      '9': 0,
      '10': 'preflopBuckets',
      '17': true
    },
    {
      '1': 'flop_buckets',
      '3': 3,
      '4': 1,
      '5': 13,
      '9': 1,
      '10': 'flopBuckets',
      '17': true
    },
    {
      '1': 'turn_buckets',
      '3': 4,
      '4': 1,
      '5': 13,
      '9': 2,
      '10': 'turnBuckets',
      '17': true
    },
    {
      '1': 'river_buckets',
      '3': 5,
      '4': 1,
      '5': 13,
      '9': 3,
      '10': 'riverBuckets',
      '17': true
    },
  ],
  '8': [
    {'1': '_preflop_buckets'},
    {'1': '_flop_buckets'},
    {'1': '_turn_buckets'},
    {'1': '_river_buckets'},
  ],
};

/// Descriptor for `Abstraction`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List abstractionDescriptor = $convert.base64Decode(
    'CgtBYnN0cmFjdGlvbhIaCghsb3NzbGVzcxgBIAEoCFIIbG9zc2xlc3MSLAoPcHJlZmxvcF9idW'
    'NrZXRzGAIgASgNSABSDnByZWZsb3BCdWNrZXRziAEBEiYKDGZsb3BfYnVja2V0cxgDIAEoDUgB'
    'UgtmbG9wQnVja2V0c4gBARImCgx0dXJuX2J1Y2tldHMYBCABKA1IAlILdHVybkJ1Y2tldHOIAQ'
    'ESKAoNcml2ZXJfYnVja2V0cxgFIAEoDUgDUgxyaXZlckJ1Y2tldHOIAQFCEgoQX3ByZWZsb3Bf'
    'YnVja2V0c0IPCg1fZmxvcF9idWNrZXRzQg8KDV90dXJuX2J1Y2tldHNCEAoOX3JpdmVyX2J1Y2'
    'tldHM=');

@$core.Deprecated('Use preflopSettingsDescriptor instead')
const PreflopSettings$json = {
  '1': 'PreflopSettings',
  '2': [
    {'1': 'players', '3': 1, '4': 1, '5': 13, '10': 'players'},
    {'1': 'stack_bb', '3': 2, '4': 1, '5': 1, '10': 'stackBb'},
    {
      '1': 'small_blind',
      '3': 3,
      '4': 1,
      '5': 1,
      '9': 0,
      '10': 'smallBlind',
      '17': true
    },
    {
      '1': 'big_blind',
      '3': 4,
      '4': 1,
      '5': 1,
      '9': 1,
      '10': 'bigBlind',
      '17': true
    },
    {'1': 'ante', '3': 5, '4': 1, '5': 1, '10': 'ante'},
    {'1': 'bb_ante', '3': 6, '4': 1, '5': 8, '10': 'bbAnte'},
    {'1': 'open_sizes_bb', '3': 7, '4': 3, '5': 1, '10': 'openSizesBb'},
    {'1': 'sb_open_sizes_bb', '3': 8, '4': 3, '5': 1, '10': 'sbOpenSizesBb'},
    {'1': 'allow_limp', '3': 9, '4': 1, '5': 8, '10': 'allowLimp'},
    {
      '1': 'three_bet_ip_multiplier',
      '3': 10,
      '4': 1,
      '5': 1,
      '9': 2,
      '10': 'threeBetIpMultiplier',
      '17': true
    },
    {
      '1': 'three_bet_oop_multiplier',
      '3': 11,
      '4': 1,
      '5': 1,
      '9': 3,
      '10': 'threeBetOopMultiplier',
      '17': true
    },
    {
      '1': 'four_bet_multiplier',
      '3': 12,
      '4': 1,
      '5': 1,
      '9': 4,
      '10': 'fourBetMultiplier',
      '17': true
    },
    {'1': 'allow_cold_calls', '3': 13, '4': 1, '5': 8, '10': 'allowColdCalls'},
    {
      '1': 'raise_cap',
      '3': 14,
      '4': 1,
      '5': 13,
      '9': 5,
      '10': 'raiseCap',
      '17': true
    },
    {'1': 'solve_postflop', '3': 15, '4': 1, '5': 8, '10': 'solvePostflop'},
  ],
  '8': [
    {'1': '_small_blind'},
    {'1': '_big_blind'},
    {'1': '_three_bet_ip_multiplier'},
    {'1': '_three_bet_oop_multiplier'},
    {'1': '_four_bet_multiplier'},
    {'1': '_raise_cap'},
  ],
};

/// Descriptor for `PreflopSettings`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List preflopSettingsDescriptor = $convert.base64Decode(
    'Cg9QcmVmbG9wU2V0dGluZ3MSGAoHcGxheWVycxgBIAEoDVIHcGxheWVycxIZCghzdGFja19iYh'
    'gCIAEoAVIHc3RhY2tCYhIkCgtzbWFsbF9ibGluZBgDIAEoAUgAUgpzbWFsbEJsaW5kiAEBEiAK'
    'CWJpZ19ibGluZBgEIAEoAUgBUghiaWdCbGluZIgBARISCgRhbnRlGAUgASgBUgRhbnRlEhcKB2'
    'JiX2FudGUYBiABKAhSBmJiQW50ZRIiCg1vcGVuX3NpemVzX2JiGAcgAygBUgtvcGVuU2l6ZXNC'
    'YhInChBzYl9vcGVuX3NpemVzX2JiGAggAygBUg1zYk9wZW5TaXplc0JiEh0KCmFsbG93X2xpbX'
    'AYCSABKAhSCWFsbG93TGltcBI6Chd0aHJlZV9iZXRfaXBfbXVsdGlwbGllchgKIAEoAUgCUhR0'
    'aHJlZUJldElwTXVsdGlwbGllcogBARI8Chh0aHJlZV9iZXRfb29wX211bHRpcGxpZXIYCyABKA'
    'FIA1IVdGhyZWVCZXRPb3BNdWx0aXBsaWVyiAEBEjMKE2ZvdXJfYmV0X211bHRpcGxpZXIYDCAB'
    'KAFIBFIRZm91ckJldE11bHRpcGxpZXKIAQESKAoQYWxsb3dfY29sZF9jYWxscxgNIAEoCFIOYW'
    'xsb3dDb2xkQ2FsbHMSIAoJcmFpc2VfY2FwGA4gASgNSAVSCHJhaXNlQ2FwiAEBEiUKDnNvbHZl'
    'X3Bvc3RmbG9wGA8gASgIUg1zb2x2ZVBvc3RmbG9wQg4KDF9zbWFsbF9ibGluZEIMCgpfYmlnX2'
    'JsaW5kQhoKGF90aHJlZV9iZXRfaXBfbXVsdGlwbGllckIbChlfdGhyZWVfYmV0X29vcF9tdWx0'
    'aXBsaWVyQhYKFF9mb3VyX2JldF9tdWx0aXBsaWVyQgwKCl9yYWlzZV9jYXA=');

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
    {
      '1': 'variant',
      '3': 9,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.Variant',
      '10': 'variant'
    },
    {
      '1': 'pot_limit',
      '3': 10,
      '4': 1,
      '5': 8,
      '9': 0,
      '10': 'potLimit',
      '17': true
    },
    {
      '1': 'players',
      '3': 11,
      '4': 3,
      '5': 11,
      '6': '.gto.v1.PlayerSpot',
      '10': 'players'
    },
    {
      '1': 'preflop',
      '3': 12,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.PreflopSettings',
      '10': 'preflop'
    },
    {
      '1': 'abstraction',
      '3': 13,
      '4': 1,
      '5': 11,
      '6': '.gto.v1.Abstraction',
      '10': 'abstraction'
    },
    {
      '1': 'engine',
      '3': 14,
      '4': 1,
      '5': 14,
      '6': '.gto.v1.Engine',
      '10': 'engine'
    },
  ],
  '8': [
    {'1': '_pot_limit'},
  ],
};

/// Descriptor for `Spot`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List spotDescriptor = $convert.base64Decode(
    'CgRTcG90EhQKBWJvYXJkGAEgASgJUgVib2FyZBIbCglvb3BfcmFuZ2UYAiABKAlSCG9vcFJhbm'
    'dlEhkKCGlwX3JhbmdlGAMgASgJUgdpcFJhbmdlEhAKA3BvdBgEIAEoAVIDcG90EicKD2VmZmVj'
    'dGl2ZV9zdGFjaxgFIAEoAVIOZWZmZWN0aXZlU3RhY2sSGQoIcmFrZV9wY3QYBiABKAFSB3Jha2'
    'VQY3QSGQoIcmFrZV9jYXAYByABKAFSB3Jha2VDYXASKAoEdHJlZRgIIAEoCzIULmd0by52MS5U'
    'cmVlU2V0dGluZ3NSBHRyZWUSKQoHdmFyaWFudBgJIAEoDjIPLmd0by52MS5WYXJpYW50Ugd2YX'
    'JpYW50EiAKCXBvdF9saW1pdBgKIAEoCEgAUghwb3RMaW1pdIgBARIsCgdwbGF5ZXJzGAsgAygL'
    'MhIuZ3RvLnYxLlBsYXllclNwb3RSB3BsYXllcnMSMQoHcHJlZmxvcBgMIAEoCzIXLmd0by52MS'
    '5QcmVmbG9wU2V0dGluZ3NSB3ByZWZsb3ASNQoLYWJzdHJhY3Rpb24YDSABKAsyEy5ndG8udjEu'
    'QWJzdHJhY3Rpb25SC2Fic3RyYWN0aW9uEiYKBmVuZ2luZRgOIAEoDjIOLmd0by52MS5Fbmdpbm'
    'VSBmVuZ2luZUIMCgpfcG90X2xpbWl0');

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
    {'1': 'max_seconds', '3': 3, '4': 1, '5': 1, '10': 'maxSeconds'},
    {'1': 'mc_iterations', '3': 4, '4': 1, '5': 4, '10': 'mcIterations'},
  ],
  '8': [
    {'1': '_target_exploitability_pct'},
  ],
};

/// Descriptor for `SolveSettings`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List solveSettingsDescriptor = $convert.base64Decode(
    'Cg1Tb2x2ZVNldHRpbmdzEiUKDm1heF9pdGVyYXRpb25zGAEgASgNUg1tYXhJdGVyYXRpb25zEj'
    '8KGXRhcmdldF9leHBsb2l0YWJpbGl0eV9wY3QYAiABKAFIAFIXdGFyZ2V0RXhwbG9pdGFiaWxp'
    'dHlQY3SIAQESHwoLbWF4X3NlY29uZHMYAyABKAFSCm1heFNlY29uZHMSIwoNbWNfaXRlcmF0aW'
    '9ucxgEIAEoBFIMbWNJdGVyYXRpb25zQhwKGl90YXJnZXRfZXhwbG9pdGFiaWxpdHlfcGN0');
