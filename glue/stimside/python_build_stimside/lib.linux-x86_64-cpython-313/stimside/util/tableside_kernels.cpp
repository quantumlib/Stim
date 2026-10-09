#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>

extern "C" {

enum OpKind : int32_t {
    OP_PASSTHROUGH = 0,
    OP_FILTER_1Q_U = 1,
    OP_FILTER_2Q_UU = 2,
    OP_TRANSITION_1_U = 3,
    OP_TRANSITION_2_UU = 4,
    OP_MEAS_PROJ_Z = 5,
    OP_NEEDS_PYTHON = 6,
    OP_FUSED_TRANS1_MEAS = 7, // Fused Mode B state-dependent LEAKAGE_TRANSITION_1 + M[LEAKAGE_PROJECTION_Z]
    OP_YIELD_IF_LEAKED = 8    // Yields to Python only if any target/condition qubit in step_mask is leaked
};

enum EmittedActionKind : int32_t {
    ACT_RUN_SLICE = 0,
    ACT_FILTERED_1Q = 1,
    ACT_FILTERED_2Q = 2,
    ACT_INJECT_RESET = 3,
    ACT_INJECT_X = 4,
    ACT_INJECT_Y = 5,
    ACT_INJECT_Z = 6,
    ACT_INJECT_DEPOL1 = 7,
    ACT_INJECT_X_ERR = 8,
    ACT_INJECT_Z_ERR = 9,
    ACT_MODIFIED_MEAS = 10,
    ACT_YIELD_PYTHON = 11,
    ACT_YIELD_FUSED_MEAS = 12
};

struct EmittedAction {
    int32_t kind;
    int32_t step_idx;
    int32_t slice_end;
    int32_t target_offset;
    int32_t target_count;
    double prob;
};

struct StepDesc {
    int32_t kind;
    int32_t target_offset;
    int32_t target_count;
    int32_t mask_offset;
    double p_total_u;
    int32_t trans_offset;
    int32_t trans_count;
    double meas_leak_prob;
};

struct TransBranch {
    double cum_prob;
    int8_t in0;
    int8_t in1;
    int8_t out0; // -1='U', -2='V', -3='D', -4='X', -5='Y', -6='Z', 0=0, 1=1, >=2=leaked
    int8_t out1;
    double raw_prob;
};

struct FastEngine {
    int num_qubits;
    int num_words;
    int num_steps;
    uint64_t rng_state;

    std::vector<uint8_t> state;
    std::vector<uint64_t> leaked_mask;
    int num_leaked;

    std::vector<StepDesc> steps;
    std::vector<uint32_t> all_raw_targets;
    std::vector<int32_t> all_qubit_targets;
    std::vector<uint64_t> all_step_masks;
    std::vector<TransBranch> all_branches;

    std::vector<EmittedAction> actions;
    std::vector<uint32_t> out_targets;

    std::vector<int32_t> buf_reset;
    std::vector<int32_t> buf_one;
    std::vector<int32_t> buf_depol;
    std::vector<int32_t> buf_x;
    std::vector<int32_t> buf_y;
    std::vector<int32_t> buf_z;
    std::vector<int32_t> buf_v;

    inline double next_double() {
        uint64_t x = rng_state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        rng_state = x;
        return ((x * 0x2545F4914F6CDD1DULL) >> 11) * (1.0 / 9007199254740992.0);
    }

    inline void set_qubit_state(int q, uint8_t new_st) {
        uint8_t old_st = state[q];
        if (old_st == new_st) return;
        state[q] = new_st;
        int w = q >> 6;
        uint64_t bit = 1ULL << (q & 63);
        if (old_st < 2 && new_st >= 2) {
            leaked_mask[w] |= bit;
            num_leaked++;
        } else if (old_st >= 2 && new_st < 2) {
            leaked_mask[w] &= ~bit;
            num_leaked--;
        }
    }

    inline bool step_has_leaked_target(const StepDesc& st) const {
        if (num_leaked == 0) return false;
        const uint64_t* sm = &all_step_masks[st.mask_offset];
        for (int w = 0; w < num_words; ++w) {
            if (leaked_mask[w] & sm[w]) return true;
        }
        return false;
    }

    inline void flush_slice(int& slice_start, int cur_step) {
        if (cur_step > slice_start) {
            actions.push_back({ACT_RUN_SLICE, slice_start, cur_step, 0, 0, 0.0});
        }
        slice_start = cur_step + 1;
    }

    inline void push_target_action(int32_t kind, int32_t step_idx, const std::vector<int32_t>& qs, double prob = 0.0) {
        int count = (int)qs.size();
        if (count <= 0) return;
        int32_t off = (int32_t)out_targets.size();
        for (int i = 0; i < count; ++i) {
            out_targets.push_back((uint32_t)qs[i]);
        }
        actions.push_back({kind, step_idx, 0, off, count, prob});
    }
};

int fast_format_stim_op(
    const char* op_name,
    int has_arg,
    double arg0,
    const uint32_t* targets,
    int count,
    char* out_buf
) {
    char* p = out_buf;
    while (*op_name) *p++ = *op_name++;
    if (has_arg) {
        *p++ = '(';
        p += std::snprintf(p, 32, "%.6g", arg0);
        *p++ = ')';
    }
    for (int i = 0; i < count; ++i) {
        *p++ = ' ';
        uint32_t v = targets[i];
        if (v & (1u << 31)) {
            *p++ = '!';
            v &= 0x7FFFFFFFu;
        }
        if (v == 0) {
            *p++ = '0';
        } else {
            char tmp[12];
            int len = 0;
            while (v > 0) {
                tmp[len++] = (char)('0' + (v % 10));
                v /= 10;
            }
            while (len > 0) *p++ = tmp[--len];
        }
    }
    *p++ = '\n';
    *p = '\0';
    return (int)(p - out_buf);
}

FastEngine* fast_engine_create(
    int num_qubits,
    int num_steps,
    uint64_t seed,
    const StepDesc* steps_in,
    int total_targets,
    const uint32_t* raw_targets_in,
    const int32_t* qubit_targets_in,
    int total_masks,
    const uint64_t* step_masks_in,
    int total_branches,
    const TransBranch* branches_in
) {
    FastEngine* eng = new FastEngine();
    eng->num_qubits = num_qubits;
    eng->num_words = std::max(1, (num_qubits + 63) / 64);
    eng->num_steps = num_steps;
    eng->rng_state = seed ? seed : 0x853c49e6748fea9bULL;
    eng->state.assign(std::max(1, num_qubits), 0);
    eng->leaked_mask.assign(eng->num_words, 0ULL);
    eng->num_leaked = 0;

    if (num_steps > 0 && steps_in != nullptr) {
        eng->steps.assign(steps_in, steps_in + num_steps);
    }
    if (total_masks > 0 && step_masks_in != nullptr) {
        eng->all_step_masks.assign(step_masks_in, step_masks_in + total_masks);
    } else {
        eng->all_step_masks.assign(eng->num_words, 0ULL);
    }
    if (total_targets > 0 && raw_targets_in != nullptr && qubit_targets_in != nullptr) {
        eng->all_raw_targets.assign(raw_targets_in, raw_targets_in + total_targets);
        eng->all_qubit_targets.assign(qubit_targets_in, qubit_targets_in + total_targets);
    }
    if (total_branches > 0 && branches_in != nullptr) {
        eng->all_branches.assign(branches_in, branches_in + total_branches);
    }
    eng->actions.reserve(256);
    eng->out_targets.reserve(2048);
    int cap = std::max(64, num_qubits);
    eng->buf_reset.reserve(cap);
    eng->buf_one.reserve(cap);
    eng->buf_depol.reserve(cap);
    eng->buf_x.reserve(cap);
    eng->buf_y.reserve(cap);
    eng->buf_z.reserve(cap);
    eng->buf_v.reserve(cap);
    return eng;
}

void fast_engine_destroy(FastEngine* eng) {
    delete eng;
}

void fast_engine_clear(FastEngine* eng, uint64_t new_seed) {
    if (new_seed != 0) eng->rng_state = new_seed;
    std::fill(eng->state.begin(), eng->state.end(), 0);
    std::fill(eng->leaked_mask.begin(), eng->leaked_mask.end(), 0ULL);
    eng->num_leaked = 0;
    eng->actions.clear();
    eng->out_targets.clear();
}

uint8_t* fast_engine_get_state_ptr(FastEngine* eng) {
    return eng->state.data();
}

void fast_engine_sync_leaked_mask(FastEngine* eng) {
    std::fill(eng->leaked_mask.begin(), eng->leaked_mask.end(), 0ULL);
    int cnt = 0;
    for (int q = 0; q < eng->num_qubits; ++q) {
        if (eng->state[q] == 1) eng->state[q] = 0;
        if (eng->state[q] >= 2) {
            eng->leaked_mask[q >> 6] |= (1ULL << (q & 63));
            cnt++;
        }
    }
    eng->num_leaked = cnt;
}

static inline void apply_2q_leg_transition(
    FastEngine* eng,
    int q,
    bool in_is_leaked,
    int8_t os
) {
    if (os == -1) { // 'U'
        if (in_is_leaked) {
            eng->buf_reset.push_back(q);
            eng->buf_depol.push_back(q);
        }
        eng->set_qubit_state(q, 0);
    } else if (os == 0) {
        eng->buf_reset.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == 1) {
        eng->buf_one.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == -2) { // 'V'
        eng->buf_reset.push_back(q);
        eng->buf_v.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == -3) { // 'D'
        if (in_is_leaked) eng->buf_reset.push_back(q);
        eng->buf_depol.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == -4) { // 'X'
        if (in_is_leaked) eng->buf_reset.push_back(q);
        eng->buf_x.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == -5) { // 'Y'
        if (in_is_leaked) eng->buf_reset.push_back(q);
        eng->buf_y.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os == -6) { // 'Z'
        if (in_is_leaked) eng->buf_reset.push_back(q);
        eng->buf_z.push_back(q);
        eng->set_qubit_state(q, 0);
    } else if (os >= 2) {
        eng->set_qubit_state(q, (uint8_t)os);
    }
}

int fast_engine_run_segment(
    FastEngine* eng,
    int start_step,
    EmittedAction** actions_out,
    uint32_t** targets_out
) {
    eng->actions.clear();
    eng->out_targets.clear();

    int slice_start = start_step;

    for (int s = start_step; s < eng->num_steps; ++s) {
        const StepDesc& st = eng->steps[s];
        switch (st.kind) {
            case OP_PASSTHROUGH:
                break;

            case OP_YIELD_IF_LEAKED: {
                if (!eng->step_has_leaked_target(st)) {
                    break;
                }
                eng->flush_slice(slice_start, s);
                eng->actions.push_back({ACT_YIELD_PYTHON, s, 0, 0, 0, 0.0});
                *actions_out = eng->actions.data();
                *targets_out = eng->out_targets.data();
                return (int)eng->actions.size();
            }

            case OP_FILTER_1Q_U: {
                if (!eng->step_has_leaked_target(st)) {
                    break;
                }
                eng->flush_slice(slice_start, s);
                int32_t off = (int32_t)eng->out_targets.size();
                int kept = 0;
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                const uint32_t* r_ptr = &eng->all_raw_targets[st.target_offset];
                for (int i = 0; i < st.target_count; ++i) {
                    if (eng->state[q_ptr[i]] < 2) {
                        eng->out_targets.push_back(r_ptr[i]);
                        kept++;
                    }
                }
                eng->actions.push_back({ACT_FILTERED_1Q, s, 0, off, kept, 0.0});
                break;
            }

            case OP_FILTER_2Q_UU: {
                if (!eng->step_has_leaked_target(st)) {
                    break;
                }
                eng->flush_slice(slice_start, s);
                int32_t off = (int32_t)eng->out_targets.size();
                int kept_targets = 0;
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                const uint32_t* r_ptr = &eng->all_raw_targets[st.target_offset];
                for (int i = 0; i < st.target_count; i += 2) {
                    if (eng->state[q_ptr[i]] < 2 && eng->state[q_ptr[i + 1]] < 2) {
                        eng->out_targets.push_back(r_ptr[i]);
                        eng->out_targets.push_back(r_ptr[i + 1]);
                        kept_targets += 2;
                    }
                }
                eng->actions.push_back({ACT_FILTERED_2Q, s, 0, off, kept_targets, 0.0});
                break;
            }

            case OP_TRANSITION_1_U: {
                bool has_leaked = eng->step_has_leaked_target(st);
                if (st.p_total_u == 0.0 && !has_leaked) {
                    break;
                }
                eng->buf_reset.clear();
                eng->buf_one.clear();
                eng->buf_depol.clear();
                eng->buf_x.clear();
                eng->buf_y.clear();
                eng->buf_z.clear();
                eng->buf_v.clear();
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                const TransBranch* br = &eng->all_branches[st.trans_offset];

                if (!has_leaked && st.p_total_u > 0.0 && st.p_total_u < 0.05) {
                    double log1mp = std::log(1.0 - st.p_total_u);
                    int idx = -1;
                    while (true) {
                        double u_skip = eng->next_double();
                        if (u_skip <= 1e-16) u_skip = 1e-16;
                        int skip = (int)(std::log(u_skip) / log1mp);
                        idx += 1 + skip;
                        if (idx >= st.target_count) break;
                        int q = q_ptr[idx];
                        double u = eng->next_double();
                        for (int b = 0; b < st.trans_count; ++b) {
                            if (br[b].in0 == -1 && u <= br[b].cum_prob) {
                                if (br[b].out0 != -1) {
                                    apply_2q_leg_transition(eng, q, false, br[b].out0);
                                }
                                break;
                            }
                        }
                    }
                } else {
                    for (int i = 0; i < st.target_count; ++i) {
                        int q = q_ptr[i];
                        uint8_t cur_s = eng->state[q];
                        if (cur_s < 2) {
                            if (st.p_total_u > 0.0 && eng->next_double() < st.p_total_u) {
                                double u = eng->next_double();
                                for (int b = 0; b < st.trans_count; ++b) {
                                    if (br[b].in0 == -1 && u <= br[b].cum_prob) {
                                        if (br[b].out0 != -1) {
                                            apply_2q_leg_transition(eng, q, false, br[b].out0);
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            double u = eng->next_double();
                            double acc = 0.0;
                            for (int b = 0; b < st.trans_count; ++b) {
                                if (br[b].in0 == (int8_t)cur_s) {
                                    acc += br[b].raw_prob;
                                    if (u < acc) {
                                        if (br[b].out0 != (int8_t)cur_s) {
                                            apply_2q_leg_transition(eng, q, true, br[b].out0);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                if (!eng->buf_reset.empty() || !eng->buf_one.empty() || !eng->buf_depol.empty() ||
                    !eng->buf_x.empty() || !eng->buf_y.empty() || !eng->buf_z.empty() || !eng->buf_v.empty()) {
                    eng->flush_slice(slice_start, s);
                    eng->push_target_action(ACT_INJECT_RESET, s, eng->buf_reset);
                    if (!eng->buf_one.empty()) {
                        eng->push_target_action(ACT_INJECT_RESET, s, eng->buf_one);
                        eng->push_target_action(ACT_INJECT_X, s, eng->buf_one);
                    }
                    eng->push_target_action(ACT_INJECT_DEPOL1, s, eng->buf_depol, 0.75);
                    eng->push_target_action(ACT_INJECT_X, s, eng->buf_x);
                    eng->push_target_action(ACT_INJECT_Y, s, eng->buf_y);
                    eng->push_target_action(ACT_INJECT_Z, s, eng->buf_z);
                    if (!eng->buf_v.empty()) {
                        eng->push_target_action(ACT_INJECT_X_ERR, s, eng->buf_v, 0.5);
                        eng->push_target_action(ACT_INJECT_Z_ERR, s, eng->buf_v, 0.5);
                    }
                }
                break;
            }

            case OP_TRANSITION_2_UU: {
                int num_pairs = st.target_count >> 1;
                bool has_leaked = eng->step_has_leaked_target(st);
                if (st.p_total_u == 0.0 && !has_leaked) {
                    break;
                }
                eng->buf_reset.clear();
                eng->buf_one.clear();
                eng->buf_depol.clear();
                eng->buf_x.clear();
                eng->buf_y.clear();
                eng->buf_z.clear();
                eng->buf_v.clear();
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                const TransBranch* br = &eng->all_branches[st.trans_offset];

                if (!has_leaked && st.p_total_u > 0.0 && st.p_total_u < 0.05) {
                    double log1mp = std::log(1.0 - st.p_total_u);
                    int pair_idx = -1;
                    while (true) {
                        double u_skip = eng->next_double();
                        if (u_skip <= 1e-16) u_skip = 1e-16;
                        int skip = (int)(std::log(u_skip) / log1mp);
                        pair_idx += 1 + skip;
                        if (pair_idx >= num_pairs) break;

                        int q0 = q_ptr[pair_idx * 2];
                        int q1 = q_ptr[pair_idx * 2 + 1];
                        double u_branch = eng->next_double();
                        for (int b = 0; b < st.trans_count; ++b) {
                            if (br[b].in0 == -1 && br[b].in1 == -1 && u_branch <= br[b].cum_prob) {
                                apply_2q_leg_transition(eng, q0, false, br[b].out0);
                                apply_2q_leg_transition(eng, q1, false, br[b].out1);
                                break;
                            }
                        }
                    }
                } else {
                    for (int p = 0; p < num_pairs; ++p) {
                        int q0 = q_ptr[p * 2];
                        int q1 = q_ptr[p * 2 + 1];
                        uint8_t s0 = eng->state[q0];
                        uint8_t s1 = eng->state[q1];
                        if (s0 < 2 && s1 < 2) {
                            if (st.p_total_u > 0.0 && eng->next_double() < st.p_total_u) {
                                double u_branch = eng->next_double();
                                for (int b = 0; b < st.trans_count; ++b) {
                                    if (br[b].in0 == -1 && br[b].in1 == -1 && u_branch <= br[b].cum_prob) {
                                        apply_2q_leg_transition(eng, q0, false, br[b].out0);
                                        apply_2q_leg_transition(eng, q1, false, br[b].out1);
                                        break;
                                    }
                                }
                            }
                        } else {
                            int8_t k0 = (s0 < 2) ? -1 : (int8_t)s0;
                            int8_t k1 = (s1 < 2) ? -1 : (int8_t)s1;
                            double u = eng->next_double();
                            double acc = 0.0;
                            for (int b = 0; b < st.trans_count; ++b) {
                                if (br[b].in0 == k0 && br[b].in1 == k1) {
                                    acc += br[b].raw_prob;
                                    if (u < acc) {
                                        if (br[b].out0 != k0) {
                                            apply_2q_leg_transition(eng, q0, s0 >= 2, br[b].out0);
                                        }
                                        if (br[b].out1 != k1) {
                                            apply_2q_leg_transition(eng, q1, s1 >= 2, br[b].out1);
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                if (!eng->buf_reset.empty() || !eng->buf_one.empty() || !eng->buf_depol.empty() ||
                    !eng->buf_x.empty() || !eng->buf_y.empty() || !eng->buf_z.empty() || !eng->buf_v.empty()) {
                    eng->flush_slice(slice_start, s);
                    eng->push_target_action(ACT_INJECT_RESET, s, eng->buf_reset);
                    if (!eng->buf_one.empty()) {
                        eng->push_target_action(ACT_INJECT_RESET, s, eng->buf_one);
                        eng->push_target_action(ACT_INJECT_X, s, eng->buf_one);
                    }
                    eng->push_target_action(ACT_INJECT_DEPOL1, s, eng->buf_depol, 0.75);
                    eng->push_target_action(ACT_INJECT_X, s, eng->buf_x);
                    eng->push_target_action(ACT_INJECT_Y, s, eng->buf_y);
                    eng->push_target_action(ACT_INJECT_Z, s, eng->buf_z);
                    if (!eng->buf_v.empty()) {
                        eng->push_target_action(ACT_INJECT_X_ERR, s, eng->buf_v, 0.5);
                        eng->push_target_action(ACT_INJECT_Z_ERR, s, eng->buf_v, 0.5);
                    }
                }
                break;
            }

            case OP_MEAS_PROJ_Z: {
                if (!eng->step_has_leaked_target(st)) {
                    break;
                }
                eng->flush_slice(slice_start, s);
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                for (int i = 0; i < st.target_count; ++i) {
                    if (eng->state[q_ptr[i]] > 2) {
                        eng->actions.push_back({ACT_YIELD_PYTHON, s, 0, 0, 0, 0.0});
                        *actions_out = eng->actions.data();
                        *targets_out = eng->out_targets.data();
                        return (int)eng->actions.size();
                    }
                }
                int32_t off = (int32_t)eng->out_targets.size();
                int leaked_cnt = 0;
                for (int i = 0; i < st.target_count; ++i) {
                    int q = q_ptr[i];
                    if (eng->state[q] >= 2) {
                        eng->out_targets.push_back((uint32_t)q);
                        leaked_cnt++;
                    }
                }
                eng->actions.push_back({ACT_MODIFIED_MEAS, s, 0, off, leaked_cnt, st.meas_leak_prob});
                break;
            }

            case OP_FUSED_TRANS1_MEAS: {
                int s_meas = s + 1;
                const StepDesc& st_meas = eng->steps[s_meas];
                bool had_pre_leaked = eng->step_has_leaked_target(st_meas);

                if (had_pre_leaked) {
                    const int32_t* q_meas_ptr = &eng->all_qubit_targets[st_meas.target_offset];
                    for (int i = 0; i < st_meas.target_count; ++i) {
                        if (eng->state[q_meas_ptr[i]] > 2) {
                            eng->flush_slice(slice_start, s);
                            eng->actions.push_back({ACT_YIELD_PYTHON, s, 0, 0, 0, 0.0});
                            *actions_out = eng->actions.data();
                            *targets_out = eng->out_targets.data();
                            return (int)eng->actions.size();
                        }
                    }
                }

                eng->buf_one.clear();
                const int32_t* q_ptr = &eng->all_qubit_targets[st.target_offset];
                if (st.p_total_u > 0.0) {
                    if (st.p_total_u < 0.05) {
                        double log1mp = std::log(1.0 - st.p_total_u);
                        int idx = -1;
                        while (true) {
                            double u_skip = eng->next_double();
                            if (u_skip <= 1e-16) u_skip = 1e-16;
                            int skip = (int)(std::log(u_skip) / log1mp);
                            idx += 1 + skip;
                            if (idx >= st.target_count) break;
                            int q = q_ptr[idx];
                            if (eng->state[q] < 2) {
                                eng->buf_one.push_back(q);
                            }
                        }
                    } else {
                        for (int i = 0; i < st.target_count; ++i) {
                            int q = q_ptr[i];
                            if (eng->state[q] < 2 && eng->next_double() < st.p_total_u) {
                                eng->buf_one.push_back(q);
                            }
                        }
                    }
                }

                int n_cand = (int)eng->buf_one.size();
                if (n_cand > 0) {
                    eng->flush_slice(slice_start, s);
                    int32_t cand_off = (int32_t)eng->out_targets.size();
                    for (int i = 0; i < n_cand; ++i) {
                        eng->out_targets.push_back((uint32_t)eng->buf_one[i]);
                    }
                    eng->actions.push_back({ACT_YIELD_FUSED_MEAS, s, s_meas, cand_off, n_cand, st_meas.meas_leak_prob});
                    *actions_out = eng->actions.data();
                    *targets_out = eng->out_targets.data();
                    return (int)eng->actions.size();
                } else {
                    if (had_pre_leaked) {
                        eng->flush_slice(slice_start, s_meas);
                        int32_t off = (int32_t)eng->out_targets.size();
                        int leaked_cnt = 0;
                        const int32_t* q_meas_ptr = &eng->all_qubit_targets[st_meas.target_offset];
                        for (int i = 0; i < st_meas.target_count; ++i) {
                            int q = q_meas_ptr[i];
                            if (eng->state[q] >= 2) {
                                eng->out_targets.push_back((uint32_t)q);
                                leaked_cnt++;
                            }
                        }
                        eng->actions.push_back({ACT_MODIFIED_MEAS, s_meas, 0, off, leaked_cnt, st_meas.meas_leak_prob});
                    }
                    s = s_meas;
                }
                break;
            }

            case OP_NEEDS_PYTHON: {
                eng->flush_slice(slice_start, s);
                eng->actions.push_back({ACT_YIELD_PYTHON, s, 0, 0, 0, 0.0});
                *actions_out = eng->actions.data();
                *targets_out = eng->out_targets.data();
                return (int)eng->actions.size();
            }
        }
    }

    if (eng->num_steps > slice_start) {
        eng->actions.push_back({ACT_RUN_SLICE, slice_start, eng->num_steps, 0, 0, 0.0});
    }
    *actions_out = eng->actions.data();
    *targets_out = eng->out_targets.data();
    return (int)eng->actions.size();
}

} // extern "C"
