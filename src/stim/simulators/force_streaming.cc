#include "stim/simulators/force_streaming.h"

#include <cstddef>

namespace stim {

static size_t force_stream_count = 0;
DebugForceResultStreamingRaii::DebugForceResultStreamingRaii() {
    force_stream_count++;
}
DebugForceResultStreamingRaii::~DebugForceResultStreamingRaii() {
    force_stream_count--;
}

bool should_use_streaming_because_bit_count_is_too_large_to_store(uint64_t bit_count) {
    return force_stream_count > 0 || bit_count > (uint64_t{1} << 32);
}

}  // namespace stim
