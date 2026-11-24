#pragma once

#include "gto/v1/server.grpc.pb.h"

namespace gto::server {

inline constexpr unsigned kApiMajor = 1;
inline constexpr unsigned kApiMinor = 0;

class ServerServiceImpl final : public gto::v1::ServerService::Service {
 public:
  grpc::Status GetInfo(grpc::ServerContext* context, const gto::v1::GetInfoRequest* request,
                       gto::v1::GetInfoResponse* response) override;

  grpc::Status Ping(grpc::ServerContext* context, const gto::v1::PingRequest* request,
                    gto::v1::PingResponse* response) override;
};

}  // namespace gto::server
