#include <cstdint>
#include <cstring>
#include <vector>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__)
#include <immintrin.h>
#endif

extern "C" {

static inline int word_dot_parity(const uint64_t* u, const uint64_t* v, int n_words) {
    uint64_t acc = 0;
    for (int w = 0; w < n_words; ++w) {
        acc ^= (u[w] & v[w]);
    }
    return __builtin_popcountll(acc) & 1;
}

static inline bool words_any_overlap(const uint64_t* u, const uint64_t* v, int n_words) {
    for (int w = 0; w < n_words; ++w) {
        if (u[w] & v[w]) return true;
    }
    return false;
}

static inline bool words_any_nonzero(const uint64_t* u, int n_words) {
    for (int w = 0; w < n_words; ++w) {
        if (u[w]) return true;
    }
    return false;
}

static inline int first_set_bit_in_words(const uint64_t* col_words, int n_words) {
    for (int w = 0; w < n_words; ++w) {
        uint64_t val = col_words[w];
        if (val) {
            return __builtin_ctzll(val) + (w << 6);
        }
    }
    return -1;
}

static inline int splitmix64_draw_bit(uint64_t* rng_state) {
    uint64_t z = (*rng_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return int((z ^ (z >> 31)) & 1ULL);
}

struct CppMRQFShot {
    int d = 0;
    int cap = 0;
    int n_words = 0;
    std::vector<uint64_t> h_words;
    std::vector<uint64_t> a_union;
    std::vector<uint64_t> A_buf;
    std::vector<uint8_t> ell_buf;
    std::vector<uint8_t> Gamma_buf;
    std::vector<int32_t> P;

    void init(int nw, int init_cap = 64) {
        n_words = nw;
        cap = init_cap;
        d = 0;
        h_words.assign(nw, 0);
        a_union.assign(nw, 0);
        A_buf.assign(cap * nw, 0);
        ell_buf.assign(cap, 0);
        Gamma_buf.assign(cap * cap, 0);
        P.assign(cap, -1);
    }

    void clear() {
        d = 0;
        std::fill(h_words.begin(), h_words.end(), 0ULL);
        std::fill(a_union.begin(), a_union.end(), 0ULL);
    }

    void ensure_capacity(int need_d) {
        if (need_d <= cap) return;
        int new_cap = cap > 0 ? cap : 16;
        while (new_cap < need_d) new_cap *= 2;
        std::vector<uint64_t> new_A(new_cap * n_words, 0);
        std::vector<uint8_t> new_ell(new_cap, 0);
        std::vector<uint8_t> new_Gamma(new_cap * new_cap, 0);
        std::vector<int32_t> new_P(new_cap, -1);
        if (d > 0) {
            std::memcpy(new_A.data(), A_buf.data(), d * n_words * sizeof(uint64_t));
            std::memcpy(new_ell.data(), ell_buf.data(), d * sizeof(uint8_t));
            for (int r = 0; r < d; ++r) {
                std::memcpy(&new_Gamma[r * new_cap], &Gamma_buf[r * cap], d * sizeof(uint8_t));
            }
            std::memcpy(new_P.data(), P.data(), d * sizeof(int32_t));
        }
        cap = new_cap;
        A_buf.swap(new_A);
        ell_buf.swap(new_ell);
        Gamma_buf.swap(new_Gamma);
        P.swap(new_P);
    }

    void recompute_a_union() {
        for (int w = 0; w < n_words; ++w) {
            uint64_t acc = 0;
            for (int r = 0; r < d; ++r) {
                acc |= A_buf[r * n_words + w];
            }
            a_union[w] = acc;
        }
    }

    void s1_add_column(int r, int s) {
        uint8_t ell_r = ell_buf[r];
        uint8_t ell_s = ell_buf[s];
        uint8_t gamma_rs = Gamma_buf[r * cap + s];

        uint64_t* A_s = &A_buf[s * n_words];
        const uint64_t* A_r = &A_buf[r * n_words];
        for (int w = 0; w < n_words; ++w) {
            A_s[w] ^= A_r[w];
        }

        ell_buf[s] = (ell_s + ell_r + (gamma_rs << 1)) & 3;
        uint8_t new_gamma_rs = gamma_rs ^ (ell_r & 1);

        uint8_t* G_s = &Gamma_buf[s * cap];
        const uint8_t* G_r = &Gamma_buf[r * cap];
        for (int k = 0; k < d; ++k) {
            uint8_t v = G_s[k] ^ G_r[k];
            G_s[k] = v;
            Gamma_buf[k * cap + s] = v;
        }
        Gamma_buf[r * cap + s] = new_gamma_rs;
        Gamma_buf[s * cap + r] = new_gamma_rs;
        Gamma_buf[s * cap + s] = 0;
    }

    void s4_drop_variable(int j) {
        int k = d;
        if (k == 1) {
            d = 0;
            std::fill(a_union.begin(), a_union.end(), 0ULL);
            return;
        }
        if (j < k - 1) {
            std::memmove(&A_buf[j * n_words], &A_buf[(j + 1) * n_words], (k - 1 - j) * n_words * sizeof(uint64_t));
            std::memmove(&ell_buf[j], &ell_buf[j + 1], (k - 1 - j) * sizeof(uint8_t));
            for (int r = j; r < k - 1; ++r) {
                std::memcpy(&Gamma_buf[r * cap], &Gamma_buf[(r + 1) * cap], k * sizeof(uint8_t));
            }
            for (int r = 0; r < k - 1; ++r) {
                std::memmove(&Gamma_buf[r * cap + j], &Gamma_buf[r * cap + j + 1], (k - 1 - j) * sizeof(uint8_t));
            }
            std::memmove(&P[j], &P[j + 1], (k - 1 - j) * sizeof(int32_t));
        }
        d = k - 1;
        recompute_a_union();
    }

    void s3_restrict(const uint8_t* lam, int eps) {
        int first_nz = -1;
        for (int idx = 0; idx < d; ++idx) {
            if (lam[idx]) {
                if (first_nz == -1) first_nz = idx;
                else s1_add_column(first_nz, idx);
            }
        }
        int eps_bit = eps & 1;
        if (first_nz == -1) return;
        if (eps_bit) {
            const uint64_t* A_f = &A_buf[first_nz * n_words];
            for (int w = 0; w < n_words; ++w) h_words[w] ^= A_f[w];
            const uint8_t* G_f = &Gamma_buf[first_nz * cap];
            for (int idx = 0; idx < d; ++idx) {
                ell_buf[idx] = (ell_buf[idx] + (G_f[idx] << 1)) & 3;
            }
        }
        s4_drop_variable(first_nz);
    }

    void s5_eliminate_zero(int j) {
        uint8_t c = ell_buf[j];
        int k = d;
        uint8_t lam_stack[256];
        std::vector<uint8_t> lam_vec;
        uint8_t* lam = lam_stack;
        if (k > 256) {
            lam_vec.resize(k);
            lam = lam_vec.data();
        }
        for (int r = 0; r < j; ++r) lam[r] = Gamma_buf[r * cap + j];
        for (int r = j + 1; r < k; ++r) lam[r - 1] = Gamma_buf[r * cap + j];
        s4_drop_variable(j);
        int d_new = d;
        if (c & 1) {
            for (int r = 0; r < d_new; ++r) {
                ell_buf[r] = (ell_buf[r] - c * lam[r]) & 3;
            }
            for (int r = 0; r < d_new; ++r) {
                if (!lam[r]) continue;
                for (int s = 0; s < d_new; ++s) {
                    if (r != s && lam[s]) Gamma_buf[r * cap + s] ^= 1;
                }
            }
            return;
        }
        bool any_lam = false;
        for (int r = 0; r < d_new; ++r) if (lam[r]) { any_lam = true; break; }
        if (any_lam) s3_restrict(lam, c >> 1);
    }

    void normalize_column(int idx) {
        for (int r = 0; r < idx; ++r) {
            int32_t p_r = P[r];
            if ((A_buf[idx * n_words + (p_r >> 6)] >> (p_r & 63)) & 1ULL) {
                s1_add_column(r, idx);
            }
        }
        int p_new = first_set_bit_in_words(&A_buf[idx * n_words], n_words);
        if (p_new >= 0) {
            int p_word = p_new >> 6;
            int p_bit = p_new & 63;
            for (int s = 0; s < d; ++s) {
                if (s != idx && ((A_buf[s * n_words + p_word] >> p_bit) & 1ULL)) {
                    s1_add_column(idx, s);
                }
            }
            P[idx] = p_new;
        } else {
            s5_eliminate_zero(idx);
        }
    }

    void p1_apply_pauli(const uint64_t* a_words, const uint64_t* b_words, int kappa) {
        if (d > 0 && words_any_overlap(b_words, a_union.data(), n_words)) {
            for (int r = 0; r < d; ++r) {
                int t = word_dot_parity(&A_buf[r * n_words], b_words, n_words);
                ell_buf[r] = (ell_buf[r] + (t << 1)) & 3;
            }
        }
        for (int w = 0; w < n_words; ++w) h_words[w] ^= a_words[w];
    }

    void p2_apply_quarter_turn(const uint64_t* a_words, const uint64_t* b_words, int kappa) {
        int d0 = d;
        ensure_capacity(d0 + 1);
        if (d0 > 0) {
            if (words_any_overlap(b_words, a_union.data(), n_words)) {
                for (int r = 0; r < d0; ++r) {
                    uint8_t t = word_dot_parity(&A_buf[r * n_words], b_words, n_words);
                    Gamma_buf[r * cap + d0] = t;
                    Gamma_buf[d0 * cap + r] = t;
                }
            } else {
                for (int r = 0; r < d0; ++r) {
                    Gamma_buf[r * cap + d0] = 0;
                    Gamma_buf[d0 * cap + r] = 0;
                }
            }
        }
        Gamma_buf[d0 * cap + d0] = 0;
        int bh = word_dot_parity(b_words, h_words.data(), n_words);
        uint8_t c = (kappa + 1 + (bh << 1)) & 3;
        for (int w = 0; w < n_words; ++w) {
            A_buf[d0 * n_words + w] = a_words[w];
            a_union[w] |= a_words[w];
        }
        ell_buf[d0] = c;
        P[d0] = -1;
        d = d0 + 1;
        normalize_column(d0);
    }

    void p2_apply_controlled_pauli(
        const uint64_t* a1_words, const uint64_t* b1_words, int kappa1,
        const uint64_t* a2_words, const uint64_t* b2_words, int kappa2
    ) {
        int d0 = d;
        ensure_capacity(d0 + 2);
        if (d0 > 0) {
            if (words_any_overlap(b1_words, a_union.data(), n_words)) {
                for (int r = 0; r < d0; ++r) {
                    uint8_t t = word_dot_parity(&A_buf[r * n_words], b1_words, n_words);
                    Gamma_buf[r * cap + d0] = t;
                    Gamma_buf[d0 * cap + r] = t;
                }
            } else {
                for (int r = 0; r < d0; ++r) {
                    Gamma_buf[r * cap + d0] = 0;
                    Gamma_buf[d0 * cap + r] = 0;
                }
            }
            if (words_any_overlap(b2_words, a_union.data(), n_words)) {
                for (int r = 0; r < d0; ++r) {
                    uint8_t t = word_dot_parity(&A_buf[r * n_words], b2_words, n_words);
                    Gamma_buf[r * cap + (d0 + 1)] = t;
                    Gamma_buf[(d0 + 1) * cap + r] = t;
                }
            } else {
                for (int r = 0; r < d0; ++r) {
                    Gamma_buf[r * cap + (d0 + 1)] = 0;
                    Gamma_buf[(d0 + 1) * cap + r] = 0;
                }
            }
        }
        int bh1 = word_dot_parity(b1_words, h_words.data(), n_words);
        int bh2 = word_dot_parity(b2_words, h_words.data(), n_words);
        uint8_t c1 = (kappa1 + (bh1 << 1)) & 3;
        uint8_t c2 = (kappa2 + (bh2 << 1)) & 3;
        uint8_t gamma12 = 1 ^ word_dot_parity(b1_words, a2_words, n_words);

        for (int w = 0; w < n_words; ++w) {
            A_buf[d0 * n_words + w] = a1_words[w];
            A_buf[(d0 + 1) * n_words + w] = a2_words[w];
            a_union[w] |= a1_words[w] | a2_words[w];
        }
        ell_buf[d0] = c1;
        ell_buf[d0 + 1] = c2;
        Gamma_buf[d0 * cap + d0] = 0;
        Gamma_buf[(d0 + 1) * cap + (d0 + 1)] = 0;
        Gamma_buf[d0 * cap + (d0 + 1)] = gamma12;
        Gamma_buf[(d0 + 1) * cap + d0] = gamma12;
        P[d0] = -1;
        P[d0 + 1] = -1;
        d = d0 + 2;

        normalize_column(d0);
        if (d > 0 && P[d - 1] == -1) {
            normalize_column(d - 1);
        }
    }

    int p3_measure_diagonal(const uint64_t* b_words, int sigma, int requested_bit, uint64_t* rng_state) {
        int bh = word_dot_parity(b_words, h_words.data(), n_words);
        int beta = (sigma ^ bh) & 1;
        if (d == 0 || !words_any_overlap(b_words, a_union.data(), n_words)) {
            return beta;
        }
        int first_nz = -1;
        int other_nz_stack[256];
        std::vector<int> other_nz_vec;
        int* other_nz = other_nz_stack;
        if (d > 256) {
            other_nz_vec.resize(d);
            other_nz = other_nz_vec.data();
        }
        int n_other = 0;
        for (int r = 0; r < d; ++r) {
            if (word_dot_parity(&A_buf[r * n_words], b_words, n_words)) {
                if (first_nz == -1) first_nz = r;
                else other_nz[n_other++] = r;
            }
        }
        if (first_nz == -1) {
            return beta;
        }
        int m;
        if (requested_bit >= 0) {
            m = requested_bit & 1;
        } else {
            m = splitmix64_draw_bit(rng_state);
        }

        int eps_bit = (m ^ beta) & 1;
        for (int i = 0; i < n_other; ++i) {
            s1_add_column(first_nz, other_nz[i]);
        }
        if (eps_bit) {
            const uint64_t* A_f = &A_buf[first_nz * n_words];
            for (int w = 0; w < n_words; ++w) h_words[w] ^= A_f[w];
            const uint8_t* G_f = &Gamma_buf[first_nz * cap];
            for (int idx = 0; idx < d; ++idx) {
                ell_buf[idx] = (ell_buf[idx] + (G_f[idx] << 1)) & 3;
            }
        }
        s4_drop_variable(first_nz);
        return m;
    }

    int q1_pauli_expectation(const uint64_t* a_words, const uint64_t* b_words, int kappa) const {
        if (d == 0) {
            for (int w = 0; w < n_words; ++w) {
                if (a_words[w]) return 0;
            }
            int bh = word_dot_parity(b_words, h_words.data(), n_words);
            int exp_mod4 = (kappa + (bh << 1)) & 3;
            return (exp_mod4 == 0) ? 1 : -1;
        }
        for (int w = 0; w < n_words; ++w) {
            if (a_words[w] & ~a_union[w]) return 0;
        }
        uint8_t s_stack[256];
        std::vector<uint8_t> s_vec;
        uint8_t* s = s_stack;
        if (d > 256) {
            s_vec.resize(d);
            s = s_vec.data();
        }
        int n_nz = 0;
        for (int r = 0; r < d; ++r) {
            int32_t p_r = P[r];
            uint8_t bit = uint8_t((a_words[p_r >> 6] >> (p_r & 63)) & 1ULL);
            s[r] = bit;
            if (bit) n_nz++;
        }
        if (n_nz == 0) {
            for (int w = 0; w < n_words; ++w) {
                if (a_words[w]) return 0;
            }
        } else {
            for (int w = 0; w < n_words; ++w) {
                uint64_t recon = 0;
                for (int r = 0; r < d; ++r) {
                    if (s[r]) recon ^= A_buf[r * n_words + w];
                }
                if (recon != a_words[w]) return 0;
            }
        }
        for (int r = 0; r < d; ++r) {
            int at_b_r = word_dot_parity(&A_buf[r * n_words], b_words, n_words);
            int gs = 0;
            const uint8_t* G_r = &Gamma_buf[r * cap];
            for (int c = 0; c < d; ++c) {
                gs ^= (G_r[c] & s[c]);
            }
            int ws_r = ((ell_buf[r] & 1) & s[r]) ^ (gs & 1);
            if (at_b_r != ws_r) return 0;
        }
        int lin = 0;
        int quad = 0;
        for (int r = 0; r < d; ++r) {
            if (s[r]) {
                lin += ell_buf[r];
                const uint8_t* G_r = &Gamma_buf[r * cap];
                for (int c = r + 1; c < d; ++c) {
                    if (s[c] && G_r[c]) quad++;
                }
            }
        }
        int q_s = (lin + (quad << 1)) & 3;
        int bh = word_dot_parity(b_words, h_words.data(), n_words);
        int exp_mod4 = (kappa + (bh << 1) - q_s) & 3;
        return (exp_mod4 == 0) ? 1 : -1;
    }
};

struct BatchedMRQFEngine {
    int num_qubits;
    int batch_size;
    int n_words;
    std::vector<uint64_t> owned_rng_states;
    uint64_t* shot_rng_states;
    std::vector<CppMRQFShot> shots;
    std::vector<uint8_t> is_active;
    std::vector<uint8_t> scratch_delta;
    std::vector<uint64_t> tmp_words_1;
    std::vector<uint64_t> tmp_words_2;

    BatchedMRQFEngine(int nq, int bs, uint64_t seed, uint64_t* ext_rng_states)
        : num_qubits(nq),
          batch_size(bs),
          n_words((nq + 63) / 64 > 0 ? (nq + 63) / 64 : 1),
          owned_rng_states(bs),
          shots(bs),
          is_active(bs, 0),
          scratch_delta(bs, 0),
          tmp_words_1(n_words, 0),
          tmp_words_2(n_words, 0) {
        uint64_t base = seed ? seed : 123456789ULL;
        for (int b = 0; b < bs; ++b) {
            owned_rng_states[b] = ((base + 1ULL) * 0x9e3779b97f4a7c15ULL) ^ (uint64_t(b) * 0xbf58476d1ce4e5b9ULL);
            shots[b].init(n_words, 64);
        }
        shot_rng_states = ext_rng_states ? ext_rng_states : owned_rng_states.data();
    }

    int draw_bit(int b) {
        return splitmix64_draw_bit(&shot_rng_states[b]);
    }

    void evict_single_shot(
        int b,
        const uint64_t* x2z_w,
        const uint64_t* z2z_w,
        uint8_t* xs_base,
        uint8_t* zs_base,
        uint8_t* out_evict_x,
        uint8_t* out_evict_z,
        int* any_evict_x,
        int* any_evict_z
    ) {
        if (!is_active[b] || shots[b].d != 0) return;
        is_active[b] = 0;
        int B = batch_size;
        int nw = n_words;
        const uint64_t* h = shots[b].h_words.data();
        if (words_any_nonzero(h, nw)) {
            for (int q = 0; q < num_qubits; ++q) {
                int xp = word_dot_parity(z2z_w + q * nw, h, nw);
                int zp = word_dot_parity(x2z_w + q * nw, h, nw);
                if (xp) {
                    out_evict_x[q * B + b] ^= 1;
                    if (xs_base) xs_base[q * B + b] ^= 1;
                    *any_evict_x = 1;
                }
                if (zp) {
                    out_evict_z[q * B + b] ^= 1;
                    if (zs_base) zs_base[q * B + b] ^= 1;
                    *any_evict_z = 1;
                }
            }
            shots[b].clear();
        }
    }

    void evict_healed_shots(
        const uint64_t* x2z_w,
        const uint64_t* z2z_w,
        uint8_t* xs_base,
        uint8_t* zs_base,
        uint8_t* out_evict_x,
        uint8_t* out_evict_z,
        int* any_evict_x,
        int* any_evict_z
    ) {
        for (int b = 0; b < batch_size; ++b) {
            if (is_active[b] && shots[b].d == 0) {
                evict_single_shot(b, x2z_w, z2z_w, xs_base, zs_base, out_evict_x, out_evict_z, any_evict_x, any_evict_z);
            }
        }
    }
};

void* engine_create(int num_qubits, int batch_size, uint64_t seed, uint64_t* ext_rng_states) {
    return new BatchedMRQFEngine(num_qubits, batch_size, seed, ext_rng_states);
}

void engine_destroy(void* ptr) {
    delete static_cast<BatchedMRQFEngine*>(ptr);
}

void engine_set_rng_states(void* ptr, uint64_t* ext_rng_states) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    eng->shot_rng_states = ext_rng_states ? ext_rng_states : eng->owned_rng_states.data();
}

void engine_clear(void* ptr) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    for (int b = 0; b < eng->batch_size; ++b) {
        if (eng->is_active[b]) {
            eng->shots[b].clear();
            eng->is_active[b] = 0;
        }
    }
}

