#include "system_info.h"

#include <algorithm>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <fstream>
#include <string>
#include <unistd.h>
#endif

namespace gto::server {

std::string_view os_name() noexcept {
#ifdef _WIN32
  return "windows";
#else
  return "linux";
#endif
}

unsigned logical_cores() noexcept { return std::max(1u, std::thread::hardware_concurrency()); }

MemoryInfo memory_info() noexcept {
  MemoryInfo info;
#ifdef _WIN32
  MEMORYSTATUSEX status{};
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status)) {
    info.total_bytes = status.ullTotalPhys;
    info.available_bytes = status.ullAvailPhys;
  }
#else
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long page_size = sysconf(_SC_PAGE_SIZE);
  if (pages > 0 && page_size > 0) {
    info.total_bytes = static_cast<std::uint64_t>(pages) * static_cast<std::uint64_t>(page_size);
  }
  // MemAvailable accounts for reclaimable page cache, unlike _SC_AVPHYS_PAGES.
  try {
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    std::uint64_t value_kb = 0;
    std::string unit;
    while (meminfo >> key >> value_kb >> unit) {
      if (key == "MemAvailable:") {
        info.available_bytes = value_kb * 1024;
        break;
      }
    }
  } catch (...) {
  }
#endif
  return info;
}

}  // namespace gto::server
