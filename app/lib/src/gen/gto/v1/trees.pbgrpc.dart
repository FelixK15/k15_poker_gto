// This is a generated file - do not edit.
//
// Generated from gto/v1/trees.proto.

// @dart = 3.3

// ignore_for_file: annotate_overrides, camel_case_types, comment_references
// ignore_for_file: constant_identifier_names
// ignore_for_file: curly_braces_in_flow_control_structures
// ignore_for_file: deprecated_member_use_from_same_package, library_prefixes
// ignore_for_file: non_constant_identifier_names, prefer_relative_imports

import 'dart:async' as $async;
import 'dart:core' as $core;

import 'package:grpc/service_api.dart' as $grpc;
import 'package:protobuf/protobuf.dart' as $pb;

import 'trees.pb.dart' as $0;

export 'trees.pb.dart';

@$pb.GrpcServiceName('gto.v1.TreeService')
class TreeServiceClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  TreeServiceClient(super.channel, {super.options, super.interceptors});

  /// Checks a spot and reports how large its tree is and how much memory a
  /// solve needs. Fails with INVALID_ARGUMENT if the spot cannot be solved at
  /// all (bad board, unreadable range, …).
  $grpc.ResponseFuture<$0.EstimateTreeResponse> estimateTree(
    $0.EstimateTreeRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$estimateTree, request, options: options);
  }

  // method descriptors

  static final _$estimateTree =
      $grpc.ClientMethod<$0.EstimateTreeRequest, $0.EstimateTreeResponse>(
          '/gto.v1.TreeService/EstimateTree',
          ($0.EstimateTreeRequest value) => value.writeToBuffer(),
          $0.EstimateTreeResponse.fromBuffer);
}

@$pb.GrpcServiceName('gto.v1.TreeService')
abstract class TreeServiceBase extends $grpc.Service {
  $core.String get $name => 'gto.v1.TreeService';

  TreeServiceBase() {
    $addMethod(
        $grpc.ServiceMethod<$0.EstimateTreeRequest, $0.EstimateTreeResponse>(
            'EstimateTree',
            estimateTree_Pre,
            false,
            false,
            ($core.List<$core.int> value) =>
                $0.EstimateTreeRequest.fromBuffer(value),
            ($0.EstimateTreeResponse value) => value.writeToBuffer()));
  }

  $async.Future<$0.EstimateTreeResponse> estimateTree_Pre(
      $grpc.ServiceCall $call,
      $async.Future<$0.EstimateTreeRequest> $request) async {
    return estimateTree($call, await $request);
  }

  $async.Future<$0.EstimateTreeResponse> estimateTree(
      $grpc.ServiceCall call, $0.EstimateTreeRequest request);
}
