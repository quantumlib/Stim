from dataclasses import dataclass, field
from typing import Callable, Literal, Any, Sequence
import stim  # type: ignore[import-untyped]
import stimside.op_handlers.leakage_handlers.leakage_tag_parsing_unified as ltp

from stimside.op_handlers.leakage_handlers.common_parsing import (
    LEAKAGE_TAG_MATCH,
    CONDITION_TAG_MATCH,
    MEASURE_TAG_MATCH,
)
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageParams,
)

# --- CORE ARCHITECTURE ---

@dataclass(frozen=True)
class TagDef:
    """Definition and validation rules for a leakage tag."""
    name: str
    parser_func: Callable[[stim.CircuitInstruction, Any, str], LeakageParams]
    allowed_arity: Literal[1, 2] | None = None
    allowed_gates: Sequence[str] | None = None
    supported_simulators: Sequence[Literal["flip", "tableau"]] = field(
        default_factory=lambda: ("flip", "tableau")
    )

    def validate(self, op: stim.CircuitInstruction, simulator: Literal["flip", "tableau"]):
        """Validate that the tag is used correctly on the given operation."""
        if simulator not in self.supported_simulators:
            raise ValueError(
                f"Tag '{self.name}' is not supported in the {simulator} simulator."
            )

        gd = stim.gate_data(op.name)
        
        if self.allowed_arity == 1 and not gd.is_single_qubit_gate:
            raise ValueError(f"1Q leakage tag '{op.tag}' attached to not-1Q stim gate '{op.name}'.")
        if self.allowed_arity == 2 and not gd.is_two_qubit_gate:
            raise ValueError(f"2Q leakage tag '{op.tag}' attached to not-2Q stim gate '{op.name}'.")

        if self.allowed_gates is not None:
            if op.name not in self.allowed_gates:
                raise ValueError(
                    f"Leakage tag '{self.name}' must be attached to one of {self.allowed_gates}, "
                    f"but was attached to '{op.name}'."
                )
        elif op.name not in ["I", "I_ERROR", "II", "II_ERROR"]:
             raise ValueError(
                f"Leakage tag '{self.name}' must be attached to a trivially acting gate "
                f"(I, I_ERROR, II, II_ERROR), but was attached to '{op.name}'."
            )


# --- THE REGISTRY ---

TAG_REGISTRY: dict[str, TagDef] = {
    "LEAKAGE_CONTROLLED_ERROR": TagDef(
        name="LEAKAGE_CONTROLLED_ERROR",
        allowed_arity=2,
        supported_simulators=("flip",),
        parser_func=ltp._parse_controlled_error
    ),
    "LEAKAGE_TRANSITION_1": TagDef(
        name="LEAKAGE_TRANSITION_1",
        allowed_arity=1,
        parser_func=ltp._parse_transition_1
    ),
    "LEAKAGE_TRANSITION_Z": TagDef(
        name="LEAKAGE_TRANSITION_Z",
        allowed_arity=1,
        allowed_gates=("I", "I_ERROR", "R"),
        supported_simulators=("flip",),
        parser_func=ltp._parse_transition_z
    ),
    "LEAKAGE_TRANSITION_2": TagDef(
        name="LEAKAGE_TRANSITION_2",
        allowed_arity=2,
        parser_func=ltp._parse_transition_2
    ),
    "LEAKAGE_PROJECTION_Z": TagDef(
        name="LEAKAGE_PROJECTION_Z",
        allowed_arity=1,
        allowed_gates=("M",),
        parser_func=ltp._parse_projection_z
    ),
    "LEAKAGE_MEASUREMENT": TagDef(
        name="LEAKAGE_MEASUREMENT",
        allowed_gates=("MPAD",),
        parser_func=ltp._parse_measurement
    ),
    "CONDITIONED_ON_SELF": TagDef(
        name="CONDITIONED_ON_SELF",
        allowed_arity=1,
        supported_simulators=("tableau",),
        parser_func=ltp._parse_conditioned_self
    ),
    "CONDITIONED_ON_OTHER": TagDef(
        name="CONDITIONED_ON_OTHER",
        allowed_arity=1,
        supported_simulators=("tableau",),
        parser_func=ltp._parse_conditioned_other
    ),
    "CONDITIONED_ON_PAIR": TagDef(
        name="CONDITIONED_ON_PAIR",
        allowed_arity=2,
        supported_simulators=("tableau",),
        parser_func=ltp._parse_conditioned_pair
    ),
}

# --- UNIFIED ENTRY POINT ---

def parse_leakage_tag(op: stim.CircuitInstruction, simulator: Literal["flip", "tableau"]) -> LeakageParams | None:
    """Parse a leakage tag using the unified registry."""
    tag = op.tag
    if not (tag.startswith("LEAKAGE") or tag.startswith("CONDITIONED_ON")):
        return None

    # 1. Match regex to identify name
    if tag.startswith("CONDITIONED_ON"):
        match = CONDITION_TAG_MATCH.fullmatch(tag)
    elif tag.startswith("LEAKAGE_MEASUREMENT"):
        match = MEASURE_TAG_MATCH.fullmatch(tag)
    else:
        match = LEAKAGE_TAG_MATCH.fullmatch(tag)
        
    if not match:
        raise ValueError(f"Malformed leakage tag structure: {tag}")
        
    # 2. Lookup definition
    name = match.group("name")
    tag_def = TAG_REGISTRY.get(name)
    if not tag_def:
        raise ValueError(f"Unrecognised leakage tag name: {name}")

    # 3. Validate & Parse
    tag_def.validate(op, simulator)
    return tag_def.parser_func(op, match, simulator)
