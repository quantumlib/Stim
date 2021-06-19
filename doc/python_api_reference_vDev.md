# Stim (Development Version) API Reference

## Index
- [`stim.Circuit`](#stim.Circuit)
    - [`stim.Circuit.__add__`](#stim.Circuit.__add__)
    - [`stim.Circuit.__eq__`](#stim.Circuit.__eq__)
    - [`stim.Circuit.__getitem__`](#stim.Circuit.__getitem__)
    - [`stim.Circuit.__iadd__`](#stim.Circuit.__iadd__)
    - [`stim.Circuit.__imul__`](#stim.Circuit.__imul__)
    - [`stim.Circuit.__init__`](#stim.Circuit.__init__)
    - [`stim.Circuit.__len__`](#stim.Circuit.__len__)
    - [`stim.Circuit.__mul__`](#stim.Circuit.__mul__)
    - [`stim.Circuit.__ne__`](#stim.Circuit.__ne__)
    - [`stim.Circuit.__repr__`](#stim.Circuit.__repr__)
    - [`stim.Circuit.__rmul__`](#stim.Circuit.__rmul__)
    - [`stim.Circuit.__str__`](#stim.Circuit.__str__)
    - [`stim.Circuit.append_from_stim_program_text`](#stim.Circuit.append_from_stim_program_text)
    - [`stim.Circuit.append_operation`](#stim.Circuit.append_operation)
    - [`stim.Circuit.clear`](#stim.Circuit.clear)
    - [`stim.Circuit.compile_detector_sampler`](#stim.Circuit.compile_detector_sampler)
    - [`stim.Circuit.compile_sampler`](#stim.Circuit.compile_sampler)
    - [`stim.Circuit.copy`](#stim.Circuit.copy)
    - [`stim.Circuit.detector_error_model`](#stim.Circuit.detector_error_model)
    - [`stim.Circuit.flattened_operations`](#stim.Circuit.flattened_operations)
    - [`stim.Circuit.generated`](#stim.Circuit.generated)
    - [`stim.Circuit.num_detectors`](#stim.Circuit.num_detectors)
    - [`stim.Circuit.num_measurements`](#stim.Circuit.num_measurements)
    - [`stim.Circuit.num_observables`](#stim.Circuit.num_observables)
    - [`stim.Circuit.num_qubits`](#stim.Circuit.num_qubits)
- [`stim.CircuitInstruction`](#stim.CircuitInstruction)
    - [`stim.CircuitInstruction.__eq__`](#stim.CircuitInstruction.__eq__)
    - [`stim.CircuitInstruction.__init__`](#stim.CircuitInstruction.__init__)
    - [`stim.CircuitInstruction.__ne__`](#stim.CircuitInstruction.__ne__)
    - [`stim.CircuitInstruction.__repr__`](#stim.CircuitInstruction.__repr__)
    - [`stim.CircuitInstruction.gate_args_copy`](#stim.CircuitInstruction.gate_args_copy)
    - [`stim.CircuitInstruction.name`](#stim.CircuitInstruction.name)
    - [`stim.CircuitInstruction.targets_copy`](#stim.CircuitInstruction.targets_copy)
- [`stim.CircuitRepeatBlock`](#stim.CircuitRepeatBlock)
    - [`stim.CircuitRepeatBlock.__eq__`](#stim.CircuitRepeatBlock.__eq__)
    - [`stim.CircuitRepeatBlock.__init__`](#stim.CircuitRepeatBlock.__init__)
    - [`stim.CircuitRepeatBlock.__ne__`](#stim.CircuitRepeatBlock.__ne__)
    - [`stim.CircuitRepeatBlock.__repr__`](#stim.CircuitRepeatBlock.__repr__)
    - [`stim.CircuitRepeatBlock.body_copy`](#stim.CircuitRepeatBlock.body_copy)
    - [`stim.CircuitRepeatBlock.repeat_count`](#stim.CircuitRepeatBlock.repeat_count)
- [`stim.CompiledDetectorSampler`](#stim.CompiledDetectorSampler)
    - [`stim.CompiledDetectorSampler.__repr__`](#stim.CompiledDetectorSampler.__repr__)
    - [`stim.CompiledDetectorSampler.sample`](#stim.CompiledDetectorSampler.sample)
    - [`stim.CompiledDetectorSampler.sample_bit_packed`](#stim.CompiledDetectorSampler.sample_bit_packed)
- [`stim.CompiledMeasurementSampler`](#stim.CompiledMeasurementSampler)
    - [`stim.CompiledMeasurementSampler.__repr__`](#stim.CompiledMeasurementSampler.__repr__)
    - [`stim.CompiledMeasurementSampler.sample`](#stim.CompiledMeasurementSampler.sample)
    - [`stim.CompiledMeasurementSampler.sample_bit_packed`](#stim.CompiledMeasurementSampler.sample_bit_packed)
- [`stim.DemInstruction`](#stim.DemInstruction)
    - [`stim.DemInstruction.__eq__`](#stim.DemInstruction.__eq__)
    - [`stim.DemInstruction.__init__`](#stim.DemInstruction.__init__)
    - [`stim.DemInstruction.__ne__`](#stim.DemInstruction.__ne__)
    - [`stim.DemInstruction.__repr__`](#stim.DemInstruction.__repr__)
    - [`stim.DemInstruction.__str__`](#stim.DemInstruction.__str__)
    - [`stim.DemInstruction.args_copy`](#stim.DemInstruction.args_copy)
    - [`stim.DemInstruction.targets_copy`](#stim.DemInstruction.targets_copy)
    - [`stim.DemInstruction.type`](#stim.DemInstruction.type)
- [`stim.DemRepeatBlock`](#stim.DemRepeatBlock)
    - [`stim.DemRepeatBlock.__eq__`](#stim.DemRepeatBlock.__eq__)
    - [`stim.DemRepeatBlock.__init__`](#stim.DemRepeatBlock.__init__)
    - [`stim.DemRepeatBlock.__ne__`](#stim.DemRepeatBlock.__ne__)
    - [`stim.DemRepeatBlock.__repr__`](#stim.DemRepeatBlock.__repr__)
    - [`stim.DemRepeatBlock.body_copy`](#stim.DemRepeatBlock.body_copy)
    - [`stim.DemRepeatBlock.repeat_count`](#stim.DemRepeatBlock.repeat_count)
- [`stim.DemTarget`](#stim.DemTarget)
    - [`stim.DemTarget.__eq__`](#stim.DemTarget.__eq__)
    - [`stim.DemTarget.__ne__`](#stim.DemTarget.__ne__)
    - [`stim.DemTarget.__repr__`](#stim.DemTarget.__repr__)
    - [`stim.DemTarget.__str__`](#stim.DemTarget.__str__)
    - [`stim.DemTarget.is_logical_observable_id`](#stim.DemTarget.is_logical_observable_id)
    - [`stim.DemTarget.is_relative_detector_id`](#stim.DemTarget.is_relative_detector_id)
    - [`stim.DemTarget.is_separator`](#stim.DemTarget.is_separator)
- [`stim.DetectorErrorModel`](#stim.DetectorErrorModel)
    - [`stim.DetectorErrorModel.__eq__`](#stim.DetectorErrorModel.__eq__)
    - [`stim.DetectorErrorModel.__getitem__`](#stim.DetectorErrorModel.__getitem__)
    - [`stim.DetectorErrorModel.__init__`](#stim.DetectorErrorModel.__init__)
    - [`stim.DetectorErrorModel.__len__`](#stim.DetectorErrorModel.__len__)
    - [`stim.DetectorErrorModel.__ne__`](#stim.DetectorErrorModel.__ne__)
    - [`stim.DetectorErrorModel.__repr__`](#stim.DetectorErrorModel.__repr__)
    - [`stim.DetectorErrorModel.__str__`](#stim.DetectorErrorModel.__str__)
    - [`stim.DetectorErrorModel.clear`](#stim.DetectorErrorModel.clear)
    - [`stim.DetectorErrorModel.copy`](#stim.DetectorErrorModel.copy)
    - [`stim.DetectorErrorModel.num_detectors`](#stim.DetectorErrorModel.num_detectors)
    - [`stim.DetectorErrorModel.num_observables`](#stim.DetectorErrorModel.num_observables)
- [`stim.GateTarget`](#stim.GateTarget)
    - [`stim.GateTarget.__eq__`](#stim.GateTarget.__eq__)
    - [`stim.GateTarget.__init__`](#stim.GateTarget.__init__)
    - [`stim.GateTarget.__ne__`](#stim.GateTarget.__ne__)
    - [`stim.GateTarget.__repr__`](#stim.GateTarget.__repr__)
    - [`stim.GateTarget.is_inverted_result_target`](#stim.GateTarget.is_inverted_result_target)
    - [`stim.GateTarget.is_measurement_record_target`](#stim.GateTarget.is_measurement_record_target)
    - [`stim.GateTarget.is_x_target`](#stim.GateTarget.is_x_target)
    - [`stim.GateTarget.is_y_target`](#stim.GateTarget.is_y_target)
    - [`stim.GateTarget.is_z_target`](#stim.GateTarget.is_z_target)
    - [`stim.GateTarget.value`](#stim.GateTarget.value)
- [`stim.PauliString`](#stim.PauliString)
    - [`stim.PauliString.__add__`](#stim.PauliString.__add__)
    - [`stim.PauliString.__eq__`](#stim.PauliString.__eq__)
    - [`stim.PauliString.__getitem__`](#stim.PauliString.__getitem__)
    - [`stim.PauliString.__iadd__`](#stim.PauliString.__iadd__)
    - [`stim.PauliString.__imul__`](#stim.PauliString.__imul__)
    - [`stim.PauliString.__init__`](#stim.PauliString.__init__)
    - [`stim.PauliString.__len__`](#stim.PauliString.__len__)
    - [`stim.PauliString.__mul__`](#stim.PauliString.__mul__)
    - [`stim.PauliString.__ne__`](#stim.PauliString.__ne__)
    - [`stim.PauliString.__neg__`](#stim.PauliString.__neg__)
    - [`stim.PauliString.__pos__`](#stim.PauliString.__pos__)
    - [`stim.PauliString.__repr__`](#stim.PauliString.__repr__)
    - [`stim.PauliString.__rmul__`](#stim.PauliString.__rmul__)
    - [`stim.PauliString.__setitem__`](#stim.PauliString.__setitem__)
    - [`stim.PauliString.__str__`](#stim.PauliString.__str__)
    - [`stim.PauliString.__truediv__`](#stim.PauliString.__truediv__)
    - [`stim.PauliString.commutes`](#stim.PauliString.commutes)
    - [`stim.PauliString.copy`](#stim.PauliString.copy)
    - [`stim.PauliString.extended_product`](#stim.PauliString.extended_product)
    - [`stim.PauliString.random`](#stim.PauliString.random)
    - [`stim.PauliString.sign`](#stim.PauliString.sign)
- [`stim.Tableau`](#stim.Tableau)
    - [`stim.Tableau.__add__`](#stim.Tableau.__add__)
    - [`stim.Tableau.__call__`](#stim.Tableau.__call__)
    - [`stim.Tableau.__eq__`](#stim.Tableau.__eq__)
    - [`stim.Tableau.__iadd__`](#stim.Tableau.__iadd__)
    - [`stim.Tableau.__init__`](#stim.Tableau.__init__)
    - [`stim.Tableau.__len__`](#stim.Tableau.__len__)
    - [`stim.Tableau.__mul__`](#stim.Tableau.__mul__)
    - [`stim.Tableau.__ne__`](#stim.Tableau.__ne__)
    - [`stim.Tableau.__pow__`](#stim.Tableau.__pow__)
    - [`stim.Tableau.__repr__`](#stim.Tableau.__repr__)
    - [`stim.Tableau.__str__`](#stim.Tableau.__str__)
    - [`stim.Tableau.append`](#stim.Tableau.append)
    - [`stim.Tableau.copy`](#stim.Tableau.copy)
    - [`stim.Tableau.from_conjugated_generators`](#stim.Tableau.from_conjugated_generators)
    - [`stim.Tableau.from_named_gate`](#stim.Tableau.from_named_gate)
    - [`stim.Tableau.inverse`](#stim.Tableau.inverse)
    - [`stim.Tableau.inverse_x_output`](#stim.Tableau.inverse_x_output)
    - [`stim.Tableau.inverse_x_output_pauli`](#stim.Tableau.inverse_x_output_pauli)
    - [`stim.Tableau.inverse_y_output`](#stim.Tableau.inverse_y_output)
    - [`stim.Tableau.inverse_y_output_pauli`](#stim.Tableau.inverse_y_output_pauli)
    - [`stim.Tableau.inverse_z_output`](#stim.Tableau.inverse_z_output)
    - [`stim.Tableau.inverse_z_output_pauli`](#stim.Tableau.inverse_z_output_pauli)
    - [`stim.Tableau.prepend`](#stim.Tableau.prepend)
    - [`stim.Tableau.random`](#stim.Tableau.random)
    - [`stim.Tableau.then`](#stim.Tableau.then)
    - [`stim.Tableau.x_output`](#stim.Tableau.x_output)
    - [`stim.Tableau.x_output_pauli`](#stim.Tableau.x_output_pauli)
    - [`stim.Tableau.y_output`](#stim.Tableau.y_output)
    - [`stim.Tableau.y_output_pauli`](#stim.Tableau.y_output_pauli)
    - [`stim.Tableau.z_output`](#stim.Tableau.z_output)
    - [`stim.Tableau.z_output_pauli`](#stim.Tableau.z_output_pauli)
- [`stim.TableauSimulator`](#stim.TableauSimulator)
    - [`stim.TableauSimulator.canonical_stabilizers`](#stim.TableauSimulator.canonical_stabilizers)
    - [`stim.TableauSimulator.cnot`](#stim.TableauSimulator.cnot)
    - [`stim.TableauSimulator.copy`](#stim.TableauSimulator.copy)
    - [`stim.TableauSimulator.current_inverse_tableau`](#stim.TableauSimulator.current_inverse_tableau)
    - [`stim.TableauSimulator.current_measurement_record`](#stim.TableauSimulator.current_measurement_record)
    - [`stim.TableauSimulator.cy`](#stim.TableauSimulator.cy)
    - [`stim.TableauSimulator.cz`](#stim.TableauSimulator.cz)
    - [`stim.TableauSimulator.do`](#stim.TableauSimulator.do)
    - [`stim.TableauSimulator.h`](#stim.TableauSimulator.h)
    - [`stim.TableauSimulator.h_xy`](#stim.TableauSimulator.h_xy)
    - [`stim.TableauSimulator.h_yz`](#stim.TableauSimulator.h_yz)
    - [`stim.TableauSimulator.iswap`](#stim.TableauSimulator.iswap)
    - [`stim.TableauSimulator.iswap_dag`](#stim.TableauSimulator.iswap_dag)
    - [`stim.TableauSimulator.measure`](#stim.TableauSimulator.measure)
    - [`stim.TableauSimulator.measure_kickback`](#stim.TableauSimulator.measure_kickback)
    - [`stim.TableauSimulator.measure_many`](#stim.TableauSimulator.measure_many)
    - [`stim.TableauSimulator.peek_bloch`](#stim.TableauSimulator.peek_bloch)
    - [`stim.TableauSimulator.reset`](#stim.TableauSimulator.reset)
    - [`stim.TableauSimulator.s`](#stim.TableauSimulator.s)
    - [`stim.TableauSimulator.s_dag`](#stim.TableauSimulator.s_dag)
    - [`stim.TableauSimulator.set_inverse_tableau`](#stim.TableauSimulator.set_inverse_tableau)
    - [`stim.TableauSimulator.set_num_qubits`](#stim.TableauSimulator.set_num_qubits)
    - [`stim.TableauSimulator.sqrt_x`](#stim.TableauSimulator.sqrt_x)
    - [`stim.TableauSimulator.sqrt_x_dag`](#stim.TableauSimulator.sqrt_x_dag)
    - [`stim.TableauSimulator.sqrt_y`](#stim.TableauSimulator.sqrt_y)
    - [`stim.TableauSimulator.sqrt_y_dag`](#stim.TableauSimulator.sqrt_y_dag)
    - [`stim.TableauSimulator.state_vector`](#stim.TableauSimulator.state_vector)
    - [`stim.TableauSimulator.swap`](#stim.TableauSimulator.swap)
    - [`stim.TableauSimulator.x`](#stim.TableauSimulator.x)
    - [`stim.TableauSimulator.xcx`](#stim.TableauSimulator.xcx)
    - [`stim.TableauSimulator.xcy`](#stim.TableauSimulator.xcy)
    - [`stim.TableauSimulator.xcz`](#stim.TableauSimulator.xcz)
    - [`stim.TableauSimulator.y`](#stim.TableauSimulator.y)
    - [`stim.TableauSimulator.ycx`](#stim.TableauSimulator.ycx)
    - [`stim.TableauSimulator.ycy`](#stim.TableauSimulator.ycy)
    - [`stim.TableauSimulator.ycz`](#stim.TableauSimulator.ycz)
    - [`stim.TableauSimulator.z`](#stim.TableauSimulator.z)
- [`stim.target_inv`](#stim.target_inv)
- [`stim.target_logical_observable_id`](#stim.target_logical_observable_id)
- [`stim.target_rec`](#stim.target_rec)
- [`stim.target_relative_detector_id`](#stim.target_relative_detector_id)
- [`stim.target_separator`](#stim.target_separator)
- [`stim.target_x`](#stim.target_x)
- [`stim.target_y`](#stim.target_y)
- [`stim.target_z`](#stim.target_z)

## `stim.Circuit`<a name="stim.Circuit"></a>
> ```
> A mutable stabilizer circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit()
>     >>> c.append_operation("X", [0])
>     >>> c.append_operation("M", [0])
>     >>> c.compile_sampler().sample(shots=1)
>     array([[1]], dtype=uint8)
> 
>     >>> stim.Circuit('''
>     ...    H 0
>     ...    CNOT 0 1
>     ...    M 0 1
>     ...    DETECTOR rec[-1] rec[-2]
>     ... ''').compile_detector_sampler().sample(shots=1)
>     array([[0]], dtype=uint8)
> 
> ```

## `stim.CircuitInstruction`<a name="stim.CircuitInstruction"></a>
> ```
> An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...     H 0
>     ...     M 0 !1
>     ...     X_ERROR(0.125) 5 3
>     ... ''')
>     >>> circuit[0]
>     stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
>     >>> circuit[1]
>     stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(stim.target_inv(1))], [])
>     >>> circuit[2]
>     stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5), stim.GateTarget(3)], [0.125])
> ```

## `stim.CircuitRepeatBlock`<a name="stim.CircuitRepeatBlock"></a>
> ```
> A REPEAT block from a circuit.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...     H 0
>     ...     REPEAT 5 {
>     ...         CX 0 1
>     ...         CZ 1 2
>     ...     }
>     ... ''')
>     >>> repeat_block = circuit[1]
>     >>> repeat_block.repeat_count
>     5
>     >>> repeat_block.body_copy()
>     stim.Circuit('''
>     CX 0 1
>     CZ 1 2
>     ''')
> ```

## `stim.CompiledDetectorSampler`<a name="stim.CompiledDetectorSampler"></a>
> ```
> An analyzed stabilizer circuit whose detection events can be sampled quickly.
> ```

## `stim.CompiledMeasurementSampler`<a name="stim.CompiledMeasurementSampler"></a>
> ```
> An analyzed stabilizer circuit whose measurements can be sampled quickly.
> ```

## `stim.DemInstruction`<a name="stim.DemInstruction"></a>
> ```
> An instruction from a detector error model.
> 
> Examples:
>     >>> import stim
>     >>> model = stim.DetectorErrorModel('''
>     ...     error(0.125) D0
>     ...     error(0.125) D0 D1 L0
>     ...     error(0.125) D1 D2
>     ...     error(0.125) D2 D3
>     ...     error(0.125) D3
>     ... ''')
>     >>> instruction = model[0]
>     >>> instruction
>     stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
> ```

## `stim.DemRepeatBlock`<a name="stim.DemRepeatBlock"></a>
> ```
> A repeat block from a detector error model.
> 
> Examples:
>     >>> import stim
>     >>> model = stim.DetectorErrorModel('''
>     ...     repeat 100 {
>     ...         error(0.125) D0 D1
>     ...         shift_detectors 1
>     ...     }
>     ... ''')
>     >>> model[0]
>     stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
>     error(0.125) D0 D1
>     shift_detectors 1
>     '''))
> ```

## `stim.DemTarget`<a name="stim.DemTarget"></a>
> ```
> An instruction target from a detector error model (.dem) file.
> ```

## `stim.DetectorErrorModel`<a name="stim.DetectorErrorModel"></a>
> ```
> A list of instructions describing error mechanisms in terms of the detection events they produce.
> 
> Examples:
>     >>> import stim
>     >>> model = stim.DetectorErrorModel('''
>     ...     error(0.125) D0
>     ...     error(0.125) D0 D1 L0
>     ...     error(0.125) D1 D2
>     ...     error(0.125) D2 D3
>     ...     error(0.125) D3
>     ... ''')
>     >>> len(model)
>     5
> 
>     >>> stim.Circuit('''
>     ...     X_ERROR(0.125) 0
>     ...     X_ERROR(0.25) 1
>     ...     CORRELATED_ERROR(0.375) X0 X1
>     ...     M 0 1
>     ...     DETECTOR rec[-2]
>     ...     DETECTOR rec[-1]
>     ... ''').detector_error_model()
>     stim.DetectorErrorModel('''
>     error(0.125) D0
>     error(0.375) D0 D1
>     error(0.25) D1
>     ''')
> ```

## `stim.GateTarget`<a name="stim.GateTarget"></a>
> ```
> Represents a gate target, like `0` or `rec[-1]`, from a circuit.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...     M 0 !1
>     ... ''')
>     >>> circuit[0].targets_copy()[0]
>     stim.GateTarget(0)
>     >>> circuit[0].targets_copy()[1]
>     stim.GateTarget(stim.target_inv(1))
> ```

## `stim.PauliString`<a name="stim.PauliString"></a>
> ```
> A signed Pauli tensor product (e.g. "+X \u2297 X \u2297 X" or "-Y \u2297 Z".
> 
> Represents a collection of Pauli operations (I, X, Y, Z) applied pairwise to a collection of qubits.
> 
> Examples:
>     >>> import stim
>     >>> stim.PauliString("XX") * stim.PauliString("YY")
>     stim.PauliString("-ZZ")
>     >>> print(stim.PauliString(5))
>     +_____
> ```

## `stim.Tableau`<a name="stim.Tableau"></a>
> ```
> A stabilizer tableau.
> 
> Represents a Clifford operation by explicitly storing how that operation conjugates a list of Pauli
> group generators into composite Pauli products.
> 
> Examples:
>     >>> import stim
>     >>> stim.Tableau.from_named_gate("H")
>     stim.Tableau.from_conjugated_generators(
>         xs=[
>             stim.PauliString("+Z"),
>         ],
>         zs=[
>             stim.PauliString("+X"),
>         ],
>     )
> 
>     >>> t = stim.Tableau.random(5)
>     >>> t_inv = t**-1
>     >>> print(t * t_inv)
>     +-xz-xz-xz-xz-xz-
>     | ++ ++ ++ ++ ++
>     | XZ __ __ __ __
>     | __ XZ __ __ __
>     | __ __ XZ __ __
>     | __ __ __ XZ __
>     | __ __ __ __ XZ
> 
>     >>> x2z3 = t.x_output(2) * t.z_output(3)
>     >>> t_inv(x2z3)
>     stim.PauliString("+__XZ_")
> ```

## `stim.TableauSimulator`<a name="stim.TableauSimulator"></a>
> ```
> A quantum stabilizer circuit simulator whose internal state is an inverse stabilizer tableau.
> 
> Supports interactive usage, where gates and measurements are applied on demand.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.h(0)
>     >>> if s.measure(0):
>     ...     s.h(1)
>     ...     s.cnot(1, 2)
>     >>> s.measure(1) == s.measure(2)
>     True
> 
>     >>> s = stim.TableauSimulator()
>     >>> s.h(0)
>     >>> s.cnot(0, 1)
>     >>> s.current_inverse_tableau()
>     stim.Tableau.from_conjugated_generators(
>         xs=[
>             stim.PauliString("+ZX"),
>             stim.PauliString("+_X"),
>         ],
>         zs=[
>             stim.PauliString("+X_"),
>             stim.PauliString("+XZ"),
>         ],
>     )
> ```

## `stim.target_inv(qubit_index: int) -> int`<a name="stim.target_inv"></a>
> ```
> Returns a target flagged as inverted that can be passed into Circuit.append_operation
> For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
> meaning the measurement result from qubit 1 should be inverted when reported.
> ```

## `stim.target_logical_observable_id(index: int) -> stim.DemTarget`<a name="stim.target_logical_observable_id"></a>
> ```
> Returns a logical observable id identifying a frame change (e.g. "L5" in a .dem file).
> 
> Args:
>     index: The index of the observable.
> 
> Returns:
>     The logical observable target.
> ```

## `stim.target_rec(lookback_index: int) -> int`<a name="stim.target_rec"></a>
> ```
> Returns a record target that can be passed into Circuit.append_operation.
> For example, the 'rec[-2]' in 'DETECTOR rec[-2]' is a record target.
> ```

## `stim.target_relative_detector_id(index: int) -> stim.DemTarget`<a name="stim.target_relative_detector_id"></a>
> ```
> Returns a relative detector id (e.g. "D5" in a .dem file).
> 
> Args:
>     index: The index of the detector, relative to the current detector offset.
> 
> Returns:
>     The relative detector target.
> ```

## `stim.target_separator() -> stim.DemTarget`<a name="stim.target_separator"></a>
> ```
> Returns a target separator (e.g. "^" in a .dem file).
> ```

## `stim.target_x(qubit_index: int) -> int`<a name="stim.target_x"></a>
> ```
> Returns a target flagged as Pauli X that can be passed into Circuit.append_operation
> For example, the 'X1' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 1 flagged as Pauli X.
> ```

## `stim.target_y(qubit_index: int) -> int`<a name="stim.target_y"></a>
> ```
> Returns a target flagged as Pauli Y that can be passed into Circuit.append_operation
> For example, the 'Y2' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 2 flagged as Pauli Y.
> ```

## `stim.target_z(qubit_index: int) -> int`<a name="stim.target_z"></a>
> ```
> Returns a target flagged as Pauli Z that can be passed into Circuit.append_operation
> For example, the 'Z3' in 'CORRELATED_ERROR(0.1) X1 Y2 Z3' is qubit 3 flagged as Pauli Z.
> ```

### `stim.Circuit.__add__(self, second: stim.Circuit) -> stim.Circuit`<a name="stim.Circuit.__add__"></a>
> ```
> Creates a circuit by appending two circuits.
> 
> Examples:
>     >>> import stim
>     >>> c1 = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> c2 = stim.Circuit('''
>     ...    M 0 1 2
>     ... ''')
>     >>> print(c1 + c2)
>     X 0
>     Y 1 2
>     M 0 1 2
> ```

### `stim.Circuit.__eq__(self, arg0: stim.Circuit) -> bool`<a name="stim.Circuit.__eq__"></a>
> ```
> Determines if two circuits have identical contents.
> ```

### `stim.Circuit.__getitem__(self, arg0: int) -> object`<a name="stim.Circuit.__getitem__"></a>
> ```
> Returns copies of instructions from the circuit.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...    X 0
>     ...    X_ERROR(0.5) 1 2
>     ...    REPEAT 100 {
>     ...        X 0
>     ...        Y 1 2
>     ...    }
>     ...    TICK
>     ...    M 0
>     ...    DETECTOR rec[-1]
>     ... ''')
>     >>> circuit[1]
>     stim.CircuitInstruction('X_ERROR', [stim.GateTarget(1), stim.GateTarget(2)], [0.5])
>     >>> circuit[2]
>     stim.CircuitRepeatBlock(100, stim.Circuit('''
>     X 0
>     Y 1 2
>     '''))
> ```

### `stim.Circuit.__iadd__(self, second: stim.Circuit) -> stim.Circuit`<a name="stim.Circuit.__iadd__"></a>
> ```
> Appends a circuit into the receiving circuit (mutating it).
> 
> Examples:
>     >>> import stim
>     >>> c1 = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> c2 = stim.Circuit('''
>     ...    M 0 1 2
>     ... ''')
>     >>> c1 += c2
>     >>> print(c1)
>     X 0
>     Y 1 2
>     M 0 1 2
> ```

### `stim.Circuit.__imul__(self, repetitions: int) -> stim.Circuit`<a name="stim.Circuit.__imul__"></a>
> ```
> Mutates the circuit by putting its contents into a REPEAT block.
> 
> Special case: if the repetition count is 0, the circuit is cleared.
> Special case: if the repetition count is 1, nothing happens.
> 
> Args:
>     repetitions: The number of times the REPEAT block should repeat.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> c *= 3
>     >>> print(c)
>     REPEAT 3 {
>         X 0
>         Y 1 2
>     }
> ```

### `stim.Circuit.__init__(self, stim_program_text: str = '') -> None`<a name="stim.Circuit.__init__"></a>
> ```
> Creates a stim.Circuit.
> 
> Args:
>     stim_program_text: Defaults to empty. Describes operations to append into the circuit.
> 
> Examples:
>     >>> import stim
>     >>> empty = stim.Circuit()
>     >>> not_empty = stim.Circuit('''
>     ...    X 0
>     ...    CNOT 0 1
>     ...    M 1
>     ... ''')
> ```

### `stim.Circuit.__len__(self) -> int`<a name="stim.Circuit.__len__"></a>
> ```
> Returns the number of top-level instructions and blocks in the circuit.
> 
> Instructions inside of blocks are not included in this count.
> 
> Examples:
>     >>> import stim
>     >>> len(stim.Circuit())
>     0
>     >>> len(stim.Circuit('''
>     ...    X 0
>     ...    X_ERROR(0.5) 1 2
>     ...    TICK
>     ...    M 0
>     ...    DETECTOR rec[-1]
>     ... '''))
>     5
>     >>> len(stim.Circuit('''
>     ...    REPEAT 100 {
>     ...        X 0
>     ...        Y 1 2
>     ...    }
>     ... '''))
>     1
> ```

### `stim.Circuit.__mul__(self, repetitions: int) -> stim.Circuit`<a name="stim.Circuit.__mul__"></a>
> ```
> Returns a circuit with a REPEAT block containing the current circuit's instructions.
> 
> Special case: if the repetition count is 0, an empty circuit is returned.
> Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.
> 
> Args:
>     repetitions: The number of times the REPEAT block should repeat.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> print(c * 3)
>     REPEAT 3 {
>         X 0
>         Y 1 2
>     }
> ```

### `stim.Circuit.__ne__(self, arg0: stim.Circuit) -> bool`<a name="stim.Circuit.__ne__"></a>
> ```
> Determines if two circuits have non-identical contents.
> ```

### `stim.Circuit.__repr__(self) -> str`<a name="stim.Circuit.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.Circuit`.
> ```

### `stim.Circuit.__rmul__(self, repetitions: int) -> stim.Circuit`<a name="stim.Circuit.__rmul__"></a>
> ```
> Returns a circuit with a REPEAT block containing the current circuit's instructions.
> 
> Special case: if the repetition count is 0, an empty circuit is returned.
> Special case: if the repetition count is 1, an equal circuit with no REPEAT block is returned.
> 
> Args:
>     repetitions: The number of times the REPEAT block should repeat.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> print(3 * c)
>     REPEAT 3 {
>         X 0
>         Y 1 2
>     }
> ```

### `stim.Circuit.__str__(self) -> str`<a name="stim.Circuit.__str__"></a>
> ```
> Returns stim instructions (that can be saved to a file and parsed by stim) for the current circuit.
> ```

### `stim.Circuit.append_from_stim_program_text(self, stim_program_text: str) -> None`<a name="stim.Circuit.append_from_stim_program_text"></a>
> ```
> Appends operations described by a STIM format program into the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit()
>     >>> c.append_from_stim_program_text('''
>     ...    H 0  # comment
>     ...    CNOT 0 2
>     ...
>     ...    M 2
>     ...    CNOT rec[-1] 1
>     ... ''')
>     >>> print(c)
>     H 0
>     CX 0 2
>     M 2
>     CX rec[-1] 1
> 
> Args:
>     text: The STIM program text containing the circuit operations to append.
> ```

### `stim.Circuit.append_operation(self, name: str, targets: List[int], arg: object = None) -> None`<a name="stim.Circuit.append_operation"></a>
> ```
> Appends an operation into the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit()
>     >>> c.append_operation("X", [0])
>     >>> c.append_operation("H", [0, 1])
>     >>> c.append_operation("M", [0, stim.target_inv(1)])
>     >>> c.append_operation("CNOT", [stim.target_rec(-1), 0])
>     >>> c.append_operation("X_ERROR", [0], 0.125)
>     >>> c.append_operation("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
>     >>> print(c)
>     X 0
>     H 0 1
>     M 0 !1
>     CX rec[-1] 0
>     X_ERROR(0.125) 0
>     E(0.25) X0 Y2
> 
> Args:
>     name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").
>     targets: The gate targets. Gates implicitly broadcast over their targets.
>     arg: A double or list of doubles parameterizing the gate. Different gates take different arguments. For
>         example, X_ERROR takes a probability, OBSERVABLE_INCLUDE takes an observable index, and PAULI_CHANNEL_1
>         takes three disjoint probabilities. For backwards compatibility reasons, defaults to (0,) for gates
>         that take one argument. Otherwise defaults to no arguments.
> ```

### `stim.Circuit.clear(self) -> None`<a name="stim.Circuit.clear"></a>
> ```
> Clears the contents of the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0
>     ...    Y 1 2
>     ... ''')
>     >>> c.clear()
>     >>> c
>     stim.Circuit()
> ```

### `stim.Circuit.compile_detector_sampler(self) -> stim.CompiledDetectorSampler`<a name="stim.Circuit.compile_detector_sampler"></a>
> ```
> Returns a CompiledDetectorSampler, which can quickly batch sample detection events, for the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    H 0
>     ...    CNOT 0 1
>     ...    M 0 1
>     ...    DETECTOR rec[-1] rec[-2]
>     ... ''')
>     >>> s = c.compile_detector_sampler()
>     >>> s.sample(shots=1)
>     array([[0]], dtype=uint8)
> ```

### `stim.Circuit.compile_sampler(self) -> stim.CompiledMeasurementSampler`<a name="stim.Circuit.compile_sampler"></a>
> ```
> Returns a CompiledMeasurementSampler, which can quickly batch sample measurements, for the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 2
>     ...    M 0 1 2
>     ... ''')
>     >>> s = c.compile_sampler()
>     >>> s.sample(shots=1)
>     array([[0, 0, 1]], dtype=uint8)
> ```

### `stim.Circuit.copy(self) -> stim.Circuit`<a name="stim.Circuit.copy"></a>
> ```
> Returns a copy of the circuit. An independent circuit with the same contents.
> 
> Examples:
>     >>> import stim
> 
>     >>> c1 = stim.Circuit("H 0")
>     >>> c2 = c1.copy()
>     >>> c2 is c1
>     False
>     >>> c2 == c1
>     True
> ```

### `stim.Circuit.detector_error_model(self, *, decompose_errors: bool = False, flatten_loops: bool = False, allow_gauge_detectors: bool = False, approximate_disjoint_errors: float = False) -> stim.DetectorErrorModel`<a name="stim.Circuit.detector_error_model"></a>
> ```
> Returns a stim.DetectorErrorModel describing the error processes in the circuit.
> 
> Args:
>     decompose_errors: Defaults to false. When set to true, the error analysis attempts to decompose the
>         components of composite error mechanisms (such as depolarization errors) into simpler errors, and
>         suggest this decomposition via `stim.target_separator()` between the components. For example, in an
>         XZ surface code, single qubit depolarization has a Y error term which can be decomposed into simpler
>         X and Z error terms. Decomposition fails (causing this method to throw) if it's not possible to
>         decompose large errors into simple errors that affect at most two detectors.
>     flatten_loops: Defaults to false. When set to true, the output will not contain any `repeat` blocks.
>         When set to false, the error analysis watches for loops in the circuit reaching a periodic steady
>         state with respect to the detectors being introduced, the error mechanisms that affect them, and the
>         locations of the logical observables. When it identifies such a steady state, it outputs a repeat
>         block. This is massively more efficient than flattening for circuits that contain loops, but creates
>         a more complex output.
>     allow_gauge_detectors: Defaults to false. When set to false, the error analysis verifies that detectors
>         in the circuit are actually deterministic under noiseless execution of the circuit. When set to
>         true, these detectors are instead considered to be part of degrees freedom that can be removed from
>         the error model. For example, if detectors D1 and D3 both anti-commute with a reset, then the error
>         model has a gauge `error(0.5) D1 D3`. When gauges are identified, one of the involved detectors is
>         removed from the system using Gaussian elimination.
> 
>         Note that logical observables are still verified to be deterministic, even if this option is set.
>     approximate_disjoint_errors: Defaults to false. When set to false, composite error mechanisms with
>         disjoint components (such as `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) can cause the error analysis to throw
>         exceptions (because detector error models can only contain independent error mechanisms). When set
>         to true, the probabilities of the disjoint cases are instead assumed to be independent
>         probabilities. For example, a ``PAULI_CHANNEL_1(0.1, 0.2, 0.0)` becomes equivalent to an
>         `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`. This assumption is an approximation, but it is a good
>         approximation for small probabilities.
> 
>         This argument can also be set to a probability between 0 and 1, setting a threshold below which the
>         approximation is acceptable. Any error mechanisms that have a component probability above the
>         threshold will cause an exception to be thrown.
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.Circuit('''
>     ...     X_ERROR(0.125) 0
>     ...     X_ERROR(0.25) 1
>     ...     CORRELATED_ERROR(0.375) X0 X1
>     ...     M 0 1
>     ...     DETECTOR rec[-2]
>     ...     DETECTOR rec[-1]
>     ... ''').detector_error_model()
>     stim.DetectorErrorModel('''
>     error(0.125) D0
>     error(0.375) D0 D1
>     error(0.25) D1
>     ''')
> ```

### `stim.Circuit.flattened_operations(self) -> list`<a name="stim.Circuit.flattened_operations"></a>
> ```
> Flattens the circuit's operations into a list.
> 
> The operations within repeat blocks are actually repeated in the output.
> 
> Returns:
>     A List[Tuple[name, targets, arg]] of the operations in the circuit.
>         name: A string with the gate's name.
>         targets: A list of things acted on by the gate. Each thing can be:
>             int: The index of a qubit.
>             Tuple["inv", int]: The index of a qubit to measure with an inverted result.
>             Tuple["rec", int]: A measurement record target like `rec[-1]`.
>             Tuple["X", int]: A Pauli X operation to apply during a correlated error.
>             Tuple["Y", int]: A Pauli Y operation to apply during a correlated error.
>             Tuple["Z", int]: A Pauli Z operation to apply during a correlated error.
>         arg: The gate's numeric argument. For most gates this is just 0. For noisy
>             gates this is the probability of the noise being applied.
> 
> Examples:
>     >>> import stim
>     >>> stim.Circuit('''
>     ...    H 0
>     ...    X_ERROR(0.125) 1
>     ...    M 0 !1
>     ... ''').flattened_operations()
>     [('H', [0], 0), ('X_ERROR', [1], 0.125), ('M', [0, ('inv', 1)], 0)]
> 
>     >>> stim.Circuit('''
>     ...    REPEAT 2 {
>     ...        H 6
>     ...    }
>     ... ''').flattened_operations()
>     [('H', [6], 0), ('H', [6], 0)]
> ```

### `stim.Circuit.generated(code_task: str, *, distance: int, rounds: int, after_clifford_depolarization: float = 0.0, before_round_data_depolarization: float = 0.0, before_measure_flip_probability: float = 0.0, after_reset_flip_probability: float = 0.0) -> stim.Circuit`<a name="stim.Circuit.generated"></a>
> ```
> Generates common circuits.
> 
> The generated circuits can include configurable noise.
> 
> The generated circuits include DETECTOR and OBSERVABLE_INCLUDE annotations so that their detection events
> and logical observables can be sampled.
> 
> The generated circuits include TICK annotations to mark the progression of time. (E.g. so that converting
> them using `stimcirq.stim_circuit_to_cirq_circuit` will produce a `cirq.Circuit` with the intended moment
> structure.)
> 
> Args:
>     type: A string identifying the type of circuit to generate. Available types are:
>         - `repetition_code:memory`
>         - `surface_code:rotated_memory_x`
>         - `surface_code:rotated_memory_z`
>         - `surface_code:unrotated_memory_x`
>         - `surface_code:unrotated_memory_z`
>         - `color_code:memory_xyz`
>     distance: The desired code distance of the generated circuit. The code distance is the minimum number
>         of physical errors needed to cause a logical error. This parameter indirectly determines how many
>         qubits the generated circuit uses.
>     rounds: How many times the measurement qubits in the generated circuit will be measured. Indirectly
>         determines the duration of the generated circuit.
>     after_clifford_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to add
>         after every single-qubit Clifford operation and `DEPOLARIZE2(p)` operations to add after every
>         two-qubit Clifford operation. The after-Clifford depolarizing operations are only included if this
>         probability is not 0.
>     before_round_data_depolarization: Defaults to 0. The probability (p) of `DEPOLARIZE1(p)` operations to
>         apply to every data qubit at the start of a round of stabilizer measurements. The start-of-round
>         depolarizing operations are only included if this probability is not 0.
>     before_measure_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
>         to qubits before each measurement (X basis measurements use `Z_ERROR(p)` instead). The
>         before-measurement flips are only included if this probability is not 0.
>     after_reset_flip_probability: Defaults to 0. The probability (p) of `X_ERROR(p)` operations applied
>         to qubits after each reset (X basis resets use `Z_ERROR(p)` instead). The after-reset flips are only
>         included if this probability is not 0.
> 
> Returns:
>     The generated circuit.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit.generated(
>     ...     "repetition_code:memory",
>     ...     distance=3,
>     ...     rounds=10000,
>     ...     after_clifford_depolarization=0.0125)
>     >>> print(circuit)
>     R 0 1 2 3 4 5 6
>     TICK
>     CX 0 1 2 3 4 5
>     DEPOLARIZE2(0.0125) 0 1 2 3 4 5
>     TICK
>     CX 2 1 4 3 6 5
>     DEPOLARIZE2(0.0125) 2 1 4 3 6 5
>     TICK
>     MR 1 3 5
>     DETECTOR(1, 0) rec[-3]
>     DETECTOR(3, 0) rec[-2]
>     DETECTOR(5, 0) rec[-1]
>     REPEAT 9999 {
>         TICK
>         CX 0 1 2 3 4 5
>         DEPOLARIZE2(0.0125) 0 1 2 3 4 5
>         TICK
>         CX 2 1 4 3 6 5
>         DEPOLARIZE2(0.0125) 2 1 4 3 6 5
>         TICK
>         MR 1 3 5
>         SHIFT_COORDS(0, 1)
>         DETECTOR(1, 0) rec[-3] rec[-6]
>         DETECTOR(3, 0) rec[-2] rec[-5]
>         DETECTOR(5, 0) rec[-1] rec[-4]
>     }
>     M 0 2 4 6
>     DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
>     DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
>     DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
>     OBSERVABLE_INCLUDE(0) rec[-1]
> ```

### `stim.Circuit.num_detectors`<a name="stim.Circuit.num_detectors"></a>
> ```
> Counts the number of bits produced when sampling the circuit's detectors.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    M 0
>     ...    DETECTOR rec[-1]
>     ...    REPEAT 100 {
>     ...        M 0 1 2
>     ...        DETECTOR rec[-1]
>     ...        DETECTOR rec[-2]
>     ...    }
>     ... ''')
>     >>> c.num_detectors
>     201
> ```

### `stim.Circuit.num_measurements`<a name="stim.Circuit.num_measurements"></a>
> ```
> Counts the number of bits produced when sampling the circuit's measurements.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    M 0
>     ...    REPEAT 100 {
>     ...        M 0 1
>     ...    }
>     ... ''')
>     >>> c.num_measurements
>     201
> ```

### `stim.Circuit.num_observables`<a name="stim.Circuit.num_observables"></a>
> ```
> Counts the number of bits produced when sampling the circuit's logical observables.
> 
> This is one more than the largest observable index given to OBSERVABLE_INCLUDE.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    M 0
>     ...    OBSERVABLE_INCLUDE(2) rec[-1]
>     ...    OBSERVABLE_INCLUDE(5) rec[-1]
>     ... ''')
>     >>> c.num_observables
>     6
> ```

### `stim.Circuit.num_qubits`<a name="stim.Circuit.num_qubits"></a>
> ```
> Counts the number of qubits used when simulating the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    M 0
>     ...    M 0 1
>     ... ''')
>     >>> c.num_qubits
>     2
>     >>> c.append_from_stim_program_text('''
>     ...    X 100
>     ... ''')
>     >>> c.num_qubits
>     101
> ```

### `stim.CircuitInstruction.__eq__(self, arg0: stim.CircuitInstruction) -> bool`<a name="stim.CircuitInstruction.__eq__"></a>
> ```
> Determines if two `stim.CircuitInstruction`s are identical.
> ```

### `stim.CircuitInstruction.__init__(self, name: str, targets: List[GateTarget], gate_args: List[float] = ()) -> None`<a name="stim.CircuitInstruction.__init__"></a>
> ```
> Initializes a `stim.CircuitInstruction`.
> 
> Args:
>     name: The name of the instruction being applied.
>     targets: The targets the instruction is being applied to. These can be raw values like `0` and
>         `stim.target_rec(-1)`, or instances of `stim.GateTarget`.
>     gate_args: The sequence of numeric arguments parameterizing a gate. For noise gates this is their
>         probabilities. For OBSERVABLE_INCLUDE it's the logical observable's index.
> ```

### `stim.CircuitInstruction.__ne__(self, arg0: stim.CircuitInstruction) -> bool`<a name="stim.CircuitInstruction.__ne__"></a>
> ```
> Determines if two `stim.CircuitInstruction`s are different.
> ```

### `stim.CircuitInstruction.__repr__(self) -> str`<a name="stim.CircuitInstruction.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.
> ```

### `stim.CircuitInstruction.gate_args_copy(self) -> List[float]`<a name="stim.CircuitInstruction.gate_args_copy"></a>
> ```
> Returns the gate's arguments (numbers parameterizing the instruction).
> 
> For noisy gates this typically a list of probabilities.
> For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable index.
> ```

### `stim.CircuitInstruction.name`<a name="stim.CircuitInstruction.name"></a>
> ```
> The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
> ```

### `stim.CircuitInstruction.targets_copy(self) -> List[GateTarget]`<a name="stim.CircuitInstruction.targets_copy"></a>
> ```
> Returns a copy of the targets of the instruction.
> ```

### `stim.CircuitRepeatBlock.__eq__(self, arg0: stim.CircuitRepeatBlock) -> bool`<a name="stim.CircuitRepeatBlock.__eq__"></a>
> ```
> Determines if two `stim.CircuitRepeatBlock`s are identical.
> ```

### `stim.CircuitRepeatBlock.__init__(self, repeat_count: int, body: stim.Circuit) -> None`<a name="stim.CircuitRepeatBlock.__init__"></a>
> ```
> Initializes a `stim.CircuitRepeatBlock`.
> 
> Args:
>     repeat_count: The number of times to repeat the block.
>     body: The body of the block, as a circuit.
> ```

### `stim.CircuitRepeatBlock.__ne__(self, arg0: stim.CircuitRepeatBlock) -> bool`<a name="stim.CircuitRepeatBlock.__ne__"></a>
> ```
> Determines if two `stim.CircuitRepeatBlock`s are different.
> ```

### `stim.CircuitRepeatBlock.__repr__(self) -> str`<a name="stim.CircuitRepeatBlock.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitRepeatBlock`.
> ```

### `stim.CircuitRepeatBlock.body_copy(self) -> stim.Circuit`<a name="stim.CircuitRepeatBlock.body_copy"></a>
> ```
> Returns a copy of the body of the repeat block.
> 
> The copy is forced to ensure it's clear that editing the result will not change the circuit that the repeat
> block came from.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...     H 0
>     ...     REPEAT 5 {
>     ...         CX 0 1
>     ...         CZ 1 2
>     ...     }
>     ... ''')
>     >>> repeat_block = circuit[1]
>     >>> repeat_block.body_copy()
>     stim.Circuit('''
>     CX 0 1
>     CZ 1 2
>     ''')
> ```

### `stim.CircuitRepeatBlock.repeat_count`<a name="stim.CircuitRepeatBlock.repeat_count"></a>
> ```
> The repetition count of the repeat block.
> 
> Examples:
>     >>> import stim
>     >>> circuit = stim.Circuit('''
>     ...     H 0
>     ...     REPEAT 5 {
>     ...         CX 0 1
>     ...         CZ 1 2
>     ...     }
>     ... ''')
>     >>> repeat_block = circuit[1]
>     >>> repeat_block.repeat_count
>     5
> ```

### `stim.CompiledDetectorSampler.__repr__(self) -> str`<a name="stim.CompiledDetectorSampler.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledDetectorSampler`.
> ```

### `stim.CompiledDetectorSampler.sample(self, shots: int, *, prepend_observables: bool = False, append_observables: bool = False) -> numpy.ndarray[numpy.uint8]`<a name="stim.CompiledDetectorSampler.sample"></a>
> ```
> Returns a numpy array containing a batch of detector samples from the circuit.
> 
> The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
> instructions can also be included in the results as honorary detectors.
> 
> Args:
>     shots: The number of times to sample every detector in the circuit.
>     prepend_observables: Defaults to false. When set, observables are included with the detectors and are
>         placed at the start of the results.
>     append_observables: Defaults to false. When set, observables are included with the detectors and are
>         placed at the end of the results.
> 
> Returns:
>     A numpy array with `dtype=uint8` and `shape=(shots, n)` where
>     `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
>     The bit for detection event `m` in shot `s` is at `result[s, m]`.
> ```

### `stim.CompiledDetectorSampler.sample_bit_packed(self, shots: int, *, prepend_observables: bool = False, append_observables: bool = False) -> numpy.ndarray[numpy.uint8]`<a name="stim.CompiledDetectorSampler.sample_bit_packed"></a>
> ```
> Returns a numpy array containing bit packed batch of detector samples from the circuit.
> 
> The circuit must define the detectors using DETECTOR instructions. Observables defined by OBSERVABLE_INCLUDE
> instructions can also be included in the results as honorary detectors.
> 
> Args:
>     shots: The number of times to sample every detector in the circuit.
>     prepend_observables: Defaults to false. When set, observables are included with the detectors and are
>         placed at the start of the results.
>     append_observables: Defaults to false. When set, observables are included with the detectors and are
>         placed at the end of the results.
> 
> Returns:
>     A numpy array with `dtype=uint8` and `shape=(shots, n)` where
>     `n = num_detectors + num_observables*(append_observables + prepend_observables)`.
>     The bit for detection event `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
> ```

### `stim.CompiledMeasurementSampler.__repr__(self) -> str`<a name="stim.CompiledMeasurementSampler.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledMeasurementSampler`.
> ```

### `stim.CompiledMeasurementSampler.sample(self, shots: int) -> numpy.ndarray[numpy.uint8]`<a name="stim.CompiledMeasurementSampler.sample"></a>
> ```
> Returns a numpy array containing a batch of measurement samples from the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0   2 3
>     ...    M 0 1 2 3
>     ... ''')
>     >>> s = c.compile_sampler()
>     >>> s.sample(shots=1)
>     array([[1, 0, 1, 1]], dtype=uint8)
> 
> Args:
>     shots: The number of times to sample every measurement in the circuit.
> 
> Returns:
>     A numpy array with `dtype=uint8` and `shape=(shots, num_measurements)`.
>     The bit for measurement `m` in shot `s` is at `result[s, m]`.
> ```

### `stim.CompiledMeasurementSampler.sample_bit_packed(self, shots: int) -> numpy.ndarray[numpy.uint8]`<a name="stim.CompiledMeasurementSampler.sample_bit_packed"></a>
> ```
> Returns a numpy array containing a bit packed batch of measurement samples from the circuit.
> 
> Examples:
>     >>> import stim
>     >>> c = stim.Circuit('''
>     ...    X 0 1 2 3 4 5 6 7     10
>     ...    M 0 1 2 3 4 5 6 7 8 9 10
>     ... ''')
>     >>> s = c.compile_sampler()
>     >>> s.sample_bit_packed(shots=1)
>     array([[255,   4]], dtype=uint8)
> 
> Args:
>     shots: The number of times to sample every measurement in the circuit.
> 
> Returns:
>     A numpy array with `dtype=uint8` and `shape=(shots, (num_measurements + 7) // 8)`.
>     The bit for measurement `m` in shot `s` is at `result[s, (m // 8)] & 2**(m % 8)`.
> ```

### `stim.DemInstruction.__eq__(self, arg0: stim.DemInstruction) -> bool`<a name="stim.DemInstruction.__eq__"></a>
> ```
> Determines if two instructions have identical contents.
> ```

### `stim.DemInstruction.__init__(self, type: str, args: List[float], targets: List[object]) -> None`<a name="stim.DemInstruction.__init__"></a>
> ```
> Creates a stim.DemInstruction.
> 
> Args:
>     type: The name of the instruction type (e.g. "error" or "shift_detectors").
>     args: Numeric values parameterizing the instruction (e.g. the 0.1 in "error(0.1)").
>     targets: The objects the instruction involves (e.g. the "D0" and "L1" in "error(0.1) D0 L1").
> 
> Examples:
>     >>> import stim
>     >>> instruction = stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(5)])
>     >>> print(instruction)
>     error(0.125) D5
> ```

### `stim.DemInstruction.__ne__(self, arg0: stim.DemInstruction) -> bool`<a name="stim.DemInstruction.__ne__"></a>
> ```
> Determines if two instructions have non-identical contents.
> ```

### `stim.DemInstruction.__repr__(self) -> str`<a name="stim.DemInstruction.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`.
> ```

### `stim.DemInstruction.__str__(self) -> str`<a name="stim.DemInstruction.__str__"></a>
> ```
> Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.
> ```

### `stim.DemInstruction.args_copy(self) -> List[float]`<a name="stim.DemInstruction.args_copy"></a>
> ```
> Returns a copy of the list of numbers parameterizing the instruction (e.g. the probability of an error).
> ```

### `stim.DemInstruction.targets_copy(self) -> List[object]`<a name="stim.DemInstruction.targets_copy"></a>
> ```
> Returns a copy of the list of objects the instruction applies to (e.g. affected detectors.
> ```

### `stim.DemInstruction.type`<a name="stim.DemInstruction.type"></a>
> ```
> The name of the instruction type (e.g. "error" or "shift_detectors").
> ```

### `stim.DemRepeatBlock.__eq__(self, arg0: stim.DemRepeatBlock) -> bool`<a name="stim.DemRepeatBlock.__eq__"></a>
> ```
> Determines if two repeat blocks are identical.
> ```

### `stim.DemRepeatBlock.__init__(self, repeat_count: int, block: stim.DetectorErrorModel) -> None`<a name="stim.DemRepeatBlock.__init__"></a>
> ```
> Creates a stim.DemRepeatBlock.
> 
> Args:
>     repeat_count: The number of times the repeat block's body is supposed to execute.
>     body: The body of the repeat block as a DetectorErrorModel containing the instructions to repeat.
> 
> Examples:
>     >>> import stim
>     >>> repeat_block = stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
>     ...     error(0.125) D0 D1
>     ...     shift_detectors 1
>     ... '''))
> ```

### `stim.DemRepeatBlock.__ne__(self, arg0: stim.DemRepeatBlock) -> bool`<a name="stim.DemRepeatBlock.__ne__"></a>
> ```
> Determines if two repeat blocks are different.
> ```

### `stim.DemRepeatBlock.__repr__(self) -> str`<a name="stim.DemRepeatBlock.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.DemRepeatBlock`.
> ```

### `stim.DemRepeatBlock.body_copy(self) -> stim.DetectorErrorModel`<a name="stim.DemRepeatBlock.body_copy"></a>
> ```
> Returns a copy of the block's body, as a stim.DetectorErrorModel.
> ```

### `stim.DemRepeatBlock.repeat_count`<a name="stim.DemRepeatBlock.repeat_count"></a>
> ```
> The number of times the repeat block's body is supposed to execute.
> ```

### `stim.DemTarget.__eq__(self, arg0: stim.DemTarget) -> bool`<a name="stim.DemTarget.__eq__"></a>
> ```
> Determines if two `stim.DemTarget`s are identical.
> ```

### `stim.DemTarget.__ne__(self, arg0: stim.DemTarget) -> bool`<a name="stim.DemTarget.__ne__"></a>
> ```
> Determines if two `stim.DemTarget`s are different.
> ```

### `stim.DemTarget.__repr__(self) -> str`<a name="stim.DemTarget.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.DemTarget`.
> ```

### `stim.DemTarget.__str__(self) -> str`<a name="stim.DemTarget.__str__"></a>
> ```
> Returns a text description of the detector error model target.
> ```

### `stim.DemTarget.is_logical_observable_id(self) -> bool`<a name="stim.DemTarget.is_logical_observable_id"></a>
> ```
> Determines if the detector error model target is a logical observable id target (like "L5" in a .dem file).
> ```

### `stim.DemTarget.is_relative_detector_id(self) -> bool`<a name="stim.DemTarget.is_relative_detector_id"></a>
> ```
> Determines if the detector error model target is a relative detector id target (like "D4" in a .dem file).
> ```

### `stim.DemTarget.is_separator(self) -> bool`<a name="stim.DemTarget.is_separator"></a>
> ```
> Determines if the detector error model target is a separator (like "^" in a .dem file).
> ```

### `stim.DetectorErrorModel.__eq__(self, arg0: stim.DetectorErrorModel) -> bool`<a name="stim.DetectorErrorModel.__eq__"></a>
> ```
> Determines if two detector error models have identical contents.
> ```

### `stim.DetectorErrorModel.__getitem__(self, arg0: int) -> object`<a name="stim.DetectorErrorModel.__getitem__"></a>
> ```
> Returns copies of instructions from the detector error model.
> 
> Examples:
>     >>> import stim
> ```

### `stim.DetectorErrorModel.__init__(self, detector_error_model_text: str = '') -> None`<a name="stim.DetectorErrorModel.__init__"></a>
> ```
> Creates a stim.DetectorErrorModel.
> 
> Args:
>     detector_error_model_text: Defaults to empty. Describes instructions to append into the circuit in the
>         detector error model (.dem) format.
> 
> Examples:
>     >>> import stim
>     >>> empty = stim.DetectorErrorModel()
>     >>> not_empty = stim.DetectorErrorModel('''
>     ...    error(0.125) D0 L0
>     ... ''')
> ```

### `stim.DetectorErrorModel.__len__(self) -> int`<a name="stim.DetectorErrorModel.__len__"></a>
> ```
> Returns the number of top-level instructions and blocks in the detector error model.
> 
> Instructions inside of blocks are not included in this count.
> 
> Examples:
>     >>> import stim
>     >>> len(stim.DetectorErrorModel())
>     0
>     >>> len(stim.DetectorErrorModel('''
>     ...    error(0.1) D0 D1
>     ...    shift_detectors 100
>     ...    logical_observable L5
>     ... '''))
>     3
>     >>> len(stim.DetectorErrorModel('''
>     ...    REPEAT 100 {
>     ...        error(0.1) D0 D1
>     ...        error(0.1) D1 D2
>     ...    }
>     ... '''))
>     1
> ```

### `stim.DetectorErrorModel.__ne__(self, arg0: stim.DetectorErrorModel) -> bool`<a name="stim.DetectorErrorModel.__ne__"></a>
> ```
> Determines if two detector error models have non-identical contents.
> ```

### `stim.DetectorErrorModel.__repr__(self) -> str`<a name="stim.DetectorErrorModel.__repr__"></a>
> ```
> "Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`."
> ```

### `stim.DetectorErrorModel.__str__(self) -> str`<a name="stim.DetectorErrorModel.__str__"></a>
> ```
> "Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.");
> ```

### `stim.DetectorErrorModel.clear(self) -> None`<a name="stim.DetectorErrorModel.clear"></a>
> ```
> Clears the contents of the detector error model.
> 
> Examples:
>     >>> import stim
>     >>> model = stim.DetectorErrorModel('''
>     ...    error(0.1) D0 D1
>     ... ''')
>     >>> model.clear()
>     >>> model
>     stim.DetectorErrorModel()
> ```

### `stim.DetectorErrorModel.copy(self) -> stim.DetectorErrorModel`<a name="stim.DetectorErrorModel.copy"></a>
> ```
> Returns a copy of the detector error model. An independent model with the same contents.
> 
> Examples:
>     >>> import stim
> 
>     >>> c1 = stim.DetectorErrorModel("error(0.1) D0 D1")
>     >>> c2 = c1.copy()
>     >>> c2 is c1
>     False
>     >>> c2 == c1
>     True
> ```

### `stim.DetectorErrorModel.num_detectors`<a name="stim.DetectorErrorModel.num_detectors"></a>
> ```
> Counts the number of detectors (e.g. `D2`) in the error model.
> 
> Detector indices are assumed to be contiguous from 0 up to whatever the maximum detector id is.
> If the largest detector's absolute id is n-1, then the number of detectors is n.
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.Circuit('''
>     ...     X_ERROR(0.125) 0
>     ...     X_ERROR(0.25) 1
>     ...     CORRELATED_ERROR(0.375) X0 X1
>     ...     M 0 1
>     ...     DETECTOR rec[-2]
>     ...     DETECTOR rec[-1]
>     ... ''').detector_error_model().num_detectors
>     2
> 
>     >>> stim.DetectorErrorModel('''
>     ...    error(0.1) D0 D199
>     ... ''').num_detectors
>     200
> 
>     >>> stim.DetectorErrorModel('''
>     ...    shift_detectors 1000
>     ...    error(0.1) D0 D199
>     ... ''').num_detectors
>     1200
> ```

### `stim.DetectorErrorModel.num_observables`<a name="stim.DetectorErrorModel.num_observables"></a>
> ```
> Counts the number of frame changes (e.g. `L2`) in the error model.
> 
> Observable indices are assumed to be contiguous from 0 up to whatever the maximum observable id is.
> If the largest observable's id is n-1, then the number of observables is n.
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.Circuit('''
>     ...     X_ERROR(0.125) 0
>     ...     M 0
>     ...     OBSERVABLE_INCLUDE(99) rec[-1]
>     ... ''').detector_error_model().num_observables
>     100
> 
>     >>> stim.DetectorErrorModel('''
>     ...    error(0.1) L399
>     ... ''').num_observables
>     400
> ```

### `stim.GateTarget.__eq__(self, arg0: stim.GateTarget) -> bool`<a name="stim.GateTarget.__eq__"></a>
> ```
> Determines if two `stim.GateTarget`s are identical.
> ```

### `stim.GateTarget.__init__(self, value: object) -> None`<a name="stim.GateTarget.__init__"></a>
> ```
> Initializes a `stim.GateTarget`.
> 
> Args:
>     value: A target like `5` or `stim.target_rec(-1)`.
> ```

### `stim.GateTarget.__ne__(self, arg0: stim.GateTarget) -> bool`<a name="stim.GateTarget.__ne__"></a>
> ```
> Determines if two `stim.GateTarget`s are different.
> ```

### `stim.GateTarget.__repr__(self) -> str`<a name="stim.GateTarget.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.GateTarget`.
> ```

### `stim.GateTarget.is_inverted_result_target`<a name="stim.GateTarget.is_inverted_result_target"></a>
> ```
> Returns whether or not this is a `stim.target_inv` target (e.g. `!5` in a circuit file).
> ```

### `stim.GateTarget.is_measurement_record_target`<a name="stim.GateTarget.is_measurement_record_target"></a>
> ```
> Returns whether or not this is a `stim.target_rec` target (e.g. `rec[-5]` in a circuit file).
> ```

### `stim.GateTarget.is_x_target`<a name="stim.GateTarget.is_x_target"></a>
> ```
> Returns whether or not this is a `stim.target_x` target (e.g. `X5` in a circuit file).
> ```

### `stim.GateTarget.is_y_target`<a name="stim.GateTarget.is_y_target"></a>
> ```
> Returns whether or not this is a `stim.target_y` target (e.g. `Y5` in a circuit file).
> ```

### `stim.GateTarget.is_z_target`<a name="stim.GateTarget.is_z_target"></a>
> ```
> Returns whether or not this is a `stim.target_z` target (e.g. `Z5` in a circuit file).
> ```

### `stim.GateTarget.value`<a name="stim.GateTarget.value"></a>
> ```
> The numeric part of the target. Positive for qubit targets, negative for measurement record targets.
> ```

### `stim.PauliString.__add__(self, rhs: stim.PauliString) -> stim.PauliString`<a name="stim.PauliString.__add__"></a>
> ```
> Returns the tensor product of two Pauli strings.
> 
> Concatenates the Pauli strings and multiplies their signs.
> 
> Args:
>     rhs: A second stim.PauliString.
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.PauliString("X") + stim.PauliString("YZ")
>     stim.PauliString("+XYZ")
> 
>     >>> stim.PauliString("iX") + stim.PauliString("-X")
>     stim.PauliString("-iXX")
> 
> Returns:
>     The tensor product.
> ```

### `stim.PauliString.__eq__(self, arg0: stim.PauliString) -> bool`<a name="stim.PauliString.__eq__"></a>
> ```
> Determines if two Pauli strings have identical contents.
> ```

### `stim.PauliString.__getitem__(*args, **kwargs)`<a name="stim.PauliString.__getitem__"></a>
> ```
> Overloaded function.
> 
> 1. __getitem__(self: stim.PauliString, index: int) -> int
> 
> Returns an individual Pauli or Pauli string slice from the pauli string.
> 
> Individual Paulis are returned as an int using the encoding 0=I, 1=X, 2=Y, 3=Z.
> Slices are returned as a stim.PauliString (always with positive sign).
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString("_XYZ")
>     >>> p[2]
>     2
>     >>> p[-1]
>     3
>     >>> p[:2]
>     stim.PauliString("+_X")
>     >>> p[::-1]
>     stim.PauliString("+ZYX_")
> 
> Args:
>     index: The index of the pauli to return or slice of paulis to return.
> 
> Returns:
>     0: Identity.
>     1: Pauli X.
>     2: Pauli Y.
>     3: Pauli Z.
> 
> 
> 2. __getitem__(self: stim.PauliString, slice: slice) -> stim.PauliString
> 
> Returns an individual Pauli or Pauli string slice from the pauli string.
> 
> Individual Paulis are returned as an int using the encoding 0=I, 1=X, 2=Y, 3=Z.
> Slices are returned as a stim.PauliString (always with positive sign).
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString("_XYZ")
>     >>> p[2]
>     2
>     >>> p[-1]
>     3
>     >>> p[:2]
>     stim.PauliString("+_X")
>     >>> p[::-1]
>     stim.PauliString("+ZYX_")
> 
> Args:
>     index: The index of the pauli to return or slice of paulis to return.
> 
> Returns:
>     0: Identity.
>     1: Pauli X.
>     2: Pauli Y.
>     3: Pauli Z.
> ```

### `stim.PauliString.__iadd__(self, rhs: stim.PauliString) -> stim.PauliString`<a name="stim.PauliString.__iadd__"></a>
> ```
> Performs an inplace tensor product.
> 
> Concatenates the given Pauli string onto the receiving string and multiplies their signs.
> 
> Args:
>     rhs: A second stim.PauliString.
> 
> Examples:
>     >>> import stim
> 
>     >>> p = stim.PauliString("iX")
>     >>> alias = p
>     >>> p += stim.PauliString("-YY")
>     >>> p
>     stim.PauliString("-iXYY")
>     >>> alias is p
>     True
> 
> Returns:
>     The mutated pauli string.
> ```

### `stim.PauliString.__imul__(self, rhs: object) -> stim.PauliString`<a name="stim.PauliString.__imul__"></a>
> ```
> Inplace right-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.
> 
> Args:
>     rhs: The right hand side of the multiplication. This can be:
>         - A stim.PauliString to right-multiply term-by-term into the paulis of the pauli string.
>         - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli string.
>         - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).
> 
> Examples:
>     >>> import stim
> 
>     >>> p = stim.PauliString("X")
>     >>> p *= 1j
>     >>> p
>     stim.PauliString("+iX")
> 
>     >>> p = stim.PauliString("iXY_")
>     >>> p *= 3
>     >>> p
>     stim.PauliString("-iXY_XY_XY_")
> 
>     >>> p = stim.PauliString("X")
>     >>> alias = p
>     >>> p *= stim.PauliString("Y")
>     >>> alias
>     stim.PauliString("+iZ")
> 
>     >>> p = stim.PauliString("X")
>     >>> p *= stim.PauliString("_YY")
>     >>> p
>     stim.PauliString("+XYY")
> 
> Returns:
>     The mutated Pauli string.
> 
> Raises:
>     ValueError: The Pauli strings have different lengths.
> ```

### `stim.PauliString.__init__(*args, **kwargs)`<a name="stim.PauliString.__init__"></a>
> ```
> Overloaded function.
> 
> 1. __init__(self: stim.PauliString, num_qubits: int) -> None
> 
> Creates an identity Pauli string over the given number of qubits.
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString(5)
>     >>> print(p)
>     +_____
> 
> Args:
>     num_qubits: The number of qubits the Pauli string acts on.
> 
> 
> 2. __init__(self: stim.PauliString, text: str) -> None
> 
> Creates a stim.PauliString from a text string.
> 
> The string can optionally start with a sign ('+', '-', 'i', '+i', or '-i').
> The rest of the string should be characters from '_IXYZ' where
> '_' and 'I' mean identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means Pauli Z.
> 
> Examples:
>     >>> import stim
>     >>> print(stim.PauliString("YZ"))
>     +YZ
>     >>> print(stim.PauliString("+IXYZ"))
>     +_XYZ
>     >>> print(stim.PauliString("-___X_"))
>     -___X_
>     >>> print(stim.PauliString("iX"))
>     +iX
> 
> Args:
>     text: A text description of the Pauli string's contents, such as "+XXX" or "-_YX".
> 
> 
> 3. __init__(self: stim.PauliString, copy: stim.PauliString) -> None
> 
> Creates a copy of a stim.PauliString.
> 
> Examples:
>     >>> import stim
>     >>> a = stim.PauliString("YZ")
>     >>> b = stim.PauliString(a)
>     >>> b is a
>     False
>     >>> b == a
>     True
> 
> Args:
>     copy: The pauli string to make a copy of.
> 
> 
> 4. __init__(self: stim.PauliString, pauli_indices: List[int]) -> None
> 
> Creates a stim.PauliString from a list of integer pauli indices.
> 
> The indexing scheme that is used is:
>     0 -> I
>     1 -> X
>     2 -> Y
>     3 -> Z
> 
> Examples:
>     >>> import stim
>     >>> stim.PauliString([0, 1, 2, 3, 0, 3])
>     stim.PauliString("+_XYZ_Z")
> 
> Args:
>     pauli_indices: A sequence of integers from 0 to 3 (inclusive) indicating paulis.
> ```

### `stim.PauliString.__len__(self) -> int`<a name="stim.PauliString.__len__"></a>
> ```
> Returns the length the pauli string; the number of qubits it operates on.
> ```

### `stim.PauliString.__mul__(self, rhs: object) -> stim.PauliString`<a name="stim.PauliString.__mul__"></a>
> ```
> Right-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.
> 
> Args:
>     rhs: The right hand side of the multiplication. This can be:
>         - A stim.PauliString to right-multiply term-by-term with the paulis of the pauli string.
>         - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli string.
>         - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.PauliString("X") * 1
>     stim.PauliString("+X")
>     >>> stim.PauliString("X") * -1
>     stim.PauliString("-X")
>     >>> stim.PauliString("X") * 1j
>     stim.PauliString("+iX")
> 
>     >>> stim.PauliString("X") * 2
>     stim.PauliString("+XX")
>     >>> stim.PauliString("-X") * 2
>     stim.PauliString("+XX")
>     >>> stim.PauliString("iX") * 2
>     stim.PauliString("-XX")
>     >>> stim.PauliString("X") * 3
>     stim.PauliString("+XXX")
>     >>> stim.PauliString("iX") * 3
>     stim.PauliString("-iXXX")
> 
>     >>> stim.PauliString("X") * stim.PauliString("Y")
>     stim.PauliString("+iZ")
>     >>> stim.PauliString("X") * stim.PauliString("XX_")
>     stim.PauliString("+_X_")
>     >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
>     stim.PauliString("+X_ZY")
> 
> Returns:
>     The product or tensor power.
> 
> Raises:
>     TypeError: The right hand side isn't a stim.PauliString, a non-negative integer, or a complex unit (1, -1, 1j, or -1j).
> ```

### `stim.PauliString.__ne__(self, arg0: stim.PauliString) -> bool`<a name="stim.PauliString.__ne__"></a>
> ```
> Determines if two Pauli strings have non-identical contents.
> ```

### `stim.PauliString.__neg__(self) -> stim.PauliString`<a name="stim.PauliString.__neg__"></a>
> ```
> Returns the negation of the pauli string.
> 
> Examples:
>     >>> import stim
>     >>> -stim.PauliString("X")
>     stim.PauliString("-X")
>     >>> -stim.PauliString("-Y")
>     stim.PauliString("+Y")
>     >>> -stim.PauliString("iZZZ")
>     stim.PauliString("-iZZZ")
> ```

### `stim.PauliString.__pos__(self) -> stim.PauliString`<a name="stim.PauliString.__pos__"></a>
> ```
> Returns a pauli string with the same contents.
> 
> Examples:
>     >>> import stim
>     >>> +stim.PauliString("+X")
>     stim.PauliString("+X")
>     >>> +stim.PauliString("-YY")
>     stim.PauliString("-YY")
>     >>> +stim.PauliString("iZZZ")
>     stim.PauliString("+iZZZ")
> ```

### `stim.PauliString.__repr__(self) -> str`<a name="stim.PauliString.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.PauliString`.
> ```

### `stim.PauliString.__rmul__(self, lhs: object) -> stim.PauliString`<a name="stim.PauliString.__rmul__"></a>
> ```
> Left-multiplies the Pauli string by another Pauli string, a complex unit, or a tensor power.
> 
> Args:
>     rhs: The left hand side of the multiplication. This can be:
>         - A stim.PauliString to left-multiply term-by-term into the paulis of the pauli string.
>         - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli string.
>         - A non-negative integer indicating the tensor power to raise the pauli string to (how many times to repeat it).
> 
> Examples:
>     >>> import stim
> 
>     >>> 1 * stim.PauliString("X")
>     stim.PauliString("+X")
>     >>> -1 * stim.PauliString("X")
>     stim.PauliString("-X")
>     >>> 1j * stim.PauliString("X")
>     stim.PauliString("+iX")
> 
>     >>> 2 * stim.PauliString("X")
>     stim.PauliString("+XX")
>     >>> 2 * stim.PauliString("-X")
>     stim.PauliString("+XX")
>     >>> 2 * stim.PauliString("iX")
>     stim.PauliString("-XX")
>     >>> 3 * stim.PauliString("X")
>     stim.PauliString("+XXX")
>     >>> 3 * stim.PauliString("iX")
>     stim.PauliString("-iXXX")
> 
>     >>> stim.PauliString("X") * stim.PauliString("Y")
>     stim.PauliString("+iZ")
>     >>> stim.PauliString("X") * stim.PauliString("XX_")
>     stim.PauliString("+_X_")
>     >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
>     stim.PauliString("+X_ZY")
> 
> Returns:
>     The product.
> 
> Raises:
>     ValueError: The scalar phase factor isn't 1, -1, 1j, or -1j.
> ```

### `stim.PauliString.__setitem__(*args, **kwargs)`<a name="stim.PauliString.__setitem__"></a>
> ```
> Overloaded function.
> 
> 1. __setitem__(self: stim.PauliString, index: int, new_pauli: str) -> None
> 
> Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString(4)
>     >>> p[2] = 1
>     >>> print(p)
>     +__X_
>     >>> p[0] = 3
>     >>> p[1] = 2
>     >>> p[3] = 0
>     >>> print(p)
>     +ZYX_
>     >>> p[0] = 'I'
>     >>> p[1] = 'X'
>     >>> p[2] = 'Y'
>     >>> p[3] = 'Z'
>     >>> print(p)
>     +_XYZ
>     >>> p[-1] = 'Y'
>     >>> print(p)
>     +_XYY
> 
> Args:
>     index: The index of the pauli to return.
> 
> 
> 2. __setitem__(self: stim.PauliString, index: int, new_pauli: int) -> None
> 
> Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString(4)
>     >>> p[2] = 1
>     >>> print(p)
>     +__X_
>     >>> p[0] = 3
>     >>> p[1] = 2
>     >>> p[3] = 0
>     >>> print(p)
>     +ZYX_
>     >>> p[0] = 'I'
>     >>> p[1] = 'X'
>     >>> p[2] = 'Y'
>     >>> p[3] = 'Z'
>     >>> print(p)
>     +_XYZ
>     >>> p[-1] = 'Y'
>     >>> print(p)
>     +_XYY
> 
> Args:
>     index: The index of the pauli to return.
> ```

### `stim.PauliString.__str__(self) -> str`<a name="stim.PauliString.__str__"></a>
> ```
> Returns a text description.
> ```

### `stim.PauliString.__truediv__(*args, **kwargs)`<a name="stim.PauliString.__truediv__"></a>
> ```
> Overloaded function.
> 
> 1. __truediv__(self: stim.PauliString, rhs: complex) -> stim.PauliString
> 
> Inplace divides the Pauli string by a complex unit.
> 
> Args:
>     rhs: The divisor. Can be 1, -1, 1j, or -1j.
> 
> Examples:
>     >>> import stim
> 
>     >>> p = stim.PauliString("X")
>     >>> p /= 1j
>     >>> p
>     stim.PauliString("-iX")
> 
> Returns:
>     The mutated Pauli string.
> 
> Raises:
>     ValueError: The divisor isn't 1, -1, 1j, or -1j.
> 
> 
> 2. __truediv__(self: stim.PauliString, rhs: complex) -> stim.PauliString
> 
> Divides the Pauli string by a complex unit.
> 
> Args:
>     rhs: The divisor. Can be 1, -1, 1j, or -1j.
> 
> Examples:
>     >>> import stim
> 
>     >>> stim.PauliString("X") / 1j
>     stim.PauliString("-iX")
> 
> Returns:
>     The quotient.
> 
> Raises:
>     ValueError: The divisor isn't 1, -1, 1j, or -1j.
> ```

### `stim.PauliString.commutes(self, other: stim.PauliString) -> bool`<a name="stim.PauliString.commutes"></a>
> ```
> Determines if two Pauli strings commute or not.
> 
> Two Pauli strings commute if they have an even number of matched
> non-equal non-identity Pauli terms. Otherwise they anticommute.
> 
> Args:
>     other: The other Pauli string.
> 
> Examples:
>     >>> import stim
>     >>> xx = stim.PauliString("XX")
>     >>> xx.commutes(stim.PauliString("X_"))
>     True
>     >>> xx.commutes(stim.PauliString("XX"))
>     True
>     >>> xx.commutes(stim.PauliString("XY"))
>     False
>     >>> xx.commutes(stim.PauliString("XZ"))
>     False
>     >>> xx.commutes(stim.PauliString("ZZ"))
>     True
>     >>> xx.commutes(stim.PauliString("X_Y__"))
>     True
>     >>> xx.commutes(stim.PauliString(""))
>     True
> 
> Returns:
>     True if the Pauli strings commute, False if they anti-commute.
> ```

### `stim.PauliString.copy(self) -> stim.PauliString`<a name="stim.PauliString.copy"></a>
> ```
> Returns a copy of the pauli string. An independent pauli string with the same contents.
> 
> Examples:
>     >>> import stim
>     >>> p1 = stim.PauliString.random(2)
>     >>> p2 = p1.copy()
>     >>> p2 is p1
>     False
>     >>> p2 == p1
>     True
> ```

### `stim.PauliString.extended_product(self, other: stim.PauliString) -> Tuple[complex, stim.PauliString]`<a name="stim.PauliString.extended_product"></a>
> ```
> [DEPRECATED] Use multiplication (__mul__ or *) instead.
> ```

### `stim.PauliString.random(num_qubits: int, *, allow_imaginary: bool = False) -> stim.PauliString`<a name="stim.PauliString.random"></a>
> ```
> Samples a uniformly random Hermitian Pauli string over the given number of qubits.
> 
> Args:
>     num_qubits: The number of qubits the Pauli string should act on.
>     allow_imaginary: Defaults to False. If True, the sign of the result may be 1j or -1j
>         in addition to +1 or -1. In other words, setting this to True allows the result
>         to be non-Hermitian.
> 
> Examples:
>     >>> import stim
>     >>> p = stim.PauliString.random(5)
>     >>> len(p)
>     5
>     >>> p.sign in [-1, +1]
>     True
> 
>     >>> p2 = stim.PauliString.random(3, allow_imaginary=True)
>     >>> len(p2)
>     3
>     >>> p2.sign in [-1, +1, 1j, -1j]
>     True
> 
> Returns:
>     The sampled Pauli string.
> ```

### `stim.PauliString.sign`<a name="stim.PauliString.sign"></a>
> ```
> The sign of the Pauli string. Can be +1, -1, 1j, or -1j.
> 
> Examples:
>     >>> import stim
>     >>> stim.PauliString("X").sign
>     (1+0j)
>     >>> stim.PauliString("-X").sign
>     (-1+0j)
>     >>> stim.PauliString("iX").sign
>     1j
>     >>> stim.PauliString("-iX").sign
>     (-0-1j)
> ```

### `stim.Tableau.__add__(self, rhs: stim.Tableau) -> stim.Tableau`<a name="stim.Tableau.__add__"></a>
> ```
> Returns the direct sum (diagonal concatenation) of two Tableaus.
> 
> Args:
>     rhs: A second stim.Tableau.
> 
> Examples:
>     >>> import stim
> 
>     >>> s = stim.Tableau.from_named_gate("S")
>     >>> cz = stim.Tableau.from_named_gate("CZ")
>     >>> print(s + cz)
>     +-xz-xz-xz-
>     | ++ ++ ++
>     | YZ __ __
>     | __ XZ Z_
>     | __ Z_ XZ
> 
> Returns:
>     The direct sum.
> ```

### `stim.Tableau.__call__(self, pauli_string: stim.PauliString) -> stim.PauliString`<a name="stim.Tableau.__call__"></a>
> ```
> Returns the result of conjugating the given PauliString by the Tableau's Clifford operation.
> 
> Args:
>     pauli_string: The pauli string to conjugate.
> 
> Returns:
>     The new conjugated pauli string.
> 
> Examples:
>     >>> import stim
>     >>> t = stim.Tableau.from_named_gate("CNOT")
>     >>> p = stim.PauliString("XX")
>     >>> result = t(p)
>     >>> print(result)
>     +X_
> ```

### `stim.Tableau.__eq__(self, arg0: stim.Tableau) -> bool`<a name="stim.Tableau.__eq__"></a>
> ```
> Determines if two tableaus have identical contents.
> ```

### `stim.Tableau.__iadd__(self, rhs: stim.Tableau) -> stim.Tableau`<a name="stim.Tableau.__iadd__"></a>
> ```
> Performs an inplace direct sum (diagonal concatenation).
> 
> Args:
>     rhs: A second stim.Tableau.
> 
> Examples:
>     >>> import stim
> 
>     >>> s = stim.Tableau.from_named_gate("S")
>     >>> cz = stim.Tableau.from_named_gate("CZ")
>     >>> alias = s
>     >>> s += cz
>     >>> alias is s
>     True
>     >>> print(s)
>     +-xz-xz-xz-
>     | ++ ++ ++
>     | YZ __ __
>     | __ XZ Z_
>     | __ Z_ XZ
> 
> Returns:
>     The mutated tableau.
> ```

### `stim.Tableau.__init__(self, num_qubits: int) -> None`<a name="stim.Tableau.__init__"></a>
> ```
> Creates an identity tableau over the given number of qubits.
> 
> Examples:
>     >>> import stim
>     >>> t = stim.Tableau(3)
>     >>> print(t)
>     +-xz-xz-xz-
>     | ++ ++ ++
>     | XZ __ __
>     | __ XZ __
>     | __ __ XZ
> 
> Args:
>     num_qubits: The number of qubits the tableau's operation acts on.
> ```

### `stim.Tableau.__len__(self) -> int`<a name="stim.Tableau.__len__"></a>
> ```
> Returns the number of qubits operated on by the tableau.
> ```

### `stim.Tableau.__mul__(self, rhs: stim.Tableau) -> stim.Tableau`<a name="stim.Tableau.__mul__"></a>
> ```
> Returns the product of two tableaus.
> 
> If the tableau T1 represents the Clifford operation with unitary C1,
> and the tableau T2 represents the Clifford operation with unitary C2,
> then the tableau T1*T2 represents the Clifford operation with unitary C1*C2.
> 
> Args:
>     rhs: The tableau  on the right hand side of the multiplication.
> 
> Examples:
>     >>> import stim
>     >>> t1 = stim.Tableau.random(4)
>     >>> t2 = stim.Tableau.random(4)
>     >>> t3 = t2 * t1
>     >>> p = stim.PauliString.random(4)
>     >>> t3(p) == t2(t1(p))
>     True
> ```

### `stim.Tableau.__ne__(self, arg0: stim.Tableau) -> bool`<a name="stim.Tableau.__ne__"></a>
> ```
> Determines if two tableaus have non-identical contents.
> ```

### `stim.Tableau.__pow__(self, exponent: int) -> stim.Tableau`<a name="stim.Tableau.__pow__"></a>
> ```
> Raises the tableau to an integer power.
> 
> Large powers are reached efficiently using repeated squaring.
> Negative powers are reached by inverting the tableau.
> 
> Args:
>     exponent: The power to raise to. Can be negative, zero, or positive.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.Tableau.from_named_gate("S")
>     >>> s**0 == stim.Tableau(1)
>     True
>     >>> s**1 == s
>     True
>     >>> s**2 == stim.Tableau.from_named_gate("Z")
>     True
>     >>> s**-1 == s**3 == stim.Tableau.from_named_gate("S_DAG")
>     True
>     >>> s**5 == s
>     True
>     >>> s**(400000000 + 1) == s
>     True
>     >>> s**(-400000000 + 1) == s
>     True
> ```

### `stim.Tableau.__repr__(self) -> str`<a name="stim.Tableau.__repr__"></a>
> ```
> Returns text that is a valid python expression evaluating to an equivalent `stim.Tableau`.
> ```

### `stim.Tableau.__str__(self) -> str`<a name="stim.Tableau.__str__"></a>
> ```
> Returns a text description.
> ```

### `stim.Tableau.append(self, gate: stim.Tableau, targets: List[int]) -> None`<a name="stim.Tableau.append"></a>
> ```
> Appends an operation's effect into this tableau, mutating this tableau.
> 
> Time cost is O(n*m*m) where n=len(self) and m=len(gate).
> 
> Args:
>     gate: The tableau of the operation being appended into this tableau.
>     targets: The qubits being targeted by the gate.
> 
> Examples:
>     >>> import stim
>     >>> cnot = stim.Tableau.from_named_gate("CNOT")
>     >>> t = stim.Tableau(2)
>     >>> t.append(cnot, [0, 1])
>     >>> t.append(cnot, [1, 0])
>     >>> t.append(cnot, [0, 1])
>     >>> t == stim.Tableau.from_named_gate("SWAP")
>     True
> ```

### `stim.Tableau.copy(self) -> stim.Tableau`<a name="stim.Tableau.copy"></a>
> ```
> Returns a copy of the tableau. An independent tableau with the same contents.
> 
> Examples:
>     >>> import stim
>     >>> t1 = stim.Tableau.random(2)
>     >>> t2 = t1.copy()
>     >>> t2 is t1
>     False
>     >>> t2 == t1
>     True
> ```

### `stim.Tableau.from_conjugated_generators(*, xs: List[stim.PauliString], zs: List[stim.PauliString]) -> stim.Tableau`<a name="stim.Tableau.from_conjugated_generators"></a>
> ```
> Creates a tableau from the given outputs for each generator.
> 
> Verifies that the tableau is well formed.
> 
> Args:
>     xs: A List[stim.PauliString] with the results of conjugating X0, X1, etc.
>     zs: A List[stim.PauliString] with the results of conjugating Z0, Z1, etc.
> 
> Returns:
>     The created tableau.
> 
> Raises:
>     ValueError: The given outputs are malformed. Their lengths are inconsistent,
>         or they don't satisfy the required commutation relationships.
> 
> Examples:
>     >>> import stim
>     >>> identity3 = stim.Tableau.from_conjugated_generators(
>     ...     xs=[
>     ...         stim.PauliString("X__"),
>     ...         stim.PauliString("_X_"),
>     ...         stim.PauliString("__X"),
>     ...     ],
>     ...     zs=[
>     ...         stim.PauliString("Z__"),
>     ...         stim.PauliString("_Z_"),
>     ...         stim.PauliString("__Z"),
>     ...     ],
>     ... )
>     >>> identity3 == stim.Tableau(3)
>     True
> ```

### `stim.Tableau.from_named_gate(name: str) -> stim.Tableau`<a name="stim.Tableau.from_named_gate"></a>
> ```
> Returns the tableau of a named Clifford gate.
> 
> Args:
>     name: The name of the Clifford gate.
> 
> Returns:
>     The gate's tableau.
> 
> Examples:
>     >>> import stim
>     >>> print(stim.Tableau.from_named_gate("H"))
>     +-xz-
>     | ++
>     | ZX
>     >>> print(stim.Tableau.from_named_gate("CNOT"))
>     +-xz-xz-
>     | ++ ++
>     | XZ _Z
>     | X_ XZ
>     >>> print(stim.Tableau.from_named_gate("S"))
>     +-xz-
>     | ++
>     | YZ
> ```

### `stim.Tableau.inverse(self, *, unsigned: bool = False) -> stim.Tableau`<a name="stim.Tableau.inverse"></a>
> ```
> Computes the inverse of the tableau.
> 
> The inverse T^-1 of a tableau T is the unique tableau with the property that T * T^-1 = T^-1 * T = I where
> I is the identity tableau.
> 
> Args:
>     unsigned: Defaults to false. When set to true, skips computing the signs of the output observables and
>         instead just set them all to be positive. This is beneficial because computing the signs takes
>         O(n^3) time and the rest of the inverse computation is O(n^2) where n is the number of qubits in the
>         tableau. So, if you only need the Pauli terms (not the signs), it is significantly cheaper.
> 
> Returns:
>     The inverse tableau.
> 
> Examples:
>     >>> import stim
> 
>     >>> # Check that the inverse agrees with hard-coded tableaus in the gate data.
>     >>> s = stim.Tableau.from_named_gate("S")
>     >>> s_dag = stim.Tableau.from_named_gate("S_DAG")
>     >>> s.inverse() == s_dag
>     True
>     >>> z = stim.Tableau.from_named_gate("Z")
>     >>> z.inverse() == z
>     True
> 
>     >>> # Check that multiplying by the inverse produces the identity.
>     >>> t = stim.Tableau.random(10)
>     >>> t_inv = t.inverse()
>     >>> identity = stim.Tableau(10)
>     >>> t * t_inv == t_inv * t == identity
>     True
> 
>     >>> # Check a manual case.
>     >>> t = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-__Z"), stim.PauliString("+XZ_"), stim.PauliString("+_ZZ")],
>     ...     zs=[stim.PauliString("-YYY"), stim.PauliString("+Z_Z"), stim.PauliString("-ZYZ")],
>     ... )
>     >>> print(t.inverse())
>     +-xz-xz-xz-
>     | -- +- --
>     | XX XX YX
>     | XZ Z_ X_
>     | X_ YX Y_
>     >>> print(t.inverse(unsigned=True))
>     +-xz-xz-xz-
>     | ++ ++ ++
>     | XX XX YX
>     | XZ Z_ X_
>     | X_ YX Y_
> ```

### `stim.Tableau.inverse_x_output(self, input_index: int, *, unsigned: bool = False) -> stim.PauliString`<a name="stim.Tableau.inverse_x_output"></a>
> ```
> Returns the result of conjugating an X Pauli generator by the inverse of the tableau.
> 
> A faster version of `tableau.inverse(unsigned).x_output(input_index)`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the X generator) to return from the inverse tableau.
>     unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
>         it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
>         parts of the computation take O(n) time where n is the number of qubits in the tableau.
> 
> Returns:
>     The result of conjugating an X generator by the inverse of the tableau.
> 
> Examples:
>     >>> import stim
> 
>     # Check equivalence with the inverse's x_output.
>     >>> t = stim.Tableau.random(4)
>     >>> expected = t.inverse().x_output(0)
>     >>> t.inverse_x_output(0) == expected
>     True
>     >>> expected.sign = +1;
>     >>> t.inverse_x_output(0, unsigned=True) == expected
>     True
> ```

### `stim.Tableau.inverse_x_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.inverse_x_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's inverse's output pauli string for an input X generator.
> 
> A constant-time equivalent for `tableau.inverse().x_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the input X generator) in the inverse tableau.
>     output_index: Identifies the row (the output qubit) in the inverse tableau.
> 
> Returns:
>     An integer identifying Pauli at the given location in the inverse tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t_inv = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... ).inverse()
>     >>> t_inv.inverse_x_output_pauli(0, 0)
>     2
>     >>> t_inv.inverse_x_output_pauli(0, 1)
>     0
>     >>> t_inv.inverse_x_output_pauli(1, 0)
>     2
>     >>> t_inv.inverse_x_output_pauli(1, 1)
>     3
> ```

### `stim.Tableau.inverse_y_output(self, input_index: int, *, unsigned: bool = False) -> stim.PauliString`<a name="stim.Tableau.inverse_y_output"></a>
> ```
> Returns the result of conjugating a Y Pauli generator by the inverse of the tableau.
> 
> A faster version of `tableau.inverse(unsigned).y_output(input_index)`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the Y generator) to return from the inverse tableau.
>     unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
>         it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
>         parts of the computation take O(n) time where n is the number of qubits in the tableau.
> 
> Returns:
>     The result of conjugating a Y generator by the inverse of the tableau.
> 
> Examples:
>     >>> import stim
> 
>     # Check equivalence with the inverse's y_output.
>     >>> t = stim.Tableau.random(4)
>     >>> expected = t.inverse().y_output(0)
>     >>> t.inverse_y_output(0) == expected
>     True
>     >>> expected.sign = +1;
>     >>> t.inverse_y_output(0, unsigned=True) == expected
>     True
> ```

### `stim.Tableau.inverse_y_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.inverse_y_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's inverse's output pauli string for an input Y generator.
> 
> A constant-time equivalent for `tableau.inverse().y_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the input Y generator) in the inverse tableau.
>     output_index: Identifies the row (the output qubit) in the inverse tableau.
> 
> Returns:
>     An integer identifying Pauli at the given location in the inverse tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t_inv = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... ).inverse()
>     >>> t_inv.inverse_y_output_pauli(0, 0)
>     1
>     >>> t_inv.inverse_y_output_pauli(0, 1)
>     2
>     >>> t_inv.inverse_y_output_pauli(1, 0)
>     0
>     >>> t_inv.inverse_y_output_pauli(1, 1)
>     2
> ```

### `stim.Tableau.inverse_z_output(self, input_index: int, *, unsigned: bool = False) -> stim.PauliString`<a name="stim.Tableau.inverse_z_output"></a>
> ```
> Returns the result of conjugating a Z Pauli generator by the inverse of the tableau.
> 
> A faster version of `tableau.inverse(unsigned).z_output(input_index)`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the Z generator) to return from the inverse tableau.
>     unsigned: Defaults to false. When set to true, skips computing the result's sign and instead just sets
>         it to positive. This is beneficial because computing the sign takes O(n^2) time whereas all other
>         parts of the computation take O(n) time where n is the number of qubits in the tableau.
> 
> Returns:
>     The result of conjugating a Z generator by the inverse of the tableau.
> 
> Examples:
>     >>> import stim
> 
>     >>> import stim
> 
>     # Check equivalence with the inverse's z_output.
>     >>> t = stim.Tableau.random(4)
>     >>> expected = t.inverse().z_output(0)
>     >>> t.inverse_z_output(0) == expected
>     True
>     >>> expected.sign = +1;
>     >>> t.inverse_z_output(0, unsigned=True) == expected
>     True
> ```

### `stim.Tableau.inverse_z_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.inverse_z_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's inverse's output pauli string for an input Z generator.
> 
> A constant-time equivalent for `tableau.inverse().z_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the column (the qubit of the input Z generator) in the inverse tableau.
>     output_index: Identifies the row (the output qubit) in the inverse tableau.
> 
> Returns:
>     An integer identifying Pauli at the given location in the inverse tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t_inv = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... ).inverse()
>     >>> t_inv.inverse_z_output_pauli(0, 0)
>     3
>     >>> t_inv.inverse_z_output_pauli(0, 1)
>     2
>     >>> t_inv.inverse_z_output_pauli(1, 0)
>     2
>     >>> t_inv.inverse_z_output_pauli(1, 1)
>     1
> ```

### `stim.Tableau.prepend(self, gate: stim.Tableau, targets: List[int]) -> None`<a name="stim.Tableau.prepend"></a>
> ```
> Prepends an operation's effect into this tableau, mutating this tableau.
> 
> Time cost is O(n*m*m) where n=len(self) and m=len(gate).
> 
> Args:
>     gate: The tableau of the operation being prepended into this tableau.
>     targets: The qubits being targeted by the gate.
> 
> Examples:
>     >>> import stim
>     >>> h = stim.Tableau.from_named_gate("H")
>     >>> cnot = stim.Tableau.from_named_gate("CNOT")
>     >>> t = stim.Tableau.from_named_gate("H")
>     >>> t.prepend(stim.Tableau.from_named_gate("X"), [0])
>     >>> t == stim.Tableau.from_named_gate("SQRT_Y_DAG")
>     True
> ```

### `stim.Tableau.random(num_qubits: int) -> stim.Tableau`<a name="stim.Tableau.random"></a>
> ```
> Samples a uniformly random Clifford operation over the given number of qubits and returns its tableau.
> 
> Args:
>     num_qubits: The number of qubits the tableau should act on.
> 
> Returns:
>     The sampled tableau.
> 
> Examples:
>     >>> import stim
>     >>> t = stim.Tableau.random(42)
> 
> References:
>     "Hadamard-free circuits expose the structure of the Clifford group"
>     Sergey Bravyi, Dmitri Maslov
>     https://arxiv.org/abs/2003.09412
> ```

### `stim.Tableau.then(self, second: stim.Tableau) -> stim.Tableau`<a name="stim.Tableau.then"></a>
> ```
> Returns the result of composing two tableaus.
> 
> If the tableau T1 represents the Clifford operation with unitary C1,
> and the tableau T2 represents the Clifford operation with unitary C2,
> then the tableau T1.then(T2) represents the Clifford operation with unitary C2*C1.
> 
> Args:
>     second: The result is equivalent to applying the second tableau after
>         the receiving tableau.
> 
> Examples:
>     >>> import stim
>     >>> t1 = stim.Tableau.random(4)
>     >>> t2 = stim.Tableau.random(4)
>     >>> t3 = t1.then(t2)
>     >>> p = stim.PauliString.random(4)
>     >>> t3(p) == t2(t1(p))
>     True
> ```

### `stim.Tableau.x_output(self, target: int) -> stim.PauliString`<a name="stim.Tableau.x_output"></a>
> ```
> Returns the result of conjugating a Pauli X by the tableau's Clifford operation.
> 
> Args:
>     target: The qubit targeted by the Pauli X operation.
> 
> Examples:
>     >>> import stim
>     >>> h = stim.Tableau.from_named_gate("H")
>     >>> h.x_output(0)
>     stim.PauliString("+Z")
> 
>     >>> cnot = stim.Tableau.from_named_gate("CNOT")
>     >>> cnot.x_output(0)
>     stim.PauliString("+XX")
>     >>> cnot.x_output(1)
>     stim.PauliString("+_X")
> ```

### `stim.Tableau.x_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.x_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's output pauli string for an input X generator.
> 
> A constant-time equivalent for `tableau.x_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the tableau column (the qubit of the input X generator).
>     output_index: Identifies the tableau row (the output qubit).
> 
> Returns:
>     An integer identifying Pauli at the given location in the tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... )
>     >>> t.x_output_pauli(0, 0)
>     2
>     >>> t.x_output_pauli(0, 1)
>     0
>     >>> t.x_output_pauli(1, 0)
>     2
>     >>> t.x_output_pauli(1, 1)
>     3
> ```

### `stim.Tableau.y_output(self, target: int) -> stim.PauliString`<a name="stim.Tableau.y_output"></a>
> ```
> Returns the result of conjugating a Pauli Y by the tableau's Clifford operation.
> 
> Args:
>     target: The qubit targeted by the Pauli Y operation.
> 
> Examples:
>     >>> import stim
>     >>> h = stim.Tableau.from_named_gate("H")
>     >>> h.y_output(0)
>     stim.PauliString("-Y")
> 
>     >>> cnot = stim.Tableau.from_named_gate("CNOT")
>     >>> cnot.y_output(0)
>     stim.PauliString("+YX")
>     >>> cnot.y_output(1)
>     stim.PauliString("+ZY")
> ```

### `stim.Tableau.y_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.y_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's output pauli string for an input Y generator.
> 
> A constant-time equivalent for `tableau.y_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the tableau column (the qubit of the input Y generator).
>     output_index: Identifies the tableau row (the output qubit).
> 
> Returns:
>     An integer identifying Pauli at the given location in the tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... )
>     >>> t.y_output_pauli(0, 0)
>     1
>     >>> t.y_output_pauli(0, 1)
>     2
>     >>> t.y_output_pauli(1, 0)
>     0
>     >>> t.y_output_pauli(1, 1)
>     2
> ```

### `stim.Tableau.z_output(self, target: int) -> stim.PauliString`<a name="stim.Tableau.z_output"></a>
> ```
> Returns the result of conjugating a Pauli Z by the tableau's Clifford operation.
> 
> Args:
>     target: The qubit targeted by the Pauli Z operation.
> 
> Examples:
>     >>> import stim
>     >>> h = stim.Tableau.from_named_gate("H")
>     >>> h.z_output(0)
>     stim.PauliString("+X")
> 
>     >>> cnot = stim.Tableau.from_named_gate("CNOT")
>     >>> cnot.z_output(0)
>     stim.PauliString("+Z_")
>     >>> cnot.z_output(1)
>     stim.PauliString("+ZZ")
> ```

### `stim.Tableau.z_output_pauli(self, input_index: int, output_index: int) -> int`<a name="stim.Tableau.z_output_pauli"></a>
> ```
> Returns a Pauli term from the tableau's output pauli string for an input Z generator.
> 
> A constant-time equivalent for `tableau.z_output(input_index)[output_index]`.
> 
> Args:
>     input_index: Identifies the tableau column (the qubit of the input Z generator).
>     output_index: Identifies the tableau row (the output qubit).
> 
> Returns:
>     An integer identifying Pauli at the given location in the tableau:
> 
>         0: I
>         1: X
>         2: Y
>         3: Z
> 
> Examples:
>     >>> import stim
> 
>     >>> t = stim.Tableau.from_conjugated_generators(
>     ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
>     ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
>     ... )
>     >>> t.z_output_pauli(0, 0)
>     3
>     >>> t.z_output_pauli(0, 1)
>     2
>     >>> t.z_output_pauli(1, 0)
>     2
>     >>> t.z_output_pauli(1, 1)
>     1
> ```

### `stim.TableauSimulator.canonical_stabilizers(self) -> List[stim.PauliString]`<a name="stim.TableauSimulator.canonical_stabilizers"></a>
> ```
> Returns a list of the stabilizers of the simulator's current state in a standard form.
> 
> Two simulators have the same canonical stabilizers if and only if their current quantum state is equal
> (and tracking the same number of qubits).
> 
> The canonical form is computed as follows:
> 
>     1. Get a list of stabilizers using the `z_output`s of `simulator.current_inverse_tableau()**-1`.
>     2. Perform Gaussian elimination on each generator g (ordered X0, Z0, X1, Z1, X2, Z2, etc).
>         2a) Pick any stabilizer that uses the generator g. If there are none, go to the next g.
>         2b) Multiply that stabilizer into all other stabilizers that use the generator g.
>         2c) Swap that stabilizer with the stabilizer at position `next_output` then increment `next_output`.
> 
> Returns:
>     A List[stim.PauliString] of the simulator's state's stabilizers.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.h(0)
>     >>> s.cnot(0, 1)
>     >>> s.x(2)
>     >>> s.canonical_stabilizers()
>     [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]
> 
>     >>> # Scramble the stabilizers then check that the canonical form is unchanged.
>     >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
>     >>> s.cnot(0, 1)
>     >>> s.cz(0, 2)
>     >>> s.s(0, 2)
>     >>> s.cy(2, 1)
>     >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
>     >>> s.canonical_stabilizers()
>     [stim.PauliString("+XX_"), stim.PauliString("+ZZ_"), stim.PauliString("-__Z")]
> ```

### `stim.TableauSimulator.cnot(self, *args) -> None`<a name="stim.TableauSimulator.cnot"></a>
> ```
> Applies a controlled X gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.copy(self) -> stim.TableauSimulator`<a name="stim.TableauSimulator.copy"></a>
> ```
> Returns a copy of the simulator. A simulator with the same internal state.
> 
> Examples:
>     >>> import stim
> 
>     >>> s1 = stim.TableauSimulator()
>     >>> s1.set_inverse_tableau(stim.Tableau.random(1))
>     >>> s2 = s1.copy()
>     >>> s2 is s1
>     False
>     >>> s2.current_inverse_tableau() == s1.current_inverse_tableau()
>     True
> 
>     >>> s = stim.TableauSimulator()
>     >>> def brute_force_post_select(qubit, desired_result):
>     ...     global s
>     ...     while True:
>     ...         copy = s.copy()
>     ...         if copy.measure(qubit) == desired_result:
>     ...             s = copy
>     ...             break
>     >>> s.h(0)
>     >>> brute_force_post_select(qubit=0, desired_result=True)
>     >>> s.measure(0)
>     True
> ```

### `stim.TableauSimulator.current_inverse_tableau(self) -> stim.Tableau`<a name="stim.TableauSimulator.current_inverse_tableau"></a>
> ```
> Returns a copy of the internal state of the simulator as a stim.Tableau.
> 
> Returns:
>     A stim.Tableau copy of the simulator's state.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.h(0)
>     >>> s.current_inverse_tableau()
>     stim.Tableau.from_conjugated_generators(
>         xs=[
>             stim.PauliString("+Z"),
>         ],
>         zs=[
>             stim.PauliString("+X"),
>         ],
>     )
>     >>> s.cnot(0, 1)
>     >>> s.current_inverse_tableau()
>     stim.Tableau.from_conjugated_generators(
>         xs=[
>             stim.PauliString("+ZX"),
>             stim.PauliString("+_X"),
>         ],
>         zs=[
>             stim.PauliString("+X_"),
>             stim.PauliString("+XZ"),
>         ],
>     )
> ```

### `stim.TableauSimulator.current_measurement_record(self) -> List[bool]`<a name="stim.TableauSimulator.current_measurement_record"></a>
> ```
> Returns a copy of the record of all measurements performed by the simulator.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.current_measurement_record()
>     []
>     >>> s.measure(0)
>     False
>     >>> s.x(0)
>     >>> s.measure(0)
>     True
>     >>> s.current_measurement_record()
>     [False, True]
>     >>> s.do(stim.Circuit("M 0"))
>     >>> s.current_measurement_record()
>     [False, True, True]
> 
> Returns:
>     A list of booleans containing the result of every measurement performed by the simulator so far.
> ```

### `stim.TableauSimulator.cy(self, *args) -> None`<a name="stim.TableauSimulator.cy"></a>
> ```
> Applies a controlled Y gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.cz(self, *args) -> None`<a name="stim.TableauSimulator.cz"></a>
> ```
> Applies a controlled Z gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.do(*args, **kwargs)`<a name="stim.TableauSimulator.do"></a>
> ```
> Overloaded function.
> 
> 1. do(self: stim.TableauSimulator, circuit: stim.Circuit) -> None
> 
> Applies all the operations in the given stim.Circuit to the simulator's state.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.do(stim.Circuit('''
>     ...     X 0
>     ...     M 0
>     ... '''))
>     >>> s.current_measurement_record()
>     [True]
> 
> Args:
>     circuit: A stim.Circuit containing operations to apply.
> 
> 
> 2. do(self: stim.TableauSimulator, pauli_string: stim.PauliString) -> None
> 
> Applies all the Pauli operations in the given stim.PauliString to the simulator's state.
> 
> The Pauli at offset k is applied to the qubit with index k.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.do(stim.PauliString("IXYZ"))
>     >>> s.measure_many(0, 1, 2, 3)
>     [False, True, True, False]
> 
> Args:
>     pauli_string: A stim.PauliString containing Pauli operations to apply.
> ```

### `stim.TableauSimulator.h(self, *args) -> None`<a name="stim.TableauSimulator.h"></a>
> ```
> Applies a Hadamard gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.h_xy(self, *args) -> None`<a name="stim.TableauSimulator.h_xy"></a>
> ```
> Applies a variant of the Hadamard gate that swaps the X and Y axes to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.h_yz(self, *args) -> None`<a name="stim.TableauSimulator.h_yz"></a>
> ```
> Applies a variant of the Hadamard gate that swaps the Y and Z axes to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.iswap(self, *args) -> None`<a name="stim.TableauSimulator.iswap"></a>
> ```
> Applies an ISWAP gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.iswap_dag(self, *args) -> None`<a name="stim.TableauSimulator.iswap_dag"></a>
> ```
> Applies an ISWAP_DAG gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.measure(self, target: int) -> bool`<a name="stim.TableauSimulator.measure"></a>
> ```
> Measures a single qubit.
> 
> Unlike the other methods on TableauSimulator, this one does not broadcast
> over multiple targets. This is to avoid returning a list, which would
> create a pitfall where typing `if sim.measure(qubit)` would be a bug.
> 
> To measure multiple qubits, use `TableauSimulator.measure_many`.
> 
> Args:
>     target: The index of the qubit to measure.
> 
> Returns:
>     The measurement result as a bool.
> ```

### `stim.TableauSimulator.measure_kickback(self, target: int) -> tuple`<a name="stim.TableauSimulator.measure_kickback"></a>
> ```
> Measures a qubit and returns the result as well as its Pauli kickback (if any).
> 
> The "Pauli kickback" of a stabilizer circuit measurement is a set of Pauli operations that
> flip the post-measurement system state between the two possible post-measurement states.
> For example, consider measuring one of the qubits in the state |00>+|11> in the Z basis.
> If the measurement result is False, then the system projects into the state |00>.
> If the measurement result is True, then the system projects into the state |11>.
> Applying a Pauli X operation to both qubits flips between |00> and |11>.
> Therefore the Pauli kickback of the measurement is `stim.PauliString("XX")`.
> Note that there are often many possible equivalent Pauli kickbacks. For example,
> if in the previous example there was a third qubit in the |0> state, then both
> `stim.PauliString("XX_")` and `stim.PauliString("XXZ")` are valid kickbacks.
> 
> Measurements with determinist results don't have a Pauli kickback.
> 
> Args:
>     target: The index of the qubit to measure.
> 
> Returns:
>     A (result, kickback) tuple.
>     The result is a bool containing the measurement's output.
>     The kickback is either None (meaning the measurement was deterministic) or a stim.PauliString
>     (meaning the measurement was random, and the operations in the Pauli string flip between the
>     two possible post-measurement states).
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
> 
>     >>> s.measure_kickback(0)
>     (False, None)
> 
>     >>> s.h(0)
>     >>> s.measure_kickback(0)[1]
>     stim.PauliString("+X")
> 
>     >>> def pseudo_post_select(qubit, desired_result):
>     ...     m, kick = s.measure_kickback(qubit)
>     ...     if m != desired_result:
>     ...         if kick is None:
>     ...             raise ValueError("Deterministic measurement differed from desired result.")
>     ...         s.do(kick)
>     >>> s = stim.TableauSimulator()
>     >>> s.h(0)
>     >>> s.cnot(0, 1)
>     >>> s.cnot(0, 2)
>     >>> pseudo_post_select(qubit=2, desired_result=True)
>     >>> s.measure_many(0, 1, 2)
>     [True, True, True]
> ```

### `stim.TableauSimulator.measure_many(self, *args) -> List[bool]`<a name="stim.TableauSimulator.measure_many"></a>
> ```
> Measures multiple qubits.
> 
> Args:
>     *targets: The indices of the qubits to measure.
> 
> Returns:
>     The measurement results as a list of bools.
> ```

### `stim.TableauSimulator.peek_bloch(self, target: int) -> stim.PauliString`<a name="stim.TableauSimulator.peek_bloch"></a>
> ```
> Returns the current bloch vector of the qubit, represented as a stim.PauliString.
> 
> This is a non-physical operation. It reports information about the qubit without disturbing it.
> 
> Args:
>     target: The qubit to peek at.
> 
> Returns:
>     stim.PauliString("I"): The qubit is entangled. Its bloch vector is x=y=z=0.
>     stim.PauliString("+Z"): The qubit is in the |0> state. Its bloch vector is z=+1, x=y=0.
>     stim.PauliString("-Z"): The qubit is in the |1> state. Its bloch vector is z=-1, x=y=0.
>     stim.PauliString("+Y"): The qubit is in the |i> state. Its bloch vector is y=+1, x=z=0.
>     stim.PauliString("-Y"): The qubit is in the |-i> state. Its bloch vector is y=-1, x=z=0.
>     stim.PauliString("+X"): The qubit is in the |+> state. Its bloch vector is x=+1, y=z=0.
>     stim.PauliString("-X"): The qubit is in the |-> state. Its bloch vector is x=-1, y=z=0.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> s.peek_bloch(0)
>     stim.PauliString("+Z")
>     >>> s.x(0)
>     >>> s.peek_bloch(0)
>     stim.PauliString("-Z")
>     >>> s.h(0)
>     >>> s.peek_bloch(0)
>     stim.PauliString("-X")
>     >>> s.sqrt_x(1)
>     >>> s.peek_bloch(1)
>     stim.PauliString("-Y")
>     >>> s.cz(0, 1)
>     >>> s.peek_bloch(0)
>     stim.PauliString("+_")
> ```

### `stim.TableauSimulator.reset(self, *args) -> None`<a name="stim.TableauSimulator.reset"></a>
> ```
> Resets qubits to zero (e.g. by swapping them for zero'd qubit from the environment).
> 
> Args:
>     *targets: The indices of the qubits to reset.
> ```

### `stim.TableauSimulator.s(self, *args) -> None`<a name="stim.TableauSimulator.s"></a>
> ```
> Applies a SQRT_Z gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.s_dag(self, *args) -> None`<a name="stim.TableauSimulator.s_dag"></a>
> ```
> Applies a SQRT_Z_DAG gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.set_inverse_tableau(self, arg0: stim.Tableau) -> None`<a name="stim.TableauSimulator.set_inverse_tableau"></a>
> ```
> Overwrites the simulator's internal state with a copy of the given inverse tableau.
> 
> The inverse tableau specifies how Pauli product observables of qubits at the current time transform
> into equivalent Pauli product observables at the beginning of time, when all qubits were in the
> |0> state. For example, if the Z observable on qubit 5 maps to a product of Z observables at the
> start of time then a Z basis measurement on qubit 5 will be deterministic and equal to the sign
> of the product. Whereas if it mapped to a product of observables including an X or a Y then the Z
> basis measurement would be random.
> 
> Any qubits not within the length of the tableau are implicitly in the |0> state.
> 
> Args:
>     new_inverse_tableau: The tableau to overwrite the internal state with.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> t = stim.Tableau.random(4)
>     >>> s.set_inverse_tableau(t)
>     >>> s.current_inverse_tableau() == t
>     True
> ```

### `stim.TableauSimulator.set_num_qubits(self, arg0: int) -> None`<a name="stim.TableauSimulator.set_num_qubits"></a>
> ```
> Forces the simulator's internal state to track exactly the qubits whose indices are in range(new_num_qubits).
> 
> Note that untracked qubits are always assumed to be in the |0> state. Therefore, calling this method
> will effectively force any qubit whose index is outside range(new_num_qubits) to be reset to |0>.
> 
> Note that this method does not prevent future operations from implicitly expanding the size of the
> tracked state (e.g. setting the number of qubits to 5 will not prevent a Hadamard from then being
> applied to qubit 100, increasing the number of qubits to 101).
> 
> Args:
>     new_num_qubits: The length of the range of qubits the internal simulator should be tracking.
> 
> Examples:
>     >>> import stim
>     >>> s = stim.TableauSimulator()
>     >>> len(s.current_inverse_tableau())
>     0
> 
>     >>> s.set_num_qubits(5)
>     >>> len(s.current_inverse_tableau())
>     5
> 
>     >>> s.x(0, 1, 2, 3)
>     >>> s.set_num_qubits(2)
>     >>> s.measure_many(0, 1, 2, 3)
>     [True, True, False, False]
> ```

### `stim.TableauSimulator.sqrt_x(self, *args) -> None`<a name="stim.TableauSimulator.sqrt_x"></a>
> ```
> Applies a SQRT_X gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.sqrt_x_dag(self, *args) -> None`<a name="stim.TableauSimulator.sqrt_x_dag"></a>
> ```
> Applies a SQRT_X_DAG gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.sqrt_y(self, *args) -> None`<a name="stim.TableauSimulator.sqrt_y"></a>
> ```
> Applies a SQRT_Y gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.sqrt_y_dag(self, *args) -> None`<a name="stim.TableauSimulator.sqrt_y_dag"></a>
> ```
> Applies a SQRT_Y_DAG gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.state_vector(self) -> numpy.ndarray[numpy.float32]`<a name="stim.TableauSimulator.state_vector"></a>
> ```
> Returns a wavefunction that satisfies the stabilizers of the simulator's current state.
> 
> This function takes O(n * 2**n) time and O(2**n) space, where n is the number of qubits. The computation is
> done by initialization a random state vector and iteratively projecting it into the +1 eigenspace of each
> stabilizer of the state. The global phase of the result is arbitrary (and will vary from call to call).
> 
> The result is in little endian order. The amplitude at offset b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is
> the amplitude for the computational basis state where the qubit with index 0 is storing the bit b_0, the
> qubit with index 1 is storing the bit b_1, etc.
> 
> Returns:
>     A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes in little endian order.
> 
> Examples:
>     >>> import stim
>     >>> import numpy as np
> 
>     >>> # Check that the qubit-to-amplitude-index ordering is little-endian.
>     >>> s = stim.TableauSimulator()
>     >>> s.x(1)
>     >>> s.x(4)
>     >>> vector = s.state_vector()
>     >>> np.abs(vector[0b_10010]).round(2)
>     1.0
>     >>> tensor = vector.reshape((2, 2, 2, 2, 2))
>     >>> np.abs(tensor[1, 0, 0, 1, 0]).round(2)
>     1.0
> ```

### `stim.TableauSimulator.swap(self, *args) -> None`<a name="stim.TableauSimulator.swap"></a>
> ```
> Applies a swap gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.x(self, *args) -> None`<a name="stim.TableauSimulator.x"></a>
> ```
> Applies a Pauli X gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.xcx(self, *args) -> None`<a name="stim.TableauSimulator.xcx"></a>
> ```
> Applies an X-controlled X gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.xcy(self, *args) -> None`<a name="stim.TableauSimulator.xcy"></a>
> ```
> Applies an X-controlled Y gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.xcz(self, *args) -> None`<a name="stim.TableauSimulator.xcz"></a>
> ```
> Applies an X-controlled Z gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.y(self, *args) -> None`<a name="stim.TableauSimulator.y"></a>
> ```
> Applies a Pauli Y gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```

### `stim.TableauSimulator.ycx(self, *args) -> None`<a name="stim.TableauSimulator.ycx"></a>
> ```
> Applies a Y-controlled X gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.ycy(self, *args) -> None`<a name="stim.TableauSimulator.ycy"></a>
> ```
> Applies a Y-controlled Y gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.ycz(self, *args) -> None`<a name="stim.TableauSimulator.ycz"></a>
> ```
> Applies a Y-controlled Z gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
>         Applies the gate to the first two targets, then the next two targets, and so forth.
>         There must be an even number of targets.
> ```

### `stim.TableauSimulator.z(self, *args) -> None`<a name="stim.TableauSimulator.z"></a>
> ```
> Applies a Pauli Z gate to the simulator's state.
> 
> Args:
>     *targets: The indices of the qubits to target with the gate.
> ```
