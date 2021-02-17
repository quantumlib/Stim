#include "error_fuser.h"

#include <algorithm>
#include <queue>
#include <sstream>

#include "../circuit/circuit.h"

void ErrorFuser::R(const OperationData &dat) {
    for (auto q : dat.targets) {
        xs[q].vec.clear();
        zs[q].vec.clear();
    }
}

void ErrorFuser::M(const OperationData &dat) {
    for (auto q : dat.targets) {
        q &= TARGET_QUBIT_MASK;
        frame_queues[q].pop([&](uint32_t id) {
            zs[q] ^= id;
        });
    }
}

void ErrorFuser::MR(const OperationData &dat) {
    R(dat);
    M(dat);
}

void ErrorFuser::H_XZ(const OperationData &dat) {
    for (auto q : dat.targets) {
        std::swap(xs[q].vec, zs[q].vec);
    }
}

void ErrorFuser::H_XY(const OperationData &dat) {
    for (auto q : dat.targets) {
        zs[q] ^= xs[q];
    }
}

void ErrorFuser::H_YZ(const OperationData &dat) {
    for (auto q : dat.targets) {
        xs[q] ^= zs[q];
    }
}

void ErrorFuser::XCX(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        xs[q1] ^= zs[q2];
        xs[q2] ^= zs[q1];
    }
}

void ErrorFuser::XCY(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto tx = dat.targets[k];
        auto ty = dat.targets[k + 1];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorFuser::YCX(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto tx = dat.targets[k + 1];
        auto ty = dat.targets[k];
        xs[tx] ^= xs[ty];
        xs[tx] ^= zs[ty];
        xs[ty] ^= zs[tx];
        zs[ty] ^= zs[tx];
    }
}

void ErrorFuser::ZCY(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto tz = dat.targets[k];
        auto ty = dat.targets[k + 1];
        zs[tz] ^= xs[ty];
        zs[tz] ^= zs[ty];
        xs[ty] ^= xs[tz];
        zs[ty] ^= xs[tz];
    }
}

void ErrorFuser::YCZ(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto ty = dat.targets[k];
        auto tz = dat.targets[k + 1];
        zs[tz] ^= xs[ty];
        zs[tz] ^= zs[ty];
        xs[ty] ^= xs[tz];
        zs[ty] ^= xs[tz];
    }
}

void ErrorFuser::YCY(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[b];
        zs[a] ^= zs[b];
        xs[a] ^= xs[b];
        xs[a] ^= zs[b];

        zs[b] ^= xs[a];
        zs[b] ^= zs[a];
        xs[b] ^= xs[a];
        xs[b] ^= zs[a];
    }
}

void ErrorFuser::ZCX(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto c = dat.targets[k];
        auto t = dat.targets[k + 1];
        zs[c] ^= zs[t];
        xs[t] ^= xs[c];
    }
}

void ErrorFuser::XCZ(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto t = dat.targets[k];
        auto c = dat.targets[k + 1];
        zs[c] ^= zs[t];
        xs[t] ^= xs[c];
    }
}

void ErrorFuser::ZCZ(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto q1 = dat.targets[k];
        auto q2 = dat.targets[k + 1];
        zs[q1] ^= xs[q2];
        zs[q2] ^= xs[q1];
    }
}

void ErrorFuser::I(const OperationData &dat) {
}

void ErrorFuser::SWAP(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        std::swap(xs[a].vec, xs[b].vec);
        std::swap(zs[a].vec, zs[b].vec);
    }
}

void ErrorFuser::ISWAP(const OperationData &dat) {
    for (size_t k = 0; k < dat.targets.size(); k += 2) {
        auto a = dat.targets[k];
        auto b = dat.targets[k + 1];
        zs[a] ^= xs[a];
        zs[a] ^= xs[b];
        zs[b] ^= xs[a];
        zs[b] ^= xs[b];
        std::swap(xs[a].vec, xs[b].vec);
        std::swap(zs[a].vec, zs[b].vec);
    }
}

void ErrorFuser::DETECTOR(const OperationData &dat) {
    uint32_t id = --next_detector_id;
    for (auto t : dat.targets) {
        frame_queues[t & TARGET_QUBIT_MASK].push(id, t >> TARGET_RECORD_SHIFT);
    }
}

void ErrorFuser::OBSERVABLE_INCLUDE(const OperationData &dat) {
    uint32_t id = (int)dat.arg;
    if (id < num_kept_observables) {
        for (auto t : dat.targets) {
            frame_queues[t & TARGET_QUBIT_MASK].push(id, t >> TARGET_RECORD_SHIFT);
        }
    }
}

ErrorFuser::ErrorFuser(size_t num_qubits, size_t num_detectors, size_t num_kept_observables)
    : xs(num_qubits),
      zs(num_qubits),
      frame_queues(num_qubits),
      next_detector_id(num_kept_observables + num_detectors),
      num_kept_observables(num_kept_observables) {
}

