#include "server_service.h"

#include <string>

#include "gto/version.h"
#include "system_info.h"

namespace gto::server {

grpc::Status ServerServiceImpl::GetInfo(grpc::ServerContext*, const gto::v1::GetInfoRequest*,
                                        gto::v1::GetInfoResponse* response) {
  const MemoryInfo memory = memory_info();
  response->set_server_version(GTO_SERVER_VERSION);
  response->set_engine_version(std::string(gto::engine_version()));
  response->set_api_major(kApiMajor);
  response->set_api_minor(kApiMinor);
  response->set_os(std::string(os_name()));
  response->set_logical_cores(logical_cores());
  response->set_total_memory_bytes(memory.total_bytes);
  response->set_available_memory_bytes(memory.available_bytes);
  return grpc::Status::OK;
}

grpc::Status ServerServiceImpl::Ping(grpc::ServerContext*, const gto::v1::PingRequest* request,
                                     gto::v1::PingResponse* response) {
  response->set_payload(request->payload());
  return grpc::Status::OK;
}

}  // namespace gto::server