int engine_num_active(void* ptr) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int count = 0;
    for (int b = 0; b < eng->batch_size; ++b) {
        if (eng->is_active[b] && (eng->shots[b].d > 0 || words_any_nonzero(eng->shots[b].h_words.data(), eng->n_words))) {
            count++;
        }
    }
    return count;
}

int engine_get_active_shot_indices(void* ptr, int32_t* out_indices, int32_t* out_d) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int count = 0;
    for (int b = 0; b < eng->batch_size; ++b) {
        if (eng->is_active[b] && (eng->shots[b].d > 0 || words_any_nonzero(eng->shots[b].h_words.data(), eng->n_words))) {
            if (out_indices) out_indices[count] = b;
            if (out_d) out_d[count] = eng->shots[b].d;
            count++;
        }
    }
    return count;
}

void engine_export_shot(
    void* ptr,
    int b,
    uint64_t* out_h,
    uint64_t* out_au,
    uint64_t* out_A,
    uint8_t* out_ell,
    uint8_t* out_Gamma,
    int32_t* out_P
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    const CppMRQFShot& s = eng->shots[b];
    int nw = eng->n_words;
    int d = s.d;
    std::memcpy(out_h, s.h_words.data(), nw * sizeof(uint64_t));
    std::memcpy(out_au, s.a_union.data(), nw * sizeof(uint64_t));
    if (d > 0) {
        std::memcpy(out_A, s.A_buf.data(), d * nw * sizeof(uint64_t));
        std::memcpy(out_ell, s.ell_buf.data(), d * sizeof(uint8_t));
        for (int r = 0; r < d; ++r) {
            std::memcpy(out_Gamma + r * d, &s.Gamma_buf[r * s.cap], d * sizeof(uint8_t));
        }
        std::memcpy(out_P, s.P.data(), d * sizeof(int32_t));
    }
}

