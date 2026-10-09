"""Moonwalking surface code of arXiv:2607.29443 with skip-gate leakage.

Samples the paper's moonwalking memory circuit (walking surface code with
``swap_time="early"``; see paper_circuits.py; even round counts only) with
erasure check schedules 4 and 8 and compares three decoders:
``BranchAndBoundDecoder`` (App. B4), ``MarginalDecoder`` (App. B3), and
``BaseDecoder``, which ignores the leakage flags. The settings are small so the
script runs in about a minute; raise ``sweep_ds``, ``max_shots`` and
``num_workers`` for real statistics.

Run from this folder (``paper_circuits`` is imported from it). B&B needs the
PyMatching version with per-shot ``edge_reweights``.
"""

import dataclasses

import sinter

from paper_circuits import paper_circuit
from stimside.dem_generators import BaseDecoder, BranchAndBoundDecoder, MarginalDecoder
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.sampler_tableau import TablesideSampler

CIRCUIT = "moonwalking"
DEM_DECODERS = [
    BranchAndBoundDecoder(),
    MarginalDecoder(decompose_errors=True, reweight_only=True),
    BaseDecoder(decompose_errors=True),
]


@dataclasses.dataclass
class TaskMetadata:
    """All metadata parameters necessary to specify a simulation of a circuit."""

    sampler: str
    circuit: str = CIRCUIT
    ec_sched: int = 8
    distance: int = 3
    rounds: int = 10
    p_leak: float = 1e-2
    p_pauli: float = 0.0  # the erasure bias is p_leak / p_pauli

    def json_metadata(self):
        return dataclasses.asdict(self)

    def make_task(self) -> sinter.Task:
        circuit = paper_circuit(
            self.circuit, self.distance, self.rounds, self.p_pauli, self.p_leak, self.ec_sched
        )
        return sinter.Task(circuit=circuit, decoder=self.sampler, json_metadata=self.json_metadata())


## Setting up sweeps (the paper uses 3 * distance + 1 rounds and erasure bias inf or 50;
## moonwalking needs an even number of rounds, which 3 * d + 1 is for odd d)
sweep_ds = [3]
sweep_p_leaks = [1e-2]
sweep_etas = [float("inf"), 50]
sweep_ec_scheds = [4, 8]

metadata = [
    TaskMetadata(
        sampler=dec.name,
        ec_sched=ec,
        distance=d,
        rounds=3 * d + 1,
        p_leak=p,
        p_pauli=p / eta,
    ).make_task()
    for d in sweep_ds
    for p in sweep_p_leaks
    for eta in sweep_etas
    for ec in sweep_ec_scheds
    for dec in DEM_DECODERS
    # BaseDecoder ignores leakage: with p_pauli = 0 its DEM has no errors to match.
    if not (isinstance(dec, BaseDecoder) and p / eta == 0)
]

## Simulation limits
num_workers = 2
max_shots = 1_000
max_errors = 100

if __name__ == "__main__":
    # unconditional_condition_on_U=True: leaked qubits skip their gates.
    op_handler = LeakageUint8(unconditional_condition_on_U=True)

    stats = sinter.collect(
        num_workers=num_workers,
        tasks=metadata,
        print_progress=True,
        max_shots=max_shots,
        max_errors=max_errors,
        custom_decoders={dec.name: TablesideSampler(op_handler, dec) for dec in DEM_DECODERS},
    )
    for s in sorted(stats, key=lambda s: (s.json_metadata["p_pauli"], s.json_metadata["ec_sched"], s.decoder)):
        m = s.json_metadata
        print(
            f"{m['circuit']} EC{m['ec_sched']} d={m['distance']} p_leak={m['p_leak']} "
            f"p_pauli={m['p_pauli']:.1e} {s.decoder:20s} {s.errors}/{s.shots} errors"
        )
