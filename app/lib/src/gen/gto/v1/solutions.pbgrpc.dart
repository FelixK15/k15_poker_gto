// This is a generated file - do not edit.
//
// Generated from gto/v1/solutions.proto.

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

import 'solutions.pb.dart' as $0;

export 'solutions.pb.dart';

@$pb.GrpcServiceName('gto.v1.SolutionService')
class SolutionServiceClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  SolutionServiceClient(super.channel, {super.options, super.interceptors});

  /// Everything about one node of a job's solution.
  $grpc.ResponseFuture<$0.GetNodeResponse> getNode(
    $0.GetNodeRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$getNode, request, options: options);
  }

  /// Where a card is dealt: how each possible card changes the spot. Fails
  /// with INVALID_ARGUMENT if no card is dealt at the path.
  $grpc.ResponseFuture<$0.GetRunoutReportResponse> getRunoutReport(
    $0.GetRunoutReportRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$getRunoutReport, request, options: options);
  }

  // method descriptors

  static final _$getNode =
      $grpc.ClientMethod<$0.GetNodeRequest, $0.GetNodeResponse>(
          '/gto.v1.SolutionService/GetNode',
          ($0.GetNodeRequest value) => value.writeToBuffer(),
          $0.GetNodeResponse.fromBuffer);
  static final _$getRunoutReport =
      $grpc.ClientMethod<$0.GetRunoutReportRequest, $0.GetRunoutReportResponse>(
          '/gto.v1.SolutionService/GetRunoutReport',
          ($0.GetRunoutReportRequest value) => value.writeToBuffer(),
          $0.GetRunoutReportResponse.fromBuffer);
}

@$pb.GrpcServiceName('gto.v1.SolutionService')
abstract class SolutionServiceBase extends $grpc.Service {
  $core.String get $name => 'gto.v1.SolutionService';

  SolutionServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.GetNodeRequest, $0.GetNodeResponse>(
        'GetNode',
        getNode_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.GetNodeRequest.fromBuffer(value),
        ($0.GetNodeResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.GetRunoutReportRequest,
            $0.GetRunoutReportResponse>(
        'GetRunoutReport',
        getRunoutReport_Pre,
        false,
        false,
        ($core.List<$core.int> value) =>
            $0.GetRunoutReportRequest.fromBuffer(value),
        ($0.GetRunoutReportResponse value) => value.writeToBuffer()));
  }

  $async.Future<$0.GetNodeResponse> getNode_Pre($grpc.ServiceCall $call,
      $async.Future<$0.GetNodeRequest> $request) async {
    return getNode($call, await $request);
  }

  $async.Future<$0.GetNodeResponse> getNode(
      $grpc.ServiceCall call, $0.GetNodeRequest request);

  $async.Future<$0.GetRunoutReportResponse> getRunoutReport_Pre(
      $grpc.ServiceCall $call,
      $async.Future<$0.GetRunoutReportRequest> $request) async {
    return getRunoutReport($call, await $request);
  }

  $async.Future<$0.GetRunoutReportResponse> getRunoutReport(
      $grpc.ServiceCall call, $0.GetRunoutReportRequest request);
}
