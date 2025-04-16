from typing import Sequence
import stim
import stimzx


def verify_stabilizers(
    specified_paulistrings: Sequence[str],
    result_networkx,
    print_stabilizers: bool = False,
) -> bool:
    result_stabilizers = [
        stab.output for stab in stimzx.zx_graph_to_external_stabilizers(result_networkx)
    ]
    specified_stabilizers = [
        stim.PauliString(paulistring) for paulistring in specified_paulistrings
    ]
    if print_stabilizers:
        print("specified:")
        for s in specified_stabilizers:
            print(s)
        print("==============================================================")
        print("resulting:")
        for s in result_stabilizers:
            print(s)
        print("==============================================================")

    for s in result_stabilizers:
        for ss in specified_stabilizers:
            if not ss.commutes(s):
                print(
                    f"result stabilizer {s} not commuting with "
                    f"specified stabilizer {ss}"
                )
                if print_stabilizers:
                    print("specified and resulting stabilizers not equivalent")
                return False
    if print_stabilizers:
        print("specified and resulting stabilizers are equivalent.")
    return True
