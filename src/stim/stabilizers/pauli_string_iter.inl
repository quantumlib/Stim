// Copyright 2021 Google LLC
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

#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/pauli_string_iter.h"

using namespace stim;

template <size_t W>
PauliStringIterator<W>::PauliStringIterator(size_t num_qubits, size_t min_weight, size_t max_weight)
    : result(num_qubits),
      min_weight(min_weight),
      max_weight(max_weight),
      cur_k(0),
      cur_w(min_weight),
      cur_p(0),
      cur_perm(num_qubits),
      terminal(num_qubits) {
    uint64_t val = cur_w ? (uint64_t{1} << (cur_w % 64)) - 1 : uint64_t{0};
    ones_mask_with_val(cur_perm, val, cur_w / 64);
}

template <size_t W>
PauliStringIterator<W>::PauliStringIterator(const PauliStringIterator &other) : result(0), cur_perm(0), terminal(0) {
    *this = other;
}

template <size_t W>
PauliStringIterator<W> &PauliStringIterator<W>::operator=(const PauliStringIterator &other) {
    min_weight = other.min_weight;
    max_weight = other.max_weight;
    cur_k = other.cur_k;
    cur_p = other.cur_p;
    cur_w = other.cur_w;
    cur_perm = other.cur_perm;
    terminal = other.terminal;
    result = other.result;
    return *this;
}

template <size_t W>
void PauliStringIterator<W>::find_set_bits(simd_bits<W> &cur_perm, std::vector<int> &bit_locs) {
    // Determine location of set bits in cur_perm.
    size_t count = 0;
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        for (int bit = 0; bit < 64; bit++) {
            if (cur_perm.u64[w] & (1ULL << bit)) {
                bit_locs[count] = bit + w * 64;
                count++;
            }
        }
    }
}

template <size_t W>
size_t PauliStringIterator<W>::count_trailing_zeros(simd_bits<W> &cur_perm) {
    // Linear time algorithm from https://graphics.stanford.edu/~seander/bithacks.html
    // TODO: Replace with compiler intrinsic or a better algorithm.
    size_t counter = 0;
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        uint64_t v = cur_perm.u64[w];
        if (v) {
            v = (v ^ (v - 1)) >> 1;
            while (v) {
                v = v >> 1;
                counter++;
            }
            break;
        } else {
            counter += 8 * sizeof(v);
        }
    }
    return counter;
}

template <size_t W>
void PauliStringIterator<W>::ones_mask_with_val(simd_bits<W> &mask, uint64_t val, size_t loc) {
    uint64_t ones_mask = ~0ULL;
    for (size_t w = 0; w < loc; w++) {
        mask.u64[w] = ones_mask;
    }
    mask.u64[loc] = val;
}

