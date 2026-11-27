#include "services.h"

#include <chrono>

#include "gto/version.h"
#include "system_info.h"

namespace gto::server {

namespace {

grpc::Status to_status(const Error& error) {
  switch (error.kind) {
    case ErrorKind::None: return grpc::Status::OK;
    case ErrorKind::InvalidArgument: return {grpc::StatusCode::INVALID_ARGUMENT, error.message};
    case ErrorKind::NotFound: return {grpc::StatusCode::NOT_FOUND, error.message};
    case ErrorKind::FailedPrecondition:
      return {grpc::StatusCode::FAILED_PRECONDITION, error.message};
    case ErrorKind::ResourceExhausted:
      return {grpc::StatusCode::RESOURCE_EXHAUSTED, error.message};
    case ErrorKind::Internal: return {grpc::StatusCode::INTERNAL, error.message};
  }
  return {grpc::StatusCode::UNKNOWN, error.message};
}

bool ends_stream(v1::JobState state) {
  return state == v1::JOB_STATE_DONE || state == v1::JOB_STATE_CANCELLED ||
         state == v1::JOB_STATE_FAILED;
}

}  // namespace

// Every handler starts by checking the caller's token.
#define GTO_REQUIRE_AUTH(context)                                   \
  if (const grpc::Status auth = auth_.check(*(context)); !auth.ok()) return auth

grpc::Status ServerServiceImpl::GetInfo(grpc::ServerContext* context, const v1::GetInfoRequest*,
                                        v1::GetInfoResponse* response) {
  GTO_REQUIRE_AUTH(context);
  const MemoryInfo memory = memory_info();
  response->set_server_version(server_version_);
  response->set_engine_version(std::string(gto::engine_version()));
  response->set_api_major(kApiMajor);
  response->set_api_minor(kApiMinor);
  response->set_os(std::string(os_name()));
  response->set_logical_cores(logical_cores());
  response->set_total_memory_bytes(memory.total_bytes);
  response->set_available_memory_bytes(memory.available_bytes);
  return grpc::Status::OK;
}

grpc::Status ServerServiceImpl::Ping(grpc::ServerContext* context, const v1::PingRequest* request,
                                     v1::PingResponse* response) {
  GTO_REQUIRE_AUTH(context);
  response->set_payload(request->payload());
  return grpc::Status::OK;
}

grpc::Status TreeServiceImpl::EstimateTree(grpc::ServerContext* context,
                                           const v1::EstimateTreeRequest* request,
                                           v1::EstimateTreeResponse* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.estimate(request->spot(), response));
}

grpc::Status JobServiceImpl::Submit(grpc::ServerContext* context,
                                    const v1::SubmitJobRequest* request, v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.submit(*request, response));
}

grpc::Status JobServiceImpl::List(grpc::ServerContext* context, const v1::ListJobsRequest*,
                                  v1::ListJobsResponse* response) {
  GTO_REQUIRE_AUTH(context);
  for (v1::Job& job : jobs_.list()) *response->add_jobs() = std::move(job);
  return grpc::Status::OK;
}

grpc::Status JobServiceImpl::Get(grpc::ServerContext* context, const v1::GetJobRequest* request,
                                 v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.get(request->id(), response));
}

grpc::Status JobServiceImpl::Pause(grpc::ServerContext* context,
                                   const v1::PauseJobRequest* request, v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.pause(request->id(), response));
}

grpc::Status JobServiceImpl::Resume(grpc::ServerContext* context,
                                    const v1::ResumeJobRequest* request, v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.resume(request->id(), response));
}

grpc::Status JobServiceImpl::Cancel(grpc::ServerContext* context,
                                    const v1::CancelJobRequest* request, v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.cancel(request->id(), response));
}

grpc::Status JobServiceImpl::Delete(grpc::ServerContext* context,
                                    const v1::DeleteJobRequest* request, v1::DeleteJobResponse*) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.remove(request->id()));
}

grpc::Status JobServiceImpl::Watch(grpc::ServerContext* context,
                                   const v1::WatchJobRequest* request,
                                   grpc::ServerWriter<v1::Job>* writer) {
  GTO_REQUIRE_AUTH(context);

  v1::Job job;
  if (const Error error = jobs_.get(request->id(), &job)) return to_status(error);

  std::uint64_t version = 0;
  while (!context->IsCancelled()) {
    bool gone = false;
    // Wake up regularly to notice a client that has gone away.
    if (!jobs_.wait_for_change(request->id(), &version, &job, &gone,
                               std::chrono::milliseconds(500))) {
      continue;
    }
    if (gone) break;
    if (!writer->Write(job)) break;
    if (ends_stream(job.state())) break;
  }
  return grpc::Status::OK;
}

grpc::Status SolutionServiceImpl::GetNode(grpc::ServerContext* context,
                                          const v1::GetNodeRequest* request,
                                          v1::GetNodeResponse* response) {
  GTO_REQUIRE_AUTH(context);
  NodeOptions options;
  options.max_hands = request->max_hands();
  options.detail = request->detail();
  return to_status(jobs_.node(request->job_id(), request->path(), options, response));
}

grpc::Status SolutionServiceImpl::GetRunoutReport(grpc::ServerContext* context,
                                                  const v1::GetRunoutReportRequest* request,
                                                  v1::GetRunoutReportResponse* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.runouts(request->job_id(), request->path(), response));
}

grpc::Status SolutionServiceImpl::ListPreflopScenarios(
    grpc::ServerContext* context, const v1::ListPreflopScenariosRequest* request,
    v1::ListPreflopScenariosResponse* response) {
  GTO_REQUIRE_AUTH(context);
  const std::uint32_t max_raises = request->max_raises() == 0 ? 4 : request->max_raises();
  return to_status(jobs_.preflop_scenarios(request->job_id(), max_raises, response));
}

grpc::Status SolutionServiceImpl::GetPreflopRanges(grpc::ServerContext* context,
                                                   const v1::GetPreflopRangesRequest* request,
                                                   v1::GetPreflopRangesResponse* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.preflop_ranges(request->job_id(), request->path(), response));
}

grpc::Status LockServiceImpl::Resolve(grpc::ServerContext* context,
                                      const v1::ResolveRequest* request, v1::Job* response) {
  GTO_REQUIRE_AUTH(context);
  return to_status(jobs_.resolve(*request, response));
}

#undef GTO_REQUIRE_AUTH

}  // namespace gto::server
