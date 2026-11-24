#pragma once

#include <cstdint>
#include <string_view>

namespace gto::server {

struct MemoryInfo {
  std::uint64_t total_bytes = 0;
  std::uint64_t available_bytes = 0;
};

/// "windows" or "linux".
std::string_view os_name() noexcept;

/// Logical cores usable by this process (at least 1).
unsigned logical_cores() noexcept;

/// Physical and currently available RAM. Zeros if the query fails.
MemoryInfo memory_info() noexcept;

}  // namespace gto::server
