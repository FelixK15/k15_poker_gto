#include "auth.h"

#include <string_view>

namespace gto::server {

namespace {

/// Compares without stopping at the first difference, so that response time
/// does not reveal how much of a guessed token was right.
bool equal_constant_time(std::string_view a, std::string_view b) {
  unsigned difference = static_cast<unsigned>(a.size() ^ b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    const char other = b.empty() ? '\0' : b[i % b.size()];
    difference |= static_cast<unsigned>(static_cast<unsigned char>(a[i]) ^
                                        static_cast<unsigned char>(other));
  }
  return difference == 0;
}

}  // namespace

grpc::Status Auth::check(const grpc::ServerContext& context) const {
  if (!required()) return grpc::Status::OK;

  constexpr std::string_view kPrefix = "Bearer ";
  const auto& metadata = context.client_metadata();
  const auto it = metadata.find("authorization");
  if (it == metadata.end()) {
    return {grpc::StatusCode::UNAUTHENTICATED, "this server needs an access token"};
  }
  const std::string_view value(it->second.data(), it->second.size());
  if (value.substr(0, kPrefix.size()) != kPrefix ||
      !equal_constant_time(value.substr(kPrefix.size()), token_)) {
    return {grpc::StatusCode::UNAUTHENTICATED, "the access token is wrong"};
  }
  return grpc::Status::OK;
}

}  // namespace gto::server
