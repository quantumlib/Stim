import numpy as np
import stim


def count_gates(circuit: stim.Circuit):
    n = 0
    for inst in circuit.flattened():
        if inst.name not in ['QUBIT_COORDS', 'DETECTOR', 'OBSERVABLE_INCLUDE']:
            n += len(inst.target_groups())
    return n


def count_file_instructions(circuit: stim.Circuit):
    n = len(circuit)
    for inst in circuit:
        if isinstance(inst, stim.CircuitRepeatBlock):
            n += count_file_instructions(inst.body_copy())
    return n


def benchmark_sample_detectors():
    c = stim.Circuit.generated("surface_code:rotated_memory_x", distance=7, rounds=7, after_clifford_depolarization=1e-3)
    sampler = c.compile_detector_sampler()

    def run():
        sampler.sample(shots=1024, bit_packed=True, separate_observables=True)

    return {
        'run': run,
        'goal': (79, 'us'),
        'rates': {
            "shots": 1024,
            "detectors": 1024*c.num_detectors,
            "ops": 1024 * count_gates(c),
        },
    }


def benchmark_sample_detectors_transposed_buffered():
    c = stim.Circuit.generated("surface_code:rotated_memory_x", distance=7, rounds=7, after_clifford_depolarization=1e-3)
    sampler = c.compile_detector_sampler()
    shots = 1024
    dets_out = np.zeros(dtype=np.uint8, shape=(c.num_detectors, shots // 8))
    obs_out = np.zeros(dtype=np.uint8, shape=(c.num_observables, shots // 8))

    def run():
        sampler.sample(shots=shots, bit_packed=True, transposed=True, separate_observables=True, dets_out=dets_out, obs_out=obs_out)

    return {
        'run': run,
        'goal': (59, 'us'),
        'rates': {
            "shots": shots,
            "detectors": shots*c.num_detectors,
            "gates": shots * count_gates(c),
        },
    }


def benchmark_circuit_append_int_in_large_chunks():
    c = stim.Circuit()
    targets = [0, 1] * 1000

    def run():
        c.clear()
        c.append("CX", targets)

    return {
        'run': run,
        'goal': (23, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_append_int_in_small_chunks():
    c = stim.Circuit()
    targets = [0, 1]

    def run():
        c.clear()
        for _ in range(1000):
            c.append("CX", targets)

    return {
        'run': run,
        'goal': (320, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_append_gate_targets_in_large_chunks():
    c = stim.Circuit()
    targets = [stim.GateTarget(0), stim.GateTarget(1)] * 1000

    def run():
        c.clear()
        c.append("CX", targets)

    return {
        'run': run,
        'goal': (25, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_append_pauli_strings_in_large_chunks():
    c = stim.Circuit()
    targets = [stim.PauliString("XX")] * 1000

    def run():
        c.clear()
        c.append("MPP", targets)

    return {
        'run': run,
        'goal': (46, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_append_text_in_large_chunks():
    c = stim.Circuit()
    content = "CX" + " 0 1" * 1000

    def run():
        c.clear()
        c.append_from_stim_program_text(content)

    return {
        'run': run,
        'goal': (15, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_append_text_in_small_chunks():
    c = stim.Circuit()
    content = "CX 0 1"

    def run():
        c.clear()
        for _ in range(1000):
            c.append_from_stim_program_text(content)

    return {
        'run': run,
        'goal': (190, 'us'),
        'rates': {
            "targets": 2000,
        },
    }


def benchmark_circuit_detector_error_model():
    c = stim.Circuit.generated("surface_code:rotated_memory_x",
                               distance=7,
                               rounds=7,
                               after_clifford_depolarization=1e-3,
                               before_measure_flip_probability=1e-3,
                               after_reset_flip_probability=1e-3,
                               before_round_data_depolarization=1e-3)
    def run():
        c.detector_error_model()

    return {
        'run': run,
        'goal': (2.5, 'ms'),
        'rates': {
            "instructions": count_file_instructions(c),
            "gates": count_gates(c),
        },
    }


def benchmark_circuit_generate():
    def run():
        return stim.Circuit.generated(
            "surface_code:rotated_memory_x",
            distance=7,
            rounds=7,
            after_clifford_depolarization=1e-3,
            before_measure_flip_probability=1e-3,
            after_reset_flip_probability=1e-3,
            before_round_data_depolarization=1e-3)
    c = run()

    return {
        'run': run,
        'goal': (57, 'us'),
        'rates': {
            "instructions": count_file_instructions(c),
            "gates": count_gates(c),
        },
    }