void engine_import_shot(
    void* ptr,
    int b,
    int d,
    const uint64_t* in_h,
    const uint64_t* in_au,
    const uint64_t* in_A,
    const uint8_t* in_ell,
    const uint8_t* in_Gamma,
    const int32_t* in_P
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    CppMRQFShot& s = eng->shots[b];
    int nw = eng->n_words;
    s.ensure_capacity(d);
    s.d = d;
    std::memcpy(s.h_words.data(), in_h, nw * sizeof(uint64_t));
    std::memcpy(s.a_union.data(), in_au, nw * sizeof(uint64_t));
    if (d > 0) {
        std::memcpy(s.A_buf.data(), in_A, d * nw * sizeof(uint64_t));
        std::memcpy(s.ell_buf.data(), in_ell, d * sizeof(uint8_t));
        for (int r = 0; r < d; ++r) {
            std::memcpy(&s.Gamma_buf[r * s.cap], in_Gamma + r * d, d * sizeof(uint8_t));
        }
        std::memcpy(s.P.data(), in_P, d * sizeof(int32_t));
    }
    eng->is_active[b] = (d > 0 || words_any_nonzero(in_h, nw)) ? 1 : 0;
}

// 1A. Fused CZ/ZCZ fastpath conditional clifford injection
void engine_batch_inject_cz(
    void* ptr,
    int num_groups,
    const int64_t* group_q0,
    const int64_t* group_q1,
    const uint8_t* cond_mask,
    uint8_t* xs_base,
    const uint64_t* z2x_w,
    const uint64_t* z2z_w,
    const uint8_t* z_kappa,
    const uint64_t* x2z_w,
    uint8_t* out_evict_x,
    uint8_t* out_evict_z,
    int* any_evict_x,
    int* any_evict_z
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int nw = eng->n_words;
    *any_evict_x = 0;
    *any_evict_z = 0;

    for (int g = 0; g < num_groups; ++g) {
        const uint8_t* cm_row = cond_mask + g * B;
        bool any_failed = false;
        for (int b = 0; b < B; ++b) {
            if (!cm_row[b]) { any_failed = true; break; }
        }
        if (!any_failed) continue;

        int64_t q0 = group_q0[g];
        int64_t q1 = group_q1[g];
        const uint64_t* aw1 = z2x_w + q0 * nw;
        const uint64_t* bw1 = z2z_w + q0 * nw;
        int kap1_base = z_kappa[q0];
        const uint64_t* aw2 = z2x_w + q1 * nw;
        const uint64_t* bw2 = z2z_w + q1 * nw;
        int kap2_base = z_kappa[q1];
        const uint8_t* xs_q0 = xs_base + q0 * B;
        const uint8_t* xs_q1 = xs_base + q1 * B;

        for (int b = 0; b < B; ++b) {
            if (!cm_row[b]) {
                eng->is_active[b] = 1;
                int kap1 = (kap1_base + (int(xs_q0[b]) << 1)) & 3;
                int kap2 = (kap2_base + (int(xs_q1[b]) << 1)) & 3;
                eng->shots[b].p2_apply_controlled_pauli(aw1, bw1, kap1, aw2, bw2, kap2);
            }
        }
    }

    eng->evict_healed_shots(x2z_w, z2z_w, xs_base, nullptr, out_evict_x, out_evict_z, any_evict_x, any_evict_z);
}

