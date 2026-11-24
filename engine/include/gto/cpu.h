#pragma once

namespace gto {

/// Whether this processor has the instructions the solver was built to use.
/// Call it first thing in main(): on a processor without them the program
/// would otherwise stop with an "illegal instruction" crash somewhere later.
bool cpu_is_supported() noexcept;

/// What a user needs to hear when cpu_is_supported() is false.
const char* cpu_requirement_message() noexcept;

}  // namespace gto
