#ifndef _STIM_SIMULATORS_FORCE_STREAMING_H
#define _STIM_SIMULATORS_FORCE_STREAMING_H

#include <cstdint>

namespace stim {

/// Facilitates testing of simulators with streaming enabled without
/// having to make enormous circuits.
bool should_use_streaming_because_bit_count_is_too_large_to_store(uint64_t result_count);
struct DebugForceResultStreamingRaii {
    DebugForceResultStreamingRaii();
    ~DebugForceResultStreamingRaii();
};

}  // namespace stim

#endif
