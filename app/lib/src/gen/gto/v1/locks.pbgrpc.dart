// This is a generated file - do not edit.
//
// Generated from gto/v1/locks.proto.

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

import 'jobs.pb.dart' as $1;
import 'locks.pb.dart' as $0;

export 'locks.pb.dart';

@$pb.GrpcServiceName('gto.v1.LockService')
class LockServiceClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  LockServiceClient(super.channel, {super.options, super.interceptors});

  /// Queues a re-solve of `parent_job_id` under `locks`. The parent must have
  /// a solution and must not be solving at the moment.
  $grpc.ResponseFuture<$1.Job> resolve(
    $0.ResolveRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$resolve, request, options: options);
  }

  // method descriptors

  static final _$resolve = $grpc.ClientMethod<$0.ResolveRequest, $1.Job>(
      '/gto.v1.LockService/Resolve',
      ($0.ResolveRequest value) => value.writeToBuffer(),
      $1.Job.fromBuffer);
}

@$pb.GrpcServiceName('gto.v1.LockService')
abstract class LockServiceBase extends $grpc.Service {
  $core.String get $name => 'gto.v1.LockService';

  LockServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.ResolveRequest, $1.Job>(
        'Resolve',
        resolve_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.ResolveRequest.fromBuffer(value),
        ($1.Job value) => value.writeToBuffer()));
  }

  $async.Future<$1.Job> resolve_Pre($grpc.ServiceCall $call,
      $async.Future<$0.ResolveRequest> $request) async {
    return resolve($call, await $request);
  }

  $async.Future<$1.Job> resolve(
      $grpc.ServiceCall call, $0.ResolveRequest request);
}
