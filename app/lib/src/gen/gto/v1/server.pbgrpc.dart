// This is a generated file - do not edit.
//
// Generated from gto/v1/server.proto.

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

import 'server.pb.dart' as $0;

export 'server.pb.dart';

/// Basic information about a solver server and liveness checks.
@$pb.GrpcServiceName('gto.v1.ServerService')
class ServerServiceClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  ServerServiceClient(super.channel, {super.options, super.interceptors});

  /// Static facts about the server and the machine it runs on.
  $grpc.ResponseFuture<$0.GetInfoResponse> getInfo(
    $0.GetInfoRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$getInfo, request, options: options);
  }

  /// Cheap liveness check. Echoes the payload.
  $grpc.ResponseFuture<$0.PingResponse> ping(
    $0.PingRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$ping, request, options: options);
  }

  // method descriptors

  static final _$getInfo =
      $grpc.ClientMethod<$0.GetInfoRequest, $0.GetInfoResponse>(
          '/gto.v1.ServerService/GetInfo',
          ($0.GetInfoRequest value) => value.writeToBuffer(),
          $0.GetInfoResponse.fromBuffer);
  static final _$ping = $grpc.ClientMethod<$0.PingRequest, $0.PingResponse>(
      '/gto.v1.ServerService/Ping',
      ($0.PingRequest value) => value.writeToBuffer(),
      $0.PingResponse.fromBuffer);
}

@$pb.GrpcServiceName('gto.v1.ServerService')
abstract class ServerServiceBase extends $grpc.Service {
  $core.String get $name => 'gto.v1.ServerService';

  ServerServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.GetInfoRequest, $0.GetInfoResponse>(
        'GetInfo',
        getInfo_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.GetInfoRequest.fromBuffer(value),
        ($0.GetInfoResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.PingRequest, $0.PingResponse>(
        'Ping',
        ping_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.PingRequest.fromBuffer(value),
        ($0.PingResponse value) => value.writeToBuffer()));
  }

  $async.Future<$0.GetInfoResponse> getInfo_Pre($grpc.ServiceCall $call,
      $async.Future<$0.GetInfoRequest> $request) async {
    return getInfo($call, await $request);
  }

  $async.Future<$0.GetInfoResponse> getInfo(
      $grpc.ServiceCall call, $0.GetInfoRequest request);

  $async.Future<$0.PingResponse> ping_Pre(
      $grpc.ServiceCall $call, $async.Future<$0.PingRequest> $request) async {
    return ping($call, await $request);
  }

  $async.Future<$0.PingResponse> ping(
      $grpc.ServiceCall call, $0.PingRequest request);
}
