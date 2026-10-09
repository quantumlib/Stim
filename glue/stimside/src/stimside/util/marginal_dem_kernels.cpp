// Copyright 2025 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr double kOne = 1.0 - 1e-9;
constexpr double kMaxUserEdgeWeight = 16777215.0;  // (1 << 24) - 1

inline double xor_prob(double p, double q) {
    return p + q - 2.0 * p * q;
}

enum MarginalOpKind : int32_t {
    MOP_NONE = 0,
    MOP_MEAS_FLAG = 1,
    MOP_MEAS_PROJ_Z = 2,
    MOP_TRANS_1 = 3,
    MOP_TRANS_2 = 4,
    MOP_UNTAGGED_1Q_UNITARY = 5,
    MOP_UNTAGGED_2Q_UNITARY = 6,
    MOP_BARE_NOOP = 7,
    MOP_BARE_1Q_UNITARY = 8,
    MOP_BARE_RESET = 9,
    MOP_BARE_OTHER = 10,
    MOP_CONDITIONED = 11,
};

struct OpDescC {
    int32_t kind;
    int32_t target_offset;
    int32_t target_count;
    int32_t aux_offset;
    int32_t aux_count;
    int32_t gate_id;
    int32_t reset_basis;      // 0=None, 1=+X, 3=+Y, 5=+Z
    int32_t ctrl_leg0_basis;  // 0=None, 1=+X, 3=+Y, 5=+Z
    int32_t ctrl_leg1_basis;  // 0=None, 1=+X, 3=+Y, 5=+Z
    int32_t cond_base_fires;
    int32_t cond_subkind;     // 0=other, 1=produces_meas, 2=is_reset, 3=noisy_gate, 4=1q_unitary, 5=2q_unitary, 6=heralded_or_noisy_nonmeas
    int32_t cond_channel_id;
};

struct SourceDescC {
    int32_t op_index;
    int32_t group;
    int32_t qubit;
    int32_t state;
    double weight;
    int32_t partner;
    int32_t partner_channel_id;  // -1 if None
};

struct Trans1RuleC {
    int32_t state;
    double p_unleak;
    int32_t next_state;  // -1 if no state has p >= kOne
};

struct Trans2RuleC {
    int32_t state;
    int32_t leg;
    double p_clear;
    double p_move;
    int32_t move_offset;
    int32_t move_count;
    int32_t max_move_state;
    int32_t stay_next_state;
    int32_t channel_id;  // -1 if None
};

struct MoveBranchC {
    int32_t state;
    double prob;
};

struct MeasStateProbC {
    int32_t state;
    double prob;
};

struct CondFireRuleC {
    int32_t state;
    int32_t mask;  // 1=subj0, 2=subj1, 3=both
    int32_t fires;
};

struct SiteKey {
    int32_t j;
    int32_t is_before;
    int32_t q0;
    int32_t q1;
    int32_t channel_id;

    bool operator==(const SiteKey& o) const {
        return j == o.j && is_before == o.is_before && q0 == o.q0 && q1 == o.q1 &&
               channel_id == o.channel_id;
    }
};

struct SiteKeyHash {
    size_t operator()(const SiteKey& k) const noexcept {
        uint64_t h = static_cast<uint32_t>(k.j);
        h = h * 1315423911ULL + static_cast<uint32_t>(k.is_before);
        h = h * 1315423911ULL + static_cast<uint32_t>(k.q0 + 1);
        h = h * 1315423911ULL + static_cast<uint32_t>(k.q1 + 1);
        h = h * 1315423911ULL + static_cast<uint32_t>(k.channel_id + 1);
        return static_cast<size_t>(h ^ (h >> 32));
    }
};

struct FlagVisit {
    int32_t flag_id;
    double prob;
    double alive;
};

struct FlowRecord {
    double src_weight;
    std::vector<int32_t> sites;
    std::vector<FlagVisit> flags;
};

struct TraceResult {
    int32_t error_op_index = -1;
    std::vector<SiteKey> unique_sites;
    std::vector<int32_t> flow_site_offsets;
    std::vector<int32_t> flow_site_ids;
    std::vector<int32_t> flag_cand_offsets;
    std::vector<int32_t> flag_cand_flows;
    std::vector<double> flag_cand_weights;
    std::vector<int32_t> pred_offsets;
    std::vector<int32_t> pred_flags;
};

struct EnvelopeResult {
    std::vector<int32_t> flag_env_offsets;
    std::vector<int32_t> flag_env_sym_ids;
    std::vector<double> flag_env_probs;
};