// 1B. Fused general conditional Clifford cocycle injection (H, CX, S, etc.)
void engine_batch_inject_general_clifford(
    void* ptr,
    int num_groups,
    int arity,
    int n_factors,
    const int64_t* target_groups,     // [num_groups * arity]
    const uint8_t* cond_mask,         // [num_groups * B]
    uint8_t* xs_base,                 // [num_qubits * B]
    uint8_t* zs_base,                 // [num_qubits * B]
    const uint8_t* factor_kinds,      // [num_groups * n_factors]
    const uint8_t* factor_lx1,        // [num_groups * n_factors * arity]
    const uint8_t* factor_lz1,        // [num_groups * n_factors * arity]
    const uint64_t* factor_a1_w,      // [num_groups * n_factors * nw]
    const uint64_t* factor_b1_w,      // [num_groups * n_factors * nw]
    const uint8_t* factor_kap1,       // [num_groups * n_factors]
    const uint8_t* factor_lx2,        // [num_groups * n_factors * arity]
    const uint8_t* factor_lz2,        // [num_groups * n_factors * arity]
    const uint64_t* factor_a2_w,      // [num_groups * n_factors * nw]
    const uint64_t* factor_b2_w,      // [num_groups * n_factors * nw]
    const uint8_t* factor_kap2,       // [num_groups * n_factors]
    const uint64_t* x2z_w,
    const uint64_t* z2z_w,
    uint8_t* out_evict_x,
    uint8_t* out_evict_z,
    int* any_evict_x,
    int* any_evict_z
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int nw = eng->n_words;
    *any_evict_x = 0;
    *any_evict_z = 0;

    for (int g = 0; g < num_groups; ++g) {
        const uint8_t* cm_row = cond_mask + g * B;
        bool any_failed = false;
        for (int b = 0; b < B; ++b) {
            if (!cm_row[b]) { any_failed = true; break; }
        }
        if (!any_failed) continue;

        const int64_t* grp = target_groups + g * arity;
        for (int b = 0; b < B; ++b) {
            if (cm_row[b]) continue;
            eng->is_active[b] = 1;
            CppMRQFShot& mrqf = eng->shots[b];

            for (int f = 0; f < n_factors; ++f) {
                int gf = g * n_factors + f;
                uint8_t kind = factor_kinds[gf];
                const uint8_t* lx1 = factor_lx1 + gf * arity;
                const uint8_t* lz1 = factor_lz1 + gf * arity;
                int gamma1 = 0;
                for (int r = 0; r < arity; ++r) {
                    int64_t q_r = grp[r];
                    uint8_t fx = xs_base[q_r * B + b];
                    uint8_t fz = zs_base[q_r * B + b];
                    gamma1 ^= (lx1[r] & fz) ^ (lz1[r] & fx);
                }
                int kap1_shot = (int(factor_kap1[gf]) + ((gamma1 & 1) << 1)) & 3;
                const uint64_t* aw1 = factor_a1_w + gf * nw;
                const uint64_t* bw1 = factor_b1_w + gf * nw;

                if (kind == 0) {
                    mrqf.p1_apply_pauli(aw1, bw1, kap1_shot);
                } else if (kind == 1) {
                    mrqf.p2_apply_quarter_turn(aw1, bw1, kap1_shot);
                } else {
                    const uint8_t* lx2 = factor_lx2 + gf * arity;
                    const uint8_t* lz2 = factor_lz2 + gf * arity;
                    int gamma2 = 0;
                    for (int r = 0; r < arity; ++r) {
                        int64_t q_r = grp[r];
                        uint8_t fx = xs_base[q_r * B + b];
                        uint8_t fz = zs_base[q_r * B + b];
                        gamma2 ^= (lx2[r] & fz) ^ (lz2[r] & fx);
                    }
                    int kap2_shot = (int(factor_kap2[gf]) + ((gamma2 & 1) << 1)) & 3;
                    const uint64_t* aw2 = factor_a2_w + gf * nw;
                    const uint64_t* bw2 = factor_b2_w + gf * nw;
                    mrqf.p2_apply_controlled_pauli(aw1, bw1, kap1_shot, aw2, bw2, kap2_shot);
                }
            }
        }
    }

    eng->evict_healed_shots(x2z_w, z2z_w, xs_base, zs_base, out_evict_x, out_evict_z, any_evict_x, any_evict_z);
}

