// This is a generated file - do not edit.
//
// Generated from gto/v1/jobs.proto.

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

import 'jobs.pb.dart' as $0;

export 'jobs.pb.dart';

@$pb.GrpcServiceName('gto.v1.JobService')
class JobServiceClient extends $grpc.Client {
  /// The hostname for this service.
  static const $core.String defaultHost = '';

  /// OAuth scopes needed for the client.
  static const $core.List<$core.String> oauthScopes = [
    '',
  ];

  JobServiceClient(super.channel, {super.options, super.interceptors});

  /// Queues a spot for solving. The server solves one job at a time.
  $grpc.ResponseFuture<$0.Job> submit(
    $0.SubmitJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$submit, request, options: options);
  }

  /// All jobs, newest first.
  $grpc.ResponseFuture<$0.ListJobsResponse> list(
    $0.ListJobsRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$list, request, options: options);
  }

  $grpc.ResponseFuture<$0.Job> get(
    $0.GetJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$get, request, options: options);
  }

  /// Stops a queued or running job, keeping what it has computed so far.
  $grpc.ResponseFuture<$0.Job> pause(
    $0.PauseJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$pause, request, options: options);
  }

  /// Puts a paused or cancelled job back in the queue. It continues where it
  /// stopped.
  $grpc.ResponseFuture<$0.Job> resume(
    $0.ResumeJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$resume, request, options: options);
  }

  /// Stops a job for good. Its solution so far stays browsable.
  $grpc.ResponseFuture<$0.Job> cancel(
    $0.CancelJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$cancel, request, options: options);
  }

  /// Removes a job and its solution from the server.
  $grpc.ResponseFuture<$0.DeleteJobResponse> delete(
    $0.DeleteJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createUnaryCall(_$delete, request, options: options);
  }

  /// Sends the job now and again whenever it changes. Ends when the job is
  /// done, cancelled, failed or deleted.
  $grpc.ResponseStream<$0.Job> watch(
    $0.WatchJobRequest request, {
    $grpc.CallOptions? options,
  }) {
    return $createStreamingCall(_$watch, $async.Stream.fromIterable([request]),
        options: options);
  }

  // method descriptors

  static final _$submit = $grpc.ClientMethod<$0.SubmitJobRequest, $0.Job>(
      '/gto.v1.JobService/Submit',
      ($0.SubmitJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
  static final _$list =
      $grpc.ClientMethod<$0.ListJobsRequest, $0.ListJobsResponse>(
          '/gto.v1.JobService/List',
          ($0.ListJobsRequest value) => value.writeToBuffer(),
          $0.ListJobsResponse.fromBuffer);
  static final _$get = $grpc.ClientMethod<$0.GetJobRequest, $0.Job>(
      '/gto.v1.JobService/Get',
      ($0.GetJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
  static final _$pause = $grpc.ClientMethod<$0.PauseJobRequest, $0.Job>(
      '/gto.v1.JobService/Pause',
      ($0.PauseJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
  static final _$resume = $grpc.ClientMethod<$0.ResumeJobRequest, $0.Job>(
      '/gto.v1.JobService/Resume',
      ($0.ResumeJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
  static final _$cancel = $grpc.ClientMethod<$0.CancelJobRequest, $0.Job>(
      '/gto.v1.JobService/Cancel',
      ($0.CancelJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
  static final _$delete =
      $grpc.ClientMethod<$0.DeleteJobRequest, $0.DeleteJobResponse>(
          '/gto.v1.JobService/Delete',
          ($0.DeleteJobRequest value) => value.writeToBuffer(),
          $0.DeleteJobResponse.fromBuffer);
  static final _$watch = $grpc.ClientMethod<$0.WatchJobRequest, $0.Job>(
      '/gto.v1.JobService/Watch',
      ($0.WatchJobRequest value) => value.writeToBuffer(),
      $0.Job.fromBuffer);
}

@$pb.GrpcServiceName('gto.v1.JobService')
abstract class JobServiceBase extends $grpc.Service {
  $core.String get $name => 'gto.v1.JobService';

  JobServiceBase() {
    $addMethod($grpc.ServiceMethod<$0.SubmitJobRequest, $0.Job>(
        'Submit',
        submit_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.SubmitJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.ListJobsRequest, $0.ListJobsResponse>(
        'List',
        list_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.ListJobsRequest.fromBuffer(value),
        ($0.ListJobsResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.GetJobRequest, $0.Job>(
        'Get',
        get_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.GetJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.PauseJobRequest, $0.Job>(
        'Pause',
        pause_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.PauseJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.ResumeJobRequest, $0.Job>(
        'Resume',
        resume_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.ResumeJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.CancelJobRequest, $0.Job>(
        'Cancel',
        cancel_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.CancelJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.DeleteJobRequest, $0.DeleteJobResponse>(
        'Delete',
        delete_Pre,
        false,
        false,
        ($core.List<$core.int> value) => $0.DeleteJobRequest.fromBuffer(value),
        ($0.DeleteJobResponse value) => value.writeToBuffer()));
    $addMethod($grpc.ServiceMethod<$0.WatchJobRequest, $0.Job>(
        'Watch',
        watch_Pre,
        false,
        true,
        ($core.List<$core.int> value) => $0.WatchJobRequest.fromBuffer(value),
        ($0.Job value) => value.writeToBuffer()));
  }

  $async.Future<$0.Job> submit_Pre($grpc.ServiceCall $call,
      $async.Future<$0.SubmitJobRequest> $request) async {
    return submit($call, await $request);
  }

  $async.Future<$0.Job> submit(
      $grpc.ServiceCall call, $0.SubmitJobRequest request);

  $async.Future<$0.ListJobsResponse> list_Pre($grpc.ServiceCall $call,
      $async.Future<$0.ListJobsRequest> $request) async {
    return list($call, await $request);
  }

  $async.Future<$0.ListJobsResponse> list(
      $grpc.ServiceCall call, $0.ListJobsRequest request);

  $async.Future<$0.Job> get_Pre(
      $grpc.ServiceCall $call, $async.Future<$0.GetJobRequest> $request) async {
    return get($call, await $request);
  }

  $async.Future<$0.Job> get($grpc.ServiceCall call, $0.GetJobRequest request);

  $async.Future<$0.Job> pause_Pre($grpc.ServiceCall $call,
      $async.Future<$0.PauseJobRequest> $request) async {
    return pause($call, await $request);
  }

  $async.Future<$0.Job> pause(
      $grpc.ServiceCall call, $0.PauseJobRequest request);

  $async.Future<$0.Job> resume_Pre($grpc.ServiceCall $call,
      $async.Future<$0.ResumeJobRequest> $request) async {
    return resume($call, await $request);
  }

  $async.Future<$0.Job> resume(
      $grpc.ServiceCall call, $0.ResumeJobRequest request);

  $async.Future<$0.Job> cancel_Pre($grpc.ServiceCall $call,
      $async.Future<$0.CancelJobRequest> $request) async {
    return cancel($call, await $request);
  }

  $async.Future<$0.Job> cancel(
      $grpc.ServiceCall call, $0.CancelJobRequest request);

  $async.Future<$0.DeleteJobResponse> delete_Pre($grpc.ServiceCall $call,
      $async.Future<$0.DeleteJobRequest> $request) async {
    return delete($call, await $request);
  }

  $async.Future<$0.DeleteJobResponse> delete(
      $grpc.ServiceCall call, $0.DeleteJobRequest request);

  $async.Stream<$0.Job> watch_Pre($grpc.ServiceCall $call,
      $async.Future<$0.WatchJobRequest> $request) async* {
    yield* watch($call, await $request);
  }

  $async.Stream<$0.Job> watch(
      $grpc.ServiceCall call, $0.WatchJobRequest request);
}
