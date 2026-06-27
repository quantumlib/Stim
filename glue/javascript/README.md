# Stim in Javascript

This directory contains glue code for exposing stim to javascript.

For build instructions, see [the "build javascript bindings" sections of the dev documentation](/dev/README.md#build-javascript-bindings).

## Exposed API

```
stim.Circuit
    static stim.Circuit.<constructor>
    stim.Circuit.append_operation
    stim.Circuit.append_from_stim_program_text
    stim.Circuit.copy
    stim.Circuit.isEqualTo
    stim.Circuit.repeated
    stim.Circuit.toString

stim.Tableau
    static stim.Tableau.<constructor>
    static stim.Tableau.random
    static stim.Tableau.from_named_gate
    static stim.Tableau.from_conjugated_generators_xs_zs
    stim.Tableau.x_output
    stim.Tableau.y_output
    stim.Tableau.z_output
    stim.Tableau.toString
    stim.Tableau.isEqualTo
    stim.Tableau.length

stim.TableauSimulator
    static stim.TableauSimulator.<constructor>
    stim.TableauSimulator.CNOT
    stim.TableauSimulator.CY
    stim.TableauSimulator.CZ
    stim.TableauSimulator.H
    stim.TableauSimulator.X
    stim.TableauSimulator.Y
    stim.TableauSimulator.Z
    stim.TableauSimulator.copy
    stim.TableauSimulator.current_inverse_tableau
    stim.TableauSimulator.do_circuit
    stim.TableauSimulator.do_pauli_string
    stim.TableauSimulator.do_tableau
    stim.TableauSimulator.measure
    stim.TableauSimulator.measure_kickback
    stim.TableauSimulator.set_inverse_tableau

stim.PauliString
    static stim.PauliString.<constructor>
    static stim.PauliString.random
    stim.PauliString.commutes
    stim.PauliString.isEqualTo
    stim.PauliString.length
    stim.PauliString.neg
    stim.PauliString.pauli
    stim.PauliString.sign
    stim.PauliString.times
    stim.PauliString.toString
```