// 2A. Fused batch deterministic Z measurement / reset step
void engine_batch_det_meas_step(
    void* ptr,
    int num_sub,
    const int64_t* q_arr,
    const uint8_t* m_ref_arr,
    const uint64_t* b_sub_words,
    uint8_t* xs_base,
    uint8_t* m_mat,
    const uint64_t* x2z_w,
    const uint64_t* z2z_w,
    uint8_t* accum_x_mask,
    uint8_t* accum_z_mask,
    int* any_evict_x,
    int* any_evict_z,
    const uint8_t* ext_rand_bits,
    int* ext_rand_cursor
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int nw = eng->n_words;
    *any_evict_x = 0;
    *any_evict_z = 0;

    for (int k = 0; k < num_sub; ++k) {
        const uint8_t* xs_row = xs_base + q_arr[k] * B;
        uint8_t* m_row = m_mat + k * B;
        uint8_t mr = m_ref_arr[k];
        for (int b = 0; b < B; ++b) {
            m_row[b] = xs_row[b] ^ mr;
        }
    }

    for (int b = 0; b < B; ++b) {
        if (!eng->is_active[b]) continue;
        CppMRQFShot& mrqf = eng->shots[b];
        if (mrqf.d == 0 && !words_any_nonzero(mrqf.h_words.data(), nw)) continue;

        for (int k = 0; k < num_sub; ++k) {
            const uint64_t* b_k = b_sub_words + k * nw;
            if (mrqf.d == 0 || !words_any_overlap(b_k, mrqf.a_union.data(), nw)) {
                if (word_dot_parity(b_k, mrqf.h_words.data(), nw)) {
                    m_mat[k * B + b] ^= 1;
                }
            } else {
                int64_t q_k = q_arr[k];
                int sigma = (int(xs_base[q_k * B + b]) ^ int(m_ref_arr[k])) & 1;
                int req_bit = -1;
                if (ext_rand_bits && ext_rand_cursor) {
                    bool any_lam = false;
                    for (int r = 0; r < mrqf.d; ++r) {
                        if (word_dot_parity(&mrqf.A_buf[r * nw], b_k, nw)) { any_lam = true; break; }
                    }
                    if (any_lam) {
                        req_bit = ext_rand_bits[b * 1024 + (ext_rand_cursor[b]++)];
                    }
                }
                int m_val = mrqf.p3_measure_diagonal(b_k, sigma, req_bit, &eng->shot_rng_states[b]);
                m_mat[k * B + b] = uint8_t(m_val);
            }
        }
    }

    eng->evict_healed_shots(x2z_w, z2z_w, xs_base, nullptr, accum_x_mask, accum_z_mask, any_evict_x, any_evict_z);
}