template <size_t W>
void PauliStringIterator<W>::next_qubit_permutation(simd_bits<W> &cur_perm) {
    // The next lexicographically ordered bitstring is given by the algorithm (for a single word):
    // 1. c1 = cur_perm | (cur_perm - 1) // set least significant zero bits to 1.
    // 2. c2 = (c1 + 1)
    // 3. c3 = (~c1 & -~c1) - 1
    // 4. c4 = c3 >> count_trailing_zeros(cur_perm) + 1
    // 5. next_perm = c2 | c4;
    // see https://graphics.stanford.edu/~seander/bithacks.html#NextBitPermutation
    // These steps need to be updated for a list of integers given by cur_perm.
    // An equivalent algorithm is
    // t1 = cur | ((1 << num_zeros) - 1)
    // t2 = (((1 << (next_zero + 1)) - 1))
    // t3 = ((1 << (next_zero - num_zeros - 1)) - 1)
    // next_perm = (t1 ^ t2) | t3
    // where num_zeros = number of trailing zeros in cur_perm
    // and next_zero = the next zero after num_zeros
    // The above operations are simpler to implement for simd_bits as they mostly
    // involve operations with "ones"-masks and already implemented operations like | and ^.
    simd_bits<W> t1(cur_perm.num_bits_padded()), t2(cur_perm.num_bits_padded());
    simd_bits<W> t3(cur_perm.num_bits_padded()), t1_inv(cur_perm.num_bits_padded());
    uint64_t ones_bit = ~0ULL;
    t1 = cur_perm;
    size_t num_zeros = count_trailing_zeros(cur_perm);
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        // Either we have an all all ones or an all zeros integer in which case we
        // skip, otherwise this word must contain the least significant bit
        if (cur_perm.u64[w] != ones_bit && cur_perm.u64[w]) {
            // Use the single word algorithm from above
            t1.u64[w] = cur_perm.u64[w] | ((1ULL << num_zeros) - 1);
            // Set all previous words to ones
            for (size_t old_w = 0; old_w < w; old_w++) {
                t1.u64[old_w] = ones_bit;
            }
            break;
        }
    }
    // invert the bits
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        if (t1.u64[w]) {
            t1_inv.u64[w] = ~t1.u64[w];
        } else {
            // all zeros so set to all ones
            t1_inv.u64[w] = ones_bit;
        }
    }
    size_t next_zero = count_trailing_zeros(t1_inv);
    for (size_t w = 0; w < cur_perm.num_u64_padded(); w++) {
        if (next_zero / 64 == w) {
            if ((next_zero + 1) % 64 == 0) {
                t2.u64[w] = ones_bit;
            } else {
                t2.u64[w] = ((1ULL << (next_zero + 1 - 64 * w)) - 1);
            }
            break;
        } else {
            t2.u64[w] = ones_bit;
        }
    }
    size_t word_loc = (next_zero - num_zeros - 1) / 64;
    for (size_t w = 0; w < word_loc; w++) {
        t3.u64[w] = ones_bit;
    }
    // next_zero - num_zeros - 1 - words * 64 is always < 64, so don't need to worry about overflow like above.
    t3.u64[word_loc] = (1ULL << (next_zero - num_zeros - 1 - word_loc * 64)) - 1;
    cur_perm = t1;
    cur_perm ^= t2;
    cur_perm |= t3;
}

template <size_t W>
bool PauliStringIterator<W>::iter_all_cur_perm(std::vector<int> &set_bits) {
    size_t num_terms = static_cast<size_t>(pow(3, cur_w));
    result.xs.clear();
    result.zs.clear();
    // Set to all Xs initially
    for (size_t i = 0; i < cur_w; i++) {
        result.xs.u64[set_bits[i] / 64] ^= uint64_t{1} << (set_bits[i] & 63);
    }
    // TODO: in principle we could store the 3^w patterns for reasonable values
    // of w and avoid repeating this loop for each new qubit permutation. We
    // would save a mod and division so probably not worth it.
    while (cur_k < num_terms) {
        int n = cur_k;
        int indx = 0;
        while (n > 0) {
            // Use ternary representation of cur_k to provide XZY (0, 1, 2) Pauli pattern
            // Recall the ternary ordering for cur_w = 2, num_qubits = 2 would yield
            // 00 = XX
            // 01 = XZ
            // 02 = XY
            // 10 = YX
            // 11 = YZ
            // 12 = YY  ...
            // This inner while loop will yield r and indx, where r is the
            // trit (0, 1, 2) and indx is the corresponding qubit index.
            int r = n % 3;
            n /= 3;
            if (r == 2) {
                // Already have an X set so just set the Z.
                result.zs.u64[set_bits[indx] / 64] ^= uint64_t{1} << (set_bits[indx] & 63);
            } else if (r == 1) {
                // Already have an X set so flip it and set Z.
                result.xs.u64[set_bits[indx] / 64] ^= uint64_t{1} << (set_bits[indx] & 63);
                result.zs.u64[set_bits[indx] / 64] ^= uint64_t{1} << (set_bits[indx] & 63);
            } else {
            }
            indx++;
        }
        return true;
    }
    return false;
}

