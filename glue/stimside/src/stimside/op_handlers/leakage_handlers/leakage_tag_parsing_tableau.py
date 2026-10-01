import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_parameters import LeakageParams
from stimside.op_handlers.leakage_handlers.tag_registry import parse_leakage_tag as _unified_parse_tag
from stimside.op_handlers.leakage_handlers.tag_registry import _parse_leakage_in_circuit_recurse as _unified_parse_circuit


def parse_leakage_tag(op: stim.CircuitInstruction) -> LeakageParams | None:
    """Parse a leakage tag for the tableau simulator."""
    return _unified_parse_tag(op, simulator="tableau")


def parse_leakage_in_circuit(
    circuit: stim.Circuit,
) -> dict[stim.CircuitInstruction, LeakageParams]:
    """Parse all present leakage tags in a circuit, including inside repeats."""
    return _unified_parse_circuit(circuit, simulator="tableau")
