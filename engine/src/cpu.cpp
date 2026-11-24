#include "gto/cpu.h"

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#include <immintrin.h>
#include <intrin.h>
#endif

namespace gto {

bool cpu_is_supported() noexcept {
#ifndef GTO_REQUIRES_AVX2
  return true;
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
  int info[4] = {0, 0, 0, 0};
  __cpuid(info, 0);
  if (info[0] < 7) return false;

  __cpuid(info, 1);
  const bool osxsave = (info[2] & (1 << 27)) != 0;
  const bool avx = (info[2] & (1 << 28)) != 0;
  const bool fma = (info[2] & (1 << 12)) != 0;
  if (!osxsave || !avx || !fma) return false;
  // The operating system must also save the wider registers.
  if ((_xgetbv(0) & 0x6) != 0x6) return false;

  __cpuidex(info, 7, 0);
  return (info[1] & (1 << 5)) != 0;  // AVX2
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
  return __builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma");
#else
  return true;
#endif
}

const char* cpu_requirement_message() noexcept {
  return "This build needs a processor with AVX2 and FMA (Intel from 2013, AMD from 2015). "
         "Build with -DGTO_ENABLE_AVX2=OFF to run on older ones.";
}

}  // namespace gto