struct VecHash {
    size_t operator()(const std::vector<int32_t>& v) const noexcept {
        uint64_t h = 1469598103934665603ULL;
        for (int32_t x : v) {
            h ^= static_cast<uint32_t>(x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return static_cast<size_t>(h);
    }
};

struct BatchResult {
    std::vector<int32_t> shot_group_ids;
    std::vector<int32_t> group_event_offsets;
    std::vector<int32_t> group_events;
    std::vector<int32_t> group_item_offsets;
    std::vector<int32_t> group_sym_ids;
    std::vector<double> group_probs;
    std::vector<double> group_reweight_triples;  // 3 doubles per item when edge_reweights mode
};

template <typename T>
inline void copy_out(T* dst, const std::vector<T>& v) {
    if (dst != nullptr && !v.empty()) std::memcpy(dst, v.data(), v.size() * sizeof(T));
}

// ---------------------------------------------------------------------------
// Graph-like DEM decomposition: a line-for-line port of the Python
// `_decompose_dem_graphlike` / `_collect_known_graphlike` /
// `_graphlike_components` / `_decompose_hyperedge_component` routines. It works
// on the text of a flattened DEM (Stim prints probabilities with full
// round-trip precision, and error probabilities are copied as their original
// tokens) so the resulting DEM is identical to the Python one.
// ---------------------------------------------------------------------------

using IdVec = std::vector<int64_t>;

struct GComp {
    IdVec dets;
    IdVec obs;
};

inline bool gcomp_less(const GComp& a, const GComp& b) {
    if (a.dets != b.dets) return a.dets < b.dets;
    return a.obs < b.obs;
}

// Sort and cancel equal pairs (Python set-XOR semantics).
inline void xor_normalize(IdVec& v) {
    std::sort(v.begin(), v.end());
    size_t w = 0;
    size_t i = 0;
    while (i < v.size()) {
        size_t j = i;
        while (j < v.size() && v[j] == v[i]) ++j;
        if ((j - i) & 1) v[w++] = v[i];
        i = j;
    }
    v.resize(w);
}

// Symmetric difference of two sorted, duplicate-free id lists.
inline IdVec xor_sorted(const IdVec& a, const IdVec& b) {
    IdVec out;
    out.reserve(a.size() + b.size());
    size_t i = 0, j = 0;
    while (i < a.size() || j < b.size()) {
        if (j >= b.size() || (i < a.size() && a[i] < b[j])) {
            out.push_back(a[i++]);
        } else if (i >= a.size() || b[j] < a[i]) {
            out.push_back(b[j++]);
        } else {
            ++i;
            ++j;
        }
    }
    return out;
}

struct DemLine {
    bool is_error = false;
    size_t begin = 0;  // [begin, end) of the line
    size_t end = 0;
    size_t p_begin = 0;  // [p_begin, p_end) of the probability token (error lines)
    size_t p_end = 0;
    double p = 0.0;
    std::vector<GComp> comps;  // `_raw_inst_components` of the instruction
};

inline void finish_raw_comp(IdVec& dets, IdVec& obs, std::vector<GComp>& comps) {
    xor_normalize(dets);
    xor_normalize(obs);
    if (!dets.empty() || !obs.empty()) {
        comps.push_back(GComp{dets, obs});
    }
    dets.clear();
    obs.clear();
}

// Parse the text of a flattened DEM (no repeat blocks / shift_detectors).
void parse_flat_dem_text(const char* text, size_t len, std::vector<DemLine>& lines) {
    size_t pos = 0;
    IdVec dets;
    IdVec obs;
    while (pos < len) {
        size_t eol = pos;
        while (eol < len && text[eol] != '\n') ++eol;
        size_t b = pos;
        while (b < eol && (text[b] == ' ' || text[b] == '\t' || text[b] == '\r')) ++b;
        if (b < eol) {
            DemLine line;
            line.begin = b;
            line.end = eol;
            if (eol - b > 5 && std::strncmp(text + b, "error", 5) == 0 &&
                (text[b + 5] == '(' || text[b + 5] == '[')) {
                line.is_error = true;
                size_t k = b + 5;
                if (text[k] == '[') {
                    while (k < eol && text[k] != ']') ++k;
                    ++k;
                }
                if (k < eol && text[k] == '(') {
                    line.p_begin = k + 1;
                    while (k < eol && text[k] != ')') ++k;
                    line.p_end = k;
                    ++k;
                    std::string tok(text + line.p_begin, line.p_end - line.p_begin);
                    line.p = std::strtod(tok.c_str(), nullptr);
                }
                dets.clear();
                obs.clear();
                while (k < eol) {
                    while (k < eol && (text[k] == ' ' || text[k] == '\t' || text[k] == '\r')) ++k;
                    if (k >= eol) break;
                    char c = text[k];
                    if (c == '^') {
                        finish_raw_comp(dets, obs, line.comps);
                        ++k;
                    } else if (c == 'D' || c == 'L') {
                        char* endp = nullptr;
                        long long v = std::strtoll(text + k + 1, &endp, 10);
                        (c == 'D' ? dets : obs).push_back(static_cast<int64_t>(v));
                        k = static_cast<size_t>(endp - text);
                    } else {
                        while (k < eol && text[k] != ' ') ++k;
                    }
                }
                finish_raw_comp(dets, obs, line.comps);
            }
            lines.push_back(std::move(line));
        }
        pos = eol + 1;
    }
}

struct GKey {
    int64_t a;
    int64_t b;  // -1 for a single-detector key
    bool operator==(const GKey& o) const { return a == o.a && b == o.b; }
};

struct GKeyHash {
    size_t operator()(const GKey& k) const noexcept {
        uint64_t h = static_cast<uint64_t>(k.a) * 0x9E3779B97F4A7C15ULL;
        h ^= static_cast<uint64_t>(k.b + 1) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return static_cast<size_t>(h);
    }
};

struct KnownGraphlike {
    std::unordered_map<GKey, int32_t, GKeyHash> index;
    std::vector<GComp> values;

    bool empty() const { return values.empty(); }

    const GComp* get(int64_t a, int64_t b) const {
        auto it = index.find(GKey{a, b});
        return it == index.end() ? nullptr : &values[it->second];
    }

    // `_collect_known_graphlike`: first-seen 1- and 2-detector components of p > 0 errors.
    void collect(const std::vector<DemLine>& lines) {
        for (const auto& line : lines) {
            if (!line.is_error || !(line.p > 0.0)) continue;
            for (const auto& c : line.comps) {
                if (c.dets.size() >= 1 && c.dets.size() <= 2) {
                    GKey key{c.dets[0], c.dets.size() == 2 ? c.dets[1] : -1};
                    if (index.find(key) == index.end()) {
                        index.emplace(key, static_cast<int32_t>(values.size()));
                        values.push_back(c);
                    }
                }
            }
        }
    }
};

struct HyperedgeDecomposer {
    const IdVec& dets;
    const IdVec& target_obs;
    const KnownGraphlike& known;
    int n;
    std::vector<const GComp*> out;

    // Cover-search state (n <= 16).
    size_t best_missed = 0;
    std::vector<const GComp*> best_peeled;
    IdVec best_missed_dets;
    std::vector<const GComp*> cur_peeled;
    IdVec cur_missed_dets;

    HyperedgeDecomposer(const IdVec& d, const IdVec& o, const KnownGraphlike& k)
        : dets(d), target_obs(o), known(k), n(static_cast<int>(d.size())) {}

    bool search(int start, uint64_t used, const IdVec& cur_obs) {
        while (start < n && ((used >> start) & 1ULL)) ++start;
        if (start >= n) return cur_obs == target_obs;
        used |= 1ULL << start;
        int64_t d0 = dets[start];
        for (int k = start + 1; k <= n; ++k) {
            const GComp* match;
            uint64_t next_mask;
            if (k < n) {
                if ((used >> k) & 1ULL) continue;
                match = known.get(d0, dets[k]);
                next_mask = used | (1ULL << k);
            } else {
                match = known.get(d0, -1);
                next_mask = used;
            }
            if (match != nullptr) {
                if (search(start + 1, next_mask, xor_sorted(cur_obs, match->obs))) {
                    out.push_back(match);
                    return true;
                }
            }
        }
        return false;
    }

    void search_cover(int start, uint32_t used) {
        if (cur_missed_dets.size() >= best_missed) return;
        while (start < n && ((used >> start) & 1U)) ++start;
        if (start >= n) {
            best_missed = cur_missed_dets.size();
            best_peeled = cur_peeled;
            best_missed_dets = cur_missed_dets;
            return;
        }
        used |= 1U << start;
        int64_t d0 = dets[start];
        for (int k = start + 1; k < n; ++k) {
            if ((used >> k) & 1U) continue;
            const GComp* match = known.get(d0, dets[k]);
            if (match != nullptr) {
                cur_peeled.push_back(match);
                search_cover(start + 1, used | (1U << k));
                cur_peeled.pop_back();
                if (best_missed == 0) return;
            }
        }
        const GComp* match_single = known.get(d0, -1);
        if (match_single != nullptr) {
            cur_peeled.push_back(match_single);
            search_cover(start + 1, used);
            cur_peeled.pop_back();
            if (best_missed == 0) return;
        }
        if (cur_missed_dets.size() + 1 < best_missed) {
            cur_missed_dets.push_back(d0);
            search_cover(start + 1, used);
            cur_missed_dets.pop_back();
        }
    }

    // `_decompose_hyperedge_component` for a component with > 2 detectors.
    void run(std::vector<GComp>& result) {
        if (known.empty()) {
            for (int idx = 0; idx < n; idx += 2) {
                GComp c;
                c.dets.assign(dets.begin() + idx, dets.begin() + std::min(n, idx + 2));
                if (idx == 0) c.obs = target_obs;
                result.push_back(std::move(c));
            }
            return;
        }
        if (n < 64 && search(0, 0, IdVec())) {
            for (auto it = out.rbegin(); it != out.rend(); ++it) result.push_back(**it);
            return;
        }
        std::vector<GComp> peeled;
        IdVec missed;
        IdVec rem_obs;
        if (n <= 16) {
            best_missed = static_cast<size_t>(n) + 1;
            search_cover(0, 0);
            for (const GComp* m : best_peeled) peeled.push_back(*m);
            missed = best_missed_dets;
            rem_obs = target_obs;
            for (const auto& c : peeled) rem_obs = xor_sorted(rem_obs, c.obs);
        } else {
            std::vector<uint8_t> done(n, 0);
            rem_obs = target_obs;
            for (int k = 0; k < n; ++k) {
                if (done[k]) continue;
                for (int k2 = k + 1; k2 < n; ++k2) {
                    if (done[k2]) continue;
                    const GComp* match = known.get(dets[k], dets[k2]);
                    if (match != nullptr) {
                        done[k] = done[k2] = 1;
                        peeled.push_back(*match);
                        rem_obs = xor_sorted(rem_obs, match->obs);
                        break;
                    }
                }
            }
            for (int k = 0; k < n; ++k) {
                if (done[k]) continue;
                const GComp* match = known.get(dets[k], -1);
                if (match != nullptr) {
                    done[k] = 1;
                    peeled.push_back(*match);
                    rem_obs = xor_sorted(rem_obs, match->obs);
                }
            }
            for (int k = 0; k < n; ++k) {
                if (!done[k]) missed.push_back(dets[k]);
            }
        }
        if (missed.size() <= 2) {
            if (!missed.empty()) {
                peeled.push_back(GComp{missed, rem_obs});
            } else if (!rem_obs.empty() && !peeled.empty()) {
                peeled[0].obs = xor_sorted(peeled[0].obs, rem_obs);
            }
        } else {
            for (size_t idx = 0; idx < missed.size(); idx += 2) {
                GComp c;
                c.dets.assign(missed.begin() + idx, missed.begin() + std::min(missed.size(), idx + 2));
                if (idx == 0) c.obs = rem_obs;
                peeled.push_back(std::move(c));
            }
        }
        for (auto& c : peeled) result.push_back(std::move(c));
    }
};

inline void append_int(std::string& s, int64_t v) {
    char buf[24];
    int len = std::snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(v));
    s.append(buf, static_cast<size_t>(len));
}

bool flat_dem_has_hyperedge(const std::vector<DemLine>& lines) {
    for (const auto& line : lines) {
        if (!line.is_error || !(line.p > 0.0)) continue;
        for (const auto& c : line.comps) {
            if (c.dets.size() > 2) return true;
        }
    }
    return false;
}

}  // namespace

