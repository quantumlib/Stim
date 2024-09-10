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
                const auto &block = inst.repeat_block_body(loop);
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
ReferenceSampleTree CompressedReferenceSampleHelper<W>::do_loop_with_tortoise_hare_folding(
    const Circuit &loop, uint64_t reps) {
    if (reps < 10) {
        // Probably not worth the overhead of tortoise-and-hare. Just run it raw.
        return do_loop_with_no_folding(loop, reps);
    }

    ReferenceSampleTree result;
    result.repetitions = 1;

    CompressedReferenceSampleHelper<W> tortoise(sim);
    CompressedReferenceSampleHelper<W> hare(std::move(sim));
    uint64_t max_feedback_lookback = max_feedback_lookback_in_loop(loop);
    uint64_t tortoise_steps = 0;
    uint64_t hare_steps = 0;
    while (hare_steps < reps) {
        hare_steps++;
        result.suffix_children.push_back(hare.do_loop_with_no_folding(loop, 1));
        assert(result.suffix_children.size() == hare_steps);

        if (tortoise.in_same_recent_state_as(hare, max_feedback_lookback, hare_steps < 10)) {
            break;
        }

        // Tortoise advances half as quickly.
        if (hare_steps & 1) {
            tortoise_steps++;
            tortoise.do_loop_with_no_folding(loop, 1);
        }
    }

    if (hare_steps == reps) {
        // No periodic state found before reaching the end of the loop.
        sim = std::move(hare.sim);
        return result;
    }

    // Run more loop iterations until the remaining iterations are a multiple of the found period.
    assert(result.suffix_children.size() == hare_steps);
    uint64_t period = hare_steps - tortoise_steps;
    size_t period_steps_left = (reps - hare_steps) / period;
    while ((reps - hare_steps) % period) {
        result.suffix_children.push_back(hare.do_loop_with_no_folding(loop, 1));
        hare_steps += 1;
    }
    assert(hare_steps + period_steps_left * period == reps);
    assert(hare_steps >= period);
    sim = std::move(hare.sim);

    // Move the periodic measurements out of the hare's tail, into a loop node.
    ReferenceSampleTree loop_contents;
    for (size_t k = hare_steps - period; k < hare_steps; k++) {
        loop_contents.suffix_children.push_back(std::move(result.suffix_children[k]));
    }
    result.suffix_children.resize(hare_steps - period);

    // Add skipped iterations' measurement data into the sim's measurement record.
    loop_contents.repetitions = 1;
    sim.measurement_record.discard_results_past_max_lookback();
    for (size_t k = 0;
         k < period_steps_left && sim.measurement_record.storage.size() < sim.measurement_record.max_lookback * 2;
         k++) {
        loop_contents.decompress_into(sim.measurement_record.storage);
    }
    sim.measurement_record.discard_results_past_max_lookback();

    // Add the loop node to the output data.
    loop_contents.repetitions = period_steps_left + 1;
    loop_contents.try_factorize(2);
    loop_contents.try_factorize(3);
    loop_contents.try_factorize(5);
    result.suffix_children.push_back(std::move(loop_contents));

    return result;
}

template <size_t W>
bool CompressedReferenceSampleHelper<W>::in_same_recent_state_as(
    const CompressedReferenceSampleHelper<W> &other, uint64_t max_record_lookback, bool allow_false_negative) const {
    const auto &s1 = sim.measurement_record.storage;
    const auto &s2 = other.sim.measurement_record.storage;

    // Check that recent measurements gave identical results.
    if (s1.size() < max_record_lookback || s2.size() < max_record_lookback) {
        return false;
    }
    for (size_t k = 0; k < max_record_lookback; k++) {
        if (s1[s1.size() - k - 1] != s2[s2.size() - k - 1]) {
            return false;
        }
    }

    // Check that quantum states are identical.
    if (allow_false_negative) {
        return sim.inv_state == other.sim.inv_state;
    }
    return sim.canonical_stabilizers() == other.sim.canonical_stabilizers();
}

}  // namespace stim