// 2B. Fused mixed/random Z measurement step (handles Round 1 where is_random == True!)
void engine_batch_mixed_substep(
    void* ptr,
    int is_random,
    int64_t q,
    uint8_t m_ref,
    int64_t p,
    const uint64_t* beta_or_a_words,
    const uint64_t* b_rot_words,
    const uint64_t* e_p_words,
    int kappa_rot,
    int num_gx,
    const int64_t* gx_indices,
    int num_gz,
    const int64_t* gz_indices,
    uint8_t* xs_base,
    uint8_t* m_shots,
    uint8_t* accum_x_mask,
    uint8_t* accum_z_mask,
    int* any_x_accum,
    int* any_z_accum,
    const uint8_t* ext_rand_bits,
    int* ext_rand_cursor
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int nw = eng->n_words;
    const uint8_t* f_q_shots = xs_base + q * B;

    if (!is_random) {
        const uint64_t* b_words = beta_or_a_words;
        for (int b = 0; b < B; ++b) {
            uint8_t m_b = f_q_shots[b] ^ m_ref;
            if (eng->is_active[b] && eng->shots[b].d > 0) {
                CppMRQFShot& mrqf = eng->shots[b];
                if (!words_any_overlap(b_words, mrqf.a_union.data(), nw)) {
                    if (word_dot_parity(b_words, mrqf.h_words.data(), nw)) {
                        m_b ^= 1;
                    }
                } else {
                    int sigma = (int(f_q_shots[b]) ^ int(m_ref)) & 1;
                    int req_bit = -1;
                    if (ext_rand_bits && ext_rand_cursor) {
                        bool any_lam = false;
                        for (int r = 0; r < mrqf.d; ++r) {
                            if (word_dot_parity(&mrqf.A_buf[r * nw], b_words, nw)) { any_lam = true; break; }
                        }
                        if (any_lam) req_bit = ext_rand_bits[b * 1024 + (ext_rand_cursor[b]++)];
                    }
                    m_b = uint8_t(mrqf.p3_measure_diagonal(b_words, sigma, req_bit, &eng->shot_rng_states[b]));
                }
            } else if (eng->is_active[b] && eng->shots[b].d == 0) {
                if (word_dot_parity(b_words, eng->shots[b].h_words.data(), nw)) {
                    m_b ^= 1;
                }
            }
            m_shots[b] = m_b;
        }
        return;
    }

    // is_random == 1
    int p_word = int(p >> 6);
    int p_bit = int(p & 63);
    uint64_t p_mask = 1ULL << p_bit;
    uint8_t* delta_shots = eng->scratch_delta.data();
    bool any_delta = false;

    for (int b = 0; b < B; ++b) {
        uint8_t m_b = m_shots[b];
        uint8_t fq_b = f_q_shots[b];
        uint8_t d_b = m_b ^ fq_b ^ m_ref;

        if (eng->is_active[b]) {
            CppMRQFShot& mrqf = eng->shots[b];
            if ((mrqf.a_union[p_word] & p_mask) == 0ULL) {
                if ((mrqf.h_words[p_word] & p_mask) != 0ULL) {
                    d_b ^= 1;
                }
            } else {
                d_b = 0;
                mrqf.p2_apply_quarter_turn(beta_or_a_words, b_rot_words, kappa_rot);
                int req_bit = int(m_b ^ fq_b);
                int m_eff = mrqf.p3_measure_diagonal(e_p_words, 0, req_bit, &eng->shot_rng_states[b]);
                m_shots[b] = uint8_t(m_eff ^ fq_b);
            }
        }
        delta_shots[b] = d_b;
        if (d_b) any_delta = true;
    }

    if (any_delta) {
        if (num_gx > 0) {
            for (int i = 0; i < num_gx; ++i) {
                int64_t gq = gx_indices[i];
                uint8_t* ax_row = accum_x_mask + gq * B;
                uint8_t* xs_row = xs_base + gq * B;
                for (int b = 0; b < B; ++b) {
                    ax_row[b] ^= delta_shots[b];
                    xs_row[b] ^= delta_shots[b];
                }
            }
            *any_x_accum = 1;
        }
        if (num_gz > 0) {
            for (int i = 0; i < num_gz; ++i) {
                int64_t gq = gz_indices[i];
                uint8_t* az_row = accum_z_mask + gq * B;
                for (int b = 0; b < B; ++b) {
                    az_row[b] ^= delta_shots[b];
                }
            }
            *any_z_accum = 1;
        }
    }
}

