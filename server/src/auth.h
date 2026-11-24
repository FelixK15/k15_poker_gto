#pragma once

#include <string>

#include <grpcpp/grpcpp.h>

namespace gto::server {

/// Checks the access token every call has to carry as the metadata entry
/// "authorization: Bearer <token>".
class Auth {
 public:
  /// An empty token turns the check off; main() only allows that on loopback.
  explicit Auth(std::string token) : token_(std::move(token)) {}

  bool required() const noexcept { return !token_.empty(); }

  /// OK, or UNAUTHENTICATED with a message for the caller.
  grpc::Status check(const grpc::ServerContext& context) const;

 private:
  std::string token_;
};

}  // namespace gto::server
