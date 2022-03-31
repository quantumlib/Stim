# Stim ZX

Stim ZX is an example of using Stim as a library for implementing other tools.
Stim ZX implements utilities for analyzing stabilizer ZX calculus graphs.

# How to Install

stimzx is not a published pypi package.
You have to install it from source using `pip install -e`.
For example:

```
git clone git@github.com:quantumlib/stim.git
cd stim
pip install -e glue/zx
```

# How to Use

StimZX implements one key method for getting the stabilizers of a stabilizer ZX graph:

```
stimzx.zx_graph_to_external_stabilizers(
    graph: Union[nx.Graph, nx.MultiGraph]
) -> List[stimzx.ExternalStabilizer]
```

and one fun helper method for creating the graphs from text diagrams:

```
stimzx.text_diagram_to_zx_graph(
    text_diagram: str
) -> nx.MultiGraph
```

For example:

```python
import stimzx


print("CNOT graph")
cnot_graph = stimzx.text_diagram_to_zx_graph(r"""
    in---Z---out
         |
    in---X---out
""")
for e in stimzx.zx_graph_to_external_stabilizers(cnot_graph):
    print(e)
# prints:
# CNOT graph
# +X_ -> +XX
# +Z_ -> +Z_
# +_X -> +_X
# +_Z -> +ZZ

print("SQRT_X graph")
sqrt_x_graph = stimzx.text_diagram_to_zx_graph(r"""
    in---X(pi/2)---out
""")
for e in stimzx.zx_graph_to_external_stabilizers(sqrt_x_graph):
    print(e)
# prints:
# SQRT_X graph
# +X -> +X
# +Z -> +Y

print("mystery graph")
graph = stimzx.text_diagram_to_zx_graph(r"""
    in---X
         |
         H *---Z(-pi/2)---out
         |/
    in---X
""")
for e in stimzx.zx_graph_to_external_stabilizers(graph):
    print(e)
# prints:
# mystery graph
# +ZX -> +_
# +XZ -> +Z
# +Z_ -> +Y


print("S distillation graph")
s_distill_graph = stimzx.text_diagram_to_zx_graph(r"""
                *                  *---------------Z--------------------Z-------Z(pi/2)
               / \                 |               |                    |
        *-----*   *------------Z---+---------------+---Z----------------+-------Z(pi/2)
        |                      |   |               |   |                |
        X---X---Z(pi/2)        X---X---Z(pi/2)     X---X---Z(pi/2)      X---X---Z(pi/2)
        |   |                  |                   |                    |   |
        *---+------------------Z-------------------+--------------------+---Z---Z(pi/2)
            |                                      |                    |
   in-------Z--------------------------------------Z-------------------Z(pi)--------out
""")
for e in stimzx.zx_graph_to_external_stabilizers(s_distill_graph):
    print(e)
# prints:
# S distillation graph
# +X -> +Y
# +Z -> +Z
```
