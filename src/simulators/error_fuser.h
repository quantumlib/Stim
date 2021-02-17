#ifndef SPARSE_BITS_H
#define SPARSE_BITS_H

#include <map>
#include <memory>
#include <vector>

#include "../circuit/circuit.h"
#include "../simd/sparse_xor_vec.h"

struct HitQueue {
    size_t ticker = 0;
    std::vector<std::pair<size_t, uint32_t>> q;

    inline void push(uint32_t id, uint8_t lookback) {
        q.emplace_back(ticker + lookback, id);
    }

    template <typename BODY>
    void pop(BODY handler) {
        ticker++;
        for (size_t k = 0; k < q.size();) {
            if (q[k].first == ticker) {
                handler(q[k].second);
                q[k] = q.back();
                q.pop_back();
            } else {
                k++;
            }
        }
    }
};

struct ErrorFuser {
    std::vector<SparseXorVec<uint32_t>> xs;
    std::vector<SparseXorVec<uint32_t>> zs;
    std::vector<HitQueue> frame_queues;
    uint32_t next_detector_id;
    uint32_t num_kept_observables;
    std::map<SparseXorVec<uint32_t>, double> probs;

    static Circuit convert_circuit(const Circuit &circuit);
    static void convert_circuit_out(const Circuit &circuit, FILE *out, bool prepend_observables);

    ErrorFuser(size_t num_qubits, size_t num_detectors, size_t num_kept_observables);

    void independent_error(double probability, const SparseXorVec<uint32_t> &detector_set);

    void R(const OperationData &dat);
    void M(const OperationData &dat);
    void MR(const OperationData &dat);
    void H_XZ(const OperationData &dat);
    void H_XY(const OperationData &dat);
    void H_YZ(const OperationData &dat);
    void XCX(const OperationData &dat);
    void XCY(const OperationData &dat);
    void XCZ(const OperationData &dat);
    void YCX(const OperationData &dat);
    void YCY(const OperationData &dat);
    void YCZ(const OperationData &dat);
    void ZCX(const OperationData &dat);
    void ZCY(const OperationData &dat);
    void ZCZ(const OperationData &dat);
    void I(const OperationData &dat);

    void SWAP(const OperationData &dat);
    void DETECTOR(const OperationData &dat);
    void OBSERVABLE_INCLUDE(const OperationData &dat);
    void X_ERROR(const OperationData &dat);
    void Y_ERROR(const OperationData &dat);
    void Z_ERROR(const OperationData &dat);
    void CORRELATED_ERROR(const OperationData &dat);
    void DEPOLARIZE1(const OperationData &dat);
    void DEPOLARIZE2(const OperationData &dat);
    void ELSE_CORRELATED_ERROR(const OperationData &dat);
    void ISWAP(const OperationData &dat);
};

#endif
