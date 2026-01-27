import dataclasses

import stim
import sinter

from stimside.sampler_tableau import TablesideSampler
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8

DECODER_NAME = "pymatching"
SAMPLER_NAME = "stimside:tableside:" + DECODER_NAME

@dataclasses.dataclass
class TaskMetadata:
    """All metadata parameters necessary to specify a simulation of a circuit."""

    sampler: str = SAMPLER_NAME
    decoder: str = DECODER_NAME

    circuit_name: str = "surface_code:rotated_memory_x"

    rounds: int = 5
    distance: int = 5
    p: float = 1e-3

    def json_metadata(self):
        return dataclasses.asdict(self)

    @classmethod
    def from_json_metadata(cls, json_metadata):
        field_names = [f.name for f in dataclasses.fields(cls)]
        kwargs = {k: v for k, v in json_metadata.items() if k in field_names}
        return cls(**kwargs)

    def make_task(self) -> sinter.Task:
    
        def _mod_circuit(circ: stim.Circuit) -> stim.Circuit:

            return_circuit = stim.Circuit()
            for op in circ:
                if isinstance(op, stim.CircuitInstruction):
                    if op.name == "CX":
                        return_circuit.append(stim.CircuitInstruction('H', op.targets_copy()[1::2]))

                        return_circuit.append(stim.CircuitInstruction(
                            'I_ERROR', 
                            op.targets_copy(),
                            tag = f"LEAKAGE_DEPOLARIZE_1"
                            ))
                        return_circuit.append(stim.CircuitInstruction(
                            'CZ', op.targets_copy(), tag="CONDITIONED_ON_PAIR: (U, U)"))
                        
                        return_circuit.append(stim.CircuitInstruction(
                            'II_ERROR', 
                            op.targets_copy(),
                            tag = f"LEAKAGE_TRANSITION_2: ({self.p/6}, U_U-->U_2) ({self.p/6}, U_U-->2_U) ({self.p/6}, U_U-->2_2) ({self.p/2}, 2_U-->2_2) ({self.p/2}, U_2-->2_2)"
                            ))

                        return_circuit.append(stim.CircuitInstruction('H', op.targets_copy()[1::2]))
                    elif stim.gate_data(op.name).produces_measurements:
                        if op.name == "MX":
                            return_circuit.append(stim.CircuitInstruction("H", op.targets_copy()))
                            return_circuit.append(stim.CircuitInstruction("I_ERROR", op.targets_copy(), tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)"))
                            return_circuit.append(stim.CircuitInstruction("M", op.targets_copy()))
                        elif op.name == "MR" or op.name == "M":
                            return_circuit.append(stim.CircuitInstruction("I_ERROR", op.targets_copy(), tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)"))
                            return_circuit.append(op)
                        else:
                            raise ValueError(f"Do not know what to do with measurement op {op.name}")
                        
                        # mpad_tag = "LEAKAGE_MEASUREMENT: (0.01, U) (0.99, 2):"
                        # for target in op.targets_copy():
                        #     mpad_tag += " " + str(target.value)
                        # return_circuit.append(stim.CircuitInstruction("MPAD", [0 for _ in range(len(op.targets_copy()))], tag=mpad_tag))

                    elif op.name == "DEPOLARIZE1":
                        return_circuit.append(stim.CircuitInstruction(op.name, op.targets_copy(), [op.gate_args_copy()[0]/3]))
                    elif op.name == "DEPOLARIZE2":
                        return_circuit.append(stim.CircuitInstruction(op.name, op.targets_copy(), [op.gate_args_copy()[0]/2]))
                    else:
                        return_circuit.append(op)
                elif isinstance(op, stim.CircuitRepeatBlock):
                    loop_body = op.body_copy()
                    for _ in range(op.repeat_count):
                        return_circuit.append(_mod_circuit(loop_body))
                else:
                    return_circuit.append(op)
                    print(op)

            return return_circuit

        raw_circuit = stim.Circuit.generated(
            code_task=self.circuit_name,
            distance=self.distance,
            rounds=self.rounds,
            after_clifford_depolarization=self.p,
            before_round_data_depolarization=self.p,
            before_measure_flip_probability=self.p,
            after_reset_flip_probability=self.p
            )
        circuit = _mod_circuit(raw_circuit)

        return sinter.Task(
            circuit=circuit,
            detector_error_model=circuit.detector_error_model(decompose_errors=True),
            json_metadata=self.json_metadata(),
        )
    
## Setting up sweeps
sweep_ds = [5]
sweep_ps = [1E-3]

## Generate all metadata combinations for the sweeps
metadata = [
    TaskMetadata(
        distance=d,
        rounds=d,
        p = p,
    ).make_task()
    for d in sweep_ds
    for p in sweep_ps
]

## Simulation limits
max_shots = 1_000_000
max_errors = 1_000_000

if __name__ == '__main__':
    op_handler = LeakageUint8(unconditional_condition_on_U=False)

    sinter.collect(
        num_workers=32, 
        tasks=metadata, 
        save_resume_filepath="./stats_time.csv", 
        print_progress=True, 
        max_shots=max_shots,
        max_errors=max_errors,
        decoders=[DECODER_NAME], 
        custom_decoders={SAMPLER_NAME: TablesideSampler(op_handler)}
        )
