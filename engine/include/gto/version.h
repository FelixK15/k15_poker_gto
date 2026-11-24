#pragma once

#include <string_view>

namespace gto {

/// Version of the solver engine library, e.g. "0.1.0".
std::string_view engine_version() noexcept;

}  // namespace gto