extern "C" {

// source_mode == false: the weighted (flag-attributed) trace of marginal_trace_flows.
// source_mode == true: the per-source trace of marginal_trace_source_flows (the
// "flags" of the result are the sources; see that function).
static TraceResult* trace_flows_impl(
    int32_t num_ops,
    int32_t num_qubits,
    int32_t num_flags,
    const OpDescC* ops,
    const int32_t* target_qubits,
    const int32_t* touch_offsets,
    const int32_t* touch_indices,
    const int32_t* conj_table,  // shape (num_gates, 7)
    const MeasStateProbC* meas_probs,
    const Trans1RuleC* trans1_rules,
    const Trans2RuleC* trans2_rules,
    const MoveBranchC* move_branches,
    const CondFireRuleC* cond_rules,
    int32_t num_sources,
    const SourceDescC* sources,
    bool source_mode
) {
    auto* res = new TraceResult();
    std::unordered_map<SiteKey, int32_t, SiteKeyHash> site_map;
    site_map.reserve(4096);

    auto get_site_id = [&](int32_t j, int32_t is_before, int32_t q0, int32_t q1, int32_t channel_id) -> int32_t {
        SiteKey key{j, is_before, q0, q1, channel_id};
        auto it = site_map.find(key);
        if (it != site_map.end()) {
            return it->second;
        }
        int32_t id = static_cast<int32_t>(res->unique_sites.size());
        res->unique_sites.push_back(key);
        site_map.emplace(key, id);
        return id;
    };

    std::vector<FlowRecord> all_flows;
    all_flows.reserve(static_cast<size_t>(num_sources) * 2);

    struct ActiveFlow {
        int32_t h;
        int32_t s;
        int32_t tracked;
        bool desynced;
        double alive;
        bool done;
        int32_t j;
        int32_t next_group;  // -1 means None
        bool can_fork;
        double src_weight;
        double mix;  // source_mode: the flow's weight within its source's envelope
        std::vector<int32_t> sites;
        std::vector<FlagVisit> flags;
    };

    auto depolarize_1 = [&](ActiveFlow& f, int32_t is_before, int32_t q) {
        f.sites.push_back(get_site_id(f.j, is_before, q, -1, 0));
        if (f.h == q) {
            f.tracked = 0;
            f.desynced = false;
        }
    };

    auto depolarize_1_or_2 = [&](ActiveFlow& f, int32_t is_before, int32_t q0, int32_t q1) {
        if (q0 < 0) {
            q0 = q1;
            q1 = -1;
        }
        if (q0 < 0) return;
        f.sites.push_back(get_site_id(f.j, is_before, q0, q1, 0));
        if (f.h == q0 || f.h == q1) {
            f.tracked = 0;
            f.desynced = false;
        }
    };

    auto do_bare = [&](ActiveFlow& f, const OpDescC& op, const int32_t* t_ptr, int32_t t_cnt, int32_t subkind) {
        if (subkind == 3 || subkind == 6 || subkind == 7) {  // heralded or noisy non-measurement
            return;
        }
        if (subkind == 4) {  // 1Q unitary
            const int32_t* c_row = conj_table + op.gate_id * 7;
            for (int32_t i = 0; i < t_cnt; ++i) {
                if (t_ptr[i] == f.h && f.tracked != 0) {
                    f.tracked = c_row[f.tracked];
                }
            }
            return;
        }
        if (subkind == 2) {  // pure reset (not producing measurements)
            f.tracked = op.reset_basis;
            f.desynced = false;
            return;
        }
        if (f.desynced) {
            depolarize_1(f, 1, f.h);
        }
        f.tracked = op.reset_basis;
    };

    auto do_skip = [&](ActiveFlow& f, const OpDescC& op, const int32_t* t_ptr, int32_t t_cnt, bool is_1q) {
        if (is_1q) {
            if (f.tracked != 0) {
                f.tracked = conj_table[op.gate_id * 7 + f.tracked];
            }
            f.desynced = true;
            return;
        }
        int32_t q0 = t_ptr[0];
        int32_t q1 = (t_cnt > 1) ? t_ptr[1] : -1;
        int32_t leg = (q0 == f.h) ? 0 : 1;
        int32_t partner = (leg == 0) ? q1 : q0;
        int32_t needed = (leg == 0) ? op.ctrl_leg0_basis : op.ctrl_leg1_basis;
        if (needed != 0 && partner >= 0) {
            if (f.tracked != needed) {
                depolarize_1(f, 1, f.h);
                f.tracked = needed;
                f.desynced = true;
            }
        } else {
            depolarize_1_or_2(f, 0, f.h, partner);
        }
    };

    std::vector<ActiveFlow> branches;

    auto run_single_flow = [&](ActiveFlow& f, std::vector<ActiveFlow>* out_branches) -> bool {
        while (!f.done) {
            if (f.next_group < 0) {
                if (f.h < 0 || f.h >= num_qubits) break;
                const int32_t* t_begin = touch_indices + touch_offsets[f.h];
                const int32_t* t_end = touch_indices + touch_offsets[f.h + 1];
                const int32_t* it = std::upper_bound(t_begin, t_end, f.j);
                if (it == t_end) break;
                f.j = *it;
                f.next_group = 0;
            }
            const OpDescC& op = ops[f.j];
            const int32_t* t_ptr = target_qubits + op.target_offset;
            int32_t t_cnt = op.target_count;

            switch (op.kind) {
                case MOP_MEAS_FLAG: {
                    double p = 0.0;
                    for (int32_t m = 0; m < op.aux_count; ++m) {
                        const MeasStateProbC& mp = meas_probs[op.aux_offset + m];
                        if (mp.state == f.s) {
                            p = mp.prob;
                            break;
                        }
                    }
                    if (p > 0.0) {
                        // target_qubits holds pairs (qubit, flag_id)
                        int32_t flag_id = -1;
                        for (int32_t i = 0; i < t_cnt; i += 2) {
                            if (t_ptr[i] == f.h) {
                                flag_id = t_ptr[i + 1];
                            }
                        }
                        if (flag_id >= 0) {
                            f.flags.push_back(FlagVisit{flag_id, p, f.alive});
                        }
                    }
                    break;
                }
                case MOP_MEAS_PROJ_Z: {
                    depolarize_1(f, 1, f.h);
                    depolarize_1(f, 0, f.h);
                    break;
                }
                case MOP_TRANS_1: {
                    int32_t start_g = f.next_group > 0 ? f.next_group : 0;
                    for (int32_t g = start_g; g < t_cnt; ++g) {
                        if (t_ptr[g] != f.h) continue;
                        double p_unleak = 0.0;
                        int32_t next_s = -1;
                        for (int32_t r = 0; r < op.aux_count; ++r) {
                            const Trans1RuleC& rule = trans1_rules[op.aux_offset + r];
                            if (rule.state == f.s) {
                                p_unleak = rule.p_unleak;
                                next_s = rule.next_state;
                                break;
                            }
                        }
                        if (p_unleak > 0.0) {
                            depolarize_1(f, 0, f.h);
                        }
                        if (p_unleak >= kOne) {
                            f.done = true;
                            break;
                        }
                        f.alive *= (1.0 - p_unleak);
                        if (next_s >= 2) {
                            f.s = next_s;
                        }
                    }
                    break;
                }
                case MOP_TRANS_2: {
                    int32_t num_pairs = t_cnt / 2;
                    int32_t start_k = f.next_group > 0 ? f.next_group : 0;
                    for (int32_t k = start_k; k < num_pairs; ++k) {
                        int32_t q0 = t_ptr[2 * k];
                        int32_t q1 = t_ptr[2 * k + 1];
                        if (f.h != q0 && f.h != q1) continue;
                        f.next_group = k + 1;
                        int32_t leg = (f.h == q0) ? 0 : 1;
                        int32_t partner = (leg == 0) ? q1 : q0;

                        const Trans2RuleC* matched_rule = nullptr;
                        for (int32_t r = 0; r < op.aux_count; ++r) {
                            const Trans2RuleC& rule = trans2_rules[op.aux_offset + r];
                            if (rule.state == f.s && rule.leg == leg) {
                                matched_rule = &rule;
                                break;
                            }
                        }
                        double p_clear = matched_rule ? matched_rule->p_clear : 0.0;
                        double p_move = matched_rule ? matched_rule->p_move : 0.0;
                        if (p_clear + p_move > 0.0) {
                            depolarize_1(f, 0, f.h);
                        }
                        if (p_move > 0.0 && p_move < kOne && f.can_fork && out_branches != nullptr && matched_rule) {
                            for (int32_t m = 0; m < matched_rule->move_count; ++m) {
                                const MoveBranchC& mb = move_branches[matched_rule->move_offset + m];
                                ActiveFlow br = f;  // inherits j, next_group, src_weight, sites, flags (done is false here)
                                br.h = partner;
                                br.s = mb.state;
                                br.tracked = 0;
                                br.desynced = false;
                                br.alive = f.alive * mb.prob;
                                br.mix = br.alive;
                                br.can_fork = false;
                                for (auto& fl : br.flags) fl.alive *= mb.prob;
                                out_branches->push_back(std::move(br));
                            }
                            double stay_scale = 1.0 - p_move;
                            for (auto& fl : f.flags) {
                                fl.alive *= stay_scale;
                            }
                        }
                        if (matched_rule && matched_rule->channel_id >= 0) {
                            f.sites.push_back(get_site_id(f.j, 0, partner, -1, matched_rule->channel_id));
                        }
                        if (p_move >= kOne && matched_rule) {
                            f.h = partner;
                            f.s = matched_rule->max_move_state;
                            f.tracked = 0;
                        } else if (p_clear + p_move >= kOne) {
                            f.done = true;
                            break;
                        } else {
                            f.alive *= (1.0 - p_clear - p_move);
                            if (matched_rule && matched_rule->stay_next_state >= 2) {
                                f.s = matched_rule->stay_next_state;
                            }
                        }
                    }
                    break;
                }
                case MOP_UNTAGGED_1Q_UNITARY: {
                    for (int32_t i = 0; i < t_cnt; ++i) {
                        if (t_ptr[i] == f.h) {
                            do_skip(f, op, t_ptr + i, 1, true);
                        }
                    }
                    break;
                }
                case MOP_UNTAGGED_2Q_UNITARY: {
                    for (int32_t i = 0; i + 1 < t_cnt; i += 2) {
                        if (t_ptr[i] == f.h || t_ptr[i + 1] == f.h) {
                            do_skip(f, op, t_ptr + i, 2, false);
                        }
                    }
                    break;
                }
                case MOP_BARE_NOOP:
                    break;
                case MOP_BARE_1Q_UNITARY:
                    do_bare(f, op, t_ptr, t_cnt, 4);
                    break;
                case MOP_BARE_RESET:
                    do_bare(f, op, t_ptr, t_cnt, 2);
                    break;
                case MOP_BARE_OTHER:
                    do_bare(f, op, t_ptr, t_cnt, 0);
                    break;
                case MOP_CONDITIONED: {
                    // Each group in target_qubits has 4 ints: (q0, q1, subj0, subj1)
                    int32_t num_groups = t_cnt / 4;
                    bool base = (op.cond_base_fires != 0);
                    for (int32_t g = 0; g < num_groups; ++g) {
                        int32_t q0 = t_ptr[4 * g + 0];
                        int32_t q1 = t_ptr[4 * g + 1];
                        int32_t subj0 = t_ptr[4 * g + 2];
                        int32_t subj1 = t_ptr[4 * g + 3];
                        bool h_in_group = (q0 == f.h) || (q1 >= 0 && q1 == f.h);
                        int32_t mask = 0;
                        if (subj0 == f.h) mask |= 1;
                        if (subj1 == f.h) mask |= 2;
                        bool traj = base;
                        if (mask != 0) {
                            for (int32_t r = 0; r < op.aux_count; ++r) {
                                const CondFireRuleC& cr = cond_rules[op.aux_offset + r];
                                if (cr.state == f.s && cr.mask == mask) {
                                    traj = (cr.fires != 0);
                                    break;
                                }
                            }
                        }
                        int32_t grp_qs[2] = {q0, q1};
                        int32_t grp_cnt = 2;
                        if (traj == base) {
                            if (traj && h_in_group) {
                                do_bare(f, op, grp_qs, grp_cnt, op.cond_subkind);
                            }
                            continue;
                        }
                        if (op.cond_subkind == 1 || op.cond_subkind == 7 || (traj && op.cond_subkind == 2)) {
                            res->error_op_index = f.j;
                            return false;
                        }
                        if (op.cond_subkind == 3 || op.cond_subkind == 6) {
                            if (traj && op.cond_channel_id >= 0 && q0 >= 0) {
                                f.sites.push_back(get_site_id(f.j, 0, q0, q1, op.cond_channel_id));
                            }
                            continue;
                        }
                        if (base && h_in_group && (op.cond_subkind == 4 || op.cond_subkind == 5)) {
                            do_skip(f, op, grp_qs, grp_cnt, op.cond_subkind == 4);
                            continue;
                        }
                        depolarize_1_or_2(f, 0, q0, q1);
                    }
                    break;
                }
                default:
                    break;
            }
            f.next_group = -1;
        }
        return true;
    };

    std::vector<int32_t> flow_source;  // source_mode only
    std::vector<double> flow_mix;      // source_mode only
    for (int32_t s_idx = 0; s_idx < num_sources; ++s_idx) {
        const SourceDescC& src = sources[s_idx];
        ActiveFlow root;
        root.h = src.qubit;
        root.s = src.state;
        root.tracked = 0;
        root.desynced = false;
        root.alive = 1.0;
        root.done = false;
        root.j = src.op_index;
        root.next_group = src.group + 1;
        root.can_fork = true;
        root.src_weight = src.weight;
        root.mix = 1.0;
        if (src.partner_channel_id >= 0 && src.partner >= 0) {
            root.sites.push_back(get_site_id(src.op_index, 0, src.partner, -1, src.partner_channel_id));
        }

        branches.clear();
        if (!run_single_flow(root, &branches)) {
            return res;
        }
        if (source_mode) {
            double branch_total = 0.0;
            for (const auto& br : branches) branch_total += br.mix;
            flow_source.push_back(s_idx);
            flow_mix.push_back(std::max(0.0, 1.0 - branch_total));
        }
        all_flows.push_back(FlowRecord{root.src_weight, std::move(root.sites), std::move(root.flags)});

        for (auto& br : branches) {
            if (!run_single_flow(br, nullptr)) {
                return res;
            }
            if (source_mode) {
                flow_source.push_back(s_idx);
                flow_mix.push_back(br.mix);
            }
            all_flows.push_back(FlowRecord{br.src_weight, std::move(br.sites), std::move(br.flags)});
        }
    }

    // Build candidates and predecessors per flag (per source in source_mode)
    const int32_t num_out = source_mode ? num_sources : num_flags;
    std::vector<std::vector<std::pair<int32_t, double>>> candidates(num_out);
    std::vector<std::vector<int32_t>> predecessors(num_out);
    std::vector<uint8_t> flow_used(all_flows.size(), 0);

    if (source_mode) {
        for (size_t k = 0; k < all_flows.size(); ++k) {
            if (flow_mix[k] > 0.0) {
                candidates[flow_source[k]].emplace_back(static_cast<int32_t>(k), flow_mix[k]);
                flow_used[k] = 1;
            }
        }
    }
    for (size_t k = 0; k < all_flows.size() && !source_mode; ++k) {
        const auto& fl = all_flows[k];
        double survive = 1.0;
        int32_t prev = -1;
        for (const auto& fv : fl.flags) {
            double weight = fl.src_weight * fv.alive * survive * fv.prob;
            if (weight > 0.0) {
                candidates[fv.flag_id].emplace_back(static_cast<int32_t>(k), weight);
                flow_used[k] = 1;
            }
            if (prev >= 0) {
                predecessors[fv.flag_id].push_back(prev);
            }
            survive *= (1.0 - fv.prob);
            prev = fv.flag_id;
        }
    }

    // Remap used flows to 0 .. num_used_flows - 1 and compact unique sites to only those used by used flows
    std::vector<int32_t> flow_remap(all_flows.size(), -1);
    std::vector<int32_t> site_remap(res->unique_sites.size(), -1);
    std::vector<SiteKey> compacted_sites;
    compacted_sites.reserve(res->unique_sites.size());

    res->flow_site_offsets.push_back(0);
    int32_t num_used_flows = 0;
    for (size_t k = 0; k < all_flows.size(); ++k) {
        if (!flow_used[k]) continue;
        flow_remap[k] = num_used_flows++;
        for (int32_t old_sid : all_flows[k].sites) {
            int32_t new_sid = site_remap[old_sid];
            if (new_sid < 0) {
                new_sid = static_cast<int32_t>(compacted_sites.size());
                site_remap[old_sid] = new_sid;
                compacted_sites.push_back(res->unique_sites[old_sid]);
            }
            res->flow_site_ids.push_back(new_sid);
        }
        res->flow_site_offsets.push_back(static_cast<int32_t>(res->flow_site_ids.size()));
    }
    res->unique_sites = std::move(compacted_sites);

    res->flag_cand_offsets.push_back(0);
    res->pred_offsets.push_back(0);
    for (int32_t f = 0; f < num_out; ++f) {
        for (const auto& cw : candidates[f]) {
            res->flag_cand_flows.push_back(flow_remap[cw.first]);
            res->flag_cand_weights.push_back(cw.second);
        }
        res->flag_cand_offsets.push_back(static_cast<int32_t>(res->flag_cand_flows.size()));

        auto& preds = predecessors[f];
        std::sort(preds.begin(), preds.end());
        preds.erase(std::unique(preds.begin(), preds.end()), preds.end());
        for (int32_t p : preds) {
            res->pred_flags.push_back(p);
        }
        res->pred_offsets.push_back(static_cast<int32_t>(res->pred_flags.size()));
    }

    return res;
}

TraceResult* marginal_trace_flows(
    int32_t num_ops,
    int32_t num_qubits,
    int32_t num_flags,
    const OpDescC* ops,
    const int32_t* target_qubits,
    const int32_t* touch_offsets,
    const int32_t* touch_indices,
    const int32_t* conj_table,  // shape (num_gates, 7)
    const MeasStateProbC* meas_probs,
    const Trans1RuleC* trans1_rules,
    const Trans2RuleC* trans2_rules,
    const MoveBranchC* move_branches,
    const CondFireRuleC* cond_rules,
    int32_t num_sources,
    const SourceDescC* sources
) {
    return trace_flows_impl(
        num_ops, num_qubits, num_flags, ops, target_qubits, touch_offsets, touch_indices,
        conj_table, meas_probs, trans1_rules, trans2_rules, move_branches, cond_rules,
        num_sources, sources, false);
}

// Per-source trace for the loss-oracle DEMs. Same arguments and result layout as
// marginal_trace_flows, but the result's "flags" are the sources (num_sources of
// them): the candidates of source s are all flows of s (its root flow and the hop
// branches forked from it), none dropped for not reaching a leakage flag, each
// with its weight pi within the source's envelope:
//   pi(branch forked at an LEAKAGE_TRANSITION_2 op into leaked state m)
//       = P(the leak is still on the root flow's qubit just before that op) * p_m,
//   pi(root) = max(0, 1 - sum of the branch weights).
// "Still on the qubit" uses the same unleak / move factors as the weighted model
// (the branch's alive at creation). There are no predecessors.
TraceResult* marginal_trace_source_flows(
    int32_t num_ops,
    int32_t num_qubits,
    int32_t num_flags,
    const OpDescC* ops,
    const int32_t* target_qubits,
    const int32_t* touch_offsets,
    const int32_t* touch_indices,
    const int32_t* conj_table,  // shape (num_gates, 7)
    const MeasStateProbC* meas_probs,
    const Trans1RuleC* trans1_rules,
    const Trans2RuleC* trans2_rules,
    const MoveBranchC* move_branches,
    const CondFireRuleC* cond_rules,
    int32_t num_sources,
    const SourceDescC* sources
) {
    return trace_flows_impl(
        num_ops, num_qubits, num_flags, ops, target_qubits, touch_offsets, touch_indices,
        conj_table, meas_probs, trans1_rules, trans2_rules, move_branches, cond_rules,
        num_sources, sources, true);
}

void marginal_trace_result_get_counts(
    const TraceResult* res,
    int32_t* out_error_op_index,
    int32_t* out_num_sites,
    int32_t* out_num_used_flows,
    int32_t* out_total_flow_sites,
    int32_t* out_total_cands,
    int32_t* out_total_preds
) {
    *out_error_op_index = res->error_op_index;
    *out_num_sites = static_cast<int32_t>(res->unique_sites.size());
    *out_num_used_flows = res->flow_site_offsets.empty() ? 0 : static_cast<int32_t>(res->flow_site_offsets.size()) - 1;
    *out_total_flow_sites = static_cast<int32_t>(res->flow_site_ids.size());
    *out_total_cands = static_cast<int32_t>(res->flag_cand_flows.size());
    *out_total_preds = static_cast<int32_t>(res->pred_flags.size());
}

void marginal_trace_result_copy_data(
    const TraceResult* res,
    int32_t* out_sites,  // 5 ints per site: j, is_before, q0, q1, channel_id
    int32_t* out_flow_site_offsets,
    int32_t* out_flow_site_ids,
    int32_t* out_flag_cand_offsets,
    int32_t* out_flag_cand_flows,
    double* out_flag_cand_weights,
    int32_t* out_pred_offsets,
    int32_t* out_pred_flags
) {
    for (size_t i = 0; i < res->unique_sites.size(); ++i) {
        const SiteKey& sk = res->unique_sites[i];
        out_sites[5 * i + 0] = sk.j;
        out_sites[5 * i + 1] = sk.is_before;
        out_sites[5 * i + 2] = sk.q0;
        out_sites[5 * i + 3] = sk.q1;
        out_sites[5 * i + 4] = sk.channel_id;
    }
    copy_out(out_flow_site_offsets, res->flow_site_offsets);
    copy_out(out_flow_site_ids, res->flow_site_ids);
    copy_out(out_flag_cand_offsets, res->flag_cand_offsets);
    copy_out(out_flag_cand_flows, res->flag_cand_flows);
    copy_out(out_flag_cand_weights, res->flag_cand_weights);
    copy_out(out_pred_offsets, res->pred_offsets);
    copy_out(out_pred_flags, res->pred_flags);
}

void marginal_free_trace_result(TraceResult* res) {
    delete res;
}

EnvelopeResult* marginal_build_envelopes(
    int32_t num_sites,
    int32_t num_symptoms,
    int32_t num_site_sym_entries,
    const int32_t* entry_site_ids,
    const int32_t* entry_sym_ids,
    const double* entry_probs,
    int32_t num_used_flows,
    const int32_t* flow_site_offsets,
    const int32_t* flow_site_ids,
    int32_t num_flags,
    const int32_t* flag_cand_offsets,
    const int32_t* flag_cand_flows,
    const double* flag_cand_weights
) {
    auto* res = new EnvelopeResult();
    if (num_flags <= 0) {
        res->flag_env_offsets.push_back(0);
        return res;
    }

    // 1. Group entries by site_id and XOR-merge duplicate sym_ids within each site
    std::vector<std::vector<std::pair<int32_t, double>>> site_syms(num_sites);
    for (int32_t i = 0; i < num_site_sym_entries; ++i) {
        int32_t sid = entry_site_ids[i];
        int32_t sym = entry_sym_ids[i];
        double p = entry_probs[i];
        if (sid >= 0 && sid < num_sites && sym >= 0 && sym < num_symptoms && p > 0.0) {
            auto& vec = site_syms[sid];
            bool found = false;
            for (auto& kv : vec) {
                if (kv.first == sym) {
                    kv.second = xor_prob(kv.second, p);
                    found = true;
                    break;
                }
            }
            if (!found) {
                vec.emplace_back(sym, p);
            }
        }
    }

    // 2. For each used flow, XOR-combine symptoms across its site_ids
    std::vector<int32_t> flow_sym_offsets(num_used_flows + 1, 0);
    std::vector<int32_t> flow_sym_ids;
    std::vector<double> flow_sym_probs;

    std::vector<double> scratch_prob(num_symptoms, 0.0);
    std::vector<int32_t> scratch_gen(num_symptoms, 0);
    std::vector<int32_t> touched;
    touched.reserve(64);
    int32_t gen = 1;

    for (int32_t k = 0; k < num_used_flows; ++k, ++gen) {
        touched.clear();
        int32_t start = flow_site_offsets[k];
        int32_t end = flow_site_offsets[k + 1];
        for (int32_t idx = start; idx < end; ++idx) {
            int32_t sid = flow_site_ids[idx];
            for (const auto& sp : site_syms[sid]) {
                int32_t sym = sp.first;
                double p = sp.second;
                if (scratch_gen[sym] != gen) {
                    scratch_gen[sym] = gen;
                    scratch_prob[sym] = p;
                    touched.push_back(sym);
                } else {
                    scratch_prob[sym] = xor_prob(scratch_prob[sym], p);
                }
            }
        }
        std::sort(touched.begin(), touched.end());
        for (int32_t sym : touched) {
            double p = scratch_prob[sym];
            if (p > 0.0) {
                flow_sym_ids.push_back(sym);
                flow_sym_probs.push_back(p);
            }
        }
        flow_sym_offsets[k + 1] = static_cast<int32_t>(flow_sym_ids.size());
    }

    // 3. For each flag, compute weighted sum over candidate flows
    res->flag_env_offsets.reserve(num_flags + 1);
    res->flag_env_offsets.push_back(0);
    for (int32_t f = 0; f < num_flags; ++f, ++gen) {
        int32_t c_start = flag_cand_offsets[f];
        int32_t c_end = flag_cand_offsets[f + 1];
        double total_w = 0.0;
        for (int32_t c = c_start; c < c_end; ++c) {
            total_w += flag_cand_weights[c];
        }
        if (total_w <= 0.0) {
            res->flag_env_offsets.push_back(static_cast<int32_t>(res->flag_env_sym_ids.size()));
            continue;
        }
        double inv_total = 1.0 / total_w;
        touched.clear();
        for (int32_t c = c_start; c < c_end; ++c) {
            int32_t k = flag_cand_flows[c];
            double w_norm = flag_cand_weights[c] * inv_total;
            int32_t s_start = flow_sym_offsets[k];
            int32_t s_end = flow_sym_offsets[k + 1];
            for (int32_t s = s_start; s < s_end; ++s) {
                int32_t sym = flow_sym_ids[s];
                double contrib = w_norm * flow_sym_probs[s];
                if (scratch_gen[sym] != gen) {
                    scratch_gen[sym] = gen;
                    scratch_prob[sym] = contrib;
                    touched.push_back(sym);
                } else {
                    scratch_prob[sym] += contrib;
                }
            }
        }
        std::sort(touched.begin(), touched.end());
        for (int32_t sym : touched) {
            double p = scratch_prob[sym];
            if (p > 0.0) {
                res->flag_env_sym_ids.push_back(sym);
                res->flag_env_probs.push_back(p);
            }
        }
        res->flag_env_offsets.push_back(static_cast<int32_t>(res->flag_env_sym_ids.size()));
    }

    return res;
}

int32_t marginal_envelope_result_get_total_entries(const EnvelopeResult* res) {
    return static_cast<int32_t>(res->flag_env_sym_ids.size());
}

void marginal_envelope_result_copy_data(
    const EnvelopeResult* res,
    int32_t* out_flag_env_offsets,
    int32_t* out_flag_env_sym_ids,
    double* out_flag_env_probs
) {
    copy_out(out_flag_env_offsets, res->flag_env_offsets);
    copy_out(out_flag_env_sym_ids, res->flag_env_sym_ids);
    copy_out(out_flag_env_probs, res->flag_env_probs);
}

void marginal_free_envelope_result(EnvelopeResult* res) {
    delete res;
}

BatchResult* marginal_process_shots(
    int32_t num_shots,
    int32_t num_flags,
    const uint8_t* raised_matrix,  // shape (num_shots, num_flags)
    const int32_t* pred_offsets,
    const int32_t* pred_flags,
    const int32_t* flag_env_offsets,
    const int32_t* flag_env_sym_ids,
    const double* flag_env_probs,
    int32_t num_symptoms,
    int32_t mode,  // 0 = leak_prob only (for full DEM), 1 = xor with base_sym_probs (reweight DEM), 2 = edge_reweights [node1, node2, weight]
    const double* base_sym_probs,  // length num_symptoms (used when mode == 1)
    int32_t num_edges,
    const int32_t* sym_edge_offsets,  // length num_symptoms + 1 (used when mode == 2)
    const int32_t* sym_edge_ids,      // CSR edge ids per symptom (used when mode == 2)
    const int32_t* edge_nodes,        // length 2 * num_edges: [node1, node2]
    const double* base_edge_probs     // length num_edges (used when mode == 2)
) {
    auto* res = new BatchResult();
    res->shot_group_ids.resize(num_shots, 0);

    std::unordered_map<std::vector<int32_t>, int32_t, VecHash> group_map;
    group_map.reserve(64);

    // Group 0 is always the empty events tuple ()
    std::vector<int32_t> empty_events;
    group_map.emplace(empty_events, 0);
    res->group_event_offsets.push_back(0);
    res->group_event_offsets.push_back(0);

    std::vector<int32_t> cur_events;
    cur_events.reserve(16);

    for (int32_t s = 0; s < num_shots; ++s) {
        const uint8_t* row = raised_matrix + static_cast<size_t>(s) * num_flags;
        cur_events.clear();
        for (int32_t f = 0; f < num_flags; ++f) {
            if (!row[f]) continue;
            // Only flags with a non-empty envelope
            if (flag_env_offsets[f] == flag_env_offsets[f + 1]) continue;
            int32_t p_start = pred_offsets[f];
            int32_t p_end = pred_offsets[f + 1];
            bool explained = false;
            for (int32_t idx = p_start; idx < p_end; ++idx) {
                if (row[pred_flags[idx]]) {
                    explained = true;
                    break;
                }
            }
            if (!explained) {
                cur_events.push_back(f);
            }
        }
        if (cur_events.empty()) {
            res->shot_group_ids[s] = 0;
            continue;
        }
        auto it = group_map.find(cur_events);
        if (it != group_map.end()) {
            res->shot_group_ids[s] = it->second;
        } else {
            int32_t gid = static_cast<int32_t>(res->group_event_offsets.size()) - 1;
            group_map.emplace(cur_events, gid);
            res->shot_group_ids[s] = gid;
            for (int32_t ev : cur_events) {
                res->group_events.push_back(ev);
            }
            res->group_event_offsets.push_back(static_cast<int32_t>(res->group_events.size()));
        }
    }

    int32_t num_groups = static_cast<int32_t>(res->group_event_offsets.size()) - 1;
    res->group_item_offsets.reserve(num_groups + 1);
    res->group_item_offsets.push_back(0);  // Group 0 (empty events) has 0 items
    res->group_item_offsets.push_back(0);

    if (num_groups <= 1) {
        return res;
    }

    std::vector<double> sym_prob(num_symptoms, 0.0);
    std::vector<int32_t> sym_gen(num_symptoms, 0);
    std::vector<int32_t> touched_syms;
    touched_syms.reserve(64);
    int32_t gen = 1;

    std::vector<double> edge_prob;
    std::vector<int32_t> edge_gen;
    std::vector<int32_t> touched_edges;
    if (mode == 2 && num_edges > 0) {
        edge_prob.assign(num_edges, 0.0);
        edge_gen.assign(num_edges, 0);
        touched_edges.reserve(64);
    }

    for (int32_t g = 1; g < num_groups; ++g, ++gen) {
        touched_syms.clear();
        int32_t ev_start = res->group_event_offsets[g];
        int32_t ev_end = res->group_event_offsets[g + 1];
        for (int32_t e_idx = ev_start; e_idx < ev_end; ++e_idx) {
            int32_t f = res->group_events[e_idx];
            int32_t s_start = flag_env_offsets[f];
            int32_t s_end = flag_env_offsets[f + 1];
            for (int32_t s_idx = s_start; s_idx < s_end; ++s_idx) {
                int32_t sym = flag_env_sym_ids[s_idx];
                double p = flag_env_probs[s_idx];
                if (sym_gen[sym] != gen) {
                    sym_gen[sym] = gen;
                    sym_prob[sym] = p;
                    touched_syms.push_back(sym);
                } else {
                    sym_prob[sym] = xor_prob(sym_prob[sym], p);
                }
            }
        }
        std::sort(touched_syms.begin(), touched_syms.end());

        if (mode == 0 || mode == 1) {
            for (int32_t sym : touched_syms) {
                double p = sym_prob[sym];
                if (p <= 0.0) continue;
                if (mode == 1 && base_sym_probs != nullptr) {
                    p = xor_prob(base_sym_probs[sym], p);
                }
                if (p > 0.0) {
                    res->group_sym_ids.push_back(sym);
                    res->group_probs.push_back(p);
                }
            }
            res->group_item_offsets.push_back(static_cast<int32_t>(res->group_sym_ids.size()));
        } else if (mode == 2) {
            touched_edges.clear();
            if (sym_edge_offsets != nullptr && sym_edge_ids != nullptr && num_edges > 0) {
                for (int32_t sym : touched_syms) {
                    double p = sym_prob[sym];
                    if (p <= 0.0) continue;
                    int32_t e_start = sym_edge_offsets[sym];
                    int32_t e_end = sym_edge_offsets[sym + 1];
                    for (int32_t e_idx = e_start; e_idx < e_end; ++e_idx) {
                        int32_t eid = sym_edge_ids[e_idx];
                        if (eid < 0 || eid >= num_edges) continue;
                        if (edge_gen[eid] != gen) {
                            edge_gen[eid] = gen;
                            edge_prob[eid] = p;
                            touched_edges.push_back(eid);
                        } else {
                            edge_prob[eid] = xor_prob(edge_prob[eid], p);
                        }
                    }
                }
            }
            std::sort(touched_edges.begin(), touched_edges.end());
            for (int32_t eid : touched_edges) {
                double p_leak = edge_prob[eid];
                if (p_leak <= 0.0) continue;
                double p_base = base_edge_probs ? base_edge_probs[eid] : 0.0;
                double p_total = xor_prob(p_base, p_leak);
                double w = 0.0;
                if (p_total <= 0.0) {
                    w = kMaxUserEdgeWeight;
                } else if (p_total < 0.5) {
                    w = std::log((1.0 - p_total) / p_total);
                    if (w < 0.0) w = 0.0;
                    if (w > kMaxUserEdgeWeight) w = kMaxUserEdgeWeight;
                } else {
                    w = 0.0;
                }
                res->group_reweight_triples.push_back(static_cast<double>(edge_nodes[2 * eid + 0]));
                res->group_reweight_triples.push_back(static_cast<double>(edge_nodes[2 * eid + 1]));
                res->group_reweight_triples.push_back(w);
            }
            res->group_item_offsets.push_back(static_cast<int32_t>(res->group_reweight_triples.size() / 3));
        }
    }

    return res;
}

void marginal_batch_result_get_counts(
    const BatchResult* res,
    int32_t* out_num_groups,
    int32_t* out_total_items
) {
    *out_num_groups = res->group_item_offsets.empty() ? 0 : static_cast<int32_t>(res->group_item_offsets.size()) - 1;
    *out_total_items = res->group_item_offsets.empty() ? 0 : res->group_item_offsets.back();
}

void marginal_batch_result_copy_data(
    const BatchResult* res,
    int32_t* out_shot_group_ids,
    int32_t* out_group_item_offsets,
    int32_t* out_group_sym_ids,
    double* out_group_probs,
    double* out_group_reweight_triples
) {
    copy_out(out_shot_group_ids, res->shot_group_ids);
    copy_out(out_group_item_offsets, res->group_item_offsets);
    copy_out(out_group_sym_ids, res->group_sym_ids);
    copy_out(out_group_probs, res->group_probs);
    copy_out(out_group_reweight_triples, res->group_reweight_triples);
}

void marginal_free_batch_result(BatchResult* res) {
    delete res;
}

// Returns 1 if the flattened DEM text has an error (p > 0) with a component of > 2 detectors.
int32_t marginal_dem_text_has_hyperedge(const char* text, int64_t len) {
    std::vector<DemLine> lines;
    parse_flat_dem_text(text, static_cast<size_t>(len), lines);
    return flat_dem_has_hyperedge(lines) ? 1 : 0;
}

// Port of the Python `_decompose_dem_graphlike(dem, base_dem)` body (after its
// hyperedge check): returns the text of the decomposed DEM. `base_text` may be null.
std::string* marginal_decompose_dem_text(
    const char* dem_text,
    int64_t dem_len,
    const char* base_text,
    int64_t base_len
) {
    std::vector<DemLine> lines;
    parse_flat_dem_text(dem_text, static_cast<size_t>(dem_len), lines);

    KnownGraphlike known;
    if (base_text != nullptr) {
        std::vector<DemLine> base_lines;
        parse_flat_dem_text(base_text, static_cast<size_t>(base_len), base_lines);
        known.collect(base_lines);
    }
    known.collect(lines);

    auto* out = new std::string();
    out->reserve(static_cast<size_t>(dem_len) + 64);
    std::vector<GComp> comps;
    std::vector<const GComp*> non_empty;
    for (const auto& line : lines) {
        if (!line.is_error) {
            out->append(dem_text + line.begin, line.end - line.begin);
            out->push_back('\n');
            continue;
        }
        comps.clear();
        for (const auto& c : line.comps) {
            if (c.dets.size() <= 2) {
                if (!c.dets.empty()) comps.push_back(c);
            } else {
                HyperedgeDecomposer dec(c.dets, c.obs, known);
                dec.run(comps);
            }
        }
        non_empty.clear();
        for (const auto& c : comps) {
            if (!c.dets.empty() || !c.obs.empty()) non_empty.push_back(&c);
        }
        if (non_empty.empty()) continue;
        if (non_empty.size() > 1) {
            std::sort(non_empty.begin(), non_empty.end(), [](const GComp* a, const GComp* b) {
                return gcomp_less(*a, *b);
            });
        }
        out->append("error(");
        out->append(dem_text + line.p_begin, line.p_end - line.p_begin);
        out->push_back(')');
        for (size_t idx = 0; idx < non_empty.size(); ++idx) {
            if (idx > 0) out->append(" ^");
            for (int64_t d : non_empty[idx]->dets) {
                out->append(" D");
                append_int(*out, d);
            }
            for (int64_t o : non_empty[idx]->obs) {
                out->append(" L");
                append_int(*out, o);
            }
        }
        out->push_back('\n');
    }
    return out;
}

int64_t marginal_text_result_size(const std::string* s) {
    return static_cast<int64_t>(s->size());
}

const char* marginal_text_result_data(const std::string* s) {
    return s->data();
}

void marginal_free_text_result(std::string* s) {
    delete s;
}

}  // extern "C"
