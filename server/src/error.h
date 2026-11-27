#pragma once

#include <string>
#include <utility>

namespace gto::server {

enum class ErrorKind {
  None,
  InvalidArgument,
  NotFound,
  FailedPrecondition,
  ResourceExhausted,
  Internal,
};

/// Outcome of a server call; the gRPC layer turns it into a status.
struct Error {
  ErrorKind kind = ErrorKind::None;
  std::string message;

  explicit operator bool() const noexcept { return kind != ErrorKind::None; }
};

inline Error make_error(ErrorKind kind, std::string message) {
  return {kind, std::move(message)};
}

}  // namespace gto::server
