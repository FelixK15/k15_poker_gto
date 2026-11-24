// This is a generated file - do not edit.
//
// Generated from gto/v1/server.proto.

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

@$core.Deprecated('Use getInfoRequestDescriptor instead')
const GetInfoRequest$json = {
  '1': 'GetInfoRequest',
};

/// Descriptor for `GetInfoRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getInfoRequestDescriptor =
    $convert.base64Decode('Cg5HZXRJbmZvUmVxdWVzdA==');

@$core.Deprecated('Use getInfoResponseDescriptor instead')
const GetInfoResponse$json = {
  '1': 'GetInfoResponse',
  '2': [
    {'1': 'server_version', '3': 1, '4': 1, '5': 9, '10': 'serverVersion'},
    {'1': 'engine_version', '3': 2, '4': 1, '5': 9, '10': 'engineVersion'},
    {'1': 'api_major', '3': 3, '4': 1, '5': 13, '10': 'apiMajor'},
    {'1': 'api_minor', '3': 4, '4': 1, '5': 13, '10': 'apiMinor'},
    {'1': 'os', '3': 5, '4': 1, '5': 9, '10': 'os'},
    {'1': 'logical_cores', '3': 6, '4': 1, '5': 13, '10': 'logicalCores'},
    {
      '1': 'total_memory_bytes',
      '3': 7,
      '4': 1,
      '5': 4,
      '10': 'totalMemoryBytes'
    },
    {
      '1': 'available_memory_bytes',
      '3': 8,
      '4': 1,
      '5': 4,
      '10': 'availableMemoryBytes'
    },
  ],
};

/// Descriptor for `GetInfoResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List getInfoResponseDescriptor = $convert.base64Decode(
    'Cg9HZXRJbmZvUmVzcG9uc2USJQoOc2VydmVyX3ZlcnNpb24YASABKAlSDXNlcnZlclZlcnNpb2'
    '4SJQoOZW5naW5lX3ZlcnNpb24YAiABKAlSDWVuZ2luZVZlcnNpb24SGwoJYXBpX21ham9yGAMg'
    'ASgNUghhcGlNYWpvchIbCglhcGlfbWlub3IYBCABKA1SCGFwaU1pbm9yEg4KAm9zGAUgASgJUg'
    'JvcxIjCg1sb2dpY2FsX2NvcmVzGAYgASgNUgxsb2dpY2FsQ29yZXMSLAoSdG90YWxfbWVtb3J5'
    'X2J5dGVzGAcgASgEUhB0b3RhbE1lbW9yeUJ5dGVzEjQKFmF2YWlsYWJsZV9tZW1vcnlfYnl0ZX'
    'MYCCABKARSFGF2YWlsYWJsZU1lbW9yeUJ5dGVz');

@$core.Deprecated('Use pingRequestDescriptor instead')
const PingRequest$json = {
  '1': 'PingRequest',
  '2': [
    {'1': 'payload', '3': 1, '4': 1, '5': 9, '10': 'payload'},
  ],
};

/// Descriptor for `PingRequest`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List pingRequestDescriptor = $convert
    .base64Decode('CgtQaW5nUmVxdWVzdBIYCgdwYXlsb2FkGAEgASgJUgdwYXlsb2Fk');

@$core.Deprecated('Use pingResponseDescriptor instead')
const PingResponse$json = {
  '1': 'PingResponse',
  '2': [
    {'1': 'payload', '3': 1, '4': 1, '5': 9, '10': 'payload'},
  ],
};

/// Descriptor for `PingResponse`. Decode as a `google.protobuf.DescriptorProto`.
final $typed_data.Uint8List pingResponseDescriptor = $convert
    .base64Decode('CgxQaW5nUmVzcG9uc2USGAoHcGF5bG9hZBgBIAEoCVIHcGF5bG9hZA==');