void engine_batch_evict(
    void* ptr,
    const uint64_t* x2z_w,
    const uint64_t* z2z_w,
    uint8_t* xs_base,
    uint8_t* zs_base,
    uint8_t* accum_x_mask,
    uint8_t* accum_z_mask,
    int* any_evict_x,
    int* any_evict_z
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    eng->evict_healed_shots(x2z_w, z2z_w, xs_base, zs_base, accum_x_mask, accum_z_mask, any_evict_x, any_evict_z);
}

// 3. Fused batch unmatched noisy reset Z
void engine_batch_unmatched_noisy_reset_z(
    void* ptr,
    const uint8_t* reset_zero_mask,  // [num_qubits * B]
    const uint8_t* reset_one_mask,   // [num_qubits * B]
    uint8_t* xs_base,                // [num_qubits * B]
    const uint64_t* z2x_w,           // [num_qubits * nw]
    const uint64_t* z2z_w,           // [num_qubits * nw]
    const uint8_t* z_kappa,          // [num_qubits]
    const uint64_t* x2z_w,           // [num_qubits * nw]
    uint8_t* out_x_kick_mask,        // [num_qubits * B]
    uint8_t* out_evict_z_mask,       // [num_qubits * B]
    int* any_x_kick,
    int* any_evict_z
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int N = eng->num_qubits;
    int nw = eng->n_words;
    *any_x_kick = 0;
    *any_evict_z = 0;

    uint64_t* b_rot_words = eng->tmp_words_1.data();
    uint64_t* e_p_words = eng->tmp_words_2.data();

    for (int q = 0; q < N; ++q) {
        const uint8_t* r0_row = reset_zero_mask + q * B;
        const uint8_t* r1_row = reset_one_mask + q * B;
        bool any_q = false;
        for (int b = 0; b < B; ++b) {
            if (r0_row[b] | r1_row[b]) { any_q = true; break; }
        }
        if (!any_q) continue;

        const uint64_t* aw = z2x_w + q * nw;
        const uint64_t* bw = z2z_w + q * nw;
        int kap = int(z_kappa[q]);
        bool a_is_zero = !words_any_nonzero(aw, nw);
        int ab_pop = 0;
        for (int w = 0; w < nw; ++w) ab_pop += __builtin_popcountll(aw[w] & bw[w]);
        int sign_bool = (((kap - ab_pop) & 3) == 2) ? 1 : 0;
        int ref_exp = sign_bool ? -1 : 1;

        int p = -1;
        int kappa_rot = 0;
        if (!a_is_zero) {
            p = first_set_bit_in_words(aw, nw);
            kappa_rot = (kap + 1) & 3;
            for (int w = 0; w < nw; ++w) {
                b_rot_words[w] = bw[w];
                e_p_words[w] = 0ULL;
            }
            b_rot_words[p >> 6] ^= (1ULL << (p & 63));
            e_p_words[p >> 6] = (1ULL << (p & 63));
        }

        uint8_t* xs_q = xs_base + q * B;
        uint8_t* x_kick_q = out_x_kick_mask + q * B;

        for (int b = 0; b < B; ++b) {
            if (!(r0_row[b] | r1_row[b])) continue;
            int target_state = r1_row[b] ? 1 : 0;
            int f_q = int(xs_q[b]);

            int z_exp;
            if (eng->is_active[b] && (eng->shots[b].d > 0 || words_any_nonzero(eng->shots[b].h_words.data(), nw))) {
                int mu = eng->shots[b].q1_pauli_expectation(aw, bw, kap);
                z_exp = f_q ? -mu : mu;
            } else {
                z_exp = a_is_zero ? (f_q ? -ref_exp : ref_exp) : 0;
            }

            if (z_exp == 1) {
                if (target_state == 1) {
                    x_kick_q[b] ^= 1;
                    xs_q[b] ^= 1;
                    *any_x_kick = 1;
                }
            } else if (z_exp == -1) {
                if (target_state == 0) {
                    x_kick_q[b] ^= 1;
                    xs_q[b] ^= 1;
                    *any_x_kick = 1;
                }
            } else {
                int m = eng->draw_bit(b);
                int m_eff = (m ^ f_q) & 1;
                eng->is_active[b] = 1;
                CppMRQFShot& mrqf = eng->shots[b];
                if (a_is_zero) {
                    int sigma = (f_q ^ sign_bool) & 1;
                    mrqf.p3_measure_diagonal(bw, sigma, m, &eng->shot_rng_states[b]);
                } else {
                    mrqf.p2_apply_quarter_turn(aw, b_rot_words, kappa_rot);
                    mrqf.p3_measure_diagonal(e_p_words, 0, m_eff, &eng->shot_rng_states[b]);
                    mrqf.p2_apply_quarter_turn(aw, b_rot_words, (kappa_rot + 2) & 3);
                }
                if (m != target_state) {
                    x_kick_q[b] ^= 1;
                    xs_q[b] ^= 1;
                    *any_x_kick = 1;
                }
                if (mrqf.d == 0) {
                    eng->evict_single_shot(b, x2z_w, z2z_w, xs_base, nullptr, out_x_kick_mask, out_evict_z_mask, any_x_kick, any_evict_z);
                }
            }
        }
    }

    eng->evict_healed_shots(x2z_w, z2z_w, xs_base, nullptr, out_x_kick_mask, out_evict_z_mask, any_x_kick, any_evict_z);
}