void ErrorFuser::X_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error(dat.arg, zs[q]);
    }
}

void ErrorFuser::Y_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error(dat.arg, xs[q] ^ zs[q]);
    }
}

void ErrorFuser::Z_ERROR(const OperationData &dat) {
    for (auto q : dat.targets) {
        independent_error(dat.arg, xs[q]);
    }
}

void ErrorFuser::CORRELATED_ERROR(const OperationData &dat) {
    SparseXorVec<uint32_t> result;
    for (auto qp : dat.targets) {
        auto q = qp & TARGET_QUBIT_MASK;
        if (qp & TARGET_PAULI_Z_MASK) {
            result ^= xs[q];
        }
        if (qp & TARGET_PAULI_X_MASK) {
            result ^= zs[q];
        }
    }
    independent_error(dat.arg, result);
}

void ErrorFuser::DEPOLARIZE1(const OperationData &dat) {
    if (dat.arg >= 3.0 / 4.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 3/4 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * sqrt(1 - (4 * dat.arg) / 3);
    for (auto q : dat.targets) {
        independent_error(p, xs[q]);
        independent_error(p, zs[q]);
        independent_error(p, xs[q] ^ zs[q]);
    }
}

void ErrorFuser::DEPOLARIZE2(const OperationData &dat) {
    if (dat.arg >= 15.0 / 16.0) {
        throw std::out_of_range(
            "DEPOLARIZE1 must have probability less than 15/16 when converting to a detector hyper graph.");
    }
    double p = 0.5 - 0.5 * pow(1 - (16 * dat.arg) / 15, 0.125);
    for (size_t i = 0; i < dat.targets.size(); i += 2) {
        auto a = dat.targets[i];
        auto b = dat.targets[i + 1];

        auto &x1 = xs[a];
        auto &x2 = xs[b];
        auto &z1 = zs[a];
        auto &z2 = zs[b];
        auto y1 = x1 ^ z1;
        auto y2 = x2 ^ z2;

        // Isolated errors.
        independent_error(p, x1);
        independent_error(p, y1);
        independent_error(p, z1);
        independent_error(p, x2);
        independent_error(p, y2);
        independent_error(p, z2);

        // Paired errors.
        independent_error(p, x1 ^ x2);
        independent_error(p, y1 ^ x2);
        independent_error(p, z1 ^ x2);
        independent_error(p, x1 ^ y2);
        independent_error(p, y1 ^ y2);
        independent_error(p, z1 ^ y2);
        independent_error(p, x1 ^ z2);
        independent_error(p, y1 ^ z2);
        independent_error(p, z1 ^ z2);
    }
}

void ErrorFuser::ELSE_CORRELATED_ERROR(const OperationData &dat) {
    throw std::out_of_range(
        "ELSE_CORRELATED_ERROR operations not supported when converting to a detector hyper graph.");
}

Circuit ErrorFuser::convert_circuit(const Circuit &circuit) {
    auto dets_obs = circuit.list_detectors_and_observables();
    ErrorFuser sim(circuit.num_qubits, dets_obs.first.size(), dets_obs.second.size());
    for (size_t k = circuit.operations.size(); k-- > 0;) {
        const auto &op = circuit.operations[k];
        (sim.*op.gate->hit_simulator_function)(op.target_data);
    }

    Circuit result;
    for (const auto &kv : sim.probs) {
        result.append_op("CORRELATED_ERROR", kv.first.vec, kv.second);
    }
    for (auto &t : result.jagged_data) {
        t |= TARGET_PAULI_X_MASK;
    }
    return result;
}

void ErrorFuser::convert_circuit_out(const Circuit &circuit, FILE *out, bool prepend_observables) {
    auto dets_obs = circuit.list_detectors_and_observables();
    ErrorFuser sim(circuit.num_qubits, dets_obs.first.size(), prepend_observables ? dets_obs.second.size() : 0);
    for (size_t k = circuit.operations.size(); k-- > 0;) {
        const auto &op = circuit.operations[k];
        (sim.*op.gate->hit_simulator_function)(op.target_data);
    }

    for (const auto &kv : sim.probs) {
        fprintf(out, "E(%f)", kv.second);
        for (auto &e : kv.first) {
            fprintf(out, " X%lld", (long long)e);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "M");
    for (long long k = dets_obs.first.size() + dets_obs.second.size(); k-- > 0;) {
        fprintf(out, " %lld", k);
    }
    fprintf(out, "\n");
}

void ErrorFuser::independent_error(double probability, const SparseXorVec<uint32_t> &detector_set) {
    if (detector_set.size()) {
        auto &p = probs[detector_set];
        p = p * (1 - probability) + (1 - p) * probability;
    }
}
