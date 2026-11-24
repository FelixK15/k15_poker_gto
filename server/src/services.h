#pragma once

#include "auth.h"
#include "gto/v1/jobs.grpc.pb.h"
#include "gto/v1/locks.grpc.pb.h"
#include "gto/v1/server.grpc.pb.h"
#include "gto/v1/solutions.grpc.pb.h"
#include "gto/v1/trees.grpc.pb.h"
#include "job_manager.h"

namespace gto::server {

inline constexpr unsigned kApiMajor = 1;
inline constexpr unsigned kApiMinor = 2;

class ServerServiceImpl final : public v1::ServerService::Service {
 public:
  ServerServiceImpl(const Auth& auth, std::string server_version)
      : auth_(auth), server_version_(std::move(server_version)) {}

  grpc::Status GetInfo(grpc::ServerContext* context, const v1::GetInfoRequest* request,
                       v1::GetInfoResponse* response) override;
  grpc::Status Ping(grpc::ServerContext* context, const v1::PingRequest* request,
                    v1::PingResponse* response) override;

 private:
  const Auth& auth_;
  std::string server_version_;
};

class TreeServiceImpl final : public v1::TreeService::Service {
 public:
  TreeServiceImpl(const Auth& auth, JobManager& jobs) : auth_(auth), jobs_(jobs) {}

  grpc::Status EstimateTree(grpc::ServerContext* context, const v1::EstimateTreeRequest* request,
                            v1::EstimateTreeResponse* response) override;

 private:
  const Auth& auth_;
  JobManager& jobs_;
};

class JobServiceImpl final : public v1::JobService::Service {
 public:
  JobServiceImpl(const Auth& auth, JobManager& jobs) : auth_(auth), jobs_(jobs) {}

  grpc::Status Submit(grpc::ServerContext* context, const v1::SubmitJobRequest* request,
                      v1::Job* response) override;
  grpc::Status List(grpc::ServerContext* context, const v1::ListJobsRequest* request,
                    v1::ListJobsResponse* response) override;
  grpc::Status Get(grpc::ServerContext* context, const v1::GetJobRequest* request,
                   v1::Job* response) override;
  grpc::Status Pause(grpc::ServerContext* context, const v1::PauseJobRequest* request,
                     v1::Job* response) override;
  grpc::Status Resume(grpc::ServerContext* context, const v1::ResumeJobRequest* request,
                      v1::Job* response) override;
  grpc::Status Cancel(grpc::ServerContext* context, const v1::CancelJobRequest* request,
                      v1::Job* response) override;
  grpc::Status Delete(grpc::ServerContext* context, const v1::DeleteJobRequest* request,
                      v1::DeleteJobResponse* response) override;
  grpc::Status Watch(grpc::ServerContext* context, const v1::WatchJobRequest* request,
                     grpc::ServerWriter<v1::Job>* writer) override;

 private:
  const Auth& auth_;
  JobManager& jobs_;
};

class SolutionServiceImpl final : public v1::SolutionService::Service {
 public:
  SolutionServiceImpl(const Auth& auth, JobManager& jobs) : auth_(auth), jobs_(jobs) {}

  grpc::Status GetNode(grpc::ServerContext* context, const v1::GetNodeRequest* request,
                       v1::GetNodeResponse* response) override;

 private:
  const Auth& auth_;
  JobManager& jobs_;
};

class LockServiceImpl final : public v1::LockService::Service {
 public:
  LockServiceImpl(const Auth& auth, JobManager& jobs) : auth_(auth), jobs_(jobs) {}

  grpc::Status Resolve(grpc::ServerContext* context, const v1::ResolveRequest* request,
                       v1::Job* response) override;

 private:
  const Auth& auth_;
  JobManager& jobs_;
};

}  // namespace gto::server