// 4. Fused peek_pauli_batch in C++
void engine_peek_pauli_batch(
    void* ptr,
    int num_targets,
    const int64_t* targets,
    int pauli_kind, // 0=X, 1=Y, 2=Z
    const uint8_t* xs_base,
    const uint8_t* zs_base,
    const uint64_t* x2x_w,
    const uint64_t* x2z_w,
    const uint64_t* z2x_w,
    const uint64_t* z2z_w,
    const uint8_t* x_kappa,
    const uint8_t* z_kappa,
    int8_t* out_exps
) {
    auto* eng = static_cast<BatchedMRQFEngine*>(ptr);
    int B = eng->batch_size;
    int nw = eng->n_words;

    uint64_t* aw_buf = eng->tmp_words_1.data();
    uint64_t* bw_buf = eng->tmp_words_2.data();

    for (int idx = 0; idx < num_targets; ++idx) {
        int64_t q = targets[idx];
        const uint64_t* aw = nullptr;
        const uint64_t* bw = nullptr;
        int kap = 0;
        if (pauli_kind == 2) { // Z
            aw = z2x_w + q * nw;
            bw = z2z_w + q * nw;
            kap = int(z_kappa[q]);
        } else if (pauli_kind == 0) { // X
            aw = x2x_w + q * nw;
            bw = x2z_w + q * nw;
            kap = int(x_kappa[q]);
        } else { // Y
            const uint64_t* x_a = x2x_w + q * nw;
            const uint64_t* x_b = x2z_w + q * nw;
            const uint64_t* z_a = z2x_w + q * nw;
            const uint64_t* z_b = z2z_w + q * nw;
            int dp = word_dot_parity(x_b, z_a, nw);
            for (int w = 0; w < nw; ++w) {
                aw_buf[w] = x_a[w] ^ z_a[w];
                bw_buf[w] = x_b[w] ^ z_b[w];
            }
            aw = aw_buf;
            bw = bw_buf;
            kap = (1 + int(x_kappa[q]) + int(z_kappa[q]) + (dp << 1)) & 3;
        }

        bool a_is_zero = !words_any_nonzero(aw, nw);
        int ab_pop = 0;
        for (int w = 0; w < nw; ++w) ab_pop += __builtin_popcountll(aw[w] & bw[w]);
        int sign_bool = (((kap - ab_pop) & 3) == 2) ? 1 : 0;
        int8_t ref_exp = sign_bool ? -1 : 1;

        const uint8_t* xs_q = xs_base + q * B;
        const uint8_t* zs_q = zs_base + q * B;
        int8_t* out_row = out_exps + idx * B;

        for (int b = 0; b < B; ++b) {
            uint8_t f_comm = (pauli_kind == 2) ? xs_q[b] : ((pauli_kind == 0) ? zs_q[b] : (xs_q[b] ^ zs_q[b]));
            if (eng->is_active[b] && (eng->shots[b].d > 0 || words_any_nonzero(eng->shots[b].h_words.data(), nw))) {
                int mu = eng->shots[b].q1_pauli_expectation(aw, bw, kap);
                out_row[b] = int8_t(f_comm ? -mu : mu);
            } else {
                out_row[b] = a_is_zero ? (f_comm ? -ref_exp : ref_exp) : 0;
            }
        }
    }
}

} // extern "C"