template <size_t W>
bool PauliStringIterator<W>::iter_next_weight() {
    // Exit while loop if cur_p breaches SIZE_MAX (might happen for very large
    // num_qubits) or we've exhausted the possible permutations for this value
    // of the weight (cur_w).
    // Just need to check that cur_perm != ((1 << cur_w) - 1) << num_qubits
    // which is the last possible permutation (combination)
    if (!terminal.not_zero()) {
        simd_bits<W> mask(cur_perm.num_bits_padded());
        size_t max_word = (result.num_qubits + 64 - 1) / 64 - 1;
        // If num_qubits is a multiple of 64 we can't left shift by 64 without overflowing, hence the need to specify
        // all set bits manually.
        uint64_t val = result.num_qubits % 64 == 0 ? ~0ULL : (1ULL << result.num_qubits - max_word * 64) - 1;
        ones_mask_with_val(terminal, val, max_word);
        // if cur_w == num_qubits then terminal already will hold the correct mask value
        if (result.num_qubits != cur_w) {
            size_t min_word = (result.num_qubits - cur_w + 64 - 1) / 64 - 1;
            val = (result.num_qubits - cur_w) % 64 == 0 ? ~0ULL
                                                        : (1ULL << ((result.num_qubits - cur_w) - min_word * 64)) - 1;
            ones_mask_with_val(mask, val, min_word);
            terminal ^= mask;
        }
        set_bits.resize(cur_w);
    }
    while (cur_p < SIZE_MAX) {
        // Find which bits are set in our current permutation.
        find_set_bits(cur_perm, set_bits);
        if (!iter_all_cur_perm(set_bits)) {
            // Find the next permutation of cur_w qubits among num_qubit possible
            // locations.  The qubit patterns are represented as bits with the
            // location of the set bits signifying where the Paulis should be
            // populated.  E.g. 0011 means qubits 0 and 1 should get paulis, 0101
            // means sites 0 and 2, etc.
            if (cur_perm == terminal) {
                terminal.clear();
                return false;
            }
            next_qubit_permutation(cur_perm);
            cur_k = 0;
            cur_p++;
        } else {
            // Still yielding PauliString product from cur_perm.
            cur_k++;
            return true;
        }
    }
    return false;
}

template <size_t W>
bool PauliStringIterator<W>::iter_next() {
    if (cur_w > max_weight) {
        return false;
    }
    if (cur_w == 0) {
        cur_w++;
        cur_perm.u64[0] = (uint64_t{1} << cur_w) - 1;
        return true;
    }
    // First iterate over all possible permutations of weight w bit strings.
    // Then for each weight w bitstring we again iterate over all combinations
    // of X,Y,Z allowed for a given qubit occupation.
    // E.g. Consider the starting point for w = 2, cur_perm = 0011
    // This means we want to find all possible ways to occupy qubit 0 and
    // qubit 1 with X, Y and Z, i.e. XX XY XZ, YX YY YZ, ZX, ZY, ZZ and there
    // are 3^w = 9 ways as expected. For an occupation pattern given by cur_perm
    // we only need to work out the combinations once. For the other further
    // weight w strings we just need to fill in the appropriate qubits.
    // TODO: use this optimization since we expecte w to be small?
    while (cur_w <= max_weight) {
        if (!iter_next_weight()) {
            cur_w++;
            cur_p = 0;
            cur_k = 0;
            // Increment the weight of the permutation bit string.
            // Assuming we won't have weight 64 paulis
            cur_perm.clear();
            // cur_perm.u64[0] = (uint64_t{1} << cur_w) - 1;
            uint64_t val = (uint64_t{1} << (cur_w % 64)) - 1;
            ones_mask_with_val(cur_perm, val, cur_w / 64);
        } else {
            // Still yielding from current weight (cur_w)
            return true;
        }
    }
    return false;
}

template <size_t W>
void PauliStringIterator<W>::restart() {
    cur_p = 0;
    cur_k = 0;
    cur_w = min_weight;
    cur_perm.clear();
    set_bits.resize(0);
    uint64_t val = cur_w ? (uint64_t{1} << (cur_w % 64)) - 1 : uint64_t{0};
    ones_mask_with_val(cur_perm, val, cur_w / 64);
    terminal.clear();
    result.xs.clear();
    result.zs.clear();
}