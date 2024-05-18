#include "stim/util_top/reference_sample_tree.h"

namespace stim {

template <size_t W>
ReferenceSampleTree CompressedReferenceSampleHelper<W>::do_loop_with_no_folding(const Circuit &loop, uint64_t reps) {
    ReferenceSampleTree result;
    result.repetitions = 1;
    size_t start_size = sim.measurement_record.storage.size();

    auto flush_recorded_into_result = [&]() {
        size_t end_size = sim.measurement_record.storage.size();
        if (end_size > start_size) {
            result.suffix_children.push_back({});
            auto &child = result.suffix_children.back();
            child.repetitions = 1;
            child.prefix_bits.insert(
                child.prefix_bits.end(),
                sim.measurement_record.storage.begin() + start_size,
                sim.measurement_record.storage.begin() + end_size);
        }
        start_size = end_size;
    };

    for (size_t k = 0; k < reps; k++) {
        for (const auto &inst : loop.operations) {
            if (inst.gate_type == GateType::REPEAT) {
                uint64_t repeats = inst.repeat_block_rep_count();
                const auto& block = inst.repeat_block_body(loop);
                flush_recorded_into_result();
                result.suffix_children.push_back(do_loop_with_tortoise_hare_folding(block, repeats));
                start_size = sim.measurement_record.storage.size();
            } else {
                sim.do_gate(inst);
            }
        }
    }

    flush_recorded_into_result();
    return result;
}

template <size_t W>
ReferenceSampleTree CompressedReferenceSampleHelper<W>::do_loop_with_tortoise_hare_folding(const Circuit &loop, uint64_t reps) {
    if (reps < 10) {
        return do_loop_with_no_folding(loop, reps);
    }

    ReferenceSampleTree result;
    result.repetitions = 1;

    CompressedReferenceSampleHelper<W> tortoise(sim);
    CompressedReferenceSampleHelper<W> hare(std::move(sim));
    uint64_t tortoise_steps = 0;
    uint64_t hare_steps = 0;
    while (hare_steps < reps) {
        hare_steps++;
        result.suffix_children.push_back(hare.do_loop_with_no_folding(loop, 1));
        assert(result.suffix_children.size() == hare_steps);

        if (hare_steps < 10) {
            // Start with cheap equality checks that can have false negatives.
            if (tortoise.sim.inv_state == hare.sim.inv_state) {
                break;
            }
        } else {
            // For higher repetition counts, transition to more expensive equality checks.
            if (tortoise.sim.canonical_stabilizers() == hare.sim.canonical_stabilizers()) {
                break;
            }
        }

        // Tortoise advances half as quickly.
        if (hare_steps & 1) {
            tortoise_steps++;
            tortoise.do_loop_with_no_folding(loop, 1);
        }
    }

    sim = std::move(hare.sim);

    if (hare_steps == reps) {
        // No loop found.
        return result;
    }

    // Move the loop steps out of the hare and into a loop node.
    ReferenceSampleTree loop_contents;
    uint64_t period = hare_steps - tortoise_steps;
    size_t period_steps_left = (reps - hare_steps) / period;
    for (size_t k = tortoise_steps; k < hare_steps; k++) {
        loop_contents.suffix_children.push_back(std::move(result.suffix_children[k]));
    }
    result.suffix_children.resize(tortoise_steps);

    // Add any remaining measurement data into the sim's measurement record.
    loop_contents.repetitions = 1;
    sim.measurement_record.discard_results_past_max_lookback();
    for (size_t k = 0; k < period_steps_left && sim.measurement_record.storage.size() < sim.measurement_record.max_lookback * 2; k++) {
        loop_contents.decompress_into(sim.measurement_record.storage);
    }
    sim.measurement_record.discard_results_past_max_lookback();

    // Add the loop node to the output data.
    loop_contents.repetitions = period_steps_left + 1;
    result.suffix_children.push_back(std::move(loop_contents));
    hare_steps += period * period_steps_left;

    // Process any iterations remaining in the loop.
    result.suffix_children.push_back(do_loop_with_no_folding(loop, reps - hare_steps));

    return result;
}

}  // namespace stim
