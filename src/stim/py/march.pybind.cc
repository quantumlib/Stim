#include "march.pybind.h"

#ifdef _WIN32
//  Windows
#include <intrin.h>
#define cpuid(info, x) __cpuidex(info, x, 0)
#else
//  GCC Intrinsics
#include <cpuid.h>
void cpuid(int info[4], int infoType) {
    __cpuid_count(infoType, 0, info[0], info[1], info[2], info[3]);
}
#endif

std::string detect_march() {
    // From: https://en.wikipedia.org/wiki/CPUID
    constexpr int EAX = 0;
    constexpr int EBX = 1;
    constexpr int EDX = 3;
    constexpr int INFO_HIGHEST_FUNCTION_PARAMETER = 0;
    constexpr int INFO_PROCESSOR_FEATURE_BITS = 1;
    constexpr int INFO_EXTENDED_FEATURES = 7;
    constexpr int avx2_bit_in_ebx = 1 << 5;
    constexpr int sse2_bit_in_edx = 1 << 26;

    int regs[4];
    cpuid(regs, INFO_HIGHEST_FUNCTION_PARAMETER);
    auto max_info_param = regs[EAX];

    if (max_info_param >= INFO_EXTENDED_FEATURES) {
        cpuid(regs, INFO_EXTENDED_FEATURES);
        if (regs[EBX] & avx2_bit_in_ebx) {
            return "avx2";
        }
    }

    if (max_info_param >= INFO_PROCESSOR_FEATURE_BITS) {
        cpuid(regs, INFO_PROCESSOR_FEATURE_BITS);
        if (regs[EDX] & sse2_bit_in_edx) {
            return "sse2";
        }
    }

    return "polyfill";
}
