# Stim ZX

Implements utilities for analyzing stabilizer ZX calculus graphs using Stim.

# Example

```python
import stimzx
import networkx


print("CNOT")
cnot_graph: networkx.MultiGraph = stimzx.text_diagram_to_zx_graph(r"""
    in---Z---out
         |
    in---X---out
""")
for e in stimzx.zx_graph_to_external_stabilizers(cnot_graph):
    print(e)
# prints:
# CNOT
# +X_ -> -XY
# +Z_ -> +Z_
# +_X -> +_Y
# +_Z -> +ZX


print("SQRT_X")
sqrt_x_graph: networkx.MultiGraph = stimzx.text_diagram_to_zx_graph(r"""
    in---X(pi/2)---out
""")
for e in stimzx.zx_graph_to_external_stabilizers(sqrt_x_graph):
    print(e)
# prints:
# SQRT_X
# +X -> +X
# +Z -> -Y


print("????")
graph: networkx.MultiGraph = stimzx.text_diagram_to_zx_graph(r"""
    in---X
         |
         H *---Z(-pi/2)---out
         |/
    in---X
""")
for e in stimzx.zx_graph_to_external_stabilizers(graph):
    print(e)
# prints:
# ????
# +ZX -> +_
# +XZ -> +Z
# +Z_ -> +Y
```
