# Stim (Development Version) API Reference

*CAUTION*: this API reference is for the in-development version of stim.
Methods and arguments mentioned here may not be accessible in stable versions, yet.
API references for stable versions are kept on the [stim github wiki](https://github.com/quantumlib/Stim/wiki)

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
    - [`stim.Circuit.append`](#stim.Circuit.append)
    - [`stim.Circuit.append_from_stim_program_text`](#stim.Circuit.append_from_stim_program_text)
    - [`stim.Circuit.approx_equals`](#stim.Circuit.approx_equals)
    - [`stim.Circuit.clear`](#stim.Circuit.clear)
    - [`stim.Circuit.compile_detector_sampler`](#stim.Circuit.compile_detector_sampler)
    - [`stim.Circuit.compile_m2d_converter`](#stim.Circuit.compile_m2d_converter)
    - [`stim.Circuit.compile_sampler`](#stim.Circuit.compile_sampler)
    - [`stim.Circuit.copy`](#stim.Circuit.copy)
    - [`stim.Circuit.count_determined_measurements`](#stim.Circuit.count_determined_measurements)
    - [`stim.Circuit.detector_error_model`](#stim.Circuit.detector_error_model)
    - [`stim.Circuit.diagram`](#stim.Circuit.diagram)
    - [`stim.Circuit.explain_detector_error_model_errors`](#stim.Circuit.explain_detector_error_model_errors)
    - [`stim.Circuit.flattened`](#stim.Circuit.flattened)
    - [`stim.Circuit.from_file`](#stim.Circuit.from_file)
    - [`stim.Circuit.generated`](#stim.Circuit.generated)
    - [`stim.Circuit.get_detector_coordinates`](#stim.Circuit.get_detector_coordinates)
    - [`stim.Circuit.get_final_qubit_coordinates`](#stim.Circuit.get_final_qubit_coordinates)
    - [`stim.Circuit.inverse`](#stim.Circuit.inverse)
    - [`stim.Circuit.num_detectors`](#stim.Circuit.num_detectors)
    - [`stim.Circuit.num_measurements`](#stim.Circuit.num_measurements)
    - [`stim.Circuit.num_observables`](#stim.Circuit.num_observables)
    - [`stim.Circuit.num_qubits`](#stim.Circuit.num_qubits)
    - [`stim.Circuit.num_sweep_bits`](#stim.Circuit.num_sweep_bits)
    - [`stim.Circuit.num_ticks`](#stim.Circuit.num_ticks)
    - [`stim.Circuit.reference_sample`](#stim.Circuit.reference_sample)
    - [`stim.Circuit.search_for_undetectable_logical_errors`](#stim.Circuit.search_for_undetectable_logical_errors)
    - [`stim.Circuit.shortest_graphlike_error`](#stim.Circuit.shortest_graphlike_error)
    - [`stim.Circuit.to_file`](#stim.Circuit.to_file)
    - [`stim.Circuit.with_inlined_feedback`](#stim.Circuit.with_inlined_feedback)
    - [`stim.Circuit.without_noise`](#stim.Circuit.without_noise)
- [`stim.CircuitErrorLocation`](#stim.CircuitErrorLocation)
    - [`stim.CircuitErrorLocation.__init__`](#stim.CircuitErrorLocation.__init__)
    - [`stim.CircuitErrorLocation.flipped_measurement`](#stim.CircuitErrorLocation.flipped_measurement)
    - [`stim.CircuitErrorLocation.flipped_pauli_product`](#stim.CircuitErrorLocation.flipped_pauli_product)
    - [`stim.CircuitErrorLocation.instruction_targets`](#stim.CircuitErrorLocation.instruction_targets)
    - [`stim.CircuitErrorLocation.stack_frames`](#stim.CircuitErrorLocation.stack_frames)
    - [`stim.CircuitErrorLocation.tick_offset`](#stim.CircuitErrorLocation.tick_offset)
- [`stim.CircuitErrorLocationStackFrame`](#stim.CircuitErrorLocationStackFrame)
    - [`stim.CircuitErrorLocationStackFrame.__init__`](#stim.CircuitErrorLocationStackFrame.__init__)
    - [`stim.CircuitErrorLocationStackFrame.instruction_offset`](#stim.CircuitErrorLocationStackFrame.instruction_offset)
    - [`stim.CircuitErrorLocationStackFrame.instruction_repetitions_arg`](#stim.CircuitErrorLocationStackFrame.instruction_repetitions_arg)
    - [`stim.CircuitErrorLocationStackFrame.iteration_index`](#stim.CircuitErrorLocationStackFrame.iteration_index)
- [`stim.CircuitInstruction`](#stim.CircuitInstruction)
    - [`stim.CircuitInstruction.__eq__`](#stim.CircuitInstruction.__eq__)
    - [`stim.CircuitInstruction.__init__`](#stim.CircuitInstruction.__init__)
    - [`stim.CircuitInstruction.__ne__`](#stim.CircuitInstruction.__ne__)
    - [`stim.CircuitInstruction.__repr__`](#stim.CircuitInstruction.__repr__)
    - [`stim.CircuitInstruction.__str__`](#stim.CircuitInstruction.__str__)
    - [`stim.CircuitInstruction.gate_args_copy`](#stim.CircuitInstruction.gate_args_copy)
    - [`stim.CircuitInstruction.name`](#stim.CircuitInstruction.name)
    - [`stim.CircuitInstruction.targets_copy`](#stim.CircuitInstruction.targets_copy)
- [`stim.CircuitRepeatBlock`](#stim.CircuitRepeatBlock)
    - [`stim.CircuitRepeatBlock.__eq__`](#stim.CircuitRepeatBlock.__eq__)
    - [`stim.CircuitRepeatBlock.__init__`](#stim.CircuitRepeatBlock.__init__)
    - [`stim.CircuitRepeatBlock.__ne__`](#stim.CircuitRepeatBlock.__ne__)
    - [`stim.CircuitRepeatBlock.__repr__`](#stim.CircuitRepeatBlock.__repr__)
    - [`stim.CircuitRepeatBlock.body_copy`](#stim.CircuitRepeatBlock.body_copy)
    - [`stim.CircuitRepeatBlock.name`](#stim.CircuitRepeatBlock.name)
    - [`stim.CircuitRepeatBlock.repeat_count`](#stim.CircuitRepeatBlock.repeat_count)
- [`stim.CircuitTargetsInsideInstruction`](#stim.CircuitTargetsInsideInstruction)
    - [`stim.CircuitTargetsInsideInstruction.__init__`](#stim.CircuitTargetsInsideInstruction.__init__)
    - [`stim.CircuitTargetsInsideInstruction.args`](#stim.CircuitTargetsInsideInstruction.args)
    - [`stim.CircuitTargetsInsideInstruction.gate`](#stim.CircuitTargetsInsideInstruction.gate)
    - [`stim.CircuitTargetsInsideInstruction.target_range_end`](#stim.CircuitTargetsInsideInstruction.target_range_end)
    - [`stim.CircuitTargetsInsideInstruction.target_range_start`](#stim.CircuitTargetsInsideInstruction.target_range_start)
    - [`stim.CircuitTargetsInsideInstruction.targets_in_range`](#stim.CircuitTargetsInsideInstruction.targets_in_range)
- [`stim.CompiledDemSampler`](#stim.CompiledDemSampler)
    - [`stim.CompiledDemSampler.sample`](#stim.CompiledDemSampler.sample)
    - [`stim.CompiledDemSampler.sample_write`](#stim.CompiledDemSampler.sample_write)
- [`stim.CompiledDetectorSampler`](#stim.CompiledDetectorSampler)
    - [`stim.CompiledDetectorSampler.__init__`](#stim.CompiledDetectorSampler.__init__)
    - [`stim.CompiledDetectorSampler.__repr__`](#stim.CompiledDetectorSampler.__repr__)
    - [`stim.CompiledDetectorSampler.sample`](#stim.CompiledDetectorSampler.sample)
    - [`stim.CompiledDetectorSampler.sample_write`](#stim.CompiledDetectorSampler.sample_write)
- [`stim.CompiledMeasurementSampler`](#stim.CompiledMeasurementSampler)
    - [`stim.CompiledMeasurementSampler.__init__`](#stim.CompiledMeasurementSampler.__init__)
    - [`stim.CompiledMeasurementSampler.__repr__`](#stim.CompiledMeasurementSampler.__repr__)
    - [`stim.CompiledMeasurementSampler.sample`](#stim.CompiledMeasurementSampler.sample)
    - [`stim.CompiledMeasurementSampler.sample_write`](#stim.CompiledMeasurementSampler.sample_write)
- [`stim.CompiledMeasurementsToDetectionEventsConverter`](#stim.CompiledMeasurementsToDetectionEventsConverter)
    - [`stim.CompiledMeasurementsToDetectionEventsConverter.__init__`](#stim.CompiledMeasurementsToDetectionEventsConverter.__init__)
    - [`stim.CompiledMeasurementsToDetectionEventsConverter.__repr__`](#stim.CompiledMeasurementsToDetectionEventsConverter.__repr__)
    - [`stim.CompiledMeasurementsToDetectionEventsConverter.convert`](#stim.CompiledMeasurementsToDetectionEventsConverter.convert)
    - [`stim.CompiledMeasurementsToDetectionEventsConverter.convert_file`](#stim.CompiledMeasurementsToDetectionEventsConverter.convert_file)
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
    - [`stim.DemRepeatBlock.type`](#stim.DemRepeatBlock.type)
- [`stim.DemTarget`](#stim.DemTarget)
    - [`stim.DemTarget.__eq__`](#stim.DemTarget.__eq__)
    - [`stim.DemTarget.__ne__`](#stim.DemTarget.__ne__)
    - [`stim.DemTarget.__repr__`](#stim.DemTarget.__repr__)
    - [`stim.DemTarget.__str__`](#stim.DemTarget.__str__)
    - [`stim.DemTarget.is_logical_observable_id`](#stim.DemTarget.is_logical_observable_id)
    - [`stim.DemTarget.is_relative_detector_id`](#stim.DemTarget.is_relative_detector_id)
    - [`stim.DemTarget.is_separator`](#stim.DemTarget.is_separator)
    - [`stim.DemTarget.logical_observable_id`](#stim.DemTarget.logical_observable_id)
    - [`stim.DemTarget.relative_detector_id`](#stim.DemTarget.relative_detector_id)
    - [`stim.DemTarget.separator`](#stim.DemTarget.separator)
    - [`stim.DemTarget.val`](#stim.DemTarget.val)
- [`stim.DemTargetWithCoords`](#stim.DemTargetWithCoords)
    - [`stim.DemTargetWithCoords.__init__`](#stim.DemTargetWithCoords.__init__)
    - [`stim.DemTargetWithCoords.coords`](#stim.DemTargetWithCoords.coords)
    - [`stim.DemTargetWithCoords.dem_target`](#stim.DemTargetWithCoords.dem_target)
- [`stim.DetectorErrorModel`](#stim.DetectorErrorModel)
    - [`stim.DetectorErrorModel.__add__`](#stim.DetectorErrorModel.__add__)
    - [`stim.DetectorErrorModel.__eq__`](#stim.DetectorErrorModel.__eq__)
    - [`stim.DetectorErrorModel.__getitem__`](#stim.DetectorErrorModel.__getitem__)
    - [`stim.DetectorErrorModel.__iadd__`](#stim.DetectorErrorModel.__iadd__)
    - [`stim.DetectorErrorModel.__imul__`](#stim.DetectorErrorModel.__imul__)
    - [`stim.DetectorErrorModel.__init__`](#stim.DetectorErrorModel.__init__)
    - [`stim.DetectorErrorModel.__len__`](#stim.DetectorErrorModel.__len__)
    - [`stim.DetectorErrorModel.__mul__`](#stim.DetectorErrorModel.__mul__)
    - [`stim.DetectorErrorModel.__ne__`](#stim.DetectorErrorModel.__ne__)
    - [`stim.DetectorErrorModel.__repr__`](#stim.DetectorErrorModel.__repr__)
    - [`stim.DetectorErrorModel.__rmul__`](#stim.DetectorErrorModel.__rmul__)
    - [`stim.DetectorErrorModel.__str__`](#stim.DetectorErrorModel.__str__)
    - [`stim.DetectorErrorModel.append`](#stim.DetectorErrorModel.append)
    - [`stim.DetectorErrorModel.approx_equals`](#stim.DetectorErrorModel.approx_equals)
    - [`stim.DetectorErrorModel.clear`](#stim.DetectorErrorModel.clear)
    - [`stim.DetectorErrorModel.compile_sampler`](#stim.DetectorErrorModel.compile_sampler)
    - [`stim.DetectorErrorModel.copy`](#stim.DetectorErrorModel.copy)
    - [`stim.DetectorErrorModel.diagram`](#stim.DetectorErrorModel.diagram)
    - [`stim.DetectorErrorModel.flattened`](#stim.DetectorErrorModel.flattened)
    - [`stim.DetectorErrorModel.from_file`](#stim.DetectorErrorModel.from_file)
    - [`stim.DetectorErrorModel.get_detector_coordinates`](#stim.DetectorErrorModel.get_detector_coordinates)
    - [`stim.DetectorErrorModel.num_detectors`](#stim.DetectorErrorModel.num_detectors)
    - [`stim.DetectorErrorModel.num_errors`](#stim.DetectorErrorModel.num_errors)
    - [`stim.DetectorErrorModel.num_observables`](#stim.DetectorErrorModel.num_observables)
    - [`stim.DetectorErrorModel.rounded`](#stim.DetectorErrorModel.rounded)
    - [`stim.DetectorErrorModel.shortest_graphlike_error`](#stim.DetectorErrorModel.shortest_graphlike_error)
    - [`stim.DetectorErrorModel.to_file`](#stim.DetectorErrorModel.to_file)
- [`stim.ExplainedError`](#stim.ExplainedError)
    - [`stim.ExplainedError.__init__`](#stim.ExplainedError.__init__)
    - [`stim.ExplainedError.circuit_error_locations`](#stim.ExplainedError.circuit_error_locations)
    - [`stim.ExplainedError.dem_error_terms`](#stim.ExplainedError.dem_error_terms)
- [`stim.FlipSimulator`](#stim.FlipSimulator)
    - [`stim.FlipSimulator.__init__`](#stim.FlipSimulator.__init__)
    - [`stim.FlipSimulator.batch_size`](#stim.FlipSimulator.batch_size)
    - [`stim.FlipSimulator.do`](#stim.FlipSimulator.do)
    - [`stim.FlipSimulator.get_detector_flips`](#stim.FlipSimulator.get_detector_flips)
    - [`stim.FlipSimulator.get_measurement_flips`](#stim.FlipSimulator.get_measurement_flips)
    - [`stim.FlipSimulator.get_observable_flips`](#stim.FlipSimulator.get_observable_flips)
    - [`stim.FlipSimulator.num_detectors`](#stim.FlipSimulator.num_detectors)
    - [`stim.FlipSimulator.num_measurements`](#stim.FlipSimulator.num_measurements)
    - [`stim.FlipSimulator.num_observables`](#stim.FlipSimulator.num_observables)
    - [`stim.FlipSimulator.num_qubits`](#stim.FlipSimulator.num_qubits)
    - [`stim.FlipSimulator.peek_pauli_flips`](#stim.FlipSimulator.peek_pauli_flips)
    - [`stim.FlipSimulator.set_pauli_flip`](#stim.FlipSimulator.set_pauli_flip)
- [`stim.FlippedMeasurement`](#stim.FlippedMeasurement)
    - [`stim.FlippedMeasurement.__init__`](#stim.FlippedMeasurement.__init__)
    - [`stim.FlippedMeasurement.observable`](#stim.FlippedMeasurement.observable)
    - [`stim.FlippedMeasurement.record_index`](#stim.FlippedMeasurement.record_index)
- [`stim.GateData`](#stim.GateData)
    - [`stim.GateData.__eq__`](#stim.GateData.__eq__)
    - [`stim.GateData.__init__`](#stim.GateData.__init__)
    - [`stim.GateData.__ne__`](#stim.GateData.__ne__)
    - [`stim.GateData.__repr__`](#stim.GateData.__repr__)
    - [`stim.GateData.__str__`](#stim.GateData.__str__)
    - [`stim.GateData.aliases`](#stim.GateData.aliases)
    - [`stim.GateData.is_noisy_gate`](#stim.GateData.is_noisy_gate)
    - [`stim.GateData.is_reset`](#stim.GateData.is_reset)
    - [`stim.GateData.is_single_qubit_gate`](#stim.GateData.is_single_qubit_gate)
    - [`stim.GateData.is_two_qubit_gate`](#stim.GateData.is_two_qubit_gate)
    - [`stim.GateData.is_unitary`](#stim.GateData.is_unitary)
    - [`stim.GateData.name`](#stim.GateData.name)
    - [`stim.GateData.num_parens_arguments_range`](#stim.GateData.num_parens_arguments_range)
    - [`stim.GateData.produces_measurements`](#stim.GateData.produces_measurements)
    - [`stim.GateData.tableau`](#stim.GateData.tableau)
    - [`stim.GateData.takes_measurement_record_targets`](#stim.GateData.takes_measurement_record_targets)
    - [`stim.GateData.takes_pauli_targets`](#stim.GateData.takes_pauli_targets)
    - [`stim.GateData.unitary_matrix`](#stim.GateData.unitary_matrix)
- [`stim.GateTarget`](#stim.GateTarget)
    - [`stim.GateTarget.__eq__`](#stim.GateTarget.__eq__)
    - [`stim.GateTarget.__init__`](#stim.GateTarget.__init__)
    - [`stim.GateTarget.__ne__`](#stim.GateTarget.__ne__)
    - [`stim.GateTarget.__repr__`](#stim.GateTarget.__repr__)
    - [`stim.GateTarget.is_combiner`](#stim.GateTarget.is_combiner)
    - [`stim.GateTarget.is_inverted_result_target`](#stim.GateTarget.is_inverted_result_target)
    - [`stim.GateTarget.is_measurement_record_target`](#stim.GateTarget.is_measurement_record_target)
    - [`stim.GateTarget.is_qubit_target`](#stim.GateTarget.is_qubit_target)
    - [`stim.GateTarget.is_sweep_bit_target`](#stim.GateTarget.is_sweep_bit_target)
    - [`stim.GateTarget.is_x_target`](#stim.GateTarget.is_x_target)
    - [`stim.GateTarget.is_y_target`](#stim.GateTarget.is_y_target)
    - [`stim.GateTarget.is_z_target`](#stim.GateTarget.is_z_target)
    - [`stim.GateTarget.pauli_type`](#stim.GateTarget.pauli_type)
    - [`stim.GateTarget.qubit_value`](#stim.GateTarget.qubit_value)
    - [`stim.GateTarget.value`](#stim.GateTarget.value)
- [`stim.GateTargetWithCoords`](#stim.GateTargetWithCoords)
    - [`stim.GateTargetWithCoords.__init__`](#stim.GateTargetWithCoords.__init__)
    - [`stim.GateTargetWithCoords.coords`](#stim.GateTargetWithCoords.coords)
    - [`stim.GateTargetWithCoords.gate_target`](#stim.GateTargetWithCoords.gate_target)
- [`stim.PauliString`](#stim.PauliString)
    - [`stim.PauliString.__add__`](#stim.PauliString.__add__)
    - [`stim.PauliString.__eq__`](#stim.PauliString.__eq__)
    - [`stim.PauliString.__getitem__`](#stim.PauliString.__getitem__)
    - [`stim.PauliString.__iadd__`](#stim.PauliString.__iadd__)
    - [`stim.PauliString.__imul__`](#stim.PauliString.__imul__)
    - [`stim.PauliString.__init__`](#stim.PauliString.__init__)
    - [`stim.PauliString.__itruediv__`](#stim.PauliString.__itruediv__)
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
    - [`stim.PauliString.after`](#stim.PauliString.after)
    - [`stim.PauliString.before`](#stim.PauliString.before)
    - [`stim.PauliString.commutes`](#stim.PauliString.commutes)
    - [`stim.PauliString.copy`](#stim.PauliString.copy)
    - [`stim.PauliString.from_numpy`](#stim.PauliString.from_numpy)
    - [`stim.PauliString.from_unitary_matrix`](#stim.PauliString.from_unitary_matrix)
    - [`stim.PauliString.iter_all`](#stim.PauliString.iter_all)
    - [`stim.PauliString.random`](#stim.PauliString.random)
    - [`stim.PauliString.sign`](#stim.PauliString.sign)
    - [`stim.PauliString.to_numpy`](#stim.PauliString.to_numpy)
    - [`stim.PauliString.to_tableau`](#stim.PauliString.to_tableau)
    - [`stim.PauliString.to_unitary_matrix`](#stim.PauliString.to_unitary_matrix)
    - [`stim.PauliString.weight`](#stim.PauliString.weight)
- [`stim.PauliStringIterator`](#stim.PauliStringIterator)
    - [`stim.PauliStringIterator.__iter__`](#stim.PauliStringIterator.__iter__)
    - [`stim.PauliStringIterator.__next__`](#stim.PauliStringIterator.__next__)
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
    - [`stim.Tableau.from_circuit`](#stim.Tableau.from_circuit)
    - [`stim.Tableau.from_conjugated_generators`](#stim.Tableau.from_conjugated_generators)
    - [`stim.Tableau.from_named_gate`](#stim.Tableau.from_named_gate)
    - [`stim.Tableau.from_numpy`](#stim.Tableau.from_numpy)
    - [`stim.Tableau.from_stabilizers`](#stim.Tableau.from_stabilizers)
    - [`stim.Tableau.from_state_vector`](#stim.Tableau.from_state_vector)
    - [`stim.Tableau.from_unitary_matrix`](#stim.Tableau.from_unitary_matrix)
    - [`stim.Tableau.inverse`](#stim.Tableau.inverse)
    - [`stim.Tableau.inverse_x_output`](#stim.Tableau.inverse_x_output)
    - [`stim.Tableau.inverse_x_output_pauli`](#stim.Tableau.inverse_x_output_pauli)
    - [`stim.Tableau.inverse_y_output`](#stim.Tableau.inverse_y_output)
    - [`stim.Tableau.inverse_y_output_pauli`](#stim.Tableau.inverse_y_output_pauli)
    - [`stim.Tableau.inverse_z_output`](#stim.Tableau.inverse_z_output)
    - [`stim.Tableau.inverse_z_output_pauli`](#stim.Tableau.inverse_z_output_pauli)
    - [`stim.Tableau.iter_all`](#stim.Tableau.iter_all)
    - [`stim.Tableau.prepend`](#stim.Tableau.prepend)
    - [`stim.Tableau.random`](#stim.Tableau.random)
    - [`stim.Tableau.then`](#stim.Tableau.then)
    - [`stim.Tableau.to_circuit`](#stim.Tableau.to_circuit)
    - [`stim.Tableau.to_numpy`](#stim.Tableau.to_numpy)
    - [`stim.Tableau.to_pauli_string`](#stim.Tableau.to_pauli_string)
    - [`stim.Tableau.to_state_vector`](#stim.Tableau.to_state_vector)
    - [`stim.Tableau.to_unitary_matrix`](#stim.Tableau.to_unitary_matrix)
    - [`stim.Tableau.x_output`](#stim.Tableau.x_output)
    - [`stim.Tableau.x_output_pauli`](#stim.Tableau.x_output_pauli)
    - [`stim.Tableau.x_sign`](#stim.Tableau.x_sign)
    - [`stim.Tableau.y_output`](#stim.Tableau.y_output)
    - [`stim.Tableau.y_output_pauli`](#stim.Tableau.y_output_pauli)
    - [`stim.Tableau.y_sign`](#stim.Tableau.y_sign)
    - [`stim.Tableau.z_output`](#stim.Tableau.z_output)
    - [`stim.Tableau.z_output_pauli`](#stim.Tableau.z_output_pauli)
    - [`stim.Tableau.z_sign`](#stim.Tableau.z_sign)
- [`stim.TableauIterator`](#stim.TableauIterator)
    - [`stim.TableauIterator.__iter__`](#stim.TableauIterator.__iter__)
    - [`stim.TableauIterator.__next__`](#stim.TableauIterator.__next__)
- [`stim.TableauSimulator`](#stim.TableauSimulator)
    - [`stim.TableauSimulator.__init__`](#stim.TableauSimulator.__init__)
    - [`stim.TableauSimulator.c_xyz`](#stim.TableauSimulator.c_xyz)
    - [`stim.TableauSimulator.c_zyx`](#stim.TableauSimulator.c_zyx)
    - [`stim.TableauSimulator.canonical_stabilizers`](#stim.TableauSimulator.canonical_stabilizers)
    - [`stim.TableauSimulator.cnot`](#stim.TableauSimulator.cnot)
    - [`stim.TableauSimulator.copy`](#stim.TableauSimulator.copy)
    - [`stim.TableauSimulator.current_inverse_tableau`](#stim.TableauSimulator.current_inverse_tableau)
    - [`stim.TableauSimulator.current_measurement_record`](#stim.TableauSimulator.current_measurement_record)
    - [`stim.TableauSimulator.cx`](#stim.TableauSimulator.cx)
    - [`stim.TableauSimulator.cy`](#stim.TableauSimulator.cy)
    - [`stim.TableauSimulator.cz`](#stim.TableauSimulator.cz)
    - [`stim.TableauSimulator.depolarize1`](#stim.TableauSimulator.depolarize1)
    - [`stim.TableauSimulator.depolarize2`](#stim.TableauSimulator.depolarize2)
    - [`stim.TableauSimulator.do`](#stim.TableauSimulator.do)
    - [`stim.TableauSimulator.do_circuit`](#stim.TableauSimulator.do_circuit)
    - [`stim.TableauSimulator.do_pauli_string`](#stim.TableauSimulator.do_pauli_string)
    - [`stim.TableauSimulator.do_tableau`](#stim.TableauSimulator.do_tableau)
    - [`stim.TableauSimulator.h`](#stim.TableauSimulator.h)
    - [`stim.TableauSimulator.h_xy`](#stim.TableauSimulator.h_xy)
    - [`stim.TableauSimulator.h_xz`](#stim.TableauSimulator.h_xz)
    - [`stim.TableauSimulator.h_yz`](#stim.TableauSimulator.h_yz)
    - [`stim.TableauSimulator.iswap`](#stim.TableauSimulator.iswap)
    - [`stim.TableauSimulator.iswap_dag`](#stim.TableauSimulator.iswap_dag)
    - [`stim.TableauSimulator.measure`](#stim.TableauSimulator.measure)
    - [`stim.TableauSimulator.measure_kickback`](#stim.TableauSimulator.measure_kickback)
    - [`stim.TableauSimulator.measure_many`](#stim.TableauSimulator.measure_many)
    - [`stim.TableauSimulator.measure_observable`](#stim.TableauSimulator.measure_observable)
    - [`stim.TableauSimulator.num_qubits`](#stim.TableauSimulator.num_qubits)
    - [`stim.TableauSimulator.peek_bloch`](#stim.TableauSimulator.peek_bloch)
    - [`stim.TableauSimulator.peek_observable_expectation`](#stim.TableauSimulator.peek_observable_expectation)
    - [`stim.TableauSimulator.peek_x`](#stim.TableauSimulator.peek_x)
    - [`stim.TableauSimulator.peek_y`](#stim.TableauSimulator.peek_y)
    - [`stim.TableauSimulator.peek_z`](#stim.TableauSimulator.peek_z)
    - [`stim.TableauSimulator.postselect_observable`](#stim.TableauSimulator.postselect_observable)
    - [`stim.TableauSimulator.postselect_x`](#stim.TableauSimulator.postselect_x)
    - [`stim.TableauSimulator.postselect_y`](#stim.TableauSimulator.postselect_y)
    - [`stim.TableauSimulator.postselect_z`](#stim.TableauSimulator.postselect_z)
    - [`stim.TableauSimulator.reset`](#stim.TableauSimulator.reset)
    - [`stim.TableauSimulator.reset_x`](#stim.TableauSimulator.reset_x)
    - [`stim.TableauSimulator.reset_y`](#stim.TableauSimulator.reset_y)
    - [`stim.TableauSimulator.reset_z`](#stim.TableauSimulator.reset_z)
    - [`stim.TableauSimulator.s`](#stim.TableauSimulator.s)
    - [`stim.TableauSimulator.s_dag`](#stim.TableauSimulator.s_dag)
    - [`stim.TableauSimulator.set_inverse_tableau`](#stim.TableauSimulator.set_inverse_tableau)
    - [`stim.TableauSimulator.set_num_qubits`](#stim.TableauSimulator.set_num_qubits)
    - [`stim.TableauSimulator.set_state_from_stabilizers`](#stim.TableauSimulator.set_state_from_stabilizers)
    - [`stim.TableauSimulator.set_state_from_state_vector`](#stim.TableauSimulator.set_state_from_state_vector)
    - [`stim.TableauSimulator.sqrt_x`](#stim.TableauSimulator.sqrt_x)
    - [`stim.TableauSimulator.sqrt_x_dag`](#stim.TableauSimulator.sqrt_x_dag)
    - [`stim.TableauSimulator.sqrt_y`](#stim.TableauSimulator.sqrt_y)
    - [`stim.TableauSimulator.sqrt_y_dag`](#stim.TableauSimulator.sqrt_y_dag)
    - [`stim.TableauSimulator.state_vector`](#stim.TableauSimulator.state_vector)
    - [`stim.TableauSimulator.swap`](#stim.TableauSimulator.swap)
    - [`stim.TableauSimulator.x`](#stim.TableauSimulator.x)
    - [`stim.TableauSimulator.x_error`](#stim.TableauSimulator.x_error)
    - [`stim.TableauSimulator.xcx`](#stim.TableauSimulator.xcx)
    - [`stim.TableauSimulator.xcy`](#stim.TableauSimulator.xcy)
    - [`stim.TableauSimulator.xcz`](#stim.TableauSimulator.xcz)
    - [`stim.TableauSimulator.y`](#stim.TableauSimulator.y)
    - [`stim.TableauSimulator.y_error`](#stim.TableauSimulator.y_error)
    - [`stim.TableauSimulator.ycx`](#stim.TableauSimulator.ycx)
    - [`stim.TableauSimulator.ycy`](#stim.TableauSimulator.ycy)
    - [`stim.TableauSimulator.ycz`](#stim.TableauSimulator.ycz)
    - [`stim.TableauSimulator.z`](#stim.TableauSimulator.z)
    - [`stim.TableauSimulator.z_error`](#stim.TableauSimulator.z_error)
    - [`stim.TableauSimulator.zcx`](#stim.TableauSimulator.zcx)
    - [`stim.TableauSimulator.zcy`](#stim.TableauSimulator.zcy)
    - [`stim.TableauSimulator.zcz`](#stim.TableauSimulator.zcz)
- [`stim.gate_data`](#stim.gate_data)
- [`stim.main`](#stim.main)
- [`stim.read_shot_data_file`](#stim.read_shot_data_file)
- [`stim.target_combiner`](#stim.target_combiner)
- [`stim.target_inv`](#stim.target_inv)
- [`stim.target_logical_observable_id`](#stim.target_logical_observable_id)
- [`stim.target_rec`](#stim.target_rec)
- [`stim.target_relative_detector_id`](#stim.target_relative_detector_id)
- [`stim.target_separator`](#stim.target_separator)
- [`stim.target_sweep_bit`](#stim.target_sweep_bit)
- [`stim.target_x`](#stim.target_x)
- [`stim.target_y`](#stim.target_y)
- [`stim.target_z`](#stim.target_z)
- [`stim.write_shot_data_file`](#stim.write_shot_data_file)
```python
# Types used by the method definitions.
from typing import overload, TYPE_CHECKING, Any, Dict, Iterable, List, Optional, Tuple, Union
import io
import pathlib
import numpy as np
```

<a name="stim.Circuit"></a>
```python
# stim.Circuit

# (at top-level in the stim module)
class Circuit:
    """A mutable stabilizer circuit.

    The stim.Circuit class is arguably the most important object in the
    entire library. It is the interface through which you explain a
    noisy quantum computation to Stim, in order to do fast bulk sampling
    or fast error analysis.

    For example, suppose you want to use a matching-based decoder on a
    new quantum error correction construction. Stim can help you do this
    but the very first step is to create a circuit implementing the
    construction. Once you have the circuit you can then use methods like
    stim.Circuit.detector_error_model() to create an object that can be
    used to configure the decoder, or like
    stim.Circuit.compile_detector_sampler() to produce problems for the
    decoder to solve, or like stim.Circuit.shortest_graphlike_error() to
    check for mistakes in the implementation of the code.

    Examples:
        >>> import stim
        >>> c = stim.Circuit()
        >>> c.append("X", 0)
        >>> c.append("M", 0)
        >>> c.compile_sampler().sample(shots=1)
        array([[ True]])

        >>> stim.Circuit('''
        ...    H 0
        ...    CNOT 0 1
        ...    M 0 1
        ...    DETECTOR rec[-1] rec[-2]
        ... ''').compile_detector_sampler().sample(shots=1)
        array([[False]])
    """
```

<a name="stim.Circuit.__add__"></a>
```python
# stim.Circuit.__add__

# (in class stim.Circuit)
def __add__(
    self,
    second: stim.Circuit,
) -> stim.Circuit:
    """Creates a circuit by appending two circuits.

    Examples:
        >>> import stim
        >>> c1 = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> c2 = stim.Circuit('''
        ...    M 0 1 2
        ... ''')
        >>> c1 + c2
        stim.Circuit('''
            X 0
            Y 1 2
            M 0 1 2
        ''')
    """
```

<a name="stim.Circuit.__eq__"></a>
```python
# stim.Circuit.__eq__

# (in class stim.Circuit)
def __eq__(
    self,
    arg0: stim.Circuit,
) -> bool:
    """Determines if two circuits have identical contents.
    """
```

<a name="stim.Circuit.__getitem__"></a>
```python
# stim.Circuit.__getitem__

# (in class stim.Circuit)
@overload
def __getitem__(
    self,
    index_or_slice: int,
) -> Union[stim.CircuitInstruction, stim.CircuitRepeatBlock]:
    pass
@overload
def __getitem__(
    self,
    index_or_slice: slice,
) -> stim.Circuit:
    pass
def __getitem__(
    self,
    index_or_slice: object,
) -> object:
    """Returns copies of instructions from the circuit.

    Args:
        index_or_slice: An integer index picking out an instruction to return, or a
            slice picking out a range of instructions to return as a circuit.

    Returns:
        If the index was an integer, then an instruction from the circuit.
        If the index was a slice, then a circuit made up of the instructions in that
        slice.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...    X 0
        ...    X_ERROR(0.5) 2
        ...    REPEAT 100 {
        ...        X 0
        ...        Y 1 2
        ...    }
        ...    TICK
        ...    M 0
        ...    DETECTOR rec[-1]
        ... ''')
        >>> circuit[1]
        stim.CircuitInstruction('X_ERROR', [stim.GateTarget(2)], [0.5])
        >>> circuit[2]
        stim.CircuitRepeatBlock(100, stim.Circuit('''
            X 0
            Y 1 2
        '''))
        >>> circuit[1::2]
        stim.Circuit('''
            X_ERROR(0.5) 2
            TICK
            DETECTOR rec[-1]
        ''')
    """
```

<a name="stim.Circuit.__iadd__"></a>
```python
# stim.Circuit.__iadd__

# (in class stim.Circuit)
def __iadd__(
    self,
    second: stim.Circuit,
) -> stim.Circuit:
    """Appends a circuit into the receiving circuit (mutating it).

    Examples:
        >>> import stim
        >>> c1 = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> c2 = stim.Circuit('''
        ...    M 0 1 2
        ... ''')
        >>> c1 += c2
        >>> print(repr(c1))
        stim.Circuit('''
            X 0
            Y 1 2
            M 0 1 2
        ''')
    """
```

<a name="stim.Circuit.__imul__"></a>
```python
# stim.Circuit.__imul__

# (in class stim.Circuit)
def __imul__(
    self,
    repetitions: int,
) -> stim.Circuit:
    """Mutates the circuit by putting its contents into a REPEAT block.

    Special case: if the repetition count is 0, the circuit is cleared.
    Special case: if the repetition count is 1, nothing happens.

    Args:
        repetitions: The number of times the REPEAT block should repeat.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> c *= 3
        >>> print(repr(c))
        stim.Circuit('''
            REPEAT 3 {
                X 0
                Y 1 2
            }
        ''')
    """
```

<a name="stim.Circuit.__init__"></a>
```python
# stim.Circuit.__init__

# (in class stim.Circuit)
def __init__(
    self,
    stim_program_text: str = '',
) -> None:
    """Creates a stim.Circuit.

    Args:
        stim_program_text: Defaults to empty. Describes operations to append into
            the circuit.

    Examples:
        >>> import stim
        >>> empty = stim.Circuit()
        >>> not_empty = stim.Circuit('''
        ...    X 0
        ...    CNOT 0 1
        ...    M 1
        ... ''')
    """
```

<a name="stim.Circuit.__len__"></a>
```python
# stim.Circuit.__len__

# (in class stim.Circuit)
def __len__(
    self,
) -> int:
    """Returns the number of top-level instructions and blocks in the circuit.

    Instructions inside of blocks are not included in this count.

    Examples:
        >>> import stim
        >>> len(stim.Circuit())
        0
        >>> len(stim.Circuit('''
        ...    X 0
        ...    X_ERROR(0.5) 1 2
        ...    TICK
        ...    M 0
        ...    DETECTOR rec[-1]
        ... '''))
        5
        >>> len(stim.Circuit('''
        ...    REPEAT 100 {
        ...        X 0
        ...        Y 1 2
        ...    }
        ... '''))
        1
    """
```

<a name="stim.Circuit.__mul__"></a>
```python
# stim.Circuit.__mul__

# (in class stim.Circuit)
def __mul__(
    self,
    repetitions: int,
) -> stim.Circuit:
    """Repeats the circuit using a REPEAT block.

    Has special cases for 0 repetitions and 1 repetitions.

    Args:
        repetitions: The number of times the REPEAT block should repeat.

    Returns:
        repetitions=0: An empty circuit.
        repetitions=1: A copy of this circuit.
        repetitions>=2: A circuit with a single REPEAT block, where the contents of
            that repeat block are this circuit.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> c * 3
        stim.Circuit('''
            REPEAT 3 {
                X 0
                Y 1 2
            }
        ''')
    """
```

<a name="stim.Circuit.__ne__"></a>
```python
# stim.Circuit.__ne__

# (in class stim.Circuit)
def __ne__(
    self,
    arg0: stim.Circuit,
) -> bool:
    """Determines if two circuits have non-identical contents.
    """
```

<a name="stim.Circuit.__repr__"></a>
```python
# stim.Circuit.__repr__

# (in class stim.Circuit)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.Circuit`.
    """
```

<a name="stim.Circuit.__rmul__"></a>
```python
# stim.Circuit.__rmul__

# (in class stim.Circuit)
def __rmul__(
    self,
    repetitions: int,
) -> stim.Circuit:
    """Repeats the circuit using a REPEAT block.

    Has special cases for 0 repetitions and 1 repetitions.

    Args:
        repetitions: The number of times the REPEAT block should repeat.

    Returns:
        repetitions=0: An empty circuit.
        repetitions=1: A copy of this circuit.
        repetitions>=2: A circuit with a single REPEAT block, where the contents of
            that repeat block are this circuit.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> 3 * c
        stim.Circuit('''
            REPEAT 3 {
                X 0
                Y 1 2
            }
        ''')
    """
```

<a name="stim.Circuit.__str__"></a>
```python
# stim.Circuit.__str__

# (in class stim.Circuit)
def __str__(
    self,
) -> str:
    """Returns stim instructions (that can be saved to a file and parsed by stim) for the current circuit.
    """
```

<a name="stim.Circuit.append"></a>
```python
# stim.Circuit.append

# (in class stim.Circuit)
@overload
def append(
    self,
    name: str,
    targets: Union[int, stim.GateTarget, Iterable[Union[int, stim.GateTarget]]],
    arg: Union[float, Iterable[float]],
) -> None:
    pass
@overload
def append(
    self,
    name: Union[stim.CircuitOperation, stim.CircuitRepeatBlock],
) -> None:
    pass
def append(
    self,
    name: object,
    targets: object = (),
    arg: object = None,
) -> None:
    """Appends an operation into the circuit.

    Note: `stim.Circuit.append_operation` is an alias of `stim.Circuit.append`.

    Examples:
        >>> import stim
        >>> c = stim.Circuit()
        >>> c.append("X", 0)
        >>> c.append("H", [0, 1])
        >>> c.append("M", [0, stim.target_inv(1)])
        >>> c.append("CNOT", [stim.target_rec(-1), 0])
        >>> c.append("X_ERROR", [0], 0.125)
        >>> c.append("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(2)], 0.25)
        >>> print(repr(c))
        stim.Circuit('''
            X 0
            H 0 1
            M 0 !1
            CX rec[-1] 0
            X_ERROR(0.125) 0
            E(0.25) X0 Y2
        ''')

    Args:
        name: The name of the operation's gate (e.g. "H" or "M" or "CNOT").

            This argument can also be set to a `stim.CircuitInstruction` or
            `stim.CircuitInstructionBlock`, which results in the instruction or
            block being appended to the circuit. The other arguments (targets
            and arg) can't be specified when doing so.

            (The argument being called `name` is no longer quite right, but
            is being kept for backwards compatibility.)
        targets: The objects operated on by the gate. This can be either a
            single target or an iterable of multiple targets to broadcast the
            gate over. Each target can be an integer (a qubit), a
            stim.GateTarget, or a special target from one of the `stim.target_*`
            methods (such as a measurement record target like `rec[-1]` from
            `stim.target_rec(-1)`).
        arg: The "parens arguments" for the gate, such as the probability for a
            noise operation. A double or list of doubles parameterizing the
            gate. Different gates take different parens arguments. For example,
            X_ERROR takes a probability, OBSERVABLE_INCLUDE takes an observable
            index, and PAULI_CHANNEL_1 takes three disjoint probabilities.

            Note: Defaults to no parens arguments. Except, for backwards
            compatibility reasons, `cirq.append_operation` (but not
            `cirq.append`) will default to a single 0.0 argument for gates that
            take exactly one argument.
    """
```

<a name="stim.Circuit.append_from_stim_program_text"></a>
```python
# stim.Circuit.append_from_stim_program_text

# (in class stim.Circuit)
def append_from_stim_program_text(
    self,
    stim_program_text: str,
) -> None:
    """Appends operations described by a STIM format program into the circuit.

    Examples:
        >>> import stim
        >>> c = stim.Circuit()
        >>> c.append_from_stim_program_text('''
        ...    H 0  # comment
        ...    CNOT 0 2
        ...
        ...    M 2
        ...    CNOT rec[-1] 1
        ... ''')
        >>> print(c)
        H 0
        CX 0 2
        M 2
        CX rec[-1] 1

    Args:
        stim_program_text: The STIM program text containing the circuit operations
            to append.
    """
```

<a name="stim.Circuit.approx_equals"></a>
```python
# stim.Circuit.approx_equals

# (in class stim.Circuit)
def approx_equals(
    self,
    other: object,
    *,
    atol: float,
) -> bool:
    """Checks if a circuit is approximately equal to another circuit.

    Two circuits are approximately equal if they are equal up to slight
    perturbations of instruction arguments such as probabilities. For example,
    `X_ERROR(0.100) 0` is approximately equal to `X_ERROR(0.099)` within an absolute
    tolerance of 0.002. All other details of the circuits (such as the ordering of
    instructions and targets) must be exactly the same.

    Args:
        other: The circuit, or other object, to compare to this one.
        atol: The absolute error tolerance. The maximum amount each probability may
            have been perturbed by.

    Returns:
        True if the given object is a circuit approximately equal up to the
        receiving circuit up to the given tolerance, otherwise False.

    Examples:
        >>> import stim
        >>> base = stim.Circuit('''
        ...    X_ERROR(0.099) 0 1 2
        ...    M 0 1 2
        ... ''')

        >>> base.approx_equals(base, atol=0)
        True

        >>> base.approx_equals(stim.Circuit('''
        ...    X_ERROR(0.101) 0 1 2
        ...    M 0 1 2
        ... '''), atol=0)
        False

        >>> base.approx_equals(stim.Circuit('''
        ...    X_ERROR(0.101) 0 1 2
        ...    M 0 1 2
        ... '''), atol=0.0001)
        False

        >>> base.approx_equals(stim.Circuit('''
        ...    X_ERROR(0.101) 0 1 2
        ...    M 0 1 2
        ... '''), atol=0.01)
        True

        >>> base.approx_equals(stim.Circuit('''
        ...    DEPOLARIZE1(0.099) 0 1 2
        ...    MRX 0 1 2
        ... '''), atol=9999)
        False
    """
```

<a name="stim.Circuit.clear"></a>
```python
# stim.Circuit.clear

# (in class stim.Circuit)
def clear(
    self,
) -> None:
    """Clears the contents of the circuit.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0
        ...    Y 1 2
        ... ''')
        >>> c.clear()
        >>> c
        stim.Circuit()
    """
```

<a name="stim.Circuit.compile_detector_sampler"></a>
```python
# stim.Circuit.compile_detector_sampler

# (in class stim.Circuit)
def compile_detector_sampler(
    self,
    *,
    seed: object = None,
) -> stim.CompiledDetectorSampler:
    """Returns an object that can batch sample detection events from the circuit.

    Args:
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how many
            shots are taken. For example, taking 10 shots and then 90 shots will
            give different results from taking 100 shots in one call.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    H 0
        ...    CNOT 0 1
        ...    M 0 1
        ...    DETECTOR rec[-1] rec[-2]
        ... ''')
        >>> s = c.compile_detector_sampler()
        >>> s.sample(shots=1)
        array([[False]])
    """
```

<a name="stim.Circuit.compile_m2d_converter"></a>
```python
# stim.Circuit.compile_m2d_converter

# (in class stim.Circuit)
def compile_m2d_converter(
    self,
    *,
    skip_reference_sample: bool = False,
) -> stim.CompiledMeasurementsToDetectionEventsConverter:
    """Creates a measurement-to-detection-event converter for the given circuit.

    The converter uses a noiseless reference sample, collected from the circuit
    using stim's Tableau simulator during initialization of the converter, as a
    baseline for determining what the expected value of a detector is.

    Note that the expected behavior of gauge detectors (detectors that are not
    actually deterministic under noiseless execution) can vary depending on the
    reference sample. Stim mitigates this by always generating the same reference
    sample for a given circuit.

    Args:
        skip_reference_sample: Defaults to False. When set to True, the reference
            sample used by the converter is initialized to all-zeroes instead of
            being collected from the circuit. This should only be used if it's known
            that the all-zeroes sample is actually a possible result from the
            circuit (under noiseless execution).

    Returns:
        An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> converter = stim.Circuit('''
        ...    X 0
        ...    M 0
        ...    DETECTOR rec[-1]
        ... ''').compile_m2d_converter()
        >>> converter.convert(
        ...     measurements=np.array([[0], [1]], dtype=np.bool_),
        ...     append_observables=False,
        ... )
        array([[ True],
               [False]])
    """
```

<a name="stim.Circuit.compile_sampler"></a>
```python
# stim.Circuit.compile_sampler

# (in class stim.Circuit)
def compile_sampler(
    self,
    *,
    skip_reference_sample: bool = False,
    seed: Optional[int] = None,
    reference_sample: Optional[np.ndarray] = None,
) -> stim.CompiledMeasurementSampler:
    """Returns an object that can quickly batch sample measurements from the circuit.

    Args:
        skip_reference_sample: Defaults to False. When set to True, the reference
            sample used by the sampler is initialized to all-zeroes instead of being
            collected from the circuit. This means that the results returned by the
            sampler are actually whether or not each measurement was *flipped*,
            instead of true measurement results.

            Forcing an all-zero reference sample is useful when you are only
            interested in error propagation and don't want to have to deal with the
            fact that some measurements want to be On when no errors occur. It is
            also useful when you know for sure that the all-zero result is actually
            a possible result from the circuit (under noiseless execution), meaning
            it is a valid reference sample as good as any other. Computing the
            reference sample is the most time consuming and memory intensive part of
            simulating the circuit, so promising that the simulator can safely skip
            that step is an effective optimization.
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how many
            shots are taken. For example, taking 10 shots and then 90 shots will
            give different results from taking 100 shots in one call.
        reference_sample: The data to xor into the measurement flips produced by the
            frame simulator, in order to produce proper measurement results.
            This can either be specified as an `np.bool_` array or a bit packed
            `np.uint8` array (little endian). Under normal conditions, the reference
            sample should be a valid noiseless sample of the circuit, such as the
            one returned by `circuit.reference_sample()`. If this argument is not
            provided, the reference sample will be set to
            `circuit.reference_sample()`, unless `skip_reference_sample=True`
            is used, in which case it will be set to all-zeros.

    Raises:
        ValueError: skip_reference_sample is True and reference_sample is not None.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 2
        ...    M 0 1 2
        ... ''')
        >>> s = c.compile_sampler()
        >>> s.sample(shots=1)
        array([[False, False,  True]])
    """
```

<a name="stim.Circuit.copy"></a>
```python
# stim.Circuit.copy

# (in class stim.Circuit)
def copy(
    self,
) -> stim.Circuit:
    """Returns a copy of the circuit. An independent circuit with the same contents.

    Examples:
        >>> import stim

        >>> c1 = stim.Circuit("H 0")
        >>> c2 = c1.copy()
        >>> c2 is c1
        False
        >>> c2 == c1
        True
    """
```

<a name="stim.Circuit.count_determined_measurements"></a>
```python
# stim.Circuit.count_determined_measurements

# (in class stim.Circuit)
def count_determined_measurements(
    self,
) -> int:
    """Counts the number of predictable measurements in the circuit.

    This method ignores any noise in the circuit.

    This method works by performing a tableau stabilizer simulation of the circuit
    and, before each measurement is simulated, checking if its expectation is
    non-zero.

    A measurement is predictable if its result can be predicted by using other
    measurements that have already been performed, assuming the circuit is executed
    without any noise.

    Note that, when multiple measurements occur at the same time, re-ordering the
    order they are resolved can change which specific measurements are predictable
    but won't change how many of them were predictable in total.

    The number of predictable measurements is a useful quantity because it's
    related to the number of detectors and observables that a circuit should
    declare. If circuit.num_detectors + circuit.num_observables is less than
    circuit.count_determined_measurements(), this is a warning sign that you've
    missed some detector declarations.

    The exact relationship between the number of determined measurements and the
    number of detectors and observables can differ from code to code. For example,
    the toric code has an extra redundant measurement compared to the surface code
    because in the toric code the last X stabilizer to be measured is equal to the
    product of all other X stabilizers even in the first round when initializing in
    the Z basis. Typically this relationship is not declared as a detector, because
    it's not local, or as an observable, because it doesn't store a qubit.

    Returns:
        The number of measurements that were predictable.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     R 0
        ...     M 0
        ... ''').count_determined_measurements()
        1

        >>> stim.Circuit('''
        ...     R 0
        ...     H 0
        ...     M 0
        ... ''').count_determined_measurements()
        0

        >>> stim.Circuit('''
        ...     R 0 1
        ...     MZZ 0 1
        ...     MYY 0 1
        ...     MXX 0 1
        ... ''').count_determined_measurements()
        2

        >>> circuit = stim.Circuit.generated(
        ...     "surface_code:rotated_memory_x",
        ...     distance=5,
        ...     rounds=9,
        ... )
        >>> circuit.count_determined_measurements()
        217
        >>> circuit.num_detectors + circuit.num_observables
        217
    """
```

<a name="stim.Circuit.detector_error_model"></a>
```python
# stim.Circuit.detector_error_model

# (in class stim.Circuit)
def detector_error_model(
    self,
    *,
    decompose_errors: bool = False,
    flatten_loops: bool = False,
    allow_gauge_detectors: bool = False,
    approximate_disjoint_errors: float = False,
    ignore_decomposition_failures: bool = False,
    block_decomposition_from_introducing_remnant_edges: bool = False,
) -> stim.DetectorErrorModel:
    """Returns a stim.DetectorErrorModel describing the error processes in the circuit.

    Args:
        decompose_errors: Defaults to false. When set to true, the error analysis
            attempts to decompose the components of composite error mechanisms (such
            as depolarization errors) into simpler errors, and suggest this
            decomposition via `stim.target_separator()` between the components. For
            example, in an XZ surface code, single qubit depolarization has a Y
            error term which can be decomposed into simpler X and Z error terms.
            Decomposition fails (causing this method to throw) if it's not possible
            to decompose large errors into simple errors that affect at most two
            detectors.
        flatten_loops: Defaults to false. When set to True, the output will not
            contain any `repeat` blocks. When set to False, the error analysis
            watches for loops in the circuit reaching a periodic steady state with
            respect to the detectors being introduced, the error mechanisms that
            affect them, and the locations of the logical observables. When it
            identifies such a steady state, it outputs a repeat block. This is
            massively more efficient than flattening for circuits that contain
            loops, but creates a more complex output.
        allow_gauge_detectors: Defaults to false. When set to false, the error
            analysis verifies that detectors in the circuit are actually
            deterministic under noiseless execution of the circuit. When set to
            True, these detectors are instead considered to be part of degrees
            freedom that can be removed from the error model. For example, if
            detectors D1 and D3 both anti-commute with a reset, then the error model
            has a gauge `error(0.5) D1 D3`. When gauges are identified, one of the
            involved detectors is removed from the system using Gaussian
            elimination.

            Note that logical observables are still verified to be deterministic,
            even if this option is set.
        approximate_disjoint_errors: Defaults to false. When set to false, composite
            error mechanisms with disjoint components (such as
            `PAULI_CHANNEL_1(0.1, 0.2, 0.0)`) can cause the error analysis to throw
            exceptions (because detector error models can only contain independent
            error mechanisms). When set to true, the probabilities of the disjoint
            cases are instead assumed to be independent probabilities. For example,
            a `PAULI_CHANNEL_1(0.1, 0.2, 0.0)` becomes equivalent to an
            `X_ERROR(0.1)` followed by a `Z_ERROR(0.2)`. This assumption is an
            approximation, but it is a good approximation for small probabilities.

            This argument can also be set to a probability between 0 and 1, setting
            a threshold below which the approximation is acceptable. Any error
            mechanisms that have a component probability above the threshold will
            cause an exception to be thrown.
        ignore_decomposition_failures: Defaults to False.
            When this is set to True, circuit errors that fail to decompose into
            graphlike detector error model errors no longer cause the conversion
            process to abort. Instead, the undecomposed error is inserted into the
            output. Whatever tool the detector error model is then given to is
            responsible for dealing with the undecomposed errors (e.g. a tool may
            choose to simply ignore them).

            Irrelevant unless decompose_errors=True.
        block_decomposition_from_introducing_remnant_edges: Defaults to False.
            Requires that both A B and C D be present elsewhere in the detector
            error model in order to decompose A B C D into A B ^ C D. Normally, only
            one of A B or C D needs to appear to allow this decomposition.

            Remnant edges can be a useful feature for ensuring decomposition
            succeeds, but they can also reduce the effective code distance by giving
            the decoder single edges that actually represent multiple errors in the
            circuit (resulting in the decoder making misinformed choices when
            decoding).

            Irrelevant unless decompose_errors=True.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     X_ERROR(0.125) 0
        ...     X_ERROR(0.25) 1
        ...     CORRELATED_ERROR(0.375) X0 X1
        ...     M 0 1
        ...     DETECTOR rec[-2]
        ...     DETECTOR rec[-1]
        ... ''').detector_error_model()
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.375) D0 D1
            error(0.25) D1
        ''')
    """
```

<a name="stim.Circuit.diagram"></a>
```python
# stim.Circuit.diagram

# (in class stim.Circuit)
@overload
def diagram(
    self,
    type: 'Literal["timeline-text"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["timeline-svg"]',
    *,
    tick: Union[None, int, range] = None,
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["timeline-3d", "timeline-3d-html"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-svg"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-3d"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-3d-html"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["detslice-text"]',
    *,
    tick: int,
    filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),),
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["detslice-svg"]',
    *,
    tick: Union[int, range],
    filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),),
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["detslice-with-ops-svg"]',
    *,
    tick: Union[int, range],
    filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),),
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["timeslice-svg"]',
    *,
    tick: Union[int, range],
    filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),),
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["interactive", "interactive-html"]',
) -> 'stim._DiagramHelper':
    pass
def diagram(
    self,
    type: str = 'timeline-text',
    *,
    tick: Union[None, int, range] = None,
    filter_coords: Iterable[Union[Iterable[float], stim.DemTarget]] = ((),),
) -> 'stim._DiagramHelper':
    """Returns a diagram of the circuit, from a variety of options.

    Args:
        type: The type of diagram. Available types are:
            "timeline-text" (default): An ASCII diagram of the
                operations applied by the circuit over time. Includes
                annotations showing the measurement record index that
                each measurement writes to, and the measurements used
                by detectors.
            "timeline-svg": An SVG image of the operations applied by
                the circuit over time. Includes annotations showing the
                measurement record index that each measurement writes
                to, and the measurements used by detectors.
            "timeline-3d": A 3d model, in GLTF format, of the operations
                applied by the circuit over time.
            "timeline-3d-html": Same 3d model as 'timeline-3d' but
                embedded into an HTML web page containing an interactive
                THREE.js viewer for the 3d model.
            "detslice-text": An ASCII diagram of the stabilizers
                that detectors declared by the circuit correspond to
                during the TICK instruction identified by the `tick`
                argument.
            "detslice-svg": An SVG image of the stabilizers
                that detectors declared by the circuit correspond to
                during the TICK instruction identified by the `tick`
                argument. For example, a detector slice diagram of a
                CSS surface code circuit during the TICK between a
                measurement layer and a reset layer will produce the
                usual diagram of a surface code.

                Uses the Pauli color convention XYZ=RGB.
            "matchgraph-svg": An SVG image of the match graph extracted
                from the circuit by stim.Circuit.detector_error_model.
            "matchgraph-3d": An 3D model of the match graph extracted
                from the circuit by stim.Circuit.detector_error_model.
            "matchgraph-3d-html": Same 3d model as 'match-graph-3d' but
                embedded into an HTML web page containing an interactive
                THREE.js viewer for the 3d model.
            "timeslice-svg": An SVG image of the operations applied
                between two TICK instructions in the circuit, with the
                operations laid out in 2d.
            "detslice-with-ops-svg": A combination of timeslice-svg
                and detslice-svg, with the operations overlaid
                over the detector slices taken from the TICK after the
                operations were applied.
            "interactive" or "interactive-html": An HTML web page
                containing Crumble (an interactive editor for 2D
                stabilizer circuits) initialized with the given circuit
                as its default contents.
        tick: Required for detector and time slice diagrams. Specifies
            which TICK instruction, or range of TICK instructions, to
            slice at. Note that the first TICK instruction in the
            circuit corresponds tick=1. The value tick=0 refers to the
            very start of the circuit.

            Passing `range(A, B)` for a detector slice will show the
            slices for ticks A through B including A but excluding B.

            Passing `range(A, B)` for a time slice will show the
            operations between tick A and tick B.
        filter_coords: A set of acceptable coordinate prefixes, or
            desired stim.DemTargets. For detector slice diagrams, only
            detectors match one of the filters are included. If no filter
            is specified, all detectors are included (but no observables).
            To include an observable, add it as one of the filters.

    Returns:
        An object whose `__str__` method returns the diagram, so that
        writing the diagram to a file works correctly. The returned
        object may also define methods such as `_repr_html_`, so that
        ipython notebooks recognize it can be shown using a specialized
        viewer instead of as raw text.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1 1 2
        ... ''')

        >>> print(circuit.diagram())
        q0: -H-@---
               |
        q1: ---X-@-
                 |
        q2: -----X-

        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1
        ...     TICK
        ...     M 0 1
        ...     DETECTOR rec[-1] rec[-2]
        ... ''')

        >>> print(circuit.diagram("detslice-text", tick=1))
        q0: -Z:D0-
             |
        q1: -Z:D0-
    """
```

<a name="stim.Circuit.explain_detector_error_model_errors"></a>
```python
# stim.Circuit.explain_detector_error_model_errors

# (in class stim.Circuit)
def explain_detector_error_model_errors(
    self,
    *,
    dem_filter: object = None,
    reduce_to_one_representative_error: bool = False,
) -> List[stim.ExplainedError]:
    """Explains how detector error model errors are produced by circuit errors.

    Args:
        dem_filter: Defaults to None (unused). When used, the output will only
            contain detector error model errors that appear in the given
            `stim.DetectorErrorModel`. Any error mechanisms from the detector error
            model that can't be reproduced using one error from the circuit will
            also be included in the result, but with an empty list of associated
            circuit error mechanisms.
        reduce_to_one_representative_error: Defaults to False. When True, the items
            in the result will contain at most one circuit error mechanism.

    Returns:
        A `List[stim.ExplainedError]` (see `stim.ExplainedError` for more
        information). Each item in the list describes how a detector error model
        error can be produced by individual circuit errors.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     # Create Bell pair.
        ...     H 0
        ...     CNOT 0 1
        ...
        ...     # Noise.
        ...     DEPOLARIZE1(0.01) 0
        ...
        ...     # Bell basis measurement.
        ...     CNOT 0 1
        ...     H 0
        ...     M 0 1
        ...
        ...     # Both measurements should be False under noiseless execution.
        ...     DETECTOR rec[-1]
        ...     DETECTOR rec[-2]
        ... ''')
        >>> explained_errors = circuit.explain_detector_error_model_errors(
        ...     dem_filter=stim.DetectorErrorModel('error(1) D0 D1'),
        ...     reduce_to_one_representative_error=True,
        ... )
        >>> print(explained_errors[0].circuit_error_locations[0])
        CircuitErrorLocation {
            flipped_pauli_product: Y0
            Circuit location stack trace:
                (after 0 TICKs)
                at instruction #3 (DEPOLARIZE1) in the circuit
                at target #1 of the instruction
                resolving to DEPOLARIZE1(0.01) 0
        }
    """
```

<a name="stim.Circuit.flattened"></a>
```python
# stim.Circuit.flattened

# (in class stim.Circuit)
def flattened(
    self,
) -> stim.Circuit:
    """Creates an equivalent circuit without REPEAT or SHIFT_COORDS.

    Returns:
        A `stim.Circuit` with the same instructions in the same order,
        but with loops flattened into repeated instructions and with
        all coordinate shifts inlined.

    Examples:
        >>> import stim
        >>> stim.Circuit('''
        ...     REPEAT 5 {
        ...         MR 0 1
        ...         DETECTOR(0, 0) rec[-2]
        ...         DETECTOR(1, 0) rec[-1]
        ...         SHIFT_COORDS(0, 1)
        ...     }
        ... ''').flattened()
        stim.Circuit('''
            MR 0 1
            DETECTOR(0, 0) rec[-2]
            DETECTOR(1, 0) rec[-1]
            MR 0 1
            DETECTOR(0, 1) rec[-2]
            DETECTOR(1, 1) rec[-1]
            MR 0 1
            DETECTOR(0, 2) rec[-2]
            DETECTOR(1, 2) rec[-1]
            MR 0 1
            DETECTOR(0, 3) rec[-2]
            DETECTOR(1, 3) rec[-1]
            MR 0 1
            DETECTOR(0, 4) rec[-2]
            DETECTOR(1, 4) rec[-1]
        ''')
    """
```

<a name="stim.Circuit.from_file"></a>
```python
# stim.Circuit.from_file

# (in class stim.Circuit)
@staticmethod
def from_file(
    file: Union[io.TextIOBase, str, pathlib.Path],
) -> stim.Circuit:
    """Reads a stim circuit from a file.

    The file format is defined at
    https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

    Args:
        file: A file path or open file object to read from.

    Returns:
        The circuit parsed from the file.

    Examples:
        >>> import stim
        >>> import tempfile

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         print('H 5', file=f)
        ...     circuit = stim.Circuit.from_file(path)
        >>> circuit
        stim.Circuit('''
            H 5
        ''')

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         print('CNOT 4 5', file=f)
        ...     with open(path) as f:
        ...         circuit = stim.Circuit.from_file(path)
        >>> circuit
        stim.Circuit('''
            CX 4 5
        ''')
    """
```

<a name="stim.Circuit.generated"></a>
```python
# stim.Circuit.generated

# (in class stim.Circuit)
@staticmethod
def generated(
    code_task: str,
    *,
    distance: int,
    rounds: int,
    after_clifford_depolarization: float = 0.0,
    before_round_data_depolarization: float = 0.0,
    before_measure_flip_probability: float = 0.0,
    after_reset_flip_probability: float = 0.0,
) -> stim.Circuit:
    """Generates common circuits.

    The generated circuits can include configurable noise.

    The generated circuits include DETECTOR and OBSERVABLE_INCLUDE annotations so
    that their detection events and logical observables can be sampled.

    The generated circuits include TICK annotations to mark the progression of time.
    (E.g. so that converting them using `stimcirq.stim_circuit_to_cirq_circuit` will
    produce a `cirq.Circuit` with the intended moment structure.)

    Args:
        code_task: A string identifying the type of circuit to generate. Available
            code tasks are:
                - "repetition_code:memory"
                - "surface_code:rotated_memory_x"
                - "surface_code:rotated_memory_z"
                - "surface_code:unrotated_memory_x"
                - "surface_code:unrotated_memory_z"
                - "color_code:memory_xyz"
        distance: The desired code distance of the generated circuit. The code
            distance is the minimum number of physical errors needed to cause a
            logical error. This parameter indirectly determines how many qubits the
            generated circuit uses.
        rounds: How many times the measurement qubits in the generated circuit will
            be measured. Indirectly determines the duration of the generated
            circuit.
        after_clifford_depolarization: Defaults to 0. The probability (p) of
            `DEPOLARIZE1(p)` operations to add after every single-qubit Clifford
            operation and `DEPOLARIZE2(p)` operations to add after every two-qubit
            Clifford operation. The after-Clifford depolarizing operations are only
            included if this probability is not 0.
        before_round_data_depolarization: Defaults to 0. The probability (p) of
            `DEPOLARIZE1(p)` operations to apply to every data qubit at the start of
            a round of stabilizer measurements. The start-of-round depolarizing
            operations are only included if this probability is not 0.
        before_measure_flip_probability: Defaults to 0. The probability (p) of
            `X_ERROR(p)` operations applied to qubits before each measurement (X
            basis measurements use `Z_ERROR(p)` instead). The before-measurement
            flips are only included if this probability is not 0.
        after_reset_flip_probability: Defaults to 0. The probability (p) of
            `X_ERROR(p)` operations applied to qubits after each reset (X basis
            resets use `Z_ERROR(p)` instead). The after-reset flips are only
            included if this probability is not 0.

    Returns:
        The generated circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit.generated(
        ...     "repetition_code:memory",
        ...     distance=4,
        ...     rounds=10000,
        ...     after_clifford_depolarization=0.0125)
        >>> print(circuit)
        R 0 1 2 3 4 5 6
        TICK
        CX 0 1 2 3 4 5
        DEPOLARIZE2(0.0125) 0 1 2 3 4 5
        TICK
        CX 2 1 4 3 6 5
        DEPOLARIZE2(0.0125) 2 1 4 3 6 5
        TICK
        MR 1 3 5
        DETECTOR(1, 0) rec[-3]
        DETECTOR(3, 0) rec[-2]
        DETECTOR(5, 0) rec[-1]
        REPEAT 9999 {
            TICK
            CX 0 1 2 3 4 5
            DEPOLARIZE2(0.0125) 0 1 2 3 4 5
            TICK
            CX 2 1 4 3 6 5
            DEPOLARIZE2(0.0125) 2 1 4 3 6 5
            TICK
            MR 1 3 5
            SHIFT_COORDS(0, 1)
            DETECTOR(1, 0) rec[-3] rec[-6]
            DETECTOR(3, 0) rec[-2] rec[-5]
            DETECTOR(5, 0) rec[-1] rec[-4]
        }
        M 0 2 4 6
        DETECTOR(1, 1) rec[-3] rec[-4] rec[-7]
        DETECTOR(3, 1) rec[-2] rec[-3] rec[-6]
        DETECTOR(5, 1) rec[-1] rec[-2] rec[-5]
        OBSERVABLE_INCLUDE(0) rec[-1]
    """
```

<a name="stim.Circuit.get_detector_coordinates"></a>
```python
# stim.Circuit.get_detector_coordinates

# (in class stim.Circuit)
def get_detector_coordinates(
    self,
    only: object = None,
) -> Dict[int, List[float]]:
    """Returns the coordinate metadata of detectors in the circuit.

    Args:
        only: Defaults to None (meaning include all detectors). A list of detector
            indices to include in the result. Detector indices beyond the end of the
            detector error model of the circuit cause an error.

    Returns:
        A dictionary mapping integers (detector indices) to lists of floats
        (coordinates).

        Detectors with no specified coordinate data are mapped to an empty tuple.
        If `only` is specified, then `set(result.keys()) == set(only)`.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...    M 0
        ...    DETECTOR rec[-1]
        ...    DETECTOR(1, 2, 3) rec[-1]
        ...    REPEAT 3 {
        ...        DETECTOR(42) rec[-1]
        ...        SHIFT_COORDS(100)
        ...    }
        ... ''')
        >>> circuit.get_detector_coordinates()
        {0: [], 1: [1.0, 2.0, 3.0], 2: [42.0], 3: [142.0], 4: [242.0]}
        >>> circuit.get_detector_coordinates(only=[1])
        {1: [1.0, 2.0, 3.0]}
    """
```

<a name="stim.Circuit.get_final_qubit_coordinates"></a>
```python
# stim.Circuit.get_final_qubit_coordinates

# (in class stim.Circuit)
def get_final_qubit_coordinates(
    self,
) -> Dict[int, List[float]]:
    """Returns the coordinate metadata of qubits in the circuit.

    If a qubit's coordinates are specified multiple times, only the last specified
    coordinates are returned.

    Returns:
        A dictionary mapping qubit indices (integers) to coordinates (lists of
        floats). Qubits that never had their coordinates specified are not included
        in the result.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...    QUBIT_COORDS(1, 2, 3) 1
        ... ''')
        >>> circuit.get_final_qubit_coordinates()
        {1: [1.0, 2.0, 3.0]}
    """
```

<a name="stim.Circuit.inverse"></a>
```python
# stim.Circuit.inverse

# (in class stim.Circuit)
def inverse(
    self,
) -> stim.Circuit:
    """Returns a circuit that applies the same operations but inverted and in reverse.

    If circuit starts with QUBIT_COORDS instructions, the returned circuit will
    still have the same QUBIT_COORDS instructions in the same order at the start.

    Returns:
        A `stim.Circuit` that applies inverted operations in the reverse order.

    Raises:
        ValueError: The circuit contains operations that don't have an inverse,
            such as measurements. There are also some unsupported operations
            such as SHIFT_COORDS.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     S 0 1
        ...     ISWAP 0 1 1 2
        ... ''').inverse()
        stim.Circuit('''
            ISWAP_DAG 1 2 0 1
            S_DAG 1 0
        ''')

        >>> stim.Circuit('''
        ...     QUBIT_COORDS(1, 2) 0
        ...     QUBIT_COORDS(4, 3) 1
        ...     QUBIT_COORDS(9, 5) 2
        ...     H 0 1
        ...     REPEAT 100 {
        ...         CX 0 1 1 2
        ...         TICK
        ...         S 1 2
        ...     }
        ... ''').inverse()
        stim.Circuit('''
            QUBIT_COORDS(1, 2) 0
            QUBIT_COORDS(4, 3) 1
            QUBIT_COORDS(9, 5) 2
            REPEAT 100 {
                S_DAG 2 1
                TICK
                CX 1 2 0 1
            }
            H 1 0
        ''')
    """
```

<a name="stim.Circuit.num_detectors"></a>
```python
# stim.Circuit.num_detectors

# (in class stim.Circuit)
@property
def num_detectors(
    self,
) -> int:
    """Counts the number of bits produced when sampling the circuit's detectors.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    M 0
        ...    DETECTOR rec[-1]
        ...    REPEAT 100 {
        ...        M 0 1 2
        ...        DETECTOR rec[-1]
        ...        DETECTOR rec[-2]
        ...    }
        ... ''')
        >>> c.num_detectors
        201
    """
```

<a name="stim.Circuit.num_measurements"></a>
```python
# stim.Circuit.num_measurements

# (in class stim.Circuit)
@property
def num_measurements(
    self,
) -> int:
    """Counts the number of bits produced when sampling the circuit's measurements.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    M 0
        ...    REPEAT 100 {
        ...        M 0 1
        ...    }
        ... ''')
        >>> c.num_measurements
        201
    """
```

<a name="stim.Circuit.num_observables"></a>
```python
# stim.Circuit.num_observables

# (in class stim.Circuit)
@property
def num_observables(
    self,
) -> int:
    """Counts the number of logical observables defined by the circuit.

    This is one more than the largest index that appears as an argument to an
    OBSERVABLE_INCLUDE instruction.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    M 0
        ...    OBSERVABLE_INCLUDE(2) rec[-1]
        ...    OBSERVABLE_INCLUDE(5) rec[-1]
        ... ''')
        >>> c.num_observables
        6
    """
```

<a name="stim.Circuit.num_qubits"></a>
```python
# stim.Circuit.num_qubits

# (in class stim.Circuit)
@property
def num_qubits(
    self,
) -> int:
    """Counts the number of qubits used when simulating the circuit.

    This is always one more than the largest qubit index used by the circuit.

    Examples:
        >>> import stim
        >>> stim.Circuit('''
        ...    X 0
        ...    M 0 1
        ... ''').num_qubits
        2
        >>> stim.Circuit('''
        ...    X 0
        ...    M 0 1
        ...    H 100
        ... ''').num_qubits
        101
    """
```

<a name="stim.Circuit.num_sweep_bits"></a>
```python
# stim.Circuit.num_sweep_bits

# (in class stim.Circuit)
@property
def num_sweep_bits(
    self,
) -> int:
    """Returns the number of sweep bits needed to completely configure the circuit.

    This is always one more than the largest sweep bit index used by the circuit.

    Examples:
        >>> import stim
        >>> stim.Circuit('''
        ...    CX sweep[2] 0
        ... ''').num_sweep_bits
        3
        >>> stim.Circuit('''
        ...    CZ sweep[5] 0
        ...    CX sweep[2] 0
        ... ''').num_sweep_bits
        6
    """
```

<a name="stim.Circuit.num_ticks"></a>
```python
# stim.Circuit.num_ticks

# (in class stim.Circuit)
@property
def num_ticks(
    self,
) -> int:
    """Counts the number of TICK instructions executed when running the circuit.

    TICKs in loops are counted once per iteration.

    Returns:
        The number of ticks executed by the circuit.

    Examples:
        >>> import stim

        >>> stim.Circuit().num_ticks
        0

        >>> stim.Circuit('''
        ...    TICK
        ... ''').num_ticks
        1

        >>> stim.Circuit('''
        ...    H 0
        ...    TICK
        ...    CX 0 1
        ...    TICK
        ... ''').num_ticks
        2

        >>> stim.Circuit('''
        ...    H 0
        ...    TICK
        ...    REPEAT 100 {
        ...        CX 0 1
        ...        TICK
        ...    }
        ... ''').num_ticks
        101
    """
```

<a name="stim.Circuit.reference_sample"></a>
```python
# stim.Circuit.reference_sample

# (in class stim.Circuit)
def reference_sample(
    self,
    *,
    bit_packed: bool = False,
) -> np.ndarray:
    """Samples the given circuit in a deterministic fashion.

    Discards all noisy operations, and biases all collapse events
    towards +Z instead of randomly +Z/-Z.

    Args:
        circuit: The circuit to "sample" from.
        bit_packed: Defaults to False. Determines whether the output numpy arrays
            use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

    Returns:
        reference_sample: reference sample sampled from the given circuit.
    """
```

<a name="stim.Circuit.search_for_undetectable_logical_errors"></a>
```python
# stim.Circuit.search_for_undetectable_logical_errors

# (in class stim.Circuit)
def search_for_undetectable_logical_errors(
    self,
    *,
    dont_explore_detection_event_sets_with_size_above: int,
    dont_explore_edges_with_degree_above: int,
    dont_explore_edges_increasing_symptom_degree: bool,
    canonicalize_circuit_errors: bool = False,
) -> List[stim.ExplainedError]:
    """Searches for small sets of errors that form an undetectable logical error.

    THIS IS A HEURISTIC METHOD. It does not guarantee that it will find errors of
    particular sizes, or with particular properties. The errors it finds are a
    tangled combination of the truncation parameters you specify, internal
    optimizations which are correct when not truncating, and minutia of the circuit
    being considered.

    If you want a well behaved method that does provide guarantees of finding errors
    of a particular type, use `stim.Circuit.shortest_graphlike_error`. This method
    is more thorough than that (assuming you don't truncate so hard you omit
    graphlike edges), but exactly how thorough is difficult to describe. It's also
    not guaranteed that the behavior of this method will not be changed in the
    future in a way that permutes which logical errors are found and which are
    missed.

    This search method considers hyper errors, so it has worst case exponential
    runtime. It is important to carefully consider the arguments you are providing,
    which truncate the search space and trade cost for quality.

    The search progresses by starting from each error that crosses a logical
    observable, noting which detection events each error produces, and then
    iteratively adding in errors touching those detection events attempting to
    cancel out the detection event with the lowest index.

    Beware that the choice of logical observable can interact with the truncation
    options. Using different observables can change whether or not the search
    succeeds, even if those observables are equal modulo the stabilizers of the
    code. This is because the edges crossing logical observables are used as
    starting points for the search, and starting from different places along a path
    will result in different numbers of symptoms in intermediate states as the
    search progresses. For example, if the logical observable is next to a boundary,
    then the starting edges are likely boundary edges (degree 1) with 'room to
    grow', whereas if the observable was running through the bulk then the starting
    edges will have degree at least 2.

    Args:
        dont_explore_detection_event_sets_with_size_above: Truncates the search
            space by refusing to cross an edge (i.e. add an error) when doing so
            would produce an intermediate state that has more detection events than
            this limit.
        dont_explore_edges_with_degree_above: Truncates the search space by refusing
            to consider errors that cause a lot of detection events. For example,
            you may only want to consider graphlike errors which have two or fewer
            detection events.
        dont_explore_edges_increasing_symptom_degree: Truncates the search space by
            refusing to cross an edge (i.e. add an error) when doing so would
            produce an intermediate state that has more detection events that the
            previous intermediate state. This massively improves the efficiency of
            the search because instead of, for example, exploring all n^4 possible
            detection event sets with 4 symptoms, the search will attempt to cancel
            out symptoms one by one.
        canonicalize_circuit_errors: Whether or not to use one representative for
            equal-symptom circuit errors.

            False (default): Each DEM error lists every possible circuit error that
                single handedly produces those symptoms as a potential match. This
                is verbose but gives complete information.
            True: Each DEM error is matched with one possible circuit error that
                single handedly produces those symptoms, with a preference towards
                errors that are simpler (e.g. apply Paulis to fewer qubits). This
                discards mostly-redundant information about different ways to
                produce the same symptoms in order to give a succinct result.

    Returns:
        A list of error mechanisms that cause an undetected logical error.

        Each entry in the list is a `stim.ExplainedError` detailing the location
        and effects of a single physical error. The effects of the entire list
        combine to produce a logical frame change without any detection events.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit.generated(
        ...     "surface_code:rotated_memory_x",
        ...     rounds=5,
        ...     distance=5,
        ...     after_clifford_depolarization=0.001)
        >>> print(len(circuit.search_for_undetectable_logical_errors(
        ...     dont_explore_detection_event_sets_with_size_above=4,
        ...     dont_explore_edges_with_degree_above=4,
        ...     dont_explore_edges_increasing_symptom_degree=True,
        ... )))
        5
    """
```

<a name="stim.Circuit.shortest_graphlike_error"></a>
```python
# stim.Circuit.shortest_graphlike_error

# (in class stim.Circuit)
def shortest_graphlike_error(
    self,
    *,
    ignore_ungraphlike_errors: bool = True,
    canonicalize_circuit_errors: bool = False,
) -> List[stim.ExplainedError]:
    """Finds a minimum set of graphlike errors to produce an undetected logical error.

    A "graphlike error" is an error that creates at most two detection events
    (causes a change in the parity of the measurement sets of at most two DETECTOR
    annotations).

    Note that this method does not pay attention to error probabilities (other than
    ignoring errors with probability 0). It searches for a logical error with the
    minimum *number* of physical errors, not the maximum probability of those
    physical errors all occurring.

    This method works by converting the circuit into a `stim.DetectorErrorModel`
    using `circuit.detector_error_model(...)`, computing the shortest graphlike
    error of the error model, and then converting the physical errors making up that
    logical error back into representative circuit errors.

    Args:
        ignore_ungraphlike_errors:
            False: Attempt to decompose any ungraphlike errors in the circuit into
                graphlike parts. If this fails, raise an exception instead of
                continuing.

                Note: in some cases, graphlike errors only appear as parts of
                decomposed ungraphlike errors. This can produce a result that lists
                DEM errors with zero matching circuit errors, because the only way
                to achieve those errors is by combining a decomposed error with a
                graphlike error. As a result, when using this option it is NOT
                guaranteed that the length of the result is an upper bound on the
                true code distance. That is only the case if every item in the
                result lists at least one matching circuit error.
            True (default): Ungraphlike errors are simply skipped as if they weren't
                present, even if they could become graphlike if decomposed. This
                guarantees the length of the result is an upper bound on the true
                code distance.
        canonicalize_circuit_errors: Whether or not to use one representative for
            equal-symptom circuit errors.

            False (default): Each DEM error lists every possible circuit error that
                single handedly produces those symptoms as a potential match. This
                is verbose but gives complete information.
            True: Each DEM error is matched with one possible circuit error that
                single handedly produces those symptoms, with a preference towards
                errors that are simpler (e.g. apply Paulis to fewer qubits). This
                discards mostly-redundant information about different ways to
                produce the same symptoms in order to give a succinct result.

    Returns:
        A list of error mechanisms that cause an undetected logical error.

        Each entry in the list is a `stim.ExplainedError` detailing the location
        and effects of a single physical error. The effects of the entire list
        combine to produce a logical frame change without any detection events.

    Examples:
        >>> import stim

        >>> circuit = stim.Circuit.generated(
        ...     "repetition_code:memory",
        ...     rounds=10,
        ...     distance=7,
        ...     before_round_data_depolarization=0.01)
        >>> len(circuit.shortest_graphlike_error())
        7
    """
```

<a name="stim.Circuit.to_file"></a>
```python
# stim.Circuit.to_file

# (in class stim.Circuit)
def to_file(
    self,
    file: Union[io.TextIOBase, str, pathlib.Path],
) -> None:
    """Writes the stim circuit to a file.

    The file format is defined at
    https://github.com/quantumlib/Stim/blob/main/doc/file_format_stim_circuit.md

    Args:
        file: A file path or an open file to write to.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> c = stim.Circuit('H 5\nX 0')

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         c.to_file(f)
        ...     with open(path) as f:
        ...         contents = f.read()
        >>> contents
        'H 5\nX 0\n'

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     c.to_file(path)
        ...     with open(path) as f:
        ...         contents = f.read()
        >>> contents
        'H 5\nX 0\n'
    """
```

<a name="stim.Circuit.with_inlined_feedback"></a>
```python
# stim.Circuit.with_inlined_feedback

# (in class stim.Circuit)
def with_inlined_feedback(
    self,
) -> stim.Circuit:
    """Returns a circuit without feedback with rewritten detectors/observables.

    When a feedback operation affects the expected parity of a detector or
    observable, the measurement controlling that feedback operation is implicitly
    part of the measurement set that defines the detector or observable. This
    method removes all feedback, but avoids changing the meaning of detectors or
    observables by turning these implicit measurement dependencies into explicit
    measurement dependencies added to the observable or detector.

    This method guarantees that the detector error model derived from the original
    circuit, and the transformed circuit, will be equivalent (modulo floating point
    rounding errors and variations in where loops are placed). Specifically, the
    following should be true for any circuit:

        dem1 = circuit.flattened().detector_error_model()
        dem2 = circuit.with_inlined_feedback().flattened().detector_error_model()
        assert dem1.approx_equals(dem2, 1e-5)

    Returns:
        A `stim.Circuit` with feedback operations removed, with rewritten DETECTOR
        instructions (as needed to avoid changing the meaning of each detector), and
        with additional OBSERVABLE_INCLUDE instructions (as needed to avoid changing
        the meaning of each observable).

        The circuit's function is permitted to differ from the original in that
        any feedback operation can be pushed to the end of the circuit and
        discarded. All non-feedback operations must stay where they are, preserving
        the structure of the circuit.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     CX 0 1        # copy to measure qubit
        ...     M 1           # measure first time
        ...     CX rec[-1] 1  # use feedback to reset measurement qubit
        ...     CX 0 1        # copy to measure qubit
        ...     M 1           # measure second time
        ...     DETECTOR rec[-1] rec[-2]
        ...     OBSERVABLE_INCLUDE(0) rec[-1]
        ... ''').with_inlined_feedback()
        stim.Circuit('''
            CX 0 1
            M 1
            OBSERVABLE_INCLUDE(0) rec[-1]
            CX 0 1
            M 1
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
        ''')
    """
```

<a name="stim.Circuit.without_noise"></a>
```python
# stim.Circuit.without_noise

# (in class stim.Circuit)
def without_noise(
    self,
) -> stim.Circuit:
    """Returns a copy of the circuit with all noise processes removed.

    Pure noise instructions, such as X_ERROR and DEPOLARIZE2, are not
    included in the result.

    Noisy measurement instructions, like `M(0.001)`, have their noise
    parameter removed.

    Returns:
        A `stim.Circuit` with the same instructions except all noise
        processes have been removed.

    Examples:
        >>> import stim
        >>> stim.Circuit('''
        ...     X_ERROR(0.25) 0
        ...     CNOT 0 1
        ...     M(0.125) 0
        ... ''').without_noise()
        stim.Circuit('''
            CX 0 1
            M 0
        ''')
    """
```

<a name="stim.CircuitErrorLocation"></a>
```python
# stim.CircuitErrorLocation

# (at top-level in the stim module)
class CircuitErrorLocation:
    """Describes the location of an error mechanism from a stim circuit.
    """
```

<a name="stim.CircuitErrorLocation.__init__"></a>
```python
# stim.CircuitErrorLocation.__init__

# (in class stim.CircuitErrorLocation)
def __init__(
    self,
    *,
    tick_offset: int,
    flipped_pauli_product: List[stim.GateTargetWithCoords],
    flipped_measurement: object,
    instruction_targets: stim.CircuitTargetsInsideInstruction,
    stack_frames: List[stim.CircuitErrorLocationStackFrame],
) -> None:
    """Creates a stim.CircuitErrorLocation.
    """
```

<a name="stim.CircuitErrorLocation.flipped_measurement"></a>
```python
# stim.CircuitErrorLocation.flipped_measurement

# (in class stim.CircuitErrorLocation)
@property
def flipped_measurement(
    self,
) -> Optional[stim.FlippedMeasurement]:
    """The measurement that was flipped by the error mechanism.
    If the error isn't a measurement error, this will be None.
    """
```

<a name="stim.CircuitErrorLocation.flipped_pauli_product"></a>
```python
# stim.CircuitErrorLocation.flipped_pauli_product

# (in class stim.CircuitErrorLocation)
@property
def flipped_pauli_product(
    self,
) -> List[stim.GateTargetWithCoords]:
    """The Pauli errors that the error mechanism applied to qubits.
    When the error is a measurement error, this will be an empty list.
    """
```

<a name="stim.CircuitErrorLocation.instruction_targets"></a>
```python
# stim.CircuitErrorLocation.instruction_targets

# (in class stim.CircuitErrorLocation)
@property
def instruction_targets(
    self,
) -> stim.CircuitTargetsInsideInstruction:
    """Within the error instruction, which may have hundreds of
    targets, which specific targets were being executed to
    produce the error.
    """
```

<a name="stim.CircuitErrorLocation.stack_frames"></a>
```python
# stim.CircuitErrorLocation.stack_frames

# (in class stim.CircuitErrorLocation)
@property
def stack_frames(
    self,
) -> List[stim.CircuitErrorLocationStackFrame]:
    """Where in the circuit's execution does the error mechanism occur,
    accounting for things like nested loops that iterate multiple times.
    """
```

<a name="stim.CircuitErrorLocation.tick_offset"></a>
```python
# stim.CircuitErrorLocation.tick_offset

# (in class stim.CircuitErrorLocation)
@property
def tick_offset(
    self,
) -> int:
    """The number of TICKs that executed before the error mechanism being discussed,
    including TICKs that occurred multiple times during loops.
    """
```

<a name="stim.CircuitErrorLocationStackFrame"></a>
```python
# stim.CircuitErrorLocationStackFrame

# (at top-level in the stim module)
class CircuitErrorLocationStackFrame:
    """Describes the location of an instruction being executed within a
    circuit or loop, distinguishing between separate loop iterations.

    The full location of an instruction is a list of these frames,
    drilling down from the top level circuit to the inner-most loop
    that the instruction is within.
    """
```

<a name="stim.CircuitErrorLocationStackFrame.__init__"></a>
```python
# stim.CircuitErrorLocationStackFrame.__init__

# (in class stim.CircuitErrorLocationStackFrame)
def __init__(
    self,
    *,
    instruction_offset: int,
    iteration_index: int,
    instruction_repetitions_arg: int,
) -> None:
    """Creates a stim.CircuitErrorLocationStackFrame.
    """
```

<a name="stim.CircuitErrorLocationStackFrame.instruction_offset"></a>
```python
# stim.CircuitErrorLocationStackFrame.instruction_offset

# (in class stim.CircuitErrorLocationStackFrame)
@property
def instruction_offset(
    self,
) -> int:
    """The index of the instruction within the circuit, or within the
    instruction's parent REPEAT block. This is slightly different
    from the line number, because blank lines and commented lines
    don't count and also because the offset of the first instruction
    is 0 instead of 1.
    """
```

<a name="stim.CircuitErrorLocationStackFrame.instruction_repetitions_arg"></a>
```python
# stim.CircuitErrorLocationStackFrame.instruction_repetitions_arg

# (in class stim.CircuitErrorLocationStackFrame)
@property
def instruction_repetitions_arg(
    self,
) -> int:
    """If the instruction being referred to is a REPEAT block,
    this is the repetition count of that REPEAT block. Otherwise
    this field defaults to 0.
    """
```

<a name="stim.CircuitErrorLocationStackFrame.iteration_index"></a>
```python
# stim.CircuitErrorLocationStackFrame.iteration_index

# (in class stim.CircuitErrorLocationStackFrame)
@property
def iteration_index(
    self,
) -> int:
    """Disambiguates which iteration of the loop containing this instruction
    is being referred to. If the instruction isn't in a REPEAT block, this
    field defaults to 0.
    """
```

<a name="stim.CircuitInstruction"></a>
```python
# stim.CircuitInstruction

# (at top-level in the stim module)
class CircuitInstruction:
    """An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     M 0 1
        ...     X_ERROR(0.125) 5
        ... ''')
        >>> circuit[0]
        stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
        >>> circuit[1]
        stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(1)], [])
        >>> circuit[2]
        stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5)], [0.125])
    """
```

<a name="stim.CircuitInstruction.__eq__"></a>
```python
# stim.CircuitInstruction.__eq__

# (in class stim.CircuitInstruction)
def __eq__(
    self,
    arg0: stim.CircuitInstruction,
) -> bool:
    """Determines if two `stim.CircuitInstruction`s are identical.
    """
```

<a name="stim.CircuitInstruction.__init__"></a>
```python
# stim.CircuitInstruction.__init__

# (in class stim.CircuitInstruction)
def __init__(
    self,
    name: str,
    targets: List[object],
    gate_args: List[float] = (),
) -> None:
    """Initializes a `stim.CircuitInstruction`.

    Args:
        name: The name of the instruction being applied.
        targets: The targets the instruction is being applied to. These can be raw
            values like `0` and `stim.target_rec(-1)`, or instances of
            `stim.GateTarget`.
        gate_args: The sequence of numeric arguments parameterizing a gate. For
            noise gates this is their probabilities. For `OBSERVABLE_INCLUDE`
            instructions it's the index of the logical observable to affect.
    """
```

<a name="stim.CircuitInstruction.__ne__"></a>
```python
# stim.CircuitInstruction.__ne__

# (in class stim.CircuitInstruction)
def __ne__(
    self,
    arg0: stim.CircuitInstruction,
) -> bool:
    """Determines if two `stim.CircuitInstruction`s are different.
    """
```

<a name="stim.CircuitInstruction.__repr__"></a>
```python
# stim.CircuitInstruction.__repr__

# (in class stim.CircuitInstruction)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.
    """
```

<a name="stim.CircuitInstruction.__str__"></a>
```python
# stim.CircuitInstruction.__str__

# (in class stim.CircuitInstruction)
def __str__(
    self,
) -> str:
    """Returns a text description of the instruction as a stim circuit file line.
    """
```

<a name="stim.CircuitInstruction.gate_args_copy"></a>
```python
# stim.CircuitInstruction.gate_args_copy

# (in class stim.CircuitInstruction)
def gate_args_copy(
    self,
) -> List[float]:
    """Returns the gate's arguments (numbers parameterizing the instruction).

    For noisy gates this typically a list of probabilities.
    For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable
    index.
    """
```

<a name="stim.CircuitInstruction.name"></a>
```python
# stim.CircuitInstruction.name

# (in class stim.CircuitInstruction)
@property
def name(
    self,
) -> str:
    """The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
    """
```

<a name="stim.CircuitInstruction.targets_copy"></a>
```python
# stim.CircuitInstruction.targets_copy

# (in class stim.CircuitInstruction)
def targets_copy(
    self,
) -> List[stim.GateTarget]:
    """Returns a copy of the targets of the instruction.
    """
```

<a name="stim.CircuitRepeatBlock"></a>
```python
# stim.CircuitRepeatBlock

# (at top-level in the stim module)
class CircuitRepeatBlock:
    """A REPEAT block from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     REPEAT 5 {
        ...         CX 0 1
        ...         CZ 1 2
        ...     }
        ... ''')
        >>> repeat_block = circuit[1]
        >>> repeat_block.repeat_count
        5
        >>> repeat_block.body_copy()
        stim.Circuit('''
            CX 0 1
            CZ 1 2
        ''')
    """
```

<a name="stim.CircuitRepeatBlock.__eq__"></a>
```python
# stim.CircuitRepeatBlock.__eq__

# (in class stim.CircuitRepeatBlock)
def __eq__(
    self,
    arg0: stim.CircuitRepeatBlock,
) -> bool:
    """Determines if two `stim.CircuitRepeatBlock`s are identical.
    """
```

<a name="stim.CircuitRepeatBlock.__init__"></a>
```python
# stim.CircuitRepeatBlock.__init__

# (in class stim.CircuitRepeatBlock)
def __init__(
    self,
    repeat_count: int,
    body: stim.Circuit,
) -> None:
    """Initializes a `stim.CircuitRepeatBlock`.

    Args:
        repeat_count: The number of times to repeat the block.
        body: The body of the block, as a circuit.
    """
```

<a name="stim.CircuitRepeatBlock.__ne__"></a>
```python
# stim.CircuitRepeatBlock.__ne__

# (in class stim.CircuitRepeatBlock)
def __ne__(
    self,
    arg0: stim.CircuitRepeatBlock,
) -> bool:
    """Determines if two `stim.CircuitRepeatBlock`s are different.
    """
```

<a name="stim.CircuitRepeatBlock.__repr__"></a>
```python
# stim.CircuitRepeatBlock.__repr__

# (in class stim.CircuitRepeatBlock)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equivalent `stim.CircuitRepeatBlock`.
    """
```

<a name="stim.CircuitRepeatBlock.body_copy"></a>
```python
# stim.CircuitRepeatBlock.body_copy

# (in class stim.CircuitRepeatBlock)
def body_copy(
    self,
) -> stim.Circuit:
    """Returns a copy of the body of the repeat block.

    (Making a copy is enforced to make it clear that editing the result won't change
    the block's body.)

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     REPEAT 5 {
        ...         CX 0 1
        ...         CZ 1 2
        ...     }
        ... ''')
        >>> repeat_block = circuit[1]
        >>> repeat_block.body_copy()
        stim.Circuit('''
            CX 0 1
            CZ 1 2
        ''')
    """
```

<a name="stim.CircuitRepeatBlock.name"></a>
```python
# stim.CircuitRepeatBlock.name

# (in class stim.CircuitRepeatBlock)
@property
def name(
    self,
) -> object:
    """Returns the name "REPEAT".

    This is a duck-typing convenience method. It exists so that code that doesn't
    know whether it has a `stim.CircuitInstruction` or a `stim.CircuitRepeatBlock`
    can check the object's name without having to do an `instanceof` check first.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     REPEAT 5 {
        ...         CX 1 2
        ...     }
        ...     S 1
        ... ''')
        >>> [instruction.name for instruction in circuit]
        ['H', 'REPEAT', 'S']
    """
```

<a name="stim.CircuitRepeatBlock.repeat_count"></a>
```python
# stim.CircuitRepeatBlock.repeat_count

# (in class stim.CircuitRepeatBlock)
@property
def repeat_count(
    self,
) -> int:
    """The repetition count of the repeat block.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     H 0
        ...     REPEAT 5 {
        ...         CX 0 1
        ...         CZ 1 2
        ...     }
        ... ''')
        >>> repeat_block = circuit[1]
        >>> repeat_block.repeat_count
        5
    """
```

<a name="stim.CircuitTargetsInsideInstruction"></a>
```python
# stim.CircuitTargetsInsideInstruction

# (at top-level in the stim module)
class CircuitTargetsInsideInstruction:
    """Describes a range of targets within a circuit instruction.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.__init__"></a>
```python
# stim.CircuitTargetsInsideInstruction.__init__

# (in class stim.CircuitTargetsInsideInstruction)
def __init__(
    self,
    *,
    gate: str,
    args: List[float],
    target_range_start: int,
    target_range_end: int,
    targets_in_range: List[stim.GateTargetWithCoords],
) -> None:
    """Creates a stim.CircuitTargetsInsideInstruction.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.args"></a>
```python
# stim.CircuitTargetsInsideInstruction.args

# (in class stim.CircuitTargetsInsideInstruction)
@property
def args(
    self,
) -> List[float]:
    """Returns parens arguments of the gate / instruction that was being executed.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.gate"></a>
```python
# stim.CircuitTargetsInsideInstruction.gate

# (in class stim.CircuitTargetsInsideInstruction)
@property
def gate(
    self,
) -> Optional[str]:
    """Returns the name of the gate / instruction that was being executed.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.target_range_end"></a>
```python
# stim.CircuitTargetsInsideInstruction.target_range_end

# (in class stim.CircuitTargetsInsideInstruction)
@property
def target_range_end(
    self,
) -> int:
    """Returns the exclusive end of the range of targets that were executing
    within the gate / instruction.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.target_range_start"></a>
```python
# stim.CircuitTargetsInsideInstruction.target_range_start

# (in class stim.CircuitTargetsInsideInstruction)
@property
def target_range_start(
    self,
) -> int:
    """Returns the inclusive start of the range of targets that were executing
    within the gate / instruction.
    """
```

<a name="stim.CircuitTargetsInsideInstruction.targets_in_range"></a>
```python
# stim.CircuitTargetsInsideInstruction.targets_in_range

# (in class stim.CircuitTargetsInsideInstruction)
@property
def targets_in_range(
    self,
) -> List[stim.GateTargetWithCoords]:
    """Returns the subset of targets of the gate/instruction that were being executed.

    Includes coordinate data with the targets.
    """
```

<a name="stim.CompiledDemSampler"></a>
```python
# stim.CompiledDemSampler

# (at top-level in the stim module)
class CompiledDemSampler:
    """A helper class for efficiently sampler from a detector error model.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0) D0
        ...    error(1) D1 D2 L0
        ... ''')
        >>> sampler = dem.compile_sampler()
        >>> det_data, obs_data, err_data = sampler.sample(
        ...     shots=4,
        ...     return_errors=True)
        >>> det_data
        array([[False,  True,  True],
               [False,  True,  True],
               [False,  True,  True],
               [False,  True,  True]])
        >>> obs_data
        array([[ True],
               [ True],
               [ True],
               [ True]])
        >>> err_data
        array([[False,  True],
               [False,  True],
               [False,  True],
               [False,  True]])
    """
```

<a name="stim.CompiledDemSampler.sample"></a>
```python
# stim.CompiledDemSampler.sample

# (in class stim.CompiledDemSampler)
def sample(
    self,
    shots: int,
    *,
    bit_packed: bool = False,
    return_errors: bool = False,
    recorded_errors_to_replay: Optional[np.ndarray] = None,
) -> Tuple[np.ndarray, np.ndarray, Optional[np.ndarray]]:
    """Samples the detector error model's error mechanisms to produce sample data.

    Args:
        shots: The number of times to sample from the model.
        bit_packed: Defaults to false.
            False: the returned numpy arrays have dtype=np.bool_.
            True: the returned numpy arrays have dtype=np.uint8 and pack 8 bits into
                each byte.

            Setting this to True is equivalent to running
            `np.packbits(data, bitorder='little', axis=1)` on each output value, but
            has the performance benefit of the data never being expanded into an
            unpacked form.
        return_errors: Defaults to False.
            False: the third entry of the returned tuple is None.
            True: the third entry of the returned tuple is a numpy array recording
            which errors were sampled.
        recorded_errors_to_replay: Defaults to None, meaning sample errors randomly.
            If not None, this is expected to be a 2d numpy array specifying which
            errors to apply (e.g. one returned from a previous call to the sample
            method). The array must have dtype=np.bool_ and
            shape=(num_shots, num_errors) or dtype=np.uint8 and
            shape=(num_shots, math.ceil(num_errors / 8)).

    Returns:
        A tuple (detector_data, obs_data, error_data).

        Assuming bit_packed is False and return_errors is True:
            - If error_data[s, k] is True, then the error with index k fired in the
                shot with index s.
            - If detector_data[s, k] is True, then the detector with index k ended
                up flipped in the shot with index s.
            - If obs_data[s, k] is True, then the observable with index k ended up
                flipped in the shot with index s.

        The dtype and shape of the data depends on the arguments:
            if bit_packed:
                detector_data.shape == (num_shots, math.ceil(num_detectors / 8))
                detector_data.dtype == np.uint8
                obs_data.shape == (num_shots, math.ceil(num_observables / 8))
                obs_data.dtype == np.uint8
                if return_errors:
                    error_data.shape = (num_shots, math.ceil(num_errors / 8))
                    error_data.dtype = np.uint8
                else:
                    error_data is None
            else:
                detector_data.shape == (num_shots, num_detectors)
                detector_data.dtype == np.bool_
                obs_data.shape == (num_shots, num_observables)
                obs_data.dtype == np.bool_
                if return_errors:
                    error_data.shape = (num_shots, num_errors)
                    error_data.dtype = np.bool_
                else:
                    error_data is None

        Note that bit packing is done using little endian order on the last axis
        (i.e. like `np.packbits(data, bitorder='little', axis=1)`).

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0) D0
        ...    error(1) D1 D2 L0
        ... ''')
        >>> sampler = dem.compile_sampler()

        >>> # Taking samples.
        >>> det_data, obs_data, err_data_not_requested = sampler.sample(shots=4)
        >>> det_data
        array([[False,  True,  True],
               [False,  True,  True],
               [False,  True,  True],
               [False,  True,  True]])
        >>> obs_data
        array([[ True],
               [ True],
               [ True],
               [ True]])
        >>> err_data_not_requested is None
        True

        >>> # Recording errors.
        >>> det_data, obs_data, err_data = sampler.sample(
        ...     shots=4,
        ...     return_errors=True)
        >>> det_data
        array([[False,  True,  True],
               [False,  True,  True],
               [False,  True,  True],
               [False,  True,  True]])
        >>> obs_data
        array([[ True],
               [ True],
               [ True],
               [ True]])
        >>> err_data
        array([[False,  True],
               [False,  True],
               [False,  True],
               [False,  True]])

        >>> # Bit packing.
        >>> det_data, obs_data, err_data = sampler.sample(
        ...     shots=4,
        ...     return_errors=True,
        ...     bit_packed=True)
        >>> det_data
        array([[6],
               [6],
               [6],
               [6]], dtype=uint8)
        >>> obs_data
        array([[1],
               [1],
               [1],
               [1]], dtype=uint8)
        >>> err_data
        array([[2],
               [2],
               [2],
               [2]], dtype=uint8)

        >>> # Recording and replaying errors.
        >>> noisy_dem = stim.DetectorErrorModel('''
        ...    error(0.125) D0
        ...    error(0.25) D1
        ... ''')
        >>> noisy_sampler = noisy_dem.compile_sampler()
        >>> det_data, obs_data, err_data = noisy_sampler.sample(
        ...     shots=100,
        ...     return_errors=True)
        >>> replay_det_data, replay_obs_data, _ = noisy_sampler.sample(
        ...     shots=100,
        ...     recorded_errors_to_replay=err_data)
        >>> np.array_equal(det_data, replay_det_data)
        True
        >>> np.array_equal(obs_data, replay_obs_data)
        True
    """
```

<a name="stim.CompiledDemSampler.sample_write"></a>
```python
# stim.CompiledDemSampler.sample_write

# (in class stim.CompiledDemSampler)
def sample_write(
    self,
    shots: int,
    *,
    det_out_file: Union[None, str, pathlib.Path],
    det_out_format: str = "01",
    obs_out_file: Union[None, str, pathlib.Path],
    obs_out_format: str = "01",
    err_out_file: Union[None, str, pathlib.Path] = None,
    err_out_format: str = "01",
    replay_err_in_file: Union[None, str, pathlib.Path] = None,
    replay_err_in_format: str = "01",
) -> None:
    """Samples the detector error model and writes the results to disk.

    Args:
        shots: The number of times to sample from the model.
        det_out_file: Where to write detection event data.
            If None: detection event data is not written.
            If str or pathlib.Path: opens and overwrites the file at the given path.
            NOT IMPLEMENTED: io.IOBase
        det_out_format: The format to write the detection event data in
            (e.g. "01" or "b8").
        obs_out_file: Where to write observable flip data.
            If None: observable flip data is not written.
            If str or pathlib.Path: opens and overwrites the file at the given path.
            NOT IMPLEMENTED: io.IOBase
        obs_out_format: The format to write the observable flip data in
            (e.g. "01" or "b8").
        err_out_file: Where to write errors-that-occurred data.
            If None: errors-that-occurred data is not written.
            If str or pathlib.Path: opens and overwrites the file at the given path.
            NOT IMPLEMENTED: io.IOBase
        err_out_format: The format to write the errors-that-occurred data in
            (e.g. "01" or "b8").
        replay_err_in_file: If this is specified, errors are replayed from data
            instead of generated randomly. The following types are supported:
            - None: errors are generated randomly according to the probabilities
                in the detector error model.
            - str or pathlib.Path: the file at the given path is opened and
                errors-to-apply data is read from there.
            - io.IOBase: NOT IMPLEMENTED
        replay_err_in_format: The format to write the errors-that-occurred data in
            (e.g. "01" or "b8").

    Returns:
        Nothing. Results are written to disk.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> import pathlib
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0) D0
        ...    error(0) D1
        ...    error(0) D0
        ...    error(1) D1 D2 L0
        ...    error(0) D0
        ... ''')
        >>> sampler = dem.compile_sampler()
        >>> with tempfile.TemporaryDirectory() as d:
        ...     d = pathlib.Path(d)
        ...     sampler.sample_write(
        ...         shots=1,
        ...         det_out_file=d / 'dets.01',
        ...         det_out_format='01',
        ...         obs_out_file=d / 'obs.01',
        ...         obs_out_format='01',
        ...         err_out_file=d / 'err.hits',
        ...         err_out_format='hits',
        ...     )
        ...     with open(d / 'dets.01') as f:
        ...         assert f.read() == "011\n"
        ...     with open(d / 'obs.01') as f:
        ...         assert f.read() == "1\n"
        ...     with open(d / 'err.hits') as f:
        ...         assert f.read() == "3\n"
    """
```

<a name="stim.CompiledDetectorSampler"></a>
```python
# stim.CompiledDetectorSampler

# (at top-level in the stim module)
class CompiledDetectorSampler:
    """An analyzed stabilizer circuit whose detection events can be sampled quickly.
    """
```

<a name="stim.CompiledDetectorSampler.__init__"></a>
```python
# stim.CompiledDetectorSampler.__init__

# (in class stim.CompiledDetectorSampler)
def __init__(
    self,
    circuit: stim.Circuit,
    *,
    seed: object = None,
) -> None:
    """Creates an object that can sample the detection events from a circuit.

    Args:
        circuit: The circuit to sample from.
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how many
            shots are taken. For example, taking 10 shots and then 90 shots will
            give different results from taking 100 shots in one call.

    Returns:
        An initialized stim.CompiledDetectorSampler.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    H 0
        ...    CNOT 0 1
        ...    X_ERROR(1.0) 0
        ...    M 0 1
        ...    DETECTOR rec[-1] rec[-2]
        ... ''')
        >>> s = c.compile_detector_sampler()
        >>> s.sample(shots=1)
        array([[ True]])
    """
```

<a name="stim.CompiledDetectorSampler.__repr__"></a>
```python
# stim.CompiledDetectorSampler.__repr__

# (in class stim.CompiledDetectorSampler)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equivalent `stim.CompiledDetectorSampler`.
    """
```

<a name="stim.CompiledDetectorSampler.sample"></a>
```python
# stim.CompiledDetectorSampler.sample

# (in class stim.CompiledDetectorSampler)
@overload
def sample(
    self,
    shots: int,
    *,
    prepend_observables: bool = False,
    append_observables: bool = False,
    bit_packed: bool = False,
) -> np.ndarray:
    pass
@overload
def sample(
    self,
    shots: int,
    *,
    separate_observables: Literal[True],
    bit_packed: bool = False,
) -> Tuple[np.ndarray, np.ndarray]:
    pass
def sample(
    self,
    shots: int,
    *,
    prepend_observables: bool = False,
    append_observables: bool = False,
    separate_observables: bool = False,
    bit_packed: bool = False,
) -> Union[np.ndarray, Tuple[np.ndarray, np.ndarray]]:
    """Returns a numpy array containing a batch of detector samples from the circuit.

    The circuit must define the detectors using DETECTOR instructions. Observables
    defined by OBSERVABLE_INCLUDE instructions can also be included in the results
    as honorary detectors.

    Args:
        shots: The number of times to sample every detector in the circuit.
        separate_observables: Defaults to False. When set to True, the return value
            is a (detection_events, observable_flips) tuple instead of a flat
            detection_events array.
        prepend_observables: Defaults to false. When set, observables are included
            with the detectors and are placed at the start of the results.
        append_observables: Defaults to false. When set, observables are included
            with the detectors and are placed at the end of the results.
        bit_packed: Returns a uint8 numpy array with 8 bits per byte, instead of
            a bool_ numpy array with 1 bit per byte. Uses little endian packing.

    Returns:
        A numpy array or tuple of numpy arrays containing the samples.

        if separate_observables=False and bit_packed=False:
            A single numpy array.
            dtype=bool_
            shape=(
                shots,
                num_detectors + num_observables * (
                    append_observables + prepend_observables),
            )
            The bit for detection event `m` in shot `s` is at
                result[s, m]

        if separate_observables=False and bit_packed=True:
            A single numpy array.
            dtype=uint8
            shape=(
                shots,
                math.ceil((num_detectors + num_observables * (
                    append_observables + prepend_observables)) / 8),
            )
            The bit for detection event `m` in shot `s` is at
                (result[s, m // 8] >> (m % 8)) & 1

        if separate_observables=True and bit_packed=False:
            A (dets, obs) tuple.
            dets.dtype=bool_
            dets.shape=(shots, num_detectors)
            obs.dtype=bool_
            obs.shape=(shots, num_observables)
            The bit for detection event `m` in shot `s` is at
                dets[s, m]
            The bit for observable `m` in shot `s` is at
                obs[s, m]

        if separate_observables=True and bit_packed=True:
            A (dets, obs) tuple.
            dets.dtype=uint8
            dets.shape=(shots, math.ceil(num_detectors / 8))
            obs.dtype=uint8
            obs.shape=(shots, math.ceil(num_observables / 8))
            The bit for detection event `m` in shot `s` is at
                (dets[s, m // 8] >> (m % 8)) & 1
            The bit for observable `m` in shot `s` is at
                (obs[s, m // 8] >> (m % 8)) & 1
    """
```

<a name="stim.CompiledDetectorSampler.sample_write"></a>
```python
# stim.CompiledDetectorSampler.sample_write

# (in class stim.CompiledDetectorSampler)
def sample_write(
    self,
    shots: int,
    *,
    filepath: str,
    format: str = '01',
    prepend_observables: bool = False,
    append_observables: bool = False,
    obs_out_filepath: str = None,
    obs_out_format: str = '01',
) -> None:
    """Samples detection events from the circuit and writes them to a file.

    Args:
        shots: The number of times to sample every measurement in the circuit.
        filepath: The file to write the results to.
        format: The output format to write the results with.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        obs_out_filepath: Sample observables as part of each shot, and write them to
            this file. This keeps the observable data separate from the detector
            data.
        obs_out_format: If writing the observables to a file, this is the format to
            write them in.

            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        prepend_observables: Sample observables as part of each shot, and put them
            at the start of the detector data.
        append_observables: Sample observables as part of each shot, and put them at
            the end of the detector data.

    Returns:
        None.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = f"{d}/tmp.dat"
        ...     c = stim.Circuit('''
        ...         X_ERROR(1) 0
        ...         M 0 1
        ...         DETECTOR rec[-2]
        ...         DETECTOR rec[-1]
        ...     ''')
        ...     c.compile_detector_sampler().sample_write(
        ...         shots=3,
        ...         filepath=path,
        ...         format="dets")
        ...     with open(path) as f:
        ...         print(f.read(), end='')
        shot D0
        shot D0
        shot D0
    """
```

<a name="stim.CompiledMeasurementSampler"></a>
```python
# stim.CompiledMeasurementSampler

# (at top-level in the stim module)
class CompiledMeasurementSampler:
    """An analyzed stabilizer circuit whose measurements can be sampled quickly.
    """
```

<a name="stim.CompiledMeasurementSampler.__init__"></a>
```python
# stim.CompiledMeasurementSampler.__init__

# (in class stim.CompiledMeasurementSampler)
def __init__(
    self,
    circuit: stim.Circuit,
    *,
    skip_reference_sample: bool = False,
    seed: object = None,
    reference_sample: object = None,
) -> None:
    """Creates a measurement sampler for the given circuit.

    The sampler uses a noiseless reference sample, collected from the circuit using
    stim's Tableau simulator during initialization of the sampler, as a baseline for
    deriving more samples using an error propagation simulator.

    Args:
        circuit: The stim circuit to sample from.
        skip_reference_sample: Defaults to False. When set to True, the reference
            sample used by the sampler is initialized to all-zeroes instead of being
            collected from the circuit. This means that the results returned by the
            sampler are actually whether or not each measurement was *flipped*,
            instead of true measurement results.

            Forcing an all-zero reference sample is useful when you are only
            interested in error propagation and don't want to have to deal with the
            fact that some measurements want to be On when no errors occur. It is
            also useful when you know for sure that the all-zero result is actually
            a possible result from the circuit (under noiseless execution), meaning
            it is a valid reference sample as good as any other. Computing the
            reference sample is the most time consuming and memory intensive part of
            simulating the circuit, so promising that the simulator can safely skip
            that step is an effective optimization.
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how many
            shots are taken. For example, taking 10 shots and then 90 shots will
            give different results from taking 100 shots in one call.
        reference_sample: The data to xor into the measurement flips produced by the
            frame simulator, in order to produce proper measurement results.
            This can either be specified as an `np.bool_` array or a bit packed
            `np.uint8` array (little endian). Under normal conditions, the reference
            sample should be a valid noiseless sample of the circuit, such as the
            one returned by `circuit.reference_sample()`. If this argument is not
            provided, the reference sample will be set to
            `circuit.reference_sample()`, unless `skip_reference_sample=True`
            is used, in which case it will be set to all-zeros.

    Returns:
        An initialized stim.CompiledMeasurementSampler.

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0   2 3
        ...    M 0 1 2 3
        ... ''')
        >>> s = c.compile_sampler()
        >>> s.sample(shots=1)
        array([[ True, False,  True,  True]])
    """
```

<a name="stim.CompiledMeasurementSampler.__repr__"></a>
```python
# stim.CompiledMeasurementSampler.__repr__

# (in class stim.CompiledMeasurementSampler)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledMeasurementSampler`.
    """
```

<a name="stim.CompiledMeasurementSampler.sample"></a>
```python
# stim.CompiledMeasurementSampler.sample

# (in class stim.CompiledMeasurementSampler)
def sample(
    self,
    shots: int,
    *,
    bit_packed: bool = False,
) -> np.ndarray:
    """Samples a batch of measurement samples from the circuit.

    Args:
        shots: The number of times to sample every measurement in the circuit.
        bit_packed: Returns a uint8 numpy array with 8 bits per byte, instead of
            a bool_ numpy array with 1 bit per byte. Uses little endian packing.

    Returns:
        A numpy array containing the samples.

        If bit_packed=False:
            dtype=bool_
            shape=(shots, circuit.num_measurements)
            The bit for measurement `m` in shot `s` is at
                result[s, m]
        If bit_packed=True:
            dtype=uint8
            shape=(shots, math.ceil(circuit.num_measurements / 8))
            The bit for measurement `m` in shot `s` is at
                (result[s, m // 8] >> (m % 8)) & 1

    Examples:
        >>> import stim
        >>> c = stim.Circuit('''
        ...    X 0   2 3
        ...    M 0 1 2 3
        ... ''')
        >>> s = c.compile_sampler()
        >>> s.sample(shots=1)
        array([[ True, False,  True,  True]])
    """
```

<a name="stim.CompiledMeasurementSampler.sample_write"></a>
```python
# stim.CompiledMeasurementSampler.sample_write

# (in class stim.CompiledMeasurementSampler)
def sample_write(
    self,
    shots: int,
    *,
    filepath: str,
    format: str = '01',
) -> None:
    """Samples measurements from the circuit and writes them to a file.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = f"{d}/tmp.dat"
        ...     c = stim.Circuit('''
        ...         X 0   2 3
        ...         M 0 1 2 3
        ...     ''')
        ...     c.compile_sampler().sample_write(5, filepath=path, format="01")
        ...     with open(path) as f:
        ...         print(f.read(), end='')
        1011
        1011
        1011
        1011
        1011

    Args:
        shots: The number of times to sample every measurement in the circuit.
        filepath: The file to write the results to.
        format: The output format to write the results with.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".

    Returns:
        None.
    """
```

<a name="stim.CompiledMeasurementsToDetectionEventsConverter"></a>
```python
# stim.CompiledMeasurementsToDetectionEventsConverter

# (at top-level in the stim module)
class CompiledMeasurementsToDetectionEventsConverter:
    """A tool for quickly converting measurements from an analyzed stabilizer circuit into detection events.
    """
```

<a name="stim.CompiledMeasurementsToDetectionEventsConverter.__init__"></a>
```python
# stim.CompiledMeasurementsToDetectionEventsConverter.__init__

# (in class stim.CompiledMeasurementsToDetectionEventsConverter)
def __init__(
    self,
    circuit: stim.Circuit,
    *,
    skip_reference_sample: bool = False,
) -> None:
    """Creates a measurement-to-detection-events converter for the given circuit.

    The converter uses a noiseless reference sample, collected from the circuit
    using stim's Tableau simulator during initialization of the converter, as a
    baseline for determining what the expected value of a detector is.

    Note that the expected behavior of gauge detectors (detectors that are not
    actually deterministic under noiseless execution) can vary depending on the
    reference sample. Stim mitigates this by always generating the same reference
    sample for a given circuit.

    Args:
        circuit: The stim circuit to use for conversions.
        skip_reference_sample: Defaults to False. When set to True, the reference
            sample used by the converter is initialized to all-zeroes instead of
            being collected from the circuit. This should only be used if it's known
            that the all-zeroes sample is actually a possible result from the
            circuit (under noiseless execution).

    Returns:
        An initialized stim.CompiledMeasurementsToDetectionEventsConverter.

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> converter = stim.Circuit('''
        ...    X 0
        ...    M 0
        ...    DETECTOR rec[-1]
        ... ''').compile_m2d_converter()
        >>> converter.convert(
        ...     measurements=np.array([[0], [1]], dtype=np.bool_),
        ...     append_observables=False,
        ... )
        array([[ True],
               [False]])
    """
```

<a name="stim.CompiledMeasurementsToDetectionEventsConverter.__repr__"></a>
```python
# stim.CompiledMeasurementsToDetectionEventsConverter.__repr__

# (in class stim.CompiledMeasurementsToDetectionEventsConverter)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.CompiledMeasurementsToDetectionEventsConverter`.
    """
```

<a name="stim.CompiledMeasurementsToDetectionEventsConverter.convert"></a>
```python
# stim.CompiledMeasurementsToDetectionEventsConverter.convert

# (in class stim.CompiledMeasurementsToDetectionEventsConverter)
@overload
def convert(
    self,
    *,
    measurements: np.ndarray,
    sweep_bits: Optional[np.ndarray] = None,
    append_observables: bool = False,
    bit_packed: bool = False,
) -> np.ndarray:
    pass
@overload
def convert(
    self,
    *,
    measurements: np.ndarray,
    sweep_bits: Optional[np.ndarray] = None,
    separate_observables: 'Literal[True]',
    append_observables: bool = False,
    bit_packed: bool = False,
) -> Tuple[np.ndarray, np.ndarray]:
    pass
def convert(
    self,
    *,
    measurements: np.ndarray,
    sweep_bits: Optional[np.ndarray] = None,
    separate_observables: bool = False,
    append_observables: bool = False,
    bit_packed: bool = False,
) -> Union[np.ndarray, Tuple[np.ndarray, np.ndarray]]:
    """Converts measurement data into detection event data.

    Args:
        measurements: A numpy array containing measurement data.

            The dtype of the array is used to determine if it is bit packed or not.
            dtype=np.bool_ (unpacked data):
                shape=(num_shots, circuit.num_measurements)
            dtype=np.uint8 (bit packed data):
                shape=(num_shots, math.ceil(circuit.num_measurements / 8))
        sweep_bits: Optional. A numpy array containing sweep data for the `sweep[k]`
            controls in the circuit.

            The dtype of the array is used to determine if it is bit packed or not.
            dtype=np.bool_ (unpacked data):
                shape=(num_shots, circuit.num_sweep_bits)
            dtype=np.uint8 (bit packed data):
                shape=(num_shots, math.ceil(circuit.num_sweep_bits / 8))
        separate_observables: Defaults to False. When set to True, two numpy arrays
            are returned instead of one, with the second array containing the
            observable flip data.
        append_observables: Defaults to False. When set to True, the observables in
            the circuit are treated as if they were additional detectors. Their
            results are appended to the end of the detection event data.
        bit_packed: Defaults to False. When set to True, the returned numpy
            array contains bit packed data (dtype=np.uint8 with 8 bits per item)
            instead of unpacked data (dtype=np.bool_).

    Returns:
        The detection event data and (optionally) observable data. The result is a
        single numpy array if separate_observables is false, otherwise it's a tuple
        of two numpy arrays.

        When returning two numpy arrays, the first array is the detection event data
        and the second is the observable flip data.

        The dtype of the returned arrays is np.bool_ if bit_packed is false,
        otherwise they're np.uint8 arrays.

        shape[0] of the array(s) is the number of shots.
        shape[1] of the array(s) is the number of bits per shot (divided by 8 if bit
        packed) (e.g. for just detection event data it would be
        circuit.num_detectors).

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> converter = stim.Circuit('''
        ...    X 0
        ...    M 0 1
        ...    DETECTOR rec[-1]
        ...    DETECTOR rec[-2]
        ...    OBSERVABLE_INCLUDE(0) rec[-2]
        ... ''').compile_m2d_converter()
        >>> dets, obs = converter.convert(
        ...     measurements=np.array([[1, 0],
        ...                            [1, 0],
        ...                            [1, 0],
        ...                            [0, 0],
        ...                            [1, 0]], dtype=np.bool_),
        ...     separate_observables=True,
        ... )
        >>> dets
        array([[False, False],
               [False, False],
               [False, False],
               [False,  True],
               [False, False]])
        >>> obs
        array([[False],
               [False],
               [False],
               [ True],
               [False]])
    """
```

<a name="stim.CompiledMeasurementsToDetectionEventsConverter.convert_file"></a>
```python
# stim.CompiledMeasurementsToDetectionEventsConverter.convert_file

# (in class stim.CompiledMeasurementsToDetectionEventsConverter)
def convert_file(
    self,
    *,
    measurements_filepath: str,
    measurements_format: str = '01',
    sweep_bits_filepath: str = None,
    sweep_bits_format: str = '01',
    detection_events_filepath: str,
    detection_events_format: str = '01',
    append_observables: bool = False,
    obs_out_filepath: str = None,
    obs_out_format: str = '01',
) -> None:
    """Reads measurement data from a file and writes detection events to another file.

    Args:
        measurements_filepath: A file containing measurement data to be converted.
        measurements_format: The format the measurement data is stored in.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        detection_events_filepath: Where to save detection event data to.
        detection_events_format: The format to save the detection event data in.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        sweep_bits_filepath: Defaults to None. A file containing sweep data, or
            None. When specified, sweep data (used for `sweep[k]` controls in the
            circuit, which can vary from shot to shot) will be read from the given
            file. When not specified, all sweep bits default to False and no
            sweep-controlled operations occur.
        sweep_bits_format: The format the sweep data is stored in.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        obs_out_filepath: Sample observables as part of each shot, and write them to
            this file. This keeps the observable data separate from the detector
            data.
        obs_out_format: If writing the observables to a file, this is the format to
            write them in.
            Valid values are "01", "b8", "r8", "hits", "dets", and "ptb64".
            Defaults to "01".
        append_observables: When True, the observables in the circuit are included
            as part of the detection event data. Specifically, they are treated as
            if they were additional detectors at the end of the circuit. When False,
            observable data is not output.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> converter = stim.Circuit('''
        ...    X 0
        ...    M 0
        ...    DETECTOR rec[-1]
        ... ''').compile_m2d_converter()
        >>> with tempfile.TemporaryDirectory() as d:
        ...    with open(f"{d}/measurements.01", "w") as f:
        ...        print("0", file=f)
        ...        print("1", file=f)
        ...    converter.convert_file(
        ...        measurements_filepath=f"{d}/measurements.01",
        ...        detection_events_filepath=f"{d}/detections.01",
        ...        append_observables=False,
        ...    )
        ...    with open(f"{d}/detections.01") as f:
        ...        print(f.read(), end="")
        1
        0
    """
```

<a name="stim.DemInstruction"></a>
```python
# stim.DemInstruction

# (at top-level in the stim module)
class DemInstruction:
    """An instruction from a detector error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     error(0.125) D0 D1 L0
        ...     error(0.125) D1 D2
        ...     error(0.125) D2 D3
        ...     error(0.125) D3
        ... ''')
        >>> instruction = model[0]
        >>> instruction
        stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
    """
```

<a name="stim.DemInstruction.__eq__"></a>
```python
# stim.DemInstruction.__eq__

# (in class stim.DemInstruction)
def __eq__(
    self,
    arg0: stim.DemInstruction,
) -> bool:
    """Determines if two instructions have identical contents.
    """
```

<a name="stim.DemInstruction.__init__"></a>
```python
# stim.DemInstruction.__init__

# (in class stim.DemInstruction)
def __init__(
    self,
    type: str,
    args: List[float],
    targets: List[object],
) -> None:
    """Creates a stim.DemInstruction.

    Args:
        type: The name of the instruction type (e.g. "error" or "shift_detectors").
        args: Numeric values parameterizing the instruction (e.g. the 0.1 in
            "error(0.1)").
        targets: The objects the instruction involves (e.g. the "D0" and "L1" in
            "error(0.1) D0 L1").

    Examples:
        >>> import stim
        >>> instruction = stim.DemInstruction(
        ...     'error',
        ...     [0.125],
        ...     [stim.target_relative_detector_id(5)])
        >>> print(instruction)
        error(0.125) D5
    """
```

<a name="stim.DemInstruction.__ne__"></a>
```python
# stim.DemInstruction.__ne__

# (in class stim.DemInstruction)
def __ne__(
    self,
    arg0: stim.DemInstruction,
) -> bool:
    """Determines if two instructions have non-identical contents.
    """
```

<a name="stim.DemInstruction.__repr__"></a>
```python
# stim.DemInstruction.__repr__

# (in class stim.DemInstruction)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`.
    """
```

<a name="stim.DemInstruction.__str__"></a>
```python
# stim.DemInstruction.__str__

# (in class stim.DemInstruction)
def __str__(
    self,
) -> str:
    """Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.
    """
```

<a name="stim.DemInstruction.args_copy"></a>
```python
# stim.DemInstruction.args_copy

# (in class stim.DemInstruction)
def args_copy(
    self,
) -> List[float]:
    """Returns a copy of the list of numbers parameterizing the instruction (e.g. the probability of an error).
    """
```

<a name="stim.DemInstruction.targets_copy"></a>
```python
# stim.DemInstruction.targets_copy

# (in class stim.DemInstruction)
def targets_copy(
    self,
) -> List[Union[int, stim.DemTarget]]:
    """Returns a copy of the instruction's targets.

    (Making a copy is enforced to make it clear that editing the result won't change
    the instruction's targets.)
    """
```

<a name="stim.DemInstruction.type"></a>
```python
# stim.DemInstruction.type

# (in class stim.DemInstruction)
@property
def type(
    self,
) -> str:
    """The name of the instruction type (e.g. "error" or "shift_detectors").
    """
```

<a name="stim.DemRepeatBlock"></a>
```python
# stim.DemRepeatBlock

# (at top-level in the stim module)
class DemRepeatBlock:
    """A repeat block from a detector error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     repeat 100 {
        ...         error(0.125) D0 D1
        ...         shift_detectors 1
        ...     }
        ... ''')
        >>> model[0]
        stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
            error(0.125) D0 D1
            shift_detectors 1
        '''))
    """
```

<a name="stim.DemRepeatBlock.__eq__"></a>
```python
# stim.DemRepeatBlock.__eq__

# (in class stim.DemRepeatBlock)
def __eq__(
    self,
    arg0: stim.DemRepeatBlock,
) -> bool:
    """Determines if two repeat blocks are identical.
    """
```

<a name="stim.DemRepeatBlock.__init__"></a>
```python
# stim.DemRepeatBlock.__init__

# (in class stim.DemRepeatBlock)
def __init__(
    self,
    repeat_count: int,
    block: stim.DetectorErrorModel,
) -> None:
    """Creates a stim.DemRepeatBlock.

    Args:
        repeat_count: The number of times the repeat block's body is supposed to
            execute.
        block: The body of the repeat block as a DetectorErrorModel containing the
            instructions to repeat.

    Examples:
        >>> import stim
        >>> repeat_block = stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
        ...     error(0.125) D0 D1
        ...     shift_detectors 1
        ... '''))
    """
```

<a name="stim.DemRepeatBlock.__ne__"></a>
```python
# stim.DemRepeatBlock.__ne__

# (in class stim.DemRepeatBlock)
def __ne__(
    self,
    arg0: stim.DemRepeatBlock,
) -> bool:
    """Determines if two repeat blocks are different.
    """
```

<a name="stim.DemRepeatBlock.__repr__"></a>
```python
# stim.DemRepeatBlock.__repr__

# (in class stim.DemRepeatBlock)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.DemRepeatBlock`.
    """
```

<a name="stim.DemRepeatBlock.body_copy"></a>
```python
# stim.DemRepeatBlock.body_copy

# (in class stim.DemRepeatBlock)
def body_copy(
    self,
) -> stim.DetectorErrorModel:
    """Returns a copy of the block's body, as a stim.DetectorErrorModel.
    """
```

<a name="stim.DemRepeatBlock.repeat_count"></a>
```python
# stim.DemRepeatBlock.repeat_count

# (in class stim.DemRepeatBlock)
@property
def repeat_count(
    self,
) -> int:
    """The number of times the repeat block's body is supposed to execute.
    """
```

<a name="stim.DemRepeatBlock.type"></a>
```python
# stim.DemRepeatBlock.type

# (in class stim.DemRepeatBlock)
@property
def type(
    self,
) -> object:
    """Returns the type name "repeat".

    This is a duck-typing convenience method. It exists so that code that doesn't
    know whether it has a `stim.DemInstruction` or a `stim.DemRepeatBlock`
    can check the type field without having to do an `instanceof` check first.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     error(0.1) D0 L0
        ...     repeat 5 {
        ...         error(0.1) D0 D1
        ...         shift_detectors 1
        ...     }
        ...     logical_observable L0
        ... ''')
        >>> [instruction.type for instruction in dem]
        ['error', 'repeat', 'logical_observable']
    """
```

<a name="stim.DemTarget"></a>
```python
# stim.DemTarget

# (at top-level in the stim module)
class DemTarget:
    """An instruction target from a detector error model (.dem) file.
    """
```

<a name="stim.DemTarget.__eq__"></a>
```python
# stim.DemTarget.__eq__

# (in class stim.DemTarget)
def __eq__(
    self,
    arg0: stim.DemTarget,
) -> bool:
    """Determines if two `stim.DemTarget`s are identical.
    """
```

<a name="stim.DemTarget.__ne__"></a>
```python
# stim.DemTarget.__ne__

# (in class stim.DemTarget)
def __ne__(
    self,
    arg0: stim.DemTarget,
) -> bool:
    """Determines if two `stim.DemTarget`s are different.
    """
```

<a name="stim.DemTarget.__repr__"></a>
```python
# stim.DemTarget.__repr__

# (in class stim.DemTarget)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equivalent `stim.DemTarget`.
    """
```

<a name="stim.DemTarget.__str__"></a>
```python
# stim.DemTarget.__str__

# (in class stim.DemTarget)
def __str__(
    self,
) -> str:
    """Returns a text description of the detector error model target.
    """
```

<a name="stim.DemTarget.is_logical_observable_id"></a>
```python
# stim.DemTarget.is_logical_observable_id

# (in class stim.DemTarget)
def is_logical_observable_id(
    self,
) -> bool:
    """Determines if the detector error model target is a logical observable id target.

    In a detector error model file, observable targets are prefixed by `L`. For
    example, in `error(0.25) D0 L1` the `L1` is an observable target.
    """
```

<a name="stim.DemTarget.is_relative_detector_id"></a>
```python
# stim.DemTarget.is_relative_detector_id

# (in class stim.DemTarget)
def is_relative_detector_id(
    self,
) -> bool:
    """Determines if the detector error model target is a relative detector id target.

    In a detector error model file, detectors are prefixed by `D`. For
    example, in `error(0.25) D0 L1` the `D0` is a relative detector target.
    """
```

<a name="stim.DemTarget.is_separator"></a>
```python
# stim.DemTarget.is_separator

# (in class stim.DemTarget)
def is_separator(
    self,
) -> bool:
    """Determines if the detector error model target is a separator.

    Separates separate the components of a suggested decompositions within an error.
    For example, the `^` in `error(0.25) D1 D2 ^ D3 D4` is the separator.
    """
```

<a name="stim.DemTarget.logical_observable_id"></a>
```python
# stim.DemTarget.logical_observable_id

# (in class stim.DemTarget)
@staticmethod
def logical_observable_id(
    index: int,
) -> stim.DemTarget:
    """Returns a logical observable id identifying a frame change.

    Args:
        index: The index of the observable.

    Returns:
        The logical observable target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.DemTarget.logical_observable_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) L13
        ''')
    """
```

<a name="stim.DemTarget.relative_detector_id"></a>
```python
# stim.DemTarget.relative_detector_id

# (in class stim.DemTarget)
@staticmethod
def relative_detector_id(
    index: int,
) -> stim.DemTarget:
    """Returns a relative detector id (e.g. "D5" in a .dem file).

    Args:
        index: The index of the detector, relative to the current detector offset.

    Returns:
        The relative detector target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.DemTarget.relative_detector_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D13
        ''')
    """
```

<a name="stim.DemTarget.separator"></a>
```python
# stim.DemTarget.separator

# (in class stim.DemTarget)
@staticmethod
def separator(
) -> stim.DemTarget:
    """Returns a target separator (e.g. "^" in a .dem file).

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.DemTarget.relative_detector_id(1),
        ...     stim.DemTarget.separator(),
        ...     stim.DemTarget.relative_detector_id(2),
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D1 ^ D2
        ''')
    """
```

<a name="stim.DemTarget.val"></a>
```python
# stim.DemTarget.val

# (in class stim.DemTarget)
@property
def val(
    self,
) -> int:
    """Returns the target's integer value.

    Example:

        >>> import stim
        >>> stim.target_relative_detector_id(5).val
        5
        >>> stim.target_logical_observable_id(6).val
        6
    """
```

<a name="stim.DemTargetWithCoords"></a>
```python
# stim.DemTargetWithCoords

# (at top-level in the stim module)
class DemTargetWithCoords:
    """A detector error model instruction target with associated coords.

    It is also guaranteed that, if the type of the DEM target is a
    relative detector id, it is actually absolute (i.e. relative to
    0).

    For example, if the DEM target is a detector from a circuit with
    coordinate arguments given to detectors, the coords field will
    contain the coordinate data for the detector.

    This is helpful information to have available when debugging a
    problem in a circuit, instead of having to constantly manually
    look up the coordinates of a detector index in order to understand
    what is happening.
    """
```

<a name="stim.DemTargetWithCoords.__init__"></a>
```python
# stim.DemTargetWithCoords.__init__

# (in class stim.DemTargetWithCoords)
def __init__(
    self,
    *,
    dem_target: stim.DemTarget,
    coords: List[float],
) -> None:
    """Creates a stim.DemTargetWithCoords.
    """
```

<a name="stim.DemTargetWithCoords.coords"></a>
```python
# stim.DemTargetWithCoords.coords

# (in class stim.DemTargetWithCoords)
@property
def coords(
    self,
) -> List[float]:
    """Returns the associated coordinate information as a list of floats.

    If there is no coordinate information, returns an empty list.
    """
```

<a name="stim.DemTargetWithCoords.dem_target"></a>
```python
# stim.DemTargetWithCoords.dem_target

# (in class stim.DemTargetWithCoords)
@property
def dem_target(
    self,
) -> stim.DemTarget:
    """Returns the actual DEM target as a `stim.DemTarget`.
    """
```

<a name="stim.DetectorErrorModel"></a>
```python
# stim.DetectorErrorModel

# (at top-level in the stim module)
class DetectorErrorModel:
    """An error model built out of independent error mechanics.

    This class is one of the most important classes in Stim, because it is the
    mechanism used to explain circuits to decoders. A typical workflow would
    look something like:

        1. Create a quantum error correction circuit annotated with detectors
            and observables.
        2. Fail at configuring your favorite decoder using the circuit, because
            it's a pain to convert circuit error mechanisms into a format
            understood by the decoder.
        2a. Call circuit.detector_error_model(), with decompose_errors=True
            if working with a matching-based code. This converts the circuit
            errors into a straightforward list of independent "with
            probability p these detectors and observables get flipped" terms.
        3. Write tedious but straightforward glue code to create whatever
            graph-like object the decoder needs from the detector error model.
        3a. Actually, ideally, someone has already done that for you. For
            example, pymatching can take detector error models directly and
            sinter knows how to explain a detector error model to fusion_blossom.
        4. Get samples using circuit.compile_detector_sampler(), feed them to
            the decoder, and compare its observable flip predictions to the
            actual flips recorded in the samples.
        4a. Actually, sinter will basically handle steps 2 through 4 for you.
            So you should probably have just generated your circuits, called
            `sinter collect` on them, then `sinter plot` on the results.
        5. Write the paper.

    Error mechanisms are described in terms of the visible detection events and the
    hidden observable frame changes that they causes. Error mechanisms can also
    suggest decompositions of their effects into components, which can be helpful
    for decoders that want to work with a simpler decomposed error model instead of
    the full error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     error(0.125) D0 D1 L0
        ...     error(0.125) D1 D2
        ...     error(0.125) D2 D3
        ...     error(0.125) D3
        ... ''')
        >>> len(model)
        5

        >>> stim.Circuit('''
        ...     X_ERROR(0.125) 0
        ...     X_ERROR(0.25) 1
        ...     CORRELATED_ERROR(0.375) X0 X1
        ...     M 0 1
        ...     DETECTOR rec[-2]
        ...     DETECTOR rec[-1]
        ... ''').detector_error_model()
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.375) D0 D1
            error(0.25) D1
        ''')
    """
```

<a name="stim.DetectorErrorModel.__add__"></a>
```python
# stim.DetectorErrorModel.__add__

# (in class stim.DetectorErrorModel)
def __add__(
    self,
    second: stim.DetectorErrorModel,
) -> stim.DetectorErrorModel:
    """Creates a detector error model by appending two models.

    Examples:
        >>> import stim
        >>> m1 = stim.DetectorErrorModel('''
        ...    error(0.125) D0
        ... ''')
        >>> m2 = stim.DetectorErrorModel('''
        ...    error(0.25) D1
        ... ''')
        >>> m1 + m2
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.25) D1
        ''')
    """
```

<a name="stim.DetectorErrorModel.__eq__"></a>
```python
# stim.DetectorErrorModel.__eq__

# (in class stim.DetectorErrorModel)
def __eq__(
    self,
    arg0: stim.DetectorErrorModel,
) -> bool:
    """Determines if two detector error models have identical contents.
    """
```

<a name="stim.DetectorErrorModel.__getitem__"></a>
```python
# stim.DetectorErrorModel.__getitem__

# (in class stim.DetectorErrorModel)
@overload
def __getitem__(
    self,
    index_or_slice: int,
) -> Union[stim.DemInstruction, stim.DemRepeatBlock]:
    pass
@overload
def __getitem__(
    self,
    index_or_slice: slice,
) -> stim.DetectorErrorModel:
    pass
def __getitem__(
    self,
    index_or_slice: object,
) -> object:
    """Returns copies of instructions from the detector error model.

    Args:
        index_or_slice: An integer index picking out an instruction to return, or a
            slice picking out a range of instructions to return as a detector error
            model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...    error(0.125) D0
        ...    error(0.125) D1 L1
        ...    repeat 100 {
        ...        error(0.125) D1 D2
        ...        shift_detectors 1
        ...    }
        ...    error(0.125) D2
        ...    logical_observable L0
        ...    detector D5
        ... ''')
        >>> model[0]
        stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
        >>> model[2]
        stim.DemRepeatBlock(100, stim.DetectorErrorModel('''
            error(0.125) D1 D2
            shift_detectors 1
        '''))
        >>> model[1::2]
        stim.DetectorErrorModel('''
            error(0.125) D1 L1
            error(0.125) D2
            detector D5
        ''')
    """
```

<a name="stim.DetectorErrorModel.__iadd__"></a>
```python
# stim.DetectorErrorModel.__iadd__

# (in class stim.DetectorErrorModel)
def __iadd__(
    self,
    second: stim.DetectorErrorModel,
) -> stim.DetectorErrorModel:
    """Appends a detector error model into the receiving model (mutating it).

    Examples:
        >>> import stim
        >>> m1 = stim.DetectorErrorModel('''
        ...    error(0.125) D0
        ... ''')
        >>> m2 = stim.DetectorErrorModel('''
        ...    error(0.25) D1
        ... ''')
        >>> m1 += m2
        >>> print(repr(m1))
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.25) D1
        ''')
    """
```

<a name="stim.DetectorErrorModel.__imul__"></a>
```python
# stim.DetectorErrorModel.__imul__

# (in class stim.DetectorErrorModel)
def __imul__(
    self,
    repetitions: int,
) -> stim.DetectorErrorModel:
    """Mutates the detector error model by putting its contents into a repeat block.

    Special case: if the repetition count is 0, the model is cleared.
    Special case: if the repetition count is 1, nothing happens.

    Args:
        repetitions: The number of times the repeat block should repeat.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel('''
        ...    error(0.25) D0
        ...    shift_detectors 1
        ... ''')
        >>> m *= 3
        >>> print(m)
        repeat 3 {
            error(0.25) D0
            shift_detectors 1
        }
    """
```

<a name="stim.DetectorErrorModel.__init__"></a>
```python
# stim.DetectorErrorModel.__init__

# (in class stim.DetectorErrorModel)
def __init__(
    self,
    detector_error_model_text: str = '',
) -> None:
    """Creates a stim.DetectorErrorModel.

    Args:
        detector_error_model_text: Defaults to empty. Describes instructions to
            append into the circuit in the detector error model (.dem) format.

    Examples:
        >>> import stim
        >>> empty = stim.DetectorErrorModel()
        >>> not_empty = stim.DetectorErrorModel('''
        ...    error(0.125) D0 L0
        ... ''')
    """
```

<a name="stim.DetectorErrorModel.__len__"></a>
```python
# stim.DetectorErrorModel.__len__

# (in class stim.DetectorErrorModel)
def __len__(
    self,
) -> int:
    """Returns the number of top-level instructions/blocks in the detector error model.

    Instructions inside of blocks are not included in this count.

    Examples:
        >>> import stim
        >>> len(stim.DetectorErrorModel())
        0
        >>> len(stim.DetectorErrorModel('''
        ...    error(0.1) D0 D1
        ...    shift_detectors 100
        ...    logical_observable L5
        ... '''))
        3
        >>> len(stim.DetectorErrorModel('''
        ...    repeat 100 {
        ...        error(0.1) D0 D1
        ...        error(0.1) D1 D2
        ...    }
        ... '''))
        1
    """
```

<a name="stim.DetectorErrorModel.__mul__"></a>
```python
# stim.DetectorErrorModel.__mul__

# (in class stim.DetectorErrorModel)
def __mul__(
    self,
    repetitions: int,
) -> stim.DetectorErrorModel:
    """Repeats the detector error model using a repeat block.

    Has special cases for 0 repetitions and 1 repetitions.

    Args:
        repetitions: The number of times the repeat block should repeat.

    Returns:
        repetitions=0: An empty detector error model.
        repetitions=1: A copy of this detector error model.
        repetitions>=2: A detector error model with a single repeat block, where the
        contents of that repeat block are this detector error model.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel('''
        ...    error(0.25) D0
        ...    shift_detectors 1
        ... ''')
        >>> m * 3
        stim.DetectorErrorModel('''
            repeat 3 {
                error(0.25) D0
                shift_detectors 1
            }
        ''')
    """
```

<a name="stim.DetectorErrorModel.__ne__"></a>
```python
# stim.DetectorErrorModel.__ne__

# (in class stim.DetectorErrorModel)
def __ne__(
    self,
    arg0: stim.DetectorErrorModel,
) -> bool:
    """Determines if two detector error models have non-identical contents.
    """
```

<a name="stim.DetectorErrorModel.__repr__"></a>
```python
# stim.DetectorErrorModel.__repr__

# (in class stim.DetectorErrorModel)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equivalent `stim.DetectorErrorModel`.
    """
```

<a name="stim.DetectorErrorModel.__rmul__"></a>
```python
# stim.DetectorErrorModel.__rmul__

# (in class stim.DetectorErrorModel)
def __rmul__(
    self,
    repetitions: int,
) -> stim.DetectorErrorModel:
    """Repeats the detector error model using a repeat block.

    Has special cases for 0 repetitions and 1 repetitions.

    Args:
        repetitions: The number of times the repeat block should repeat.

    Returns:
        repetitions=0: An empty detector error model.
        repetitions=1: A copy of this detector error model.
        repetitions>=2: A detector error model with a single repeat block, where the
        contents of that repeat block are this detector error model.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel('''
        ...    error(0.25) D0
        ...    shift_detectors 1
        ... ''')
        >>> 3 * m
        stim.DetectorErrorModel('''
            repeat 3 {
                error(0.25) D0
                shift_detectors 1
            }
        ''')
    """
```

<a name="stim.DetectorErrorModel.__str__"></a>
```python
# stim.DetectorErrorModel.__str__

# (in class stim.DetectorErrorModel)
def __str__(
    self,
) -> str:
    """Returns the contents of a detector error model file (.dem) encoding the model.
    """
```

<a name="stim.DetectorErrorModel.append"></a>
```python
# stim.DetectorErrorModel.append

# (in class stim.DetectorErrorModel)
def append(
    self,
    instruction: object,
    parens_arguments: object = None,
    targets: List[object] = (),
) -> None:
    """Appends an instruction to the detector error model.

    Args:
        instruction: Either the name of an instruction, a stim.DemInstruction, or a
            stim.DemRepeatBlock. The `parens_arguments` and `targets` arguments are
            given if and only if the instruction is a name.
        parens_arguments: Numeric values parameterizing the instruction. The numbers
            inside parentheses in a detector error model file (eg. the `0.25` in
            `error(0.25) D0`). This argument can be given either a list of doubles,
            or a single double (which will be implicitly wrapped into a list).
        targets: The instruction targets, such as the `D0` in `error(0.25) D0`.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.125, [
        ...     stim.DemTarget.relative_detector_id(1),
        ... ])
        >>> m.append("error", 0.25, [
        ...     stim.DemTarget.relative_detector_id(1),
        ...     stim.DemTarget.separator(),
        ...     stim.DemTarget.relative_detector_id(2),
        ...     stim.DemTarget.logical_observable_id(3),
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.125) D1
            error(0.25) D1 ^ D2 L3
        ''')

        >>> m.append("shift_detectors", (1, 2, 3), [5])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.125) D1
            error(0.25) D1 ^ D2 L3
            shift_detectors(1, 2, 3) 5
        ''')

        >>> m += m * 3
        >>> m.append(m[0])
        >>> m.append(m[-2])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.125) D1
            error(0.25) D1 ^ D2 L3
            shift_detectors(1, 2, 3) 5
            repeat 3 {
                error(0.125) D1
                error(0.25) D1 ^ D2 L3
                shift_detectors(1, 2, 3) 5
            }
            error(0.125) D1
            repeat 3 {
                error(0.125) D1
                error(0.25) D1 ^ D2 L3
                shift_detectors(1, 2, 3) 5
            }
        ''')
    """
```

<a name="stim.DetectorErrorModel.approx_equals"></a>
```python
# stim.DetectorErrorModel.approx_equals

# (in class stim.DetectorErrorModel)
def approx_equals(
    self,
    other: object,
    *,
    atol: float,
) -> bool:
    """Checks if detector error models are approximately equal.

    Two detector error model are approximately equal if they are equal up to slight
    perturbations of instruction arguments such as probabilities. For example
    `error(0.100) D0` is approximately equal to `error(0.099) D0` within an absolute
    tolerance of 0.002. All other details of the models (such as the ordering of
    errors and their targets) must be exactly the same.

    Args:
        other: The detector error model, or other object, to compare to this one.
        atol: The absolute error tolerance. The maximum amount each probability may
            have been perturbed by.

    Returns:
        True if the given object is a detector error model approximately equal up to
        the receiving circuit up to the given tolerance, otherwise False.

    Examples:
        >>> import stim
        >>> base = stim.DetectorErrorModel('''
        ...    error(0.099) D0 D1
        ... ''')

        >>> base.approx_equals(base, atol=0)
        True

        >>> base.approx_equals(stim.DetectorErrorModel('''
        ...    error(0.101) D0 D1
        ... '''), atol=0)
        False

        >>> base.approx_equals(stim.DetectorErrorModel('''
        ...    error(0.101) D0 D1
        ... '''), atol=0.0001)
        False

        >>> base.approx_equals(stim.DetectorErrorModel('''
        ...    error(0.101) D0 D1
        ... '''), atol=0.01)
        True

        >>> base.approx_equals(stim.DetectorErrorModel('''
        ...    error(0.099) D0 D1 L0 L1 L2 L3 L4
        ... '''), atol=9999)
        False
    """
```

<a name="stim.DetectorErrorModel.clear"></a>
```python
# stim.DetectorErrorModel.clear

# (in class stim.DetectorErrorModel)
def clear(
    self,
) -> None:
    """Clears the contents of the detector error model.

    Examples:
        >>> import stim
        >>> model = stim.DetectorErrorModel('''
        ...    error(0.1) D0 D1
        ... ''')
        >>> model.clear()
        >>> model
        stim.DetectorErrorModel()
    """
```

<a name="stim.DetectorErrorModel.compile_sampler"></a>
```python
# stim.DetectorErrorModel.compile_sampler

# (in class stim.DetectorErrorModel)
def compile_sampler(
    self,
    *,
    seed: object = None,
) -> stim.CompiledDemSampler:
    """Returns a CompiledDemSampler that can batch sample from detector error models.

    Args:
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how many
            shots are taken. For example, taking 10 shots and then 90 shots will
            give different results from taking 100 shots in one call.

    Returns:
        A seeded stim.CompiledDemSampler for the given detector error model.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0) D0
        ...    error(1) D1 D2 L0
        ... ''')
        >>> sampler = dem.compile_sampler()
        >>> det_data, obs_data, err_data = sampler.sample(
        ...     shots=4,
        ...     return_errors=True)
        >>> det_data
        array([[False,  True,  True],
               [False,  True,  True],
               [False,  True,  True],
               [False,  True,  True]])
        >>> obs_data
        array([[ True],
               [ True],
               [ True],
               [ True]])
        >>> err_data
        array([[False,  True],
               [False,  True],
               [False,  True],
               [False,  True]])
    """
```

<a name="stim.DetectorErrorModel.copy"></a>
```python
# stim.DetectorErrorModel.copy

# (in class stim.DetectorErrorModel)
def copy(
    self,
) -> stim.DetectorErrorModel:
    """Returns a copy of the detector error model.

    The copy is an independent detector error model with the same contents.

    Examples:
        >>> import stim

        >>> c1 = stim.DetectorErrorModel("error(0.1) D0 D1")
        >>> c2 = c1.copy()
        >>> c2 is c1
        False
        >>> c2 == c1
        True
    """
```

<a name="stim.DetectorErrorModel.diagram"></a>
```python
# stim.DetectorErrorModel.diagram

# (in class stim.DetectorErrorModel)
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-svg"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-3d"]',
) -> 'stim._DiagramHelper':
    pass
@overload
def diagram(
    self,
    type: 'Literal["matchgraph-3d-html"]',
) -> 'stim._DiagramHelper':
    pass
def diagram(
    self,
    type: str,
) -> Any:
    """Returns a diagram of the circuit, from a variety of options.

    Args:
        type: The type of diagram. Available types are:
            "matchgraph-svg": An image of the decoding graph of the
                detector error model. Red lines are errors crossing a
                logical observable. Blue lines are undecomposed hyper
                errors.
            "matchgraph-3d": A 3d model of the decoding graph of the
                detector error model. Red lines are errors crossing a
                logical observable. Blue lines are undecomposed hyper
                errors.

                GLTF files can be opened with a variety of programs, or
                opened online in viewers such as
                https://gltf-viewer.donmccurdy.com/ . Red lines are
                errors crossing a logical observable.
            "matchgraph-3d-html": Same 3d model as 'match-graph-3d' but
                embedded into an HTML web page containing an interactive
                THREE.js viewer for the 3d model.

    Returns:
        An object whose `__str__` method returns the diagram, so that
        writing the diagram to a file works correctly. The returned
        object also defines a `_repr_html_` method, so that ipython
        notebooks recognize it can be shown using a specialized
        viewer instead of as raw text.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> circuit = stim.Circuit.generated(
        ...     "repetition_code:memory",
        ...     rounds=10,
        ...     distance=7,
        ...     after_clifford_depolarization=0.01)
        >>> dem = circuit.detector_error_model(decompose_errors=True)

        >>> with tempfile.TemporaryDirectory() as d:
        ...     diagram = circuit.diagram(type="match-graph-svg")
        ...     with open(f"{d}/dem_image.svg", "w") as f:
        ...         print(diagram, file=f)

        >>> with tempfile.TemporaryDirectory() as d:
        ...     diagram = circuit.diagram(type="match-graph-3d")
        ...     with open(f"{d}/dem_3d_model.gltf", "w") as f:
        ...         print(diagram, file=f)
    """
```

<a name="stim.DetectorErrorModel.flattened"></a>
```python
# stim.DetectorErrorModel.flattened

# (in class stim.DetectorErrorModel)
def flattened(
    self,
) -> stim.DetectorErrorModel:
    """Returns the detector error model without repeat or detector_shift instructions.

    Returns:
        A `stim.DetectorErrorModel` with the same errors in the same order, but with
        repeat loops flattened into actually repeated instructions and with all
        coordinate/index shifts inlined.

    Examples:
        >>> import stim
        >>> stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     REPEAT 5 {
        ...         error(0.25) D0 D1
        ...         shift_detectors 1
        ...     }
        ...     error(0.125) D0 L0
        ... ''').flattened()
        stim.DetectorErrorModel('''
            error(0.125) D0
            error(0.25) D0 D1
            error(0.25) D1 D2
            error(0.25) D2 D3
            error(0.25) D3 D4
            error(0.25) D4 D5
            error(0.125) D5 L0
        ''')
    """
```

<a name="stim.DetectorErrorModel.from_file"></a>
```python
# stim.DetectorErrorModel.from_file

# (in class stim.DetectorErrorModel)
@staticmethod
def from_file(
    file: Union[io.TextIOBase, str, pathlib.Path],
) -> stim.DetectorErrorModel:
    """Reads a detector error model from a file.

    The file format is defined at
    https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

    Args:
        file: A file path or open file object to read from.

    Returns:
        The circuit parsed from the file.

    Examples:
        >>> import stim
        >>> import tempfile

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         print('error(0.25) D2 D3', file=f)
        ...     circuit = stim.DetectorErrorModel.from_file(path)
        >>> circuit
        stim.DetectorErrorModel('''
            error(0.25) D2 D3
        ''')

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         print('error(0.25) D2 D3', file=f)
        ...     with open(path) as f:
        ...         circuit = stim.DetectorErrorModel.from_file(path)
        >>> circuit
        stim.DetectorErrorModel('''
            error(0.25) D2 D3
        ''')
    """
```

<a name="stim.DetectorErrorModel.get_detector_coordinates"></a>
```python
# stim.DetectorErrorModel.get_detector_coordinates

# (in class stim.DetectorErrorModel)
def get_detector_coordinates(
    self,
    only: object = None,
) -> Dict[int, List[float]]:
    """Returns the coordinate metadata of detectors in the detector error model.

    Args:
        only: Defaults to None (meaning include all detectors). A list of detector
            indices to include in the result. Detector indices beyond the end of the
            detector error model cause an error.

    Returns:
        A dictionary mapping integers (detector indices) to lists of floats
        (coordinates). Detectors with no specified coordinate data are mapped to an
        empty tuple. If `only` is specified, then `set(result.keys()) == set(only)`.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...    error(0.25) D0 D1
        ...    detector(1, 2, 3) D1
        ...    shift_detectors(5) 1
        ...    detector(1, 2) D2
        ... ''')
        >>> dem.get_detector_coordinates()
        {0: [], 1: [1.0, 2.0, 3.0], 2: [], 3: [6.0, 2.0]}
        >>> dem.get_detector_coordinates(only=[1])
        {1: [1.0, 2.0, 3.0]}
    """
```

<a name="stim.DetectorErrorModel.num_detectors"></a>
```python
# stim.DetectorErrorModel.num_detectors

# (in class stim.DetectorErrorModel)
@property
def num_detectors(
    self,
) -> int:
    """Counts the number of detectors (e.g. `D2`) in the error model.

    Detector indices are assumed to be contiguous from 0 up to whatever the maximum
    detector id is. If the largest detector's absolute id is n-1, then the number of
    detectors is n.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     X_ERROR(0.125) 0
        ...     X_ERROR(0.25) 1
        ...     CORRELATED_ERROR(0.375) X0 X1
        ...     M 0 1
        ...     DETECTOR rec[-2]
        ...     DETECTOR rec[-1]
        ... ''').detector_error_model().num_detectors
        2

        >>> stim.DetectorErrorModel('''
        ...    error(0.1) D0 D199
        ... ''').num_detectors
        200

        >>> stim.DetectorErrorModel('''
        ...    shift_detectors 1000
        ...    error(0.1) D0 D199
        ... ''').num_detectors
        1200
    """
```

<a name="stim.DetectorErrorModel.num_errors"></a>
```python
# stim.DetectorErrorModel.num_errors

# (in class stim.DetectorErrorModel)
@property
def num_errors(
    self,
) -> int:
    """Counts the number of errors (e.g. `error(0.1) D0`) in the error model.

    Error instructions inside repeat blocks count once per repetition.
    Redundant errors with the same targets count as separate errors.

    Examples:
        >>> import stim

        >>> stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     repeat 100 {
        ...         repeat 5 {
        ...             error(0.25) D1
        ...         }
        ...     }
        ... ''').num_errors
        501
    """
```

<a name="stim.DetectorErrorModel.num_observables"></a>
```python
# stim.DetectorErrorModel.num_observables

# (in class stim.DetectorErrorModel)
@property
def num_observables(
    self,
) -> int:
    """Counts the number of frame changes (e.g. `L2`) in the error model.

    Observable indices are assumed to be contiguous from 0 up to whatever the
    maximum observable id is. If the largest observable's id is n-1, then the number
    of observables is n.

    Examples:
        >>> import stim

        >>> stim.Circuit('''
        ...     X_ERROR(0.125) 0
        ...     M 0
        ...     OBSERVABLE_INCLUDE(99) rec[-1]
        ... ''').detector_error_model().num_observables
        100

        >>> stim.DetectorErrorModel('''
        ...    error(0.1) L399
        ... ''').num_observables
        400
    """
```

<a name="stim.DetectorErrorModel.rounded"></a>
```python
# stim.DetectorErrorModel.rounded

# (in class stim.DetectorErrorModel)
def rounded(
    self,
    arg0: int,
) -> stim.DetectorErrorModel:
    """Creates an equivalent detector error model but with rounded error probabilities.

    Args:
        digits: The number of digits to round to.

    Returns:
        A `stim.DetectorErrorModel` with the same instructions in the same order,
        but with the parens arguments of error instructions rounded to the given
        precision.

        Instructions whose error probability was rounded to zero are still
        included in the output.

    Examples:
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     error(0.019499) D0
        ...     error(0.000001) D0 D1
        ... ''')

        >>> dem.rounded(2)
        stim.DetectorErrorModel('''
            error(0.02) D0
            error(0) D0 D1
        ''')

        >>> dem.rounded(3)
        stim.DetectorErrorModel('''
            error(0.019) D0
            error(0) D0 D1
        ''')
    """
```

<a name="stim.DetectorErrorModel.shortest_graphlike_error"></a>
```python
# stim.DetectorErrorModel.shortest_graphlike_error

# (in class stim.DetectorErrorModel)
def shortest_graphlike_error(
    self,
    ignore_ungraphlike_errors: bool = True,
) -> stim.DetectorErrorModel:
    """Finds a minimum set of graphlike errors to produce an undetected logical error.

    Note that this method does not pay attention to error probabilities (other than
    ignoring errors with probability 0). It searches for a logical error with the
    minimum *number* of physical errors, not the maximum probability of those
    physical errors all occurring.

    This method works by looking for errors that have frame changes (eg.
    "error(0.1) D0 D1 L5" flips the frame of observable 5). These errors are
    converted into one or two symptoms and a net frame change. The symptoms can then
    be moved around by following errors touching that symptom. Each symptom is moved
    until it disappears into a boundary or cancels against another remaining
    symptom, while leaving the other symptoms alone (ensuring only one symptom is
    allowed to move significantly reduces waste in the search space). Eventually a
    path or cycle of errors is found that cancels out the symptoms, and if there is
    still a frame change at that point then that path or cycle is a logical error
    (otherwise all that was found was a stabilizer of the system; a dead end). The
    search process advances like a breadth first search, seeded from all the
    frame-change errors and branching them outward in tandem, until one of them wins
    the race to find a solution.

    Args:
        ignore_ungraphlike_errors: Defaults to False. When False, an exception is
            raised if there are any errors in the model that are not graphlike. When
            True, those errors are skipped as if they weren't present.

            A graphlike error is an error with less than two symptoms. For the
            purposes of this method, errors are also considered graphlike if they
            are decomposed into graphlike components:

            graphlike:
                error(0.1) D0
                error(0.1) D0 D1
                error(0.1) D0 D1 L0
            not graphlike but decomposed into graphlike components:
                error(0.1) D0 D1 ^ D2
            not graphlike, not decomposed into graphlike components:
                error(0.1) D0 D1 D2
                error(0.1) D0 D1 D2 ^ D3

    Returns:
        A detector error model containing just the error instructions corresponding
        to an undetectable logical error. There will be no other kinds of
        instructions (no `repeat`s, no `shift_detectors`, etc). The error
        probabilities will all be set to 1.

        The `len` of the returned model is the graphlike code distance of the
        circuit. But beware that in general the true code distance may be smaller.
        For example, in the XZ surface code with twists, the true minimum sized
        logical error is likely to use Y errors. But each Y error decomposes into
        two graphlike components (the X part and the Z part). As a result, the
        graphlike code distance in that context is likely to be nearly twice as
        large as the true code distance.

    Examples:
        >>> import stim

        >>> stim.DetectorErrorModel('''
        ...     error(0.125) D0
        ...     error(0.125) D0 D1
        ...     error(0.125) D1 L55
        ...     error(0.125) D1
        ... ''').shortest_graphlike_error()
        stim.DetectorErrorModel('''
            error(1) D1
            error(1) D1 L55
        ''')

        >>> stim.DetectorErrorModel('''
        ...     error(0.125) D0 D1 D2
        ...     error(0.125) L0
        ... ''').shortest_graphlike_error(ignore_ungraphlike_errors=True)
        stim.DetectorErrorModel('''
            error(1) L0
        ''')

        >>> circuit = stim.Circuit.generated(
        ...     "repetition_code:memory",
        ...     rounds=10,
        ...     distance=7,
        ...     before_round_data_depolarization=0.01)
        >>> model = circuit.detector_error_model(decompose_errors=True)
        >>> len(model.shortest_graphlike_error())
        7
    """
```

<a name="stim.DetectorErrorModel.to_file"></a>
```python
# stim.DetectorErrorModel.to_file

# (in class stim.DetectorErrorModel)
def to_file(
    self,
    file: Union[io.TextIOBase, str, pathlib.Path],
) -> None:
    """Writes the detector error model to a file.

    The file format is defined at
    https://github.com/quantumlib/Stim/blob/main/doc/file_format_dem_detector_error_model.md

    Args:
        file: A file path or an open file to write to.

    Examples:
        >>> import stim
        >>> import tempfile
        >>> c = stim.DetectorErrorModel('error(0.25) D2 D3')

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     with open(path, 'w') as f:
        ...         c.to_file(f)
        ...     with open(path) as f:
        ...         contents = f.read()
        >>> contents
        'error(0.25) D2 D3\n'

        >>> with tempfile.TemporaryDirectory() as tmpdir:
        ...     path = tmpdir + '/tmp.stim'
        ...     c.to_file(path)
        ...     with open(path) as f:
        ...         contents = f.read()
        >>> contents
        'error(0.25) D2 D3\n'
    """
```

<a name="stim.ExplainedError"></a>
```python
# stim.ExplainedError

# (at top-level in the stim module)
class ExplainedError:
    """Describes the location of an error mechanism from a stim circuit.
    """
```

<a name="stim.ExplainedError.__init__"></a>
```python
# stim.ExplainedError.__init__

# (in class stim.ExplainedError)
def __init__(
    self,
    *,
    dem_error_terms: List[stim.DemTargetWithCoords],
    circuit_error_locations: List[stim.CircuitErrorLocation],
) -> None:
    """Creates a stim.ExplainedError.
    """
```

<a name="stim.ExplainedError.circuit_error_locations"></a>
```python
# stim.ExplainedError.circuit_error_locations

# (in class stim.ExplainedError)
@property
def circuit_error_locations(
    self,
) -> List[stim.CircuitErrorLocation]:
    """The locations of circuit errors that produce the symptoms in dem_error_terms.

    Note: if this list contains a single entry, it may be because a result
    with a single representative error was requested (as opposed to all possible
    errors).

    Note: if this list is empty, it may be because there was a DEM error decomposed
    into parts where one of the parts is impossible to make on its own from a single
    circuit error.
    """
```

<a name="stim.ExplainedError.dem_error_terms"></a>
```python
# stim.ExplainedError.dem_error_terms

# (in class stim.ExplainedError)
@property
def dem_error_terms(
    self,
) -> List[stim.DemTargetWithCoords]:
    """The detectors and observables flipped by this error mechanism.
    """
```

<a name="stim.FlipSimulator"></a>
```python
# stim.FlipSimulator

# (at top-level in the stim module)
class FlipSimulator:
    """A simulator that tracks whether things are flipped, instead of what they are.

    Tracking flips is significantly cheaper than tracking actual values, requiring
    O(1) work per gate (compared to O(n) for unitary operations and O(n^2) for
    collapsing operations in the tableau simulator, where n is the qubit count).

    Supports interactive usage, where gates and measurements are applied on demand.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
    """
```

<a name="stim.FlipSimulator.__init__"></a>
```python
# stim.FlipSimulator.__init__

# (in class stim.FlipSimulator)
def __init__(
    self,
    *,
    batch_size: int,
    disable_stabilizer_randomization: bool = False,
    num_qubits: int = 0,
    seed: Optional[int] = None,
) -> None:
    """Initializes a stim.FlipSimulator.

    Args:
        batch_size: For speed, the flip simulator simulates many instances in
            parallel. This argument determines the number of parallel instances.

            It's recommended to use a multiple of 256, because internally the state
            of the instances is striped across SSE (128 bit) or AVX (256 bit)
            words with one bit in the word belonging to each instance. The result is
            that, even if you only ask for 1 instance, probably the same amount of
            work is being done as if you'd asked for 256 instances. The extra
            results just aren't being used, creating waste.

        disable_stabilizer_randomization: Determines whether or not the flip
            simulator uses stabilizer randomization. Defaults to False (stabilizer
            randomization used). Set to True to disable stabilizer randomization.

            Stabilizer randomization means that, when a qubit is initialized or
            measured in the Z basis, a Z error is added to the qubit with 50%
            probability. More generally, anytime a stabilizer is introduced into
            the system by any means, an error equal to that stabilizer is applied
            with 50% probability. This ensures that observables anticommuting with
            stabilizers of the system must be maximally uncertain. In other words,
            this feature enforces Heisenberg's uncertainty principle.

            This is a safety feature that you should not turn off unless you have a
            reason to do so. Stabilizer randomization is turned on by default
            because it catches mistakes. For example, suppose you are trying to
            create a stabilizer code but you accidentally have the code measure two
            anticommuting stabilizers. With stabilizer randomization turned off, it
            will look like this code works. With stabilizer randomization turned on,
            the two measurements will correctly randomize each other revealing that
            the code doesn't work.

            In some use cases, stabilizer randomization is a hindrance instead of
            helpful. For example, if you are using the flip simulator to understand
            how an error propagates through the system, the stabilizer randomization
            will be introducing error terms that you don't want.

        num_qubits: Sets the initial number of qubits tracked by the simulation.
            The simulator will still automatically resize as needed when qubits
            beyond this limit are touched.

            This parameter exists as a way to hint at the desired size of the
            simulator's state for performance, and to ensure methods that
            peek at the size have the expected size from the start instead of
            only after the relevant qubits have been touched.

        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how the
            circuit is executed. For example, reordering whether a reset on one
            qubit happens before or after a reset on another qubit can result in
            different measurement results being observed starting from the same
            seed.

    Returns:
        An initialized stim.FlipSimulator.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
    """
```

<a name="stim.FlipSimulator.batch_size"></a>
```python
# stim.FlipSimulator.batch_size

# (in class stim.FlipSimulator)
@property
def batch_size(
    self,
) -> int:
    """Returns the number of instances being simulated by the simulator.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
        >>> sim.batch_size
        256
        >>> sim = stim.FlipSimulator(batch_size=42)
        >>> sim.batch_size
        42
    """
```

<a name="stim.FlipSimulator.do"></a>
```python
# stim.FlipSimulator.do

# (in class stim.FlipSimulator)
def do(
    self,
    obj: Union[stim.Circuit, stim.CircuitInstruction, stim.CircuitRepeatBlock],
) -> None:
    """Applies a circuit or circuit instruction to the simulator's state.

    The results of any measurements performed can be retrieved using the
    `get_measurement_flips` method.

    Args:
        obj: The circuit or instruction to apply to the simulator's state.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(
        ...     batch_size=1,
        ...     disable_stabilizer_randomization=True,
        ... )
        >>> circuit = stim.Circuit('''
        ...     X_ERROR(1) 0 1 3
        ...     REPEAT 5 {
        ...         H 0
        ...         C_XYZ 1
        ...     }
        ... ''')
        >>> sim.do(circuit)
        >>> sim.peek_pauli_flips()
        [stim.PauliString("+ZZ_X")]

        >>> sim.do(circuit[0])
        >>> sim.peek_pauli_flips()
        [stim.PauliString("+YY__")]

        >>> sim.do(circuit[1])
        >>> sim.peek_pauli_flips()
        [stim.PauliString("+YX__")]
    """
```

<a name="stim.FlipSimulator.get_detector_flips"></a>
```python
# stim.FlipSimulator.get_detector_flips

# (in class stim.FlipSimulator)
def get_detector_flips(
    self,
    *,
    detector_index: Optional[int] = None,
    instance_index: Optional[int] = None,
    bit_packed: bool = False,
) -> np.ndarray:
    """Retrieves detector flip data from the simulator's detection event record.

    Args:
        record_index: Identifies a detector to read results from.
            Setting this to None (default) returns results from all detectors.
            Otherwise this should be an integer in range(0, self.num_detectors).
        instance_index: Identifies a simulation instance to read results from.
            Setting this to None (the default) returns results from all instances.
            Otherwise this should be an integer in range(0, self.batch_size).
        bit_packed: Defaults to False. Determines whether the result is bit packed.
            If this is set to true, the returned numpy array will be bit packed as
            if by applying

                out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

            Behind the scenes the data is always bit packed, so setting this
            argument avoids ever unpacking in the first place. This substantially
            improves performance when there is a lot of data.

    Returns:
        A numpy array containing the requested data. By default this is a 2d array
        of shape (self.num_detectors, self.batch_size), where the first index is
        the detector_index and the second index is the instance_index and the
        dtype is np.bool_.

        Specifying detector_index slices away the first index, leaving a 1d array
        with only an instance_index.

        Specifying instance_index slices away the last index, leaving a 1d array
        with only a detector_index (or a 0d array, a boolean, if detector_index
        was also specified).

        Specifying bit_packed=True bit packs the last remaining index, changing
        the dtype to np.uint8.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=9)
        >>> sim.do(stim.Circuit('''
        ...     M 0 0 0
        ...     DETECTOR rec[-2] rec[-3]
        ...     DETECTOR rec[-1] rec[-2]
        ... '''))

        >>> sim.get_detector_flips()
        array([[False, False, False, False, False, False, False, False, False],
               [False, False, False, False, False, False, False, False, False]])

        >>> sim.get_detector_flips(bit_packed=True)
        array([[0, 0],
               [0, 0]], dtype=uint8)

        >>> sim.get_detector_flips(instance_index=2)
        array([False, False])

        >>> sim.get_detector_flips(detector_index=1)
        array([False, False, False, False, False, False, False, False, False])

        >>> sim.get_detector_flips(instance_index=2, detector_index=1)
        array(False)
    """
```

<a name="stim.FlipSimulator.get_measurement_flips"></a>
```python
# stim.FlipSimulator.get_measurement_flips

# (in class stim.FlipSimulator)
def get_measurement_flips(
    self,
    *,
    record_index: Optional[int] = None,
    instance_index: Optional[int] = None,
    bit_packed: bool = False,
) -> np.ndarray:
    """Retrieves measurement flip data from the simulator's measurement record.

    Args:
        record_index: Identifies a measurement to read results from.
            Setting this to None (default) returns results from all measurements.
            Setting this to a non-negative integer indexes measurements by the order
                they occurred. For example, record index 0 is the first measurement.
            Setting this to a negative integer indexes measurements by recency.
                For example, recording index -1 is the most recent measurement.
        instance_index: Identifies a simulation instance to read results from.
            Setting this to None (the default) returns results from all instances.
            Otherwise this should be set to an integer in range(0, self.batch_size).
        bit_packed: Defaults to False. Determines whether the result is bit packed.
            If this is set to true, the returned numpy array will be bit packed as
            if by applying

                out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

            Behind the scenes the data is always bit packed, so setting this
            argument avoids ever unpacking in the first place. This substantially
            improves performance when there is a lot of data.

    Returns:
        A numpy array containing the requested data. By default this is a 2d array
        of shape (self.num_measurements, self.batch_size), where the first index is
        the measurement_index and the second index is the instance_index and the
        dtype is np.bool_.

        Specifying record_index slices away the first index, leaving a 1d array
        with only an instance_index.

        Specifying instance_index slices away the last index, leaving a 1d array
        with only a measurement_index (or a 0d array, a boolean, if record_index
        was also specified).

        Specifying bit_packed=True bit packs the last remaining index, changing
        the dtype to np.uint8.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=9)
        >>> sim.do(stim.Circuit('M 0 1 2'))

        >>> sim.get_measurement_flips()
        array([[False, False, False, False, False, False, False, False, False],
               [False, False, False, False, False, False, False, False, False],
               [False, False, False, False, False, False, False, False, False]])

        >>> sim.get_measurement_flips(bit_packed=True)
        array([[0, 0],
               [0, 0],
               [0, 0]], dtype=uint8)

        >>> sim.get_measurement_flips(instance_index=1)
        array([False, False, False])

        >>> sim.get_measurement_flips(record_index=2)
        array([False, False, False, False, False, False, False, False, False])

        >>> sim.get_measurement_flips(instance_index=1, record_index=2)
        array(False)
    """
```

<a name="stim.FlipSimulator.get_observable_flips"></a>
```python
# stim.FlipSimulator.get_observable_flips

# (in class stim.FlipSimulator)
def get_observable_flips(
    self,
    *,
    observable_index: Optional[int] = None,
    instance_index: Optional[int] = None,
    bit_packed: bool = False,
) -> np.ndarray:
    """Retrieves observable flip data from the simulator's detection event record.

    Args:
        record_index: Identifies a observable to read results from.
            Setting this to None (default) returns results from all observables.
            Otherwise this should be an integer in range(0, self.num_observables).
        instance_index: Identifies a simulation instance to read results from.
            Setting this to None (the default) returns results from all instances.
            Otherwise this should be an integer in range(0, self.batch_size).
        bit_packed: Defaults to False. Determines whether the result is bit packed.
            If this is set to true, the returned numpy array will be bit packed as
            if by applying

                out = np.packbits(out, axis=len(out.shape) - 1, bitorder='little')

            Behind the scenes the data is always bit packed, so setting this
            argument avoids ever unpacking in the first place. This substantially
            improves performance when there is a lot of data.

    Returns:
        A numpy array containing the requested data. By default this is a 2d array
        of shape (self.num_observables, self.batch_size), where the first index is
        the observable_index and the second index is the instance_index and the
        dtype is np.bool_.

        Specifying observable_index slices away the first index, leaving a 1d array
        with only an instance_index.

        Specifying instance_index slices away the last index, leaving a 1d array
        with only a observable_index (or a 0d array, a boolean, if observable_index
        was also specified).

        Specifying bit_packed=True bit packs the last remaining index, changing
        the dtype to np.uint8.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=9)
        >>> sim.do(stim.Circuit('''
        ...     M 0 0 0
        ...     OBSERVABLE_INCLUDE(0) rec[-2]
        ...     OBSERVABLE_INCLUDE(1) rec[-1]
        ... '''))

        >>> sim.get_observable_flips()
        array([[False, False, False, False, False, False, False, False, False],
               [False, False, False, False, False, False, False, False, False]])

        >>> sim.get_observable_flips(bit_packed=True)
        array([[0, 0],
               [0, 0]], dtype=uint8)

        >>> sim.get_observable_flips(instance_index=2)
        array([False, False])

        >>> sim.get_observable_flips(observable_index=1)
        array([False, False, False, False, False, False, False, False, False])

        >>> sim.get_observable_flips(instance_index=2, observable_index=1)
        array(False)
    """
```

<a name="stim.FlipSimulator.num_detectors"></a>
```python
# stim.FlipSimulator.num_detectors

# (in class stim.FlipSimulator)
@property
def num_detectors(
    self,
) -> int:
    """Returns the number of detectors that have been simulated and stored.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
        >>> sim.num_detectors
        0
        >>> sim.do(stim.Circuit('''
        ...     M 0 0
        ...     DETECTOR rec[-1] rec[-2]
        ... '''))
        >>> sim.num_detectors
        1
    """
```

<a name="stim.FlipSimulator.num_measurements"></a>
```python
# stim.FlipSimulator.num_measurements

# (in class stim.FlipSimulator)
@property
def num_measurements(
    self,
) -> int:
    """Returns the number of measurements that have been simulated and stored.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
        >>> sim.num_measurements
        0
        >>> sim.do(stim.Circuit('M 3 5'))
        >>> sim.num_measurements
        2
    """
```

<a name="stim.FlipSimulator.num_observables"></a>
```python
# stim.FlipSimulator.num_observables

# (in class stim.FlipSimulator)
@property
def num_observables(
    self,
) -> int:
    """Returns the number of observables currently tracked by the simulator.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
        >>> sim.num_observables
        0
        >>> sim.do(stim.Circuit('''
        ...     M 0
        ...     OBSERVABLE_INCLUDE(4) rec[-1]
        ... '''))
        >>> sim.num_observables
        5
    """
```

<a name="stim.FlipSimulator.num_qubits"></a>
```python
# stim.FlipSimulator.num_qubits

# (in class stim.FlipSimulator)
@property
def num_qubits(
    self,
) -> int:
    """Returns the number of qubits currently tracked by the simulator.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(batch_size=256)
        >>> sim.num_qubits
        0
        >>> sim = stim.FlipSimulator(batch_size=256, num_qubits=4)
        >>> sim.num_qubits
        4
        >>> sim.do(stim.Circuit('H 5'))
        >>> sim.num_qubits
        6
    """
```

<a name="stim.FlipSimulator.peek_pauli_flips"></a>
```python
# stim.FlipSimulator.peek_pauli_flips

# (in class stim.FlipSimulator)
@overload
def peek_pauli_flips(
    self,
) -> List[stim.PauliString]:
    pass
@overload
def peek_pauli_flips(
    self,
    *,
    instance_index: int,
) -> stim.PauliString:
    pass
def peek_pauli_flips(
    self,
    *,
    instance_index: Optional[int] = None,
) -> Union[stim.PauliString, List[stim.PauliString]]:
    """Returns the current pauli errors packed into stim.PauliString instances.

    Args:
        instance_index: Defaults to None. When set to None, the pauli errors from
            all instances are returned as a list of `stim.PauliString`. When set to
            an integer, a single `stim.PauliString` is returned containing the
            errors for the indexed instance.

    Returns:
        if instance_index is None:
            A list of stim.PauliString, with the k'th entry being the errors from
            the k'th simulation instance.
        else:
            A stim.PauliString with the errors from the k'th simulation instance.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(
        ...     batch_size=2,
        ...     disable_stabilizer_randomization=True,
        ...     num_qubits=10,
        ... )

        >>> sim.peek_pauli_flips()
        [stim.PauliString("+__________"), stim.PauliString("+__________")]

        >>> sim.peek_pauli_flips(instance_index=0)
        stim.PauliString("+__________")

        >>> sim.do(stim.Circuit('''
        ...     X_ERROR(1) 0 3 5
        ...     Z_ERROR(1) 3 6
        ... '''))

        >>> sim.peek_pauli_flips()
        [stim.PauliString("+X__Y_XZ___"), stim.PauliString("+X__Y_XZ___")]

        >>> sim = stim.FlipSimulator(
        ...     batch_size=1,
        ...     num_qubits=100,
        ... )
        >>> flips: stim.PauliString = sim.peek_pauli_flips(instance_index=0)
        >>> sorted(set(str(flips)))  # Should have Zs from stabilizer randomization
        ['+', 'Z', '_']
    """
```

<a name="stim.FlipSimulator.set_pauli_flip"></a>
```python
# stim.FlipSimulator.set_pauli_flip

# (in class stim.FlipSimulator)
def set_pauli_flip(
    self,
    pauli: Union[str, int],
    *,
    qubit_index: int,
    instance_index: int,
) -> None:
    """Sets the pauli flip on a given qubit in a given simulation instance.

    Args:
        pauli: The pauli, specified as an integer or string.
            Uses the convention 0=I, 1=X, 2=Y, 3=Z.
            Any value from [0, 1, 2, 3, 'X', 'Y', 'Z', 'I', '_'] is allowed.
        qubit_index: The qubit to put the error on. Must be non-negative. The state
            will automatically expand as needed to store the error.
        instance_index: The simulation index to put the error inside. Use negative
            indices to index from the end of the list.

    Examples:
        >>> import stim
        >>> sim = stim.FlipSimulator(
        ...     batch_size=2,
        ...     num_qubits=3,
        ...     disable_stabilizer_randomization=True,
        ... )
        >>> sim.set_pauli_flip('X', qubit_index=2, instance_index=1)
        >>> sim.peek_pauli_flips()
        [stim.PauliString("+___"), stim.PauliString("+__X")]
    """
```

<a name="stim.FlippedMeasurement"></a>
```python
# stim.FlippedMeasurement

# (at top-level in the stim module)
class FlippedMeasurement:
    """Describes a measurement that was flipped.

    Gives the measurement's index in the measurement record, and also
    the observable of the measurement.
    """
```

<a name="stim.FlippedMeasurement.__init__"></a>
```python
# stim.FlippedMeasurement.__init__

# (in class stim.FlippedMeasurement)
def __init__(
    self,
    *,
    record_index: int,
    observable: object,
) -> None:
    """Creates a stim.FlippedMeasurement.
    """
```

<a name="stim.FlippedMeasurement.observable"></a>
```python
# stim.FlippedMeasurement.observable

# (in class stim.FlippedMeasurement)
@property
def observable(
    self,
) -> List[stim.GateTargetWithCoords]:
    """Returns the observable of the flipped measurement.

    For example, an `MX 5` measurement will have the observable X5.
    """
```

<a name="stim.FlippedMeasurement.record_index"></a>
```python
# stim.FlippedMeasurement.record_index

# (in class stim.FlippedMeasurement)
@property
def record_index(
    self,
) -> int:
    """The measurement record index of the flipped measurement.
    For example, the fifth measurement in a circuit has a measurement
    record index of 4.
    """
```

<a name="stim.GateData"></a>
```python
# stim.GateData

# (at top-level in the stim module)
class GateData:
    """Details about a gate supported by stim.

    Examples:
        >>> import stim
        >>> stim.gate_data('h').name
        'H'
        >>> stim.gate_data('h').is_unitary
        True
        >>> stim.gate_data('h').tableau
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+X"),
            ],
        )
    """
```

<a name="stim.GateData.__eq__"></a>
```python
# stim.GateData.__eq__

# (in class stim.GateData)
def __eq__(
    self,
    arg0: stim.GateData,
) -> bool:
    """Determines if two GateData instances are identical.
    """
```

<a name="stim.GateData.__init__"></a>
```python
# stim.GateData.__init__

# (in class stim.GateData)
def __init__(
    self,
    name: str,
) -> None:
    """Finds gate data for the named gate.

    Examples:
        >>> import stim
        >>> stim.GateData('H').is_unitary
        True
    """
```

<a name="stim.GateData.__ne__"></a>
```python
# stim.GateData.__ne__

# (in class stim.GateData)
def __ne__(
    self,
    arg0: stim.GateData,
) -> bool:
    """Determines if two GateData instances are not identical.
    """
```

<a name="stim.GateData.__repr__"></a>
```python
# stim.GateData.__repr__

# (in class stim.GateData)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.GateData`.
    """
```

<a name="stim.GateData.__str__"></a>
```python
# stim.GateData.__str__

# (in class stim.GateData)
def __str__(
    self,
) -> str:
    """Returns text describing the gate data.
    """
```

<a name="stim.GateData.aliases"></a>
```python
# stim.GateData.aliases

# (in class stim.GateData)
@property
def aliases(
    self,
) -> List[str]:
    """Returns all aliases that can be used to name the gate.

    Although gates can be referred to by lower case and mixed
    case named, the result only includes upper cased aliases.

    Examples:
        >>> import stim
        >>> stim.gate_data('H').aliases
        ['H', 'H_XZ']
        >>> stim.gate_data('cnot').aliases
        ['CNOT', 'CX', 'ZCX']
    """
```

<a name="stim.GateData.is_noisy_gate"></a>
```python
# stim.GateData.is_noisy_gate

# (in class stim.GateData)
@property
def is_noisy_gate(
    self,
) -> bool:
    """Returns whether or not the gate can produce noise.

    Note that measurement operations are considered noisy,
    because for example `M(0.001) 2 3 5` will include
    noise that flips its result 0.1% of the time.

    Examples:
        >>> import stim

        >>> stim.gate_data('M').is_noisy_gate
        True
        >>> stim.gate_data('MXX').is_noisy_gate
        True
        >>> stim.gate_data('X_ERROR').is_noisy_gate
        True
        >>> stim.gate_data('CORRELATED_ERROR').is_noisy_gate
        True
        >>> stim.gate_data('MPP').is_noisy_gate
        True

        >>> stim.gate_data('H').is_noisy_gate
        False
        >>> stim.gate_data('CX').is_noisy_gate
        False
        >>> stim.gate_data('R').is_noisy_gate
        False
        >>> stim.gate_data('DETECTOR').is_noisy_gate
        False
    """
```

<a name="stim.GateData.is_reset"></a>
```python
# stim.GateData.is_reset

# (in class stim.GateData)
@property
def is_reset(
    self,
) -> bool:
    """Returns whether or not the gate resets qubits in any basis.

    Examples:
        >>> import stim

        >>> stim.gate_data('R').is_reset
        True
        >>> stim.gate_data('RX').is_reset
        True
        >>> stim.gate_data('MR').is_reset
        True

        >>> stim.gate_data('M').is_reset
        False
        >>> stim.gate_data('MXX').is_reset
        False
        >>> stim.gate_data('MPP').is_reset
        False
        >>> stim.gate_data('H').is_reset
        False
        >>> stim.gate_data('CX').is_reset
        False
        >>> stim.gate_data('HERALDED_ERASE').is_reset
        False
        >>> stim.gate_data('DEPOLARIZE2').is_reset
        False
        >>> stim.gate_data('X_ERROR').is_reset
        False
        >>> stim.gate_data('CORRELATED_ERROR').is_reset
        False
        >>> stim.gate_data('DETECTOR').is_reset
        False
    """
```

<a name="stim.GateData.is_single_qubit_gate"></a>
```python
# stim.GateData.is_single_qubit_gate

# (in class stim.GateData)
@property
def is_single_qubit_gate(
    self,
) -> bool:
    """Returns whether or not the gate is a single qubit gate.

    Single qubit gates apply separately to each of their targets.

    Variable-qubit gates like CORRELATED_ERROR and MPP are not
    considered single qubit gates.

    Examples:
        >>> import stim

        >>> stim.gate_data('H').is_single_qubit_gate
        True
        >>> stim.gate_data('R').is_single_qubit_gate
        True
        >>> stim.gate_data('M').is_single_qubit_gate
        True
        >>> stim.gate_data('X_ERROR').is_single_qubit_gate
        True

        >>> stim.gate_data('CX').is_single_qubit_gate
        False
        >>> stim.gate_data('MXX').is_single_qubit_gate
        False
        >>> stim.gate_data('CORRELATED_ERROR').is_single_qubit_gate
        False
        >>> stim.gate_data('MPP').is_single_qubit_gate
        False
        >>> stim.gate_data('DETECTOR').is_single_qubit_gate
        False
        >>> stim.gate_data('TICK').is_single_qubit_gate
        False
        >>> stim.gate_data('REPEAT').is_single_qubit_gate
        False
    """
```

<a name="stim.GateData.is_two_qubit_gate"></a>
```python
# stim.GateData.is_two_qubit_gate

# (in class stim.GateData)
@property
def is_two_qubit_gate(
    self,
) -> bool:
    """Returns whether or not the gate is a two qubit gate.

    Two qubit gates must be given an even number of targets.

    Variable-qubit gates like CORRELATED_ERROR and MPP are not
    considered two qubit gates.

    Examples:
        >>> import stim

        >>> stim.gate_data('CX').is_two_qubit_gate
        True
        >>> stim.gate_data('MXX').is_two_qubit_gate
        True

        >>> stim.gate_data('H').is_two_qubit_gate
        False
        >>> stim.gate_data('R').is_two_qubit_gate
        False
        >>> stim.gate_data('M').is_two_qubit_gate
        False
        >>> stim.gate_data('X_ERROR').is_two_qubit_gate
        False
        >>> stim.gate_data('CORRELATED_ERROR').is_two_qubit_gate
        False
        >>> stim.gate_data('MPP').is_two_qubit_gate
        False
        >>> stim.gate_data('DETECTOR').is_two_qubit_gate
        False
    """
```

<a name="stim.GateData.is_unitary"></a>
```python
# stim.GateData.is_unitary

# (in class stim.GateData)
@property
def is_unitary(
    self,
) -> bool:
    """Returns whether or not the gate is a unitary gate.

    Examples:
        >>> import stim

        >>> stim.gate_data('H').is_unitary
        True
        >>> stim.gate_data('CX').is_unitary
        True

        >>> stim.gate_data('R').is_unitary
        False
        >>> stim.gate_data('M').is_unitary
        False
        >>> stim.gate_data('MXX').is_unitary
        False
        >>> stim.gate_data('X_ERROR').is_unitary
        False
        >>> stim.gate_data('CORRELATED_ERROR').is_unitary
        False
        >>> stim.gate_data('MPP').is_unitary
        False
        >>> stim.gate_data('DETECTOR').is_unitary
        False
    """
```

<a name="stim.GateData.name"></a>
```python
# stim.GateData.name

# (in class stim.GateData)
@property
def name(
    self,
) -> str:
    """Returns the canonical name of the gate.

    Examples:
        >>> import stim
        >>> stim.gate_data('H').name
        'H'
        >>> stim.gate_data('cnot').name
        'CX'
    """
```

<a name="stim.GateData.num_parens_arguments_range"></a>
```python
# stim.GateData.num_parens_arguments_range

# (in class stim.GateData)
@property
def num_parens_arguments_range(
    self,
) -> range:
    """Returns the min/max parens arguments taken by the gate, as a python range.

    Examples:
        >>> import stim

        >>> stim.gate_data('M').num_parens_arguments_range
        range(0, 2)
        >>> list(stim.gate_data('M').num_parens_arguments_range)
        [0, 1]
        >>> list(stim.gate_data('R').num_parens_arguments_range)
        [0]
        >>> list(stim.gate_data('H').num_parens_arguments_range)
        [0]
        >>> list(stim.gate_data('X_ERROR').num_parens_arguments_range)
        [1]
        >>> list(stim.gate_data('PAULI_CHANNEL_1').num_parens_arguments_range)
        [3]
        >>> list(stim.gate_data('PAULI_CHANNEL_2').num_parens_arguments_range)
        [15]
        >>> stim.gate_data('DETECTOR').num_parens_arguments_range
        range(0, 256)
        >>> list(stim.gate_data('OBSERVABLE_INCLUDE').num_parens_arguments_range)
        [1]
    """
```

<a name="stim.GateData.produces_measurements"></a>
```python
# stim.GateData.produces_measurements

# (in class stim.GateData)
@property
def produces_measurements(
    self,
) -> bool:
    """Returns whether or not the gate produces measurement results.

    Examples:
        >>> import stim

        >>> stim.gate_data('M').produces_measurements
        True
        >>> stim.gate_data('MRY').produces_measurements
        True
        >>> stim.gate_data('MXX').produces_measurements
        True
        >>> stim.gate_data('MPP').produces_measurements
        True
        >>> stim.gate_data('HERALDED_ERASE').produces_measurements
        True

        >>> stim.gate_data('H').produces_measurements
        False
        >>> stim.gate_data('CX').produces_measurements
        False
        >>> stim.gate_data('R').produces_measurements
        False
        >>> stim.gate_data('X_ERROR').produces_measurements
        False
        >>> stim.gate_data('CORRELATED_ERROR').produces_measurements
        False
        >>> stim.gate_data('DETECTOR').produces_measurements
        False
    """
```

<a name="stim.GateData.tableau"></a>
```python
# stim.GateData.tableau

# (in class stim.GateData)
@property
def tableau(
    self,
) -> Optional[stim.Tableau]:
    """Returns the gate's tableau, or None if the gate has no tableau.

    Examples:
        >>> import stim
        >>> print(stim.gate_data('M').tableau)
        None
        >>> stim.gate_data('H').tableau
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+X"),
            ],
        )
        >>> stim.gate_data('ISWAP').tableau
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+ZY"),
                stim.PauliString("+YZ"),
            ],
            zs=[
                stim.PauliString("+_Z"),
                stim.PauliString("+Z_"),
            ],
        )
    """
```

<a name="stim.GateData.takes_measurement_record_targets"></a>
```python
# stim.GateData.takes_measurement_record_targets

# (in class stim.GateData)
@property
def takes_measurement_record_targets(
    self,
) -> bool:
    """Returns whether or not the gate can accept rec targets.

    For example, `CX` can take a measurement record target
    like `CX rec[-1] 1`.

    Examples:
        >>> import stim

        >>> stim.gate_data('CX').takes_measurement_record_targets
        True
        >>> stim.gate_data('DETECTOR').takes_measurement_record_targets
        True

        >>> stim.gate_data('H').takes_measurement_record_targets
        False
        >>> stim.gate_data('SWAP').takes_measurement_record_targets
        False
        >>> stim.gate_data('R').takes_measurement_record_targets
        False
        >>> stim.gate_data('M').takes_measurement_record_targets
        False
        >>> stim.gate_data('MRY').takes_measurement_record_targets
        False
        >>> stim.gate_data('MXX').takes_measurement_record_targets
        False
        >>> stim.gate_data('X_ERROR').takes_measurement_record_targets
        False
        >>> stim.gate_data('CORRELATED_ERROR').takes_measurement_record_targets
        False
        >>> stim.gate_data('MPP').takes_measurement_record_targets
        False
    """
```

<a name="stim.GateData.takes_pauli_targets"></a>
```python
# stim.GateData.takes_pauli_targets

# (in class stim.GateData)
@property
def takes_pauli_targets(
    self,
) -> bool:
    """Returns whether or not the gate expects pauli targets.

    For example, `CORRELATED_ERROR` takes targets like `X0` and `Y1`
    instead of `0` or `1`.

    Examples:
        >>> import stim

        >>> stim.gate_data('CORRELATED_ERROR').takes_pauli_targets
        True
        >>> stim.gate_data('MPP').takes_pauli_targets
        True

        >>> stim.gate_data('H').takes_pauli_targets
        False
        >>> stim.gate_data('CX').takes_pauli_targets
        False
        >>> stim.gate_data('R').takes_pauli_targets
        False
        >>> stim.gate_data('M').takes_pauli_targets
        False
        >>> stim.gate_data('MRY').takes_pauli_targets
        False
        >>> stim.gate_data('MXX').takes_pauli_targets
        False
        >>> stim.gate_data('X_ERROR').takes_pauli_targets
        False
        >>> stim.gate_data('DETECTOR').takes_pauli_targets
        False
    """
```

<a name="stim.GateData.unitary_matrix"></a>
```python
# stim.GateData.unitary_matrix

# (in class stim.GateData)
@property
def unitary_matrix(
    self,
) -> Optional[np.ndarray]:
    """Returns the gate's unitary matrix, or None if the gate isn't unitary.

    Examples:
        >>> import stim

        >>> print(stim.gate_data('M').unitary_matrix)
        None

        >>> stim.gate_data('X').unitary_matrix
        array([[0.+0.j, 1.+0.j],
               [1.+0.j, 0.+0.j]], dtype=complex64)

        >>> stim.gate_data('ISWAP').unitary_matrix
        array([[1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
               [0.+0.j, 0.+0.j, 0.+1.j, 0.+0.j],
               [0.+0.j, 0.+1.j, 0.+0.j, 0.+0.j],
               [0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j]], dtype=complex64)
    """
```

<a name="stim.GateTarget"></a>
```python
# stim.GateTarget

# (at top-level in the stim module)
class GateTarget:
    """Represents a gate target, like `0` or `rec[-1]`, from a circuit.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit('''
        ...     M 0 !1
        ... ''')
        >>> circuit[0].targets_copy()[0]
        stim.GateTarget(0)
        >>> circuit[0].targets_copy()[1]
        stim.GateTarget(stim.target_inv(1))
    """
```

<a name="stim.GateTarget.__eq__"></a>
```python
# stim.GateTarget.__eq__

# (in class stim.GateTarget)
def __eq__(
    self,
    arg0: stim.GateTarget,
) -> bool:
    """Determines if two `stim.GateTarget`s are identical.
    """
```

<a name="stim.GateTarget.__init__"></a>
```python
# stim.GateTarget.__init__

# (in class stim.GateTarget)
def __init__(
    self,
    value: object,
) -> None:
    """Initializes a `stim.GateTarget`.

    Args:
        value: A target like `5` or `stim.target_rec(-1)`.
    """
```

<a name="stim.GateTarget.__ne__"></a>
```python
# stim.GateTarget.__ne__

# (in class stim.GateTarget)
def __ne__(
    self,
    arg0: stim.GateTarget,
) -> bool:
    """Determines if two `stim.GateTarget`s are different.
    """
```

<a name="stim.GateTarget.__repr__"></a>
```python
# stim.GateTarget.__repr__

# (in class stim.GateTarget)
def __repr__(
    self,
) -> str:
    """Returns text that is a valid python expression evaluating to an equivalent `stim.GateTarget`.
    """
```

<a name="stim.GateTarget.is_combiner"></a>
```python
# stim.GateTarget.is_combiner

# (in class stim.GateTarget)
@property
def is_combiner(
    self,
) -> bool:
    """Returns whether or not this is a combiner target like `*`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_combiner
        False
        >>> stim.target_inv(7).is_combiner
        False
        >>> stim.target_x(8).is_combiner
        False
        >>> stim.target_y(2).is_combiner
        False
        >>> stim.target_z(3).is_combiner
        False
        >>> stim.target_sweep_bit(9).is_combiner
        False
        >>> stim.target_rec(-5).is_combiner
        False
        >>> stim.target_combiner().is_combiner
        True
    """
```

<a name="stim.GateTarget.is_inverted_result_target"></a>
```python
# stim.GateTarget.is_inverted_result_target

# (in class stim.GateTarget)
@property
def is_inverted_result_target(
    self,
) -> bool:
    """Returns whether or not this is an inverted target like `!5` or `!X4`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_inverted_result_target
        False
        >>> stim.target_inv(7).is_inverted_result_target
        True
        >>> stim.target_x(8).is_inverted_result_target
        False
        >>> stim.target_x(8, invert=True).is_inverted_result_target
        True
        >>> stim.target_y(2).is_inverted_result_target
        False
        >>> stim.target_z(3).is_inverted_result_target
        False
        >>> stim.target_sweep_bit(9).is_inverted_result_target
        False
        >>> stim.target_rec(-5).is_inverted_result_target
        False
    """
```

<a name="stim.GateTarget.is_measurement_record_target"></a>
```python
# stim.GateTarget.is_measurement_record_target

# (in class stim.GateTarget)
@property
def is_measurement_record_target(
    self,
) -> bool:
    """Returns whether or not this is a measurement record target like `rec[-5]`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_measurement_record_target
        False
        >>> stim.target_inv(7).is_measurement_record_target
        False
        >>> stim.target_x(8).is_measurement_record_target
        False
        >>> stim.target_y(2).is_measurement_record_target
        False
        >>> stim.target_z(3).is_measurement_record_target
        False
        >>> stim.target_sweep_bit(9).is_measurement_record_target
        False
        >>> stim.target_rec(-5).is_measurement_record_target
        True
    """
```

<a name="stim.GateTarget.is_qubit_target"></a>
```python
# stim.GateTarget.is_qubit_target

# (in class stim.GateTarget)
@property
def is_qubit_target(
    self,
) -> bool:
    """Returns whether or not this is a qubit target like `5` or `!6`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_qubit_target
        True
        >>> stim.target_inv(7).is_qubit_target
        True
        >>> stim.target_x(8).is_qubit_target
        False
        >>> stim.target_y(2).is_qubit_target
        False
        >>> stim.target_z(3).is_qubit_target
        False
        >>> stim.target_sweep_bit(9).is_qubit_target
        False
        >>> stim.target_rec(-5).is_qubit_target
        False
    """
```

<a name="stim.GateTarget.is_sweep_bit_target"></a>
```python
# stim.GateTarget.is_sweep_bit_target

# (in class stim.GateTarget)
@property
def is_sweep_bit_target(
    self,
) -> bool:
    """Returns whether or not this is a sweep bit target like `sweep[4]`.


    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_sweep_bit_target
        False
        >>> stim.target_inv(7).is_sweep_bit_target
        False
        >>> stim.target_x(8).is_sweep_bit_target
        False
        >>> stim.target_y(2).is_sweep_bit_target
        False
        >>> stim.target_z(3).is_sweep_bit_target
        False
        >>> stim.target_sweep_bit(9).is_sweep_bit_target
        True
        >>> stim.target_rec(-5).is_sweep_bit_target
        False
    """
```

<a name="stim.GateTarget.is_x_target"></a>
```python
# stim.GateTarget.is_x_target

# (in class stim.GateTarget)
@property
def is_x_target(
    self,
) -> bool:
    """Returns whether or not this is an X pauli target like `X2` or `!X7`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_x_target
        False
        >>> stim.target_inv(7).is_x_target
        False
        >>> stim.target_x(8).is_x_target
        True
        >>> stim.target_y(2).is_x_target
        False
        >>> stim.target_z(3).is_x_target
        False
        >>> stim.target_sweep_bit(9).is_x_target
        False
        >>> stim.target_rec(-5).is_x_target
        False
    """
```

<a name="stim.GateTarget.is_y_target"></a>
```python
# stim.GateTarget.is_y_target

# (in class stim.GateTarget)
@property
def is_y_target(
    self,
) -> bool:
    """Returns whether or not this is a Y pauli target like `Y2` or `!Y7`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_y_target
        False
        >>> stim.target_inv(7).is_y_target
        False
        >>> stim.target_x(8).is_y_target
        False
        >>> stim.target_y(2).is_y_target
        True
        >>> stim.target_z(3).is_y_target
        False
        >>> stim.target_sweep_bit(9).is_y_target
        False
        >>> stim.target_rec(-5).is_y_target
        False
    """
```

<a name="stim.GateTarget.is_z_target"></a>
```python
# stim.GateTarget.is_z_target

# (in class stim.GateTarget)
@property
def is_z_target(
    self,
) -> bool:
    """Returns whether or not this is a Z pauli target like `Z2` or `!Z7`.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).is_z_target
        False
        >>> stim.target_inv(7).is_z_target
        False
        >>> stim.target_x(8).is_z_target
        False
        >>> stim.target_y(2).is_z_target
        False
        >>> stim.target_z(3).is_z_target
        True
        >>> stim.target_sweep_bit(9).is_z_target
        False
        >>> stim.target_rec(-5).is_z_target
        False
    """
```

<a name="stim.GateTarget.pauli_type"></a>
```python
# stim.GateTarget.pauli_type

# (in class stim.GateTarget)
@property
def pauli_type(
    self,
) -> str:
    """Returns whether this is an 'X', 'Y', or 'Z' target.

    For non-pauli targets, this property evaluates to 'I'.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).pauli_type
        'I'
        >>> stim.target_inv(7).pauli_type
        'I'
        >>> stim.target_x(8).pauli_type
        'X'
        >>> stim.target_y(2).pauli_type
        'Y'
        >>> stim.target_z(3).pauli_type
        'Z'
        >>> stim.target_sweep_bit(9).pauli_type
        'I'
        >>> stim.target_rec(-5).pauli_type
        'I'
    """
```

<a name="stim.GateTarget.qubit_value"></a>
```python
# stim.GateTarget.qubit_value

# (in class stim.GateTarget)
@property
def qubit_value(
    self,
) -> Optional[int]:
    """Returns the integer value of the targeted qubit, or else None.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).qubit_value
        6
        >>> stim.target_inv(7).qubit_value
        7
        >>> stim.target_x(8).qubit_value
        8
        >>> stim.target_y(2).qubit_value
        2
        >>> stim.target_z(3).qubit_value
        3
        >>> print(stim.target_sweep_bit(9).qubit_value)
        None
        >>> print(stim.target_rec(-5).qubit_value)
        None
    """
```

<a name="stim.GateTarget.value"></a>
```python
# stim.GateTarget.value

# (in class stim.GateTarget)
@property
def value(
    self,
) -> int:
    """The numeric part of the target.

    This is non-negative integer for qubit targets, and a negative integer for
    measurement record targets.

    Examples:
        >>> import stim
        >>> stim.GateTarget(6).value
        6
        >>> stim.target_inv(7).value
        7
        >>> stim.target_x(8).value
        8
        >>> stim.target_y(2).value
        2
        >>> stim.target_z(3).value
        3
        >>> stim.target_sweep_bit(9).value
        9
        >>> stim.target_rec(-5).value
        -5
    """
```

<a name="stim.GateTargetWithCoords"></a>
```python
# stim.GateTargetWithCoords

# (at top-level in the stim module)
class GateTargetWithCoords:
    """A gate target with associated coordinate information.

    For example, if the gate target is a qubit from a circuit with
    QUBIT_COORDS instructions, the coords field will contain the
    coordinate data from the QUBIT_COORDS instruction for the qubit.

    This is helpful information to have available when debugging a
    problem in a circuit, instead of having to constantly manually
    look up the coordinates of a qubit index in order to understand
    what is happening.
    """
```

<a name="stim.GateTargetWithCoords.__init__"></a>
```python
# stim.GateTargetWithCoords.__init__

# (in class stim.GateTargetWithCoords)
def __init__(
    self,
    *,
    gate_target: object,
    coords: List[float],
) -> None:
    """Creates a stim.GateTargetWithCoords.
    """
```

<a name="stim.GateTargetWithCoords.coords"></a>
```python
# stim.GateTargetWithCoords.coords

# (in class stim.GateTargetWithCoords)
@property
def coords(
    self,
) -> List[float]:
    """Returns the associated coordinate information as a list of floats.

    If there is no coordinate information, returns an empty list.
    """
```

<a name="stim.GateTargetWithCoords.gate_target"></a>
```python
# stim.GateTargetWithCoords.gate_target

# (in class stim.GateTargetWithCoords)
@property
def gate_target(
    self,
) -> stim.GateTarget:
    """Returns the actual gate target as a `stim.GateTarget`.
    """
```

<a name="stim.PauliString"></a>
```python
# stim.PauliString

# (at top-level in the stim module)
class PauliString:
    """A signed Pauli tensor product (e.g. "+X \u2297 X \u2297 X" or "-Y \u2297 Z".

    Represents a collection of Pauli operations (I, X, Y, Z) applied pairwise to a
    collection of qubits.

    Examples:
        >>> import stim
        >>> stim.PauliString("XX") * stim.PauliString("YY")
        stim.PauliString("-ZZ")
        >>> print(stim.PauliString(5))
        +_____
    """
```

<a name="stim.PauliString.__add__"></a>
```python
# stim.PauliString.__add__

# (in class stim.PauliString)
def __add__(
    self,
    rhs: stim.PauliString,
) -> stim.PauliString:
    """Returns the tensor product of two Pauli strings.

    Concatenates the Pauli strings and multiplies their signs.

    Args:
        rhs: A second stim.PauliString.

    Examples:
        >>> import stim

        >>> stim.PauliString("X") + stim.PauliString("YZ")
        stim.PauliString("+XYZ")

        >>> stim.PauliString("iX") + stim.PauliString("-X")
        stim.PauliString("-iXX")

    Returns:
        The tensor product.
    """
```

<a name="stim.PauliString.__eq__"></a>
```python
# stim.PauliString.__eq__

# (in class stim.PauliString)
def __eq__(
    self,
    arg0: stim.PauliString,
) -> bool:
    """Determines if two Pauli strings have identical contents.
    """
```

<a name="stim.PauliString.__getitem__"></a>
```python
# stim.PauliString.__getitem__

# (in class stim.PauliString)
@overload
def __getitem__(
    self,
    index_or_slice: int,
) -> int:
    pass
@overload
def __getitem__(
    self,
    index_or_slice: slice,
) -> stim.PauliString:
    pass
def __getitem__(
    self,
    index_or_slice: object,
) -> object:
    """Returns an individual Pauli or Pauli string slice from the pauli string.

    Individual Paulis are returned as an int using the encoding 0=I, 1=X, 2=Y, 3=Z.
    Slices are returned as a stim.PauliString (always with positive sign).

    Examples:
        >>> import stim
        >>> p = stim.PauliString("_XYZ")
        >>> p[2]
        2
        >>> p[-1]
        3
        >>> p[:2]
        stim.PauliString("+_X")
        >>> p[::-1]
        stim.PauliString("+ZYX_")

    Args:
        index_or_slice: The index of the pauli to return, or the slice of paulis to
            return.

    Returns:
        0: Identity.
        1: Pauli X.
        2: Pauli Y.
        3: Pauli Z.
    """
```

<a name="stim.PauliString.__iadd__"></a>
```python
# stim.PauliString.__iadd__

# (in class stim.PauliString)
def __iadd__(
    self,
    rhs: stim.PauliString,
) -> stim.PauliString:
    """Performs an inplace tensor product.

    Concatenates the given Pauli string onto the receiving string and multiplies
    their signs.

    Args:
        rhs: A second stim.PauliString.

    Examples:
        >>> import stim

        >>> p = stim.PauliString("iX")
        >>> alias = p
        >>> p += stim.PauliString("-YY")
        >>> p
        stim.PauliString("-iXYY")
        >>> alias is p
        True

    Returns:
        The mutated pauli string.
    """
```

<a name="stim.PauliString.__imul__"></a>
```python
# stim.PauliString.__imul__

# (in class stim.PauliString)
def __imul__(
    self,
    rhs: object,
) -> stim.PauliString:
    """Inplace right-multiplies the Pauli string.

    Can multiply by another Pauli string, a complex unit, or a tensor power.

    Args:
        rhs: The right hand side of the multiplication. This can be:
            - A stim.PauliString to right-multiply term-by-term into the paulis of
                the pauli string.
            - A complex unit (1, -1, 1j, -1j) to multiply into the sign of the pauli
                string.
            - A non-negative integer indicating the tensor power to raise the pauli
                string to (how many times to repeat it).

    Examples:
        >>> import stim

        >>> p = stim.PauliString("X")
        >>> p *= 1j
        >>> p
        stim.PauliString("+iX")

        >>> p = stim.PauliString("iXY_")
        >>> p *= 3
        >>> p
        stim.PauliString("-iXY_XY_XY_")

        >>> p = stim.PauliString("X")
        >>> alias = p
        >>> p *= stim.PauliString("Y")
        >>> alias
        stim.PauliString("+iZ")

        >>> p = stim.PauliString("X")
        >>> p *= stim.PauliString("_YY")
        >>> p
        stim.PauliString("+XYY")

    Returns:
        The mutated Pauli string.
    """
```

<a name="stim.PauliString.__init__"></a>
```python
# stim.PauliString.__init__

# (in class stim.PauliString)
@staticmethod
def __init__(
    *args,
    **kwargs,
):
    """Overloaded function.

    1. __init__(self: stim.PauliString, num_qubits: int) -> None

    Creates an identity Pauli string over the given number of qubits.

    Examples:
        >>> import stim
        >>> p = stim.PauliString(5)
        >>> print(p)
        +_____

    Args:
        num_qubits: The number of qubits the Pauli string acts on.


    2. __init__(self: stim.PauliString, text: str) -> None

    Creates a stim.PauliString from a text string.

    The string can optionally start with a sign ('+', '-', 'i', '+i', or '-i').
    The rest of the string should be characters from '_IXYZ' where
    '_' and 'I' mean identity, 'X' means Pauli X, 'Y' means Pauli Y, and 'Z' means
    Pauli Z.

    Examples:
        >>> import stim
        >>> print(stim.PauliString("YZ"))
        +YZ
        >>> print(stim.PauliString("+IXYZ"))
        +_XYZ
        >>> print(stim.PauliString("-___X_"))
        -___X_
        >>> print(stim.PauliString("iX"))
        +iX

    Args:
        text: A text description of the Pauli string's contents, such as "+XXX" or
            "-_YX" or "-iZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZY".

    Returns:
        The created stim.PauliString.


    3. __init__(self: stim.PauliString, copy: stim.PauliString) -> None

    Creates a copy of a stim.PauliString.

    Examples:
        >>> import stim
        >>> a = stim.PauliString("YZ")
        >>> b = stim.PauliString(a)
        >>> b is a
        False
        >>> b == a
        True

    Args:
        copy: The pauli string to make a copy of.


    4. __init__(self: stim.PauliString, pauli_indices: List[int]) -> None

    Creates a stim.PauliString from a list of integer pauli indices.

    The indexing scheme that is used is:
        0 -> I
        1 -> X
        2 -> Y
        3 -> Z

    Examples:
        >>> import stim
        >>> stim.PauliString([0, 1, 2, 3, 0, 3])
        stim.PauliString("+_XYZ_Z")

    Args:
        pauli_indices: A sequence of integers from 0 to 3 (inclusive) indicating
            paulis.
    """
```

<a name="stim.PauliString.__itruediv__"></a>
```python
# stim.PauliString.__itruediv__

# (in class stim.PauliString)
def __itruediv__(
    self,
    rhs: complex,
) -> stim.PauliString:
    """Inplace divides the Pauli string by a complex unit.

    Args:
        rhs: The divisor. Can be 1, -1, 1j, or -1j.

    Examples:
        >>> import stim

        >>> p = stim.PauliString("X")
        >>> p /= 1j
        >>> p
        stim.PauliString("-iX")

    Returns:
        The mutated Pauli string.

    Raises:
        ValueError: The divisor isn't 1, -1, 1j, or -1j.
    """
```

<a name="stim.PauliString.__len__"></a>
```python
# stim.PauliString.__len__

# (in class stim.PauliString)
def __len__(
    self,
) -> int:
    """Returns the length the pauli string; the number of qubits it operates on.
    """
```

<a name="stim.PauliString.__mul__"></a>
```python
# stim.PauliString.__mul__

# (in class stim.PauliString)
def __mul__(
    self,
    rhs: object,
) -> stim.PauliString:
    """Right-multiplies the Pauli string.

    Can multiply by another Pauli string, a complex unit, or a tensor power.

    Args:
        rhs: The right hand side of the multiplication. This can be:
            - A stim.PauliString to right-multiply term-by-term with the paulis of
                the pauli string.
            - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli
                string.
            - A non-negative integer indicating the tensor power to raise the pauli
                string to (how many times to repeat it).

    Examples:
        >>> import stim

        >>> stim.PauliString("X") * 1
        stim.PauliString("+X")
        >>> stim.PauliString("X") * -1
        stim.PauliString("-X")
        >>> stim.PauliString("X") * 1j
        stim.PauliString("+iX")

        >>> stim.PauliString("X") * 2
        stim.PauliString("+XX")
        >>> stim.PauliString("-X") * 2
        stim.PauliString("+XX")
        >>> stim.PauliString("iX") * 2
        stim.PauliString("-XX")
        >>> stim.PauliString("X") * 3
        stim.PauliString("+XXX")
        >>> stim.PauliString("iX") * 3
        stim.PauliString("-iXXX")

        >>> stim.PauliString("X") * stim.PauliString("Y")
        stim.PauliString("+iZ")
        >>> stim.PauliString("X") * stim.PauliString("XX_")
        stim.PauliString("+_X_")
        >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
        stim.PauliString("+X_ZY")

    Returns:
        The product or tensor power.

    Raises:
        TypeError: The right hand side isn't a stim.PauliString, a non-negative
            integer, or a complex unit (1, -1, 1j, or -1j).
    """
```

<a name="stim.PauliString.__ne__"></a>
```python
# stim.PauliString.__ne__

# (in class stim.PauliString)
def __ne__(
    self,
    arg0: stim.PauliString,
) -> bool:
    """Determines if two Pauli strings have non-identical contents.
    """
```

<a name="stim.PauliString.__neg__"></a>
```python
# stim.PauliString.__neg__

# (in class stim.PauliString)
def __neg__(
    self,
) -> stim.PauliString:
    """Returns the negation of the pauli string.

    Examples:
        >>> import stim
        >>> -stim.PauliString("X")
        stim.PauliString("-X")
        >>> -stim.PauliString("-Y")
        stim.PauliString("+Y")
        >>> -stim.PauliString("iZZZ")
        stim.PauliString("-iZZZ")
    """
```

<a name="stim.PauliString.__pos__"></a>
```python
# stim.PauliString.__pos__

# (in class stim.PauliString)
def __pos__(
    self,
) -> stim.PauliString:
    """Returns a pauli string with the same contents.

    Examples:
        >>> import stim
        >>> +stim.PauliString("+X")
        stim.PauliString("+X")
        >>> +stim.PauliString("-YY")
        stim.PauliString("-YY")
        >>> +stim.PauliString("iZZZ")
        stim.PauliString("+iZZZ")
    """
```

<a name="stim.PauliString.__repr__"></a>
```python
# stim.PauliString.__repr__

# (in class stim.PauliString)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equivalent `stim.PauliString`.
    """
```

<a name="stim.PauliString.__rmul__"></a>
```python
# stim.PauliString.__rmul__

# (in class stim.PauliString)
def __rmul__(
    self,
    lhs: object,
) -> stim.PauliString:
    """Left-multiplies the Pauli string.

    Can multiply by another Pauli string, a complex unit, or a tensor power.

    Args:
        lhs: The left hand side of the multiplication. This can be:
            - A stim.PauliString to right-multiply term-by-term with the paulis of
                the pauli string.
            - A complex unit (1, -1, 1j, -1j) to multiply with the sign of the pauli
                string.
            - A non-negative integer indicating the tensor power to raise the pauli
                string to (how many times to repeat it).

    Examples:
        >>> import stim

        >>> 1 * stim.PauliString("X")
        stim.PauliString("+X")
        >>> -1 * stim.PauliString("X")
        stim.PauliString("-X")
        >>> 1j * stim.PauliString("X")
        stim.PauliString("+iX")

        >>> 2 * stim.PauliString("X")
        stim.PauliString("+XX")
        >>> 2 * stim.PauliString("-X")
        stim.PauliString("+XX")
        >>> 2 * stim.PauliString("iX")
        stim.PauliString("-XX")
        >>> 3 * stim.PauliString("X")
        stim.PauliString("+XXX")
        >>> 3 * stim.PauliString("iX")
        stim.PauliString("-iXXX")

        >>> stim.PauliString("X") * stim.PauliString("Y")
        stim.PauliString("+iZ")
        >>> stim.PauliString("X") * stim.PauliString("XX_")
        stim.PauliString("+_X_")
        >>> stim.PauliString("XXXX") * stim.PauliString("_XYZ")
        stim.PauliString("+X_ZY")

    Returns:
        The product.

    Raises:
        ValueError: The scalar phase factor isn't 1, -1, 1j, or -1j.
    """
```

<a name="stim.PauliString.__setitem__"></a>
```python
# stim.PauliString.__setitem__

# (in class stim.PauliString)
def __setitem__(
    self,
    index: int,
    new_pauli: object,
) -> None:
    """Mutates an entry in the pauli string using the encoding 0=I, 1=X, 2=Y, 3=Z.

    Args:
        index: The index of the pauli to overwrite.
        new_pauli: Either a character from '_IXYZ' or an integer from range(4).

    Examples:
        >>> import stim
        >>> p = stim.PauliString(4)
        >>> p[2] = 1
        >>> print(p)
        +__X_
        >>> p[0] = 3
        >>> p[1] = 2
        >>> p[3] = 0
        >>> print(p)
        +ZYX_
        >>> p[0] = 'I'
        >>> p[1] = 'X'
        >>> p[2] = 'Y'
        >>> p[3] = 'Z'
        >>> print(p)
        +_XYZ
        >>> p[-1] = 'Y'
        >>> print(p)
        +_XYY
    """
```

<a name="stim.PauliString.__str__"></a>
```python
# stim.PauliString.__str__

# (in class stim.PauliString)
def __str__(
    self,
) -> str:
    """Returns a text description.
    """
```

<a name="stim.PauliString.__truediv__"></a>
```python
# stim.PauliString.__truediv__

# (in class stim.PauliString)
def __truediv__(
    self,
    rhs: complex,
) -> stim.PauliString:
    """Divides the Pauli string by a complex unit.

    Args:
        rhs: The divisor. Can be 1, -1, 1j, or -1j.

    Examples:
        >>> import stim

        >>> stim.PauliString("X") / 1j
        stim.PauliString("-iX")

    Returns:
        The quotient.

    Raises:
        ValueError: The divisor isn't 1, -1, 1j, or -1j.
    """
```

<a name="stim.PauliString.after"></a>
```python
# stim.PauliString.after

# (in class stim.PauliString)
@overload
def after(
    self,
    operation: Union[stim.Circuit, stim.CircuitInstruction],
) -> stim.PauliString:
    pass
@overload
def after(
    self,
    operation: stim.Tableau,
    targets: Iterable[int],
) -> stim.PauliString:
    pass
def after(
    self,
    operation: Union[stim.Circuit, stim.Tableau, stim.CircuitInstruction],
    targets: Optional[Iterable[int]] = None,
) -> stim.PauliString:
    """Returns the result of conjugating the Pauli string by an operation.

    Args:
        operation: A circuit, tableau, or circuit instruction to
            conjugate the Pauli string by. Must be Clifford (e.g.
            if it's a circuit, the circuit can't have noise or
            measurements).
        targets: Required if and only if the operation is a tableau.
            Specifies which qubits to target.

    Examples:
        >>> import stim
        >>> p = stim.PauliString("_XYZ")

        >>> p.after(stim.CircuitInstruction("H", [1]))
        stim.PauliString("+_ZYZ")

        >>> p.after(stim.Circuit('''
        ...     C_XYZ 1 2 3
        ... '''))
        stim.PauliString("+_YZX")

        >>> p.after(stim.Tableau.from_named_gate('CZ'), targets=[0, 1])
        stim.PauliString("+ZXYZ")

    Returns:
        The conjugated Pauli string. The Pauli string after the
        operation that is exactly equivalent to the given Pauli
        string before the operation.
    """
```

<a name="stim.PauliString.before"></a>
```python
# stim.PauliString.before

# (in class stim.PauliString)
@overload
def after(
    self,
    operation: Union[stim.Circuit, stim.CircuitInstruction],
) -> stim.PauliString:
    pass
@overload
def after(
    self,
    operation: stim.Tableau,
    targets: Iterable[int],
) -> stim.PauliString:
    pass
def after(
    self,
    operation: Union[stim.Circuit, stim.Tableau, stim.CircuitInstruction],
    targets: Optional[Iterable[int]] = None,
) -> stim.PauliString:
    """Returns the result of conjugating the Pauli string by an operation.

    Args:
        operation: A circuit, tableau, or circuit instruction to
            anti-conjugate the Pauli string by. Must be Clifford (e.g.
            if it's a circuit, the circuit can't have noise or
            measurements).
        targets: Required if and only if the operation is a tableau.
            Specifies which qubits to target.

    Examples:
        >>> import stim
        >>> p = stim.PauliString("_XYZ")

        >>> p.before(stim.CircuitInstruction("H", [1]))
        stim.PauliString("+_ZYZ")

        >>> p.before(stim.Circuit('''
        ...     C_XYZ 1 2 3
        ... '''))
        stim.PauliString("+_ZXY")

        >>> p.before(stim.Tableau.from_named_gate('CZ'), targets=[0, 1])
        stim.PauliString("+ZXYZ")

    Returns:
        The conjugated Pauli string. The Pauli string before the
        operation that is exactly equivalent to the given Pauli
        string after the operation.
    """
```

<a name="stim.PauliString.commutes"></a>
```python
# stim.PauliString.commutes

# (in class stim.PauliString)
def commutes(
    self,
    other: stim.PauliString,
) -> bool:
    """Determines if two Pauli strings commute or not.

    Two Pauli strings commute if they have an even number of matched
    non-equal non-identity Pauli terms. Otherwise they anticommute.

    Args:
        other: The other Pauli string.

    Examples:
        >>> import stim
        >>> xx = stim.PauliString("XX")
        >>> xx.commutes(stim.PauliString("X_"))
        True
        >>> xx.commutes(stim.PauliString("XX"))
        True
        >>> xx.commutes(stim.PauliString("XY"))
        False
        >>> xx.commutes(stim.PauliString("XZ"))
        False
        >>> xx.commutes(stim.PauliString("ZZ"))
        True
        >>> xx.commutes(stim.PauliString("X_Y__"))
        True
        >>> xx.commutes(stim.PauliString(""))
        True

    Returns:
        True if the Pauli strings commute, False if they anti-commute.
    """
```

<a name="stim.PauliString.copy"></a>
```python
# stim.PauliString.copy

# (in class stim.PauliString)
def copy(
    self,
) -> stim.PauliString:
    """Returns a copy of the pauli string.

    The copy is an independent pauli string with the same contents.

    Examples:
        >>> import stim
        >>> p1 = stim.PauliString.random(2)
        >>> p2 = p1.copy()
        >>> p2 is p1
        False
        >>> p2 == p1
        True
    """
```

<a name="stim.PauliString.from_numpy"></a>
```python
# stim.PauliString.from_numpy

# (in class stim.PauliString)
@staticmethod
def from_numpy(
    *,
    xs: np.ndarray,
    zs: np.ndarray,
    sign: Union[int, float, complex] = +1,
    num_qubits: Optional[int] = None,
) -> stim.PauliString:
    """Creates a pauli string from X bit and Z bit numpy arrays, using the encoding:

        x=0 and z=0 -> P=I
        x=1 and z=0 -> P=X
        x=1 and z=1 -> P=Y
        x=0 and z=1 -> P=Z

    Args:
        xs: The X bits of the pauli string. This array can either be a 1-dimensional
            numpy array with dtype=np.bool_, or a bit packed 1-dimensional numpy
            array with dtype=np.uint8. If the dtype is np.uint8 then the array is
            assumed to be bit packed in little endian order and the "num_qubits"
            argument must be specified. When bit packed, the x bit with offset k is
            stored at (xs[k // 8] >> (k % 8)) & 1.
        zs: The Z bits of the pauli string. This array can either be a 1-dimensional
            numpy array with dtype=np.bool_, or a bit packed 1-dimensional numpy
            array with dtype=np.uint8. If the dtype is np.uint8 then the array is
            assumed to be bit packed in little endian order and the "num_qubits"
            argument must be specified. When bit packed, the x bit with offset k is
            stored at (xs[k // 8] >> (k % 8)) & 1.
        sign: Defaults to +1. Set to +1, -1, 1j, or -1j to control the sign of the
            returned Pauli string.
        num_qubits: Must be specified if xs or zs is a bit packed array. Specifies
            the expected length of the Pauli string.

    Returns:
        The created pauli string.

    Examples:
        >>> import stim
        >>> import numpy as np

        >>> xs = np.array([1, 1, 1, 1, 1, 1, 1, 0, 0], dtype=np.bool_)
        >>> zs = np.array([0, 0, 0, 0, 1, 1, 1, 1, 1], dtype=np.bool_)
        >>> stim.PauliString.from_numpy(xs=xs, zs=zs, sign=-1)
        stim.PauliString("-XXXXYYYZZ")

        >>> xs = np.array([127, 0], dtype=np.uint8)
        >>> zs = np.array([240, 1], dtype=np.uint8)
        >>> stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=9)
        stim.PauliString("+XXXXYYYZZ")
    """
```

<a name="stim.PauliString.from_unitary_matrix"></a>
```python
# stim.PauliString.from_unitary_matrix

# (in class stim.PauliString)
@staticmethod
def from_unitary_matrix(
    matrix: Iterable[Iterable[float]],
    *,
    endian: str = 'little',
    unsigned: bool = False,
) -> stim.PauliString:
    """Creates a stim.PauliString from the unitary matrix of a Pauli group member.

    Args:
        matrix: A unitary matrix specified as an iterable of rows, with each row is
            an iterable of amplitudes. The unitary matrix must correspond to a
            Pauli string, including global phase.
        endian:
            "little": matrix entries are in little endian order, where higher index
                qubits correspond to larger changes in row/col indices.
            "big": matrix entries are in big endian order, where higher index
                qubits correspond to smaller changes in row/col indices.
        unsigned: When False, the input must only contain the values
            [0, 1, -1, 1j, -1j] and the output will have the correct global phase.
            When True, the input is permitted to be scaled by an arbitrary unit
            complex value and the output will always have positive sign.
            False is stricter but provides more information, while True is more
            flexible but provides less information.

    Returns:
        The pauli string equal to the given unitary matrix.

    Raises:
        ValueError: The given matrix isn't the unitary matrix of a Pauli string.

    Examples:
        >>> import stim
        >>> stim.PauliString.from_unitary_matrix([
        ...     [1j, 0],
        ...     [0, -1j],
        ... ], endian='little')
        stim.PauliString("+iZ")

        >>> stim.PauliString.from_unitary_matrix([
        ...     [1j**0.1, 0],
        ...     [0, -(1j**0.1)],
        ... ], endian='little', unsigned=True)
        stim.PauliString("+Z")

        >>> stim.PauliString.from_unitary_matrix([
        ...     [0, 1, 0, 0],
        ...     [1, 0, 0, 0],
        ...     [0, 0, 0, -1],
        ...     [0, 0, -1, 0],
        ... ], endian='little')
        stim.PauliString("+XZ")
    """
```

<a name="stim.PauliString.iter_all"></a>
```python
# stim.PauliString.iter_all

# (in class stim.PauliString)
@staticmethod
def iter_all(
    num_qubits: int,
    *,
    min_weight: Optional[int] = None,
    max_weight: Optional[int] = None,
) -> stim.PauliStringIterator:
    """Returns an iterator that iterates over all PauliStrings.

    The length of PauliString and its weight (the number of
    non-identity terms) are controlled by num_qubits and
    min_/max_weight.

    Args:
        num_qubits: The number of qubits the Pauli string acts on.
        min_weight: The minimum weight PauliString to consider. If None
            min_weight is set to zero and the identity string is included.
        max_weight: The maximum weight PauliString to consider. If None
            then max_weight is set to num_qubits.

    Returns:
        An Iterable[stim.PauliString] that yields the requested PauliStrings.

    Examples:
        >>> import stim
        >>> pauli_iter = stim.PauliString.iter_all(10, min_weight=2, max_weight=3)
        >>> n = 0
        >>> for pauli_string in pauli_iter:
        ...     n += 1
        >>> n
        3645
    """
```

<a name="stim.PauliString.random"></a>
```python
# stim.PauliString.random

# (in class stim.PauliString)
@staticmethod
def random(
    num_qubits: int,
    *,
    allow_imaginary: bool = False,
) -> stim.PauliString:
    """Samples a uniformly random Hermitian Pauli string.

    Args:
        num_qubits: The number of qubits the Pauli string should act on.
        allow_imaginary: Defaults to False. If True, the sign of the result may be
            1j or -1j in addition to +1 or -1. In other words, setting this to True
            allows the result to be non-Hermitian.

    Examples:
        >>> import stim
        >>> p = stim.PauliString.random(5)
        >>> len(p)
        5
        >>> p.sign in [-1, +1]
        True

        >>> p2 = stim.PauliString.random(3, allow_imaginary=True)
        >>> len(p2)
        3
        >>> p2.sign in [-1, +1, 1j, -1j]
        True

    Returns:
        The sampled Pauli string.
    """
```

<a name="stim.PauliString.sign"></a>
```python
# stim.PauliString.sign

# (in class stim.PauliString)
@property
def sign(
    self,
) -> complex:
    """The sign of the Pauli string. Can be +1, -1, 1j, or -1j.

    Examples:
        >>> import stim
        >>> stim.PauliString("X").sign
        (1+0j)
        >>> stim.PauliString("-X").sign
        (-1+0j)
        >>> stim.PauliString("iX").sign
        1j
        >>> stim.PauliString("-iX").sign
        (-0-1j)
    """
@sign.setter
def sign(self, value: complex):
    pass
```

<a name="stim.PauliString.to_numpy"></a>
```python
# stim.PauliString.to_numpy

# (in class stim.PauliString)
def to_numpy(
    self,
    *,
    bit_packed: bool = False,
) -> Tuple[np.ndarray, np.ndarray]:
    """Decomposes the contents of the pauli string into X bit and Z bit numpy arrays.

    Args:
        bit_packed: Defaults to False. Determines whether the output numpy arrays
            use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

    Returns:
        An (xs, zs) tuple encoding the paulis from the string. The k'th Pauli from
        the string is encoded into k'th bit of xs and the k'th bit of zs using
        the "xz" encoding:

            P=I -> x=0 and z=0
            P=X -> x=1 and z=0
            P=Y -> x=1 and z=1
            P=Z -> x=0 and z=1

        The dtype and shape of the result depends on the bit_packed argument.

        If bit_packed=False:
            Each bit gets its own byte.
            xs.dtype = zs.dtype = np.bool_
            xs.shape = zs.shape = len(self)
            xs_k = xs[k]
            zs_k = zs[k]

        If bit_packed=True:
            Equivalent to applying np.packbits(bitorder='little') to the result.
            xs.dtype = zs.dtype = np.uint8
            xs.shape = zs.shape = math.ceil(len(self) / 8)
            xs_k = (xs[k // 8] >> (k % 8)) & 1
            zs_k = (zs[k // 8] >> (k % 8)) & 1

    Examples:
        >>> import stim

        >>> xs, zs = stim.PauliString("XXXXYYYZZ").to_numpy()
        >>> xs
        array([ True,  True,  True,  True,  True,  True,  True, False, False])
        >>> zs
        array([False, False, False, False,  True,  True,  True,  True,  True])

        >>> xs, zs = stim.PauliString("XXXXYYYZZ").to_numpy(bit_packed=True)
        >>> xs
        array([127,   0], dtype=uint8)
        >>> zs
        array([240,   1], dtype=uint8)
    """
```

<a name="stim.PauliString.to_tableau"></a>
```python
# stim.PauliString.to_tableau

# (in class stim.PauliString)
def to_tableau(
    self,
) -> stim.Tableau:
    """Creates a Tableau equivalent to this Pauli string.

    The tableau represents a Clifford operation that multiplies qubits
    by the corresponding Pauli operations from this Pauli string.
    The global phase of the pauli operation is lost in the conversion.

    Returns:
        The created tableau.

    Examples:
        >>> import stim
        >>> p = stim.PauliString("ZZ")
        >>> p.to_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("-X_"),
                stim.PauliString("-_X"),
            ],
            zs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_Z"),
            ],
        )
        >>> q = stim.PauliString("YX_Z")
        >>> q.to_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("-X___"),
                stim.PauliString("+_X__"),
                stim.PauliString("+__X_"),
                stim.PauliString("-___X"),
            ],
            zs=[
                stim.PauliString("-Z___"),
                stim.PauliString("-_Z__"),
                stim.PauliString("+__Z_"),
                stim.PauliString("+___Z"),
            ],
        )
    """
```

<a name="stim.PauliString.to_unitary_matrix"></a>
```python
# stim.PauliString.to_unitary_matrix

# (in class stim.PauliString)
def to_unitary_matrix(
    self,
    *,
    endian: str,
) -> np.ndarray[np.complex64]:
    """Converts the pauli string into a unitary matrix.

    Args:
        endian:
            "little": The first qubit is the least significant (corresponds
                to an offset of 1 in the matrix).
            "big": The first qubit is the most significant (corresponds
                to an offset of 2**(n - 1) in the matrix).

    Returns:
        A numpy array with dtype=np.complex64 and
        shape=(1 << len(pauli_string), 1 << len(pauli_string)).

    Example:
        >>> import stim
        >>> stim.PauliString("-YZ").to_unitary_matrix(endian="little")
        array([[0.+0.j, 0.+1.j, 0.+0.j, 0.+0.j],
               [0.-1.j, 0.+0.j, 0.+0.j, 0.+0.j],
               [0.+0.j, 0.+0.j, 0.+0.j, 0.-1.j],
               [0.+0.j, 0.+0.j, 0.+1.j, 0.+0.j]], dtype=complex64)
    """
```

<a name="stim.PauliString.weight"></a>
```python
# stim.PauliString.weight

# (in class stim.PauliString)
@property
def weight(
    self,
) -> int:
    """Returns the number of non-identity pauli terms in the pauli string.

    Examples:
        >>> import stim
        >>> stim.PauliString("+___").weight
        0
        >>> stim.PauliString("+__X").weight
        1
        >>> stim.PauliString("+XYZ").weight
        3
        >>> stim.PauliString("-XXX___XXYZ").weight
        7
    """
```

<a name="stim.PauliStringIterator"></a>
```python
# stim.PauliStringIterator

# (at top-level in the stim module)
class PauliStringIterator:
    """Iterates over all possible pauli_strings of weight specfied by
    min_weight and max_weight.

    Returns:
        An Iterable[stim.PauliString] that yields the requested Pauli
            string.

    Examples:
        >>> import stim
        >>> pauli_iter = stim.PauliString.iter_all(10, min_weight=2, max_weight=3)
        >>> n = 0
        >>> for pauli_string in pauli_iter:
        ...     n += 1
        >>> n
        3645
    """
```

<a name="stim.PauliStringIterator.__iter__"></a>
```python
# stim.PauliStringIterator.__iter__

# (in class stim.PauliStringIterator)
def __iter__(
    self,
) -> stim.PauliStringIterator:
    """Returns an independent copy of the pauli string iterator.
    """
```

<a name="stim.PauliStringIterator.__next__"></a>
```python
# stim.PauliStringIterator.__next__

# (in class stim.PauliStringIterator)
def __next__(
    self,
) -> stim.PauliString:
    """Returns the next iterated pauli string.
    """
```

<a name="stim.Tableau"></a>
```python
# stim.Tableau

# (at top-level in the stim module)
class Tableau:
    """A stabilizer tableau.

    Represents a Clifford operation by explicitly storing how that operation
    conjugates a list of Pauli group generators into composite Pauli products.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_named_gate("H")
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+X"),
            ],
        )

        >>> t = stim.Tableau.random(5)
        >>> t_inv = t**-1
        >>> print(t * t_inv)
        +-xz-xz-xz-xz-xz-
        | ++ ++ ++ ++ ++
        | XZ __ __ __ __
        | __ XZ __ __ __
        | __ __ XZ __ __
        | __ __ __ XZ __
        | __ __ __ __ XZ

        >>> x2z3 = t.x_output(2) * t.z_output(3)
        >>> t_inv(x2z3)
        stim.PauliString("+__XZ_")
    """
```

<a name="stim.Tableau.__add__"></a>
```python
# stim.Tableau.__add__

# (in class stim.Tableau)
def __add__(
    self,
    rhs: stim.Tableau,
) -> stim.Tableau:
    """Returns the direct sum (diagonal concatenation) of two Tableaus.

    Args:
        rhs: A second stim.Tableau.

    Examples:
        >>> import stim

        >>> s = stim.Tableau.from_named_gate("S")
        >>> cz = stim.Tableau.from_named_gate("CZ")
        >>> print(s + cz)
        +-xz-xz-xz-
        | ++ ++ ++
        | YZ __ __
        | __ XZ Z_
        | __ Z_ XZ

    Returns:
        The direct sum.
    """
```

<a name="stim.Tableau.__call__"></a>
```python
# stim.Tableau.__call__

# (in class stim.Tableau)
def __call__(
    self,
    pauli_string: stim.PauliString,
) -> stim.PauliString:
    """Returns the conjugation of a PauliString by the Tableau's Clifford operation.

    The conjugation of P by C is equal to C**-1 * P * C. If P is a Pauli product
    before C, then P2 = C**-1 * P * C is an equivalent Pauli product after C.

    Args:
        pauli_string: The pauli string to conjugate.

    Returns:
        The new conjugated pauli string.

    Examples:
        >>> import stim
        >>> t = stim.Tableau.from_named_gate("CNOT")
        >>> p = stim.PauliString("XX")
        >>> result = t(p)
        >>> print(result)
        +X_
    """
```

<a name="stim.Tableau.__eq__"></a>
```python
# stim.Tableau.__eq__

# (in class stim.Tableau)
def __eq__(
    self,
    arg0: stim.Tableau,
) -> bool:
    """Determines if two tableaus have identical contents.
    """
```

<a name="stim.Tableau.__iadd__"></a>
```python
# stim.Tableau.__iadd__

# (in class stim.Tableau)
def __iadd__(
    self,
    rhs: stim.Tableau,
) -> stim.Tableau:
    """Performs an inplace direct sum (diagonal concatenation).

    Args:
        rhs: A second stim.Tableau.

    Examples:
        >>> import stim

        >>> s = stim.Tableau.from_named_gate("S")
        >>> cz = stim.Tableau.from_named_gate("CZ")
        >>> alias = s
        >>> s += cz
        >>> alias is s
        True
        >>> print(s)
        +-xz-xz-xz-
        | ++ ++ ++
        | YZ __ __
        | __ XZ Z_
        | __ Z_ XZ

    Returns:
        The mutated tableau.
    """
```

<a name="stim.Tableau.__init__"></a>
```python
# stim.Tableau.__init__

# (in class stim.Tableau)
def __init__(
    self,
    num_qubits: int,
) -> None:
    """Creates an identity tableau over the given number of qubits.

    Examples:
        >>> import stim
        >>> t = stim.Tableau(3)
        >>> print(t)
        +-xz-xz-xz-
        | ++ ++ ++
        | XZ __ __
        | __ XZ __
        | __ __ XZ

    Args:
        num_qubits: The number of qubits the tableau's operation acts on.
    """
```

<a name="stim.Tableau.__len__"></a>
```python
# stim.Tableau.__len__

# (in class stim.Tableau)
def __len__(
    self,
) -> int:
    """Returns the number of qubits operated on by the tableau.
    """
```

<a name="stim.Tableau.__mul__"></a>
```python
# stim.Tableau.__mul__

# (in class stim.Tableau)
def __mul__(
    self,
    rhs: stim.Tableau,
) -> stim.Tableau:
    """Returns the product of two tableaus.

    If the tableau T1 represents the Clifford operation with unitary C1,
    and the tableau T2 represents the Clifford operation with unitary C2,
    then the tableau T1*T2 represents the Clifford operation with unitary C1*C2.

    Args:
        rhs: The tableau  on the right hand side of the multiplication.

    Examples:
        >>> import stim
        >>> t1 = stim.Tableau.random(4)
        >>> t2 = stim.Tableau.random(4)
        >>> t3 = t2 * t1
        >>> p = stim.PauliString.random(4)
        >>> t3(p) == t2(t1(p))
        True
    """
```

<a name="stim.Tableau.__ne__"></a>
```python
# stim.Tableau.__ne__

# (in class stim.Tableau)
def __ne__(
    self,
    arg0: stim.Tableau,
) -> bool:
    """Determines if two tableaus have non-identical contents.
    """
```

<a name="stim.Tableau.__pow__"></a>
```python
# stim.Tableau.__pow__

# (in class stim.Tableau)
def __pow__(
    self,
    exponent: int,
) -> stim.Tableau:
    """Raises the tableau to an integer power.

    Large powers are reached efficiently using repeated squaring.
    Negative powers are reached by inverting the tableau.

    Args:
        exponent: The power to raise to. Can be negative, zero, or positive.

    Examples:
        >>> import stim
        >>> s = stim.Tableau.from_named_gate("S")
        >>> s**0 == stim.Tableau(1)
        True
        >>> s**1 == s
        True
        >>> s**2 == stim.Tableau.from_named_gate("Z")
        True
        >>> s**-1 == s**3 == stim.Tableau.from_named_gate("S_DAG")
        True
        >>> s**5 == s
        True
        >>> s**(400000000 + 1) == s
        True
        >>> s**(-400000000 + 1) == s
        True
    """
```

<a name="stim.Tableau.__repr__"></a>
```python
# stim.Tableau.__repr__

# (in class stim.Tableau)
def __repr__(
    self,
) -> str:
    """Returns valid python code evaluating to an equal `stim.Tableau`.
    """
```

<a name="stim.Tableau.__str__"></a>
```python
# stim.Tableau.__str__

# (in class stim.Tableau)
def __str__(
    self,
) -> str:
    """Returns a text description.
    """
```

<a name="stim.Tableau.append"></a>
```python
# stim.Tableau.append

# (in class stim.Tableau)
def append(
    self,
    gate: stim.Tableau,
    targets: Sequence[int],
) -> None:
    """Appends an operation's effect into this tableau, mutating this tableau.

    Time cost is O(n*m*m) where n=len(self) and m=len(gate).

    Args:
        gate: The tableau of the operation being appended into this tableau.
        targets: The qubits being targeted by the gate.

    Examples:
        >>> import stim
        >>> cnot = stim.Tableau.from_named_gate("CNOT")
        >>> t = stim.Tableau(2)
        >>> t.append(cnot, [0, 1])
        >>> t.append(cnot, [1, 0])
        >>> t.append(cnot, [0, 1])
        >>> t == stim.Tableau.from_named_gate("SWAP")
        True
    """
```

<a name="stim.Tableau.copy"></a>
```python
# stim.Tableau.copy

# (in class stim.Tableau)
def copy(
    self,
) -> stim.Tableau:
    """Returns a copy of the tableau. An independent tableau with the same contents.

    Examples:
        >>> import stim
        >>> t1 = stim.Tableau.random(2)
        >>> t2 = t1.copy()
        >>> t2 is t1
        False
        >>> t2 == t1
        True
    """
```

<a name="stim.Tableau.from_circuit"></a>
```python
# stim.Tableau.from_circuit

# (in class stim.Tableau)
@staticmethod
def from_circuit(
    circuit: stim.Circuit,
    *,
    ignore_noise: bool = False,
    ignore_measurement: bool = False,
    ignore_reset: bool = False,
) -> stim.Tableau:
    """Converts a circuit into an equivalent stabilizer tableau.

    Args:
        circuit: The circuit to compile into a tableau.
        ignore_noise: Defaults to False. When False, any noise operations in the
            circuit will cause the conversion to fail with an exception. When True,
            noise operations are skipped over as if they weren't even present in the
            circuit.
        ignore_measurement: Defaults to False. When False, any measurement
            operations in the circuit will cause the conversion to fail with an
            exception. When True, measurement operations are skipped over as if they
            weren't even present in the circuit.
        ignore_reset: Defaults to False. When False, any reset operations in the
            circuit will cause the conversion to fail with an exception. When True,
            reset operations are skipped over as if they weren't even present in the
            circuit.

    Returns:
        The tableau equivalent to the given circuit (up to global phase).

    Raises:
        ValueError:
            The circuit contains noise operations but ignore_noise=False.
            OR
            The circuit contains measurement operations but
            ignore_measurement=False.
            OR
            The circuit contains reset operations but ignore_reset=False.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_circuit(stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1
        ... '''))
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+XX"),
                stim.PauliString("+ZZ"),
            ],
        )
    """
```

<a name="stim.Tableau.from_conjugated_generators"></a>
```python
# stim.Tableau.from_conjugated_generators

# (in class stim.Tableau)
@staticmethod
def from_conjugated_generators(
    *,
    xs: List[stim.PauliString],
    zs: List[stim.PauliString],
) -> stim.Tableau:
    """Creates a tableau from the given outputs for each generator.

    Verifies that the tableau is well formed.

    Args:
        xs: A List[stim.PauliString] with the results of conjugating X0, X1, etc.
        zs: A List[stim.PauliString] with the results of conjugating Z0, Z1, etc.

    Returns:
        The created tableau.

    Raises:
        ValueError: The given outputs are malformed. Their lengths are inconsistent,
            or they don't satisfy the required commutation relationships.

    Examples:
        >>> import stim
        >>> identity3 = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("X__"),
        ...         stim.PauliString("_X_"),
        ...         stim.PauliString("__X"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("Z__"),
        ...         stim.PauliString("_Z_"),
        ...         stim.PauliString("__Z"),
        ...     ],
        ... )
        >>> identity3 == stim.Tableau(3)
        True
    """
```

<a name="stim.Tableau.from_named_gate"></a>
```python
# stim.Tableau.from_named_gate

# (in class stim.Tableau)
@staticmethod
def from_named_gate(
    name: str,
) -> stim.Tableau:
    """Returns the tableau of a named Clifford gate.

    Args:
        name: The name of the Clifford gate.

    Returns:
        The gate's tableau.

    Examples:
        >>> import stim
        >>> print(stim.Tableau.from_named_gate("H"))
        +-xz-
        | ++
        | ZX
        >>> print(stim.Tableau.from_named_gate("CNOT"))
        +-xz-xz-
        | ++ ++
        | XZ _Z
        | X_ XZ
        >>> print(stim.Tableau.from_named_gate("S"))
        +-xz-
        | ++
        | YZ
    """
```

<a name="stim.Tableau.from_numpy"></a>
```python
# stim.Tableau.from_numpy

# (in class stim.Tableau)
def from_numpy(
    self,
    *,
    bit_packed: bool = False,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Creates a tableau from numpy arrays x2x, x2z, z2x, z2z, x_signs, and z_signs.

    The x2x, x2z, z2x, z2z arrays are the four quadrants of the table defined in
    Aaronson and Gottesman's "Improved Simulation of Stabilizer Circuits"
    ( https://arxiv.org/abs/quant-ph/0406196 ).

    Args:
        x2x: A 2d numpy array containing the x-to-x coupling bits. The bits can be
            bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
            result will satisfy result.x_output_pauli(i, j) in [1, 2] == x2x[i, j].
            Bit packing must be in little endian order and only applies to the
            second axis.
        x2z: A 2d numpy array containing the x-to-z coupling bits. The bits can be
            bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
            result will satisfy result.x_output_pauli(i, j) in [2, 3] == x2z[i, j].
            Bit packing must be in little endian order and only applies to the
            second axis.
        z2x: A 2d numpy array containing the z-to-x coupling bits. The bits can be
            bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
            result will satisfy result.z_output_pauli(i, j) in [1, 2] == z2x[i, j].
            Bit packing must be in little endian order and only applies to the
            second axis.
        z2z: A 2d numpy array containing the z-to-z coupling bits. The bits can be
            bit packed (dtype=uint8) or not (dtype=bool_). When not bit packed, the
            result will satisfy result.z_output_pauli(i, j) in [2, 3] == z2z[i, j].
            Bit packing must be in little endian order and only applies to the
            second axis.
        x_signs: Defaults to all-positive if not specified. A 1d numpy array
            containing the sign bits for the X generator outputs. False means
            positive and True means negative. The bits can be bit packed
            (dtype=uint8) or not (dtype=bool_). Bit packing must be in little endian
            order.
        z_signs: Defaults to all-positive if not specified. A 1d numpy array
            containing the sign bits for the Z generator outputs. False means
            positive and True means negative. The bits can be bit packed
            (dtype=uint8) or not (dtype=bool_). Bit packing must be in little endian
            order.

    Returns:
        The tableau created from the numpy data.

    Examples:
        >>> import stim
        >>> import numpy as np

        >>> tableau = stim.Tableau.from_numpy(
        ...     x2x=np.array([[1, 1], [0, 1]], dtype=np.bool_),
        ...     z2x=np.array([[0, 0], [0, 0]], dtype=np.bool_),
        ...     x2z=np.array([[0, 0], [0, 0]], dtype=np.bool_),
        ...     z2z=np.array([[1, 0], [1, 1]], dtype=np.bool_),
        ... )
        >>> print(repr(tableau))
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+XX"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+ZZ"),
            ],
        )
        >>> tableau == stim.Tableau.from_named_gate("CNOT")
        True

        >>> tableau = stim.Tableau.from_numpy(
        ...     x2x=np.array([[9], [5], [7], [6]], dtype=np.uint8),
        ...     x2z=np.array([[13], [13], [0], [3]], dtype=np.uint8),
        ...     z2x=np.array([[8], [5], [9], [15]], dtype=np.uint8),
        ...     z2z=np.array([[6], [11], [2], [3]], dtype=np.uint8),
        ...     x_signs=np.array([7], dtype=np.uint8),
        ...     z_signs=np.array([9], dtype=np.uint8),
        ... )
        >>> print(repr(tableau))
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("-Y_ZY"),
                stim.PauliString("-Y_YZ"),
                stim.PauliString("-XXX_"),
                stim.PauliString("+ZYX_"),
            ],
            zs=[
                stim.PauliString("-_ZZX"),
                stim.PauliString("+YZXZ"),
                stim.PauliString("+XZ_X"),
                stim.PauliString("-YYXX"),
            ],
        )
    """
```

<a name="stim.Tableau.from_stabilizers"></a>
```python
# stim.Tableau.from_stabilizers

# (in class stim.Tableau)
@staticmethod
def from_stabilizers(
    stabilizers: Iterable[stim.PauliString],
    *,
    allow_redundant: bool = False,
    allow_underconstrained: bool = False,
) -> stim.Tableau:
    """Creates a tableau representing a state with the given stabilizers.

    Args:
        stabilizers: A list of `stim.PauliString`s specifying the stabilizers that
            the state must have. It is permitted for stabilizers to have different
            lengths. All stabilizers are padded up to the length of the longest
            stabilizer by appending identity terms.
        allow_redundant: Defaults to False. If set to False, then the given
            stabilizers must all be independent. If any one of them is a product of
            the others (including the empty product), an exception will be raised.
            If set to True, then redundant stabilizers are simply ignored.
        allow_underconstrained: Defaults to False. If set to False, then the given
            stabilizers must form a complete set of generators. They must exactly
            specify the desired stabilizer state, with no degrees of freedom left
            over. For an n-qubit state there must be n independent stabilizers. If
            set to True, then there can be leftover degrees of freedom which can be
            set arbitrarily.

    Returns:
        A tableau which, when applied to the all-zeroes state, produces a state
        with the given stabilizers.

        Guarantees that result.z_output(k) will be equal to the k'th independent
        stabilizer from the `stabilizers` argument.

    Raises:
        ValueError:
            A stabilizer is redundant but allow_redundant=True wasn't set.
            OR
            The given stabilizers are contradictory (e.g. "+Z" and "-Z" both
            specified).
            OR
            The given stabilizers anticommute (e.g. "+Z" and "+X" both specified).
            OR
            The stabilizers left behind a degree of freedom but
            allow_underconstrained=True wasn't set.
            OR
            A stabilizer has an imaginary sign (i or -i).

    Examples:

        >>> import stim
        >>> stim.Tableau.from_stabilizers([
        ...     stim.PauliString("XX"),
        ...     stim.PauliString("ZZ"),
        ... ])
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+XX"),
                stim.PauliString("+ZZ"),
            ],
        )

        >>> stim.Tableau.from_stabilizers([
        ...     stim.PauliString("XX_"),
        ...     stim.PauliString("ZZ_"),
        ...     stim.PauliString("-YY_"),
        ...     stim.PauliString(""),
        ... ], allow_underconstrained=True, allow_redundant=True)
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z__"),
                stim.PauliString("+_X_"),
                stim.PauliString("+__X"),
            ],
            zs=[
                stim.PauliString("+XX_"),
                stim.PauliString("+ZZ_"),
                stim.PauliString("+__Z"),
            ],
        )
    """
```

<a name="stim.Tableau.from_state_vector"></a>
```python
# stim.Tableau.from_state_vector

# (in class stim.Tableau)
@staticmethod
def from_state_vector(
    state_vector: Iterable[float],
    *,
    endian: str,
) -> stim.Tableau:
    """Creates a tableau representing the stabilizer state of the given state vector.

    Args:
        state_vector: A list of complex amplitudes specifying a superposition. The
            vector must correspond to a state that is reachable using Clifford
            operations, and must be normalized (i.e. it must be a unit vector).
        endian:
            "little": state vector is in little endian order, where higher index
                qubits correspond to larger changes in the state index.
            "big": state vector is in big endian order, where higher index qubits
                correspond to smaller changes in the state index.

    Returns:
        A tableau which, when applied to the all-zeroes state, produces a state
        with the given state vector.

    Raises:
        ValueError:
            The given state vector isn't a list of complex values specifying a
            stabilizer state.
            OR
            The given endian value isn't 'little' or 'big'.

    Examples:

        >>> import stim
        >>> stim.Tableau.from_state_vector([
        ...     0.5**0.5,
        ...     0.5**0.5 * 1j,
        ... ], endian='little')
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+Y"),
            ],
        )
        >>> stim.Tableau.from_state_vector([
        ...     0.5**0.5,
        ...     0,
        ...     0,
        ...     0.5**0.5,
        ... ], endian='little')
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+XX"),
                stim.PauliString("+ZZ"),
            ],
        )
    """
```

<a name="stim.Tableau.from_unitary_matrix"></a>
```python
# stim.Tableau.from_unitary_matrix

# (in class stim.Tableau)
@staticmethod
def from_unitary_matrix(
    matrix: Iterable[Iterable[float]],
    *,
    endian: str = 'little',
) -> stim.Tableau:
    """Creates a tableau from the unitary matrix of a Clifford operation.

    Args:
        matrix: A unitary matrix specified as an iterable of rows, with each row is
            an iterable of amplitudes. The unitary matrix must correspond to a
            Clifford operation.
        endian:
            "little": matrix entries are in little endian order, where higher index
                qubits correspond to larger changes in row/col indices.
            "big": matrix entries are in big endian order, where higher index
                qubits correspond to smaller changes in row/col indices.
    Returns:
        The tableau equivalent to the given unitary matrix (up to global phase).

    Raises:
        ValueError: The given matrix isn't the unitary matrix of a Clifford
            operation.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_unitary_matrix([
        ...     [1, 0],
        ...     [0, 1j],
        ... ], endian='little')
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Y"),
            ],
            zs=[
                stim.PauliString("+Z"),
            ],
        )

        >>> stim.Tableau.from_unitary_matrix([
        ...     [1, 0, 0, 0],
        ...     [0, 1, 0, 0],
        ...     [0, 0, 0, -1j],
        ...     [0, 0, 1j, 0],
        ... ], endian='little')
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+XZ"),
                stim.PauliString("+YX"),
            ],
            zs=[
                stim.PauliString("+ZZ"),
                stim.PauliString("+_Z"),
            ],
        )
    """
```

<a name="stim.Tableau.inverse"></a>
```python
# stim.Tableau.inverse

# (in class stim.Tableau)
def inverse(
    self,
    *,
    unsigned: bool = False,
) -> stim.Tableau:
    """Computes the inverse of the tableau.

    The inverse T^-1 of a tableau T is the unique tableau with the property that
    T * T^-1 = T^-1 * T = I where I is the identity tableau.

    Args:
        unsigned: Defaults to false. When set to true, skips computing the signs of
            the output observables and instead just set them all to be positive.
            This is beneficial because computing the signs takes O(n^3) time and the
            rest of the inverse computation is O(n^2) where n is the number of
            qubits in the tableau. So, if you only need the Pauli terms (not the
            signs), it is significantly cheaper.

    Returns:
        The inverse tableau.

    Examples:
        >>> import stim

        >>> # Check that the inverse agrees with hard-coded tableaus.
        >>> s = stim.Tableau.from_named_gate("S")
        >>> s_dag = stim.Tableau.from_named_gate("S_DAG")
        >>> s.inverse() == s_dag
        True
        >>> z = stim.Tableau.from_named_gate("Z")
        >>> z.inverse() == z
        True

        >>> # Check that multiplying by the inverse produces the identity.
        >>> t = stim.Tableau.random(10)
        >>> t_inv = t.inverse()
        >>> identity = stim.Tableau(10)
        >>> t * t_inv == t_inv * t == identity
        True

        >>> # Check a manual case.
        >>> t = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("-__Z"),
        ...         stim.PauliString("+XZ_"),
        ...         stim.PauliString("+_ZZ"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("-YYY"),
        ...         stim.PauliString("+Z_Z"),
        ...         stim.PauliString("-ZYZ")
        ...     ],
        ... )
        >>> print(t.inverse())
        +-xz-xz-xz-
        | -- +- --
        | XX XX YX
        | XZ Z_ X_
        | X_ YX Y_
        >>> print(t.inverse(unsigned=True))
        +-xz-xz-xz-
        | ++ ++ ++
        | XX XX YX
        | XZ Z_ X_
        | X_ YX Y_
    """
```

<a name="stim.Tableau.inverse_x_output"></a>
```python
# stim.Tableau.inverse_x_output

# (in class stim.Tableau)
def inverse_x_output(
    self,
    input_index: int,
    *,
    unsigned: bool = False,
) -> stim.PauliString:
    """Conjugates a single-qubit X Pauli generator by the inverse of the tableau.

    A faster version of `tableau.inverse(unsigned).x_output(input_index)`.

    Args:
        input_index: Identifies the column (the qubit of the X generator) to return
            from the inverse tableau.
        unsigned: Defaults to false. When set to true, skips computing the result's
            sign and instead just sets it to positive. This is beneficial because
            computing the sign takes O(n^2) time whereas all other parts of the
            computation take O(n) time where n is the number of qubits in the
            tableau.

    Returns:
        The result of conjugating an X generator by the inverse of the tableau.

    Examples:
        >>> import stim

        # Check equivalence with the inverse's x_output.
        >>> t = stim.Tableau.random(4)
        >>> expected = t.inverse().x_output(0)
        >>> t.inverse_x_output(0) == expected
        True
        >>> expected.sign = +1
        >>> t.inverse_x_output(0, unsigned=True) == expected
        True
    """
```

<a name="stim.Tableau.inverse_x_output_pauli"></a>
```python
# stim.Tableau.inverse_x_output_pauli

# (in class stim.Tableau)
def inverse_x_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.inverse().x_output(input_index)[output_index]`

    Args:
        input_index: Identifies the column (the qubit of the input X generator) in
            the inverse tableau.
        output_index: Identifies the row (the output qubit) in the inverse tableau.

    Returns:
        An integer identifying Pauli at the given location in the inverse tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t_inv = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... ).inverse()
        >>> t_inv.inverse_x_output_pauli(0, 0)
        2
        >>> t_inv.inverse_x_output_pauli(0, 1)
        0
        >>> t_inv.inverse_x_output_pauli(1, 0)
        2
        >>> t_inv.inverse_x_output_pauli(1, 1)
        3
    """
```

<a name="stim.Tableau.inverse_y_output"></a>
```python
# stim.Tableau.inverse_y_output

# (in class stim.Tableau)
def inverse_y_output(
    self,
    input_index: int,
    *,
    unsigned: bool = False,
) -> stim.PauliString:
    """Conjugates a single-qubit Y Pauli generator by the inverse of the tableau.

    A faster version of `tableau.inverse(unsigned).y_output(input_index)`.

    Args:
        input_index: Identifies the column (the qubit of the Y generator) to return
            from the inverse tableau.
        unsigned: Defaults to false. When set to true, skips computing the result's
            sign and instead just sets it to positive. This is beneficial because
            computing the sign takes O(n^2) time whereas all other parts of the
            computation take O(n) time where n is the number of qubits in the
            tableau.

    Returns:
        The result of conjugating a Y generator by the inverse of the tableau.

    Examples:
        >>> import stim

        # Check equivalence with the inverse's y_output.
        >>> t = stim.Tableau.random(4)
        >>> expected = t.inverse().y_output(0)
        >>> t.inverse_y_output(0) == expected
        True
        >>> expected.sign = +1
        >>> t.inverse_y_output(0, unsigned=True) == expected
        True
    """
```

<a name="stim.Tableau.inverse_y_output_pauli"></a>
```python
# stim.Tableau.inverse_y_output_pauli

# (in class stim.Tableau)
def inverse_y_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.inverse().y_output(input_index)[output_index]`

    Args:
        input_index: Identifies the column (the qubit of the input Y generator) in
            the inverse tableau.
        output_index: Identifies the row (the output qubit) in the inverse tableau.

    Returns:
        An integer identifying Pauli at the given location in the inverse tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t_inv = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... ).inverse()
        >>> t_inv.inverse_y_output_pauli(0, 0)
        1
        >>> t_inv.inverse_y_output_pauli(0, 1)
        2
        >>> t_inv.inverse_y_output_pauli(1, 0)
        0
        >>> t_inv.inverse_y_output_pauli(1, 1)
        2
    """
```

<a name="stim.Tableau.inverse_z_output"></a>
```python
# stim.Tableau.inverse_z_output

# (in class stim.Tableau)
def inverse_z_output(
    self,
    input_index: int,
    *,
    unsigned: bool = False,
) -> stim.PauliString:
    """Conjugates a single-qubit Z Pauli generator by the inverse of the tableau.

    A faster version of `tableau.inverse(unsigned).z_output(input_index)`.

    Args:
        input_index: Identifies the column (the qubit of the Z generator) to return
            from the inverse tableau.
        unsigned: Defaults to false. When set to true, skips computing the result's
            sign and instead just sets it to positive. This is beneficial because
            computing the sign takes O(n^2) time whereas all other parts of the
            computation take O(n) time where n is the number of qubits in the
            tableau.

    Returns:
        The result of conjugating a Z generator by the inverse of the tableau.

    Examples:
        >>> import stim

        >>> import stim

        # Check equivalence with the inverse's z_output.
        >>> t = stim.Tableau.random(4)
        >>> expected = t.inverse().z_output(0)
        >>> t.inverse_z_output(0) == expected
        True
        >>> expected.sign = +1
        >>> t.inverse_z_output(0, unsigned=True) == expected
        True
    """
```

<a name="stim.Tableau.inverse_z_output_pauli"></a>
```python
# stim.Tableau.inverse_z_output_pauli

# (in class stim.Tableau)
def inverse_z_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.inverse().z_output(input_index)[output_index]`

    Args:
        input_index: Identifies the column (the qubit of the input Z generator) in
            the inverse tableau.
        output_index: Identifies the row (the output qubit) in the inverse tableau.

    Returns:
        An integer identifying Pauli at the given location in the inverse tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t_inv = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... ).inverse()
        >>> t_inv.inverse_z_output_pauli(0, 0)
        3
        >>> t_inv.inverse_z_output_pauli(0, 1)
        2
        >>> t_inv.inverse_z_output_pauli(1, 0)
        2
        >>> t_inv.inverse_z_output_pauli(1, 1)
        1
    """
```

<a name="stim.Tableau.iter_all"></a>
```python
# stim.Tableau.iter_all

# (in class stim.Tableau)
@staticmethod
def iter_all(
    num_qubits: int,
    *,
    unsigned: bool = False,
) -> stim.TableauIterator:
    """Returns an iterator that iterates over all Tableaus of a given size.

    Args:
        num_qubits: The size of tableau to iterate over.
        unsigned: Defaults to False. If set to True, only tableaus where
            all columns have positive sign are yielded by the iterator.
            This substantially reduces the total number of tableaus to
            iterate over.

    Returns:
        An Iterable[stim.Tableau] that yields the requested tableaus.

    Examples:
        >>> import stim
        >>> single_qubit_gate_reprs = set()
        >>> for t in stim.Tableau.iter_all(1):
        ...     single_qubit_gate_reprs.add(repr(t))
        >>> len(single_qubit_gate_reprs)
        24

        >>> num_2q_gates_mod_paulis = 0
        >>> for _ in stim.Tableau.iter_all(2, unsigned=True):
        ...     num_2q_gates_mod_paulis += 1
        >>> num_2q_gates_mod_paulis
        720
    """
```

<a name="stim.Tableau.prepend"></a>
```python
# stim.Tableau.prepend

# (in class stim.Tableau)
def prepend(
    self,
    gate: stim.Tableau,
    targets: Sequence[int],
) -> None:
    """Prepends an operation's effect into this tableau, mutating this tableau.

    Time cost is O(n*m*m) where n=len(self) and m=len(gate).

    Args:
        gate: The tableau of the operation being prepended into this tableau.
        targets: The qubits being targeted by the gate.

    Examples:
        >>> import stim
        >>> t = stim.Tableau.from_named_gate("H")
        >>> t.prepend(stim.Tableau.from_named_gate("X"), [0])
        >>> t == stim.Tableau.from_named_gate("SQRT_Y_DAG")
        True
    """
```

<a name="stim.Tableau.random"></a>
```python
# stim.Tableau.random

# (in class stim.Tableau)
@staticmethod
def random(
    num_qubits: int,
) -> stim.Tableau:
    """Samples a uniformly random Clifford operation and returns its tableau.

    Args:
        num_qubits: The number of qubits the tableau should act on.

    Returns:
        The sampled tableau.

    Examples:
        >>> import stim
        >>> t = stim.Tableau.random(42)

    References:
        "Hadamard-free circuits expose the structure of the Clifford group"
        Sergey Bravyi, Dmitri Maslov
        https://arxiv.org/abs/2003.09412
    """
```

<a name="stim.Tableau.then"></a>
```python
# stim.Tableau.then

# (in class stim.Tableau)
def then(
    self,
    second: stim.Tableau,
) -> stim.Tableau:
    """Returns the result of composing two tableaus.

    If the tableau T1 represents the Clifford operation with unitary C1,
    and the tableau T2 represents the Clifford operation with unitary C2,
    then the tableau T1.then(T2) represents the Clifford operation with unitary
    C2*C1.

    Args:
        second: The result is equivalent to applying the second tableau after
            the receiving tableau.

    Examples:
        >>> import stim
        >>> t1 = stim.Tableau.random(4)
        >>> t2 = stim.Tableau.random(4)
        >>> t3 = t1.then(t2)
        >>> p = stim.PauliString.random(4)
        >>> t3(p) == t2(t1(p))
        True
    """
```

<a name="stim.Tableau.to_circuit"></a>
```python
# stim.Tableau.to_circuit

# (in class stim.Tableau)
def to_circuit(
    self,
    *,
    method: str = 'elimination',
) -> stim.Circuit:
    """Synthesizes a circuit that implements the tableau's Clifford operation.

    The circuits returned by this method are not guaranteed to be stable
    from version to version, and may be produced using randomization.

    Args:
        method: The method to use when synthesizing the circuit. Available values:
            "elimination": Cancels off-diagonal terms using Gaussian elimination.
                Gate set: H, S, CX
                Circuit qubit count: n
                Circuit operation count: O(n^2)
                Circuit depth: O(n^2)

    Returns:
        The synthesized circuit.

    Example:
        >>> import stim
        >>> tableau = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("-_YZ"),
        ...         stim.PauliString("-YY_"),
        ...         stim.PauliString("-XZX"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("+Y_Y"),
        ...         stim.PauliString("-_XY"),
        ...         stim.PauliString("-Y__"),
        ...     ],
        ... )
        >>> tableau.to_circuit(method="elimination")
        stim.Circuit('''
            CX 2 0 0 2 2 0
            S 0
            H 0
            S 0
            H 1
            CX 0 1 0 2
            H 1 2
            CX 1 0 2 0 2 1 1 2 2 1
            H 1
            S 1 2
            H 2
            CX 2 1
            S 2
            H 0 1 2
            S 0 0 1 1 2 2
            H 0 1 2
            S 1 1 2 2
        ''')
    """
```

<a name="stim.Tableau.to_numpy"></a>
```python
# stim.Tableau.to_numpy

# (in class stim.Tableau)
def to_numpy(
    self,
    *,
    bit_packed: bool = False,
) -> Tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
    """Decomposes the contents of the tableau into six numpy arrays.

    The first four numpy arrays correspond to the four quadrants of the table
    defined in Aaronson and Gottesman's "Improved Simulation of Stabilizer Circuits"
    ( https://arxiv.org/abs/quant-ph/0406196 ).

    The last two numpy arrays are the X and Z sign bit vectors of the tableau.

    Args:
        bit_packed: Defaults to False. Determines whether the output numpy arrays
            use dtype=bool_ or dtype=uint8 with 8 bools packed into each byte.

    Returns:
        An (x2x, x2z, z2x, z2z, x_signs, z_signs) tuple encoding the tableau.

        x2x: A 2d table of whether tableau(X_i)_j is X or Y (instead of I or Z).
        x2z: A 2d table of whether tableau(X_i)_j is Z or Y (instead of I or X).
        z2x: A 2d table of whether tableau(Z_i)_j is X or Y (instead of I or Z).
        z2z: A 2d table of whether tableau(Z_i)_j is Z or Y (instead of I or X).
        x_signs: A vector of whether tableau(X_i) is negative.
        z_signs: A vector of whether tableau(Z_i) is negative.

        If bit_packed=False then:
            x2x.dtype = np.bool_
            x2z.dtype = np.bool_
            z2x.dtype = np.bool_
            z2z.dtype = np.bool_
            x_signs.dtype = np.bool_
            z_signs.dtype = np.bool_
            x2x.shape = (len(tableau), len(tableau))
            x2z.shape = (len(tableau), len(tableau))
            z2x.shape = (len(tableau), len(tableau))
            z2z.shape = (len(tableau), len(tableau))
            x_signs.shape = len(tableau)
            z_signs.shape = len(tableau)
            x2x[i, j] = tableau.x_output_pauli(i, j) in [1, 2]
            x2z[i, j] = tableau.x_output_pauli(i, j) in [2, 3]
            z2x[i, j] = tableau.z_output_pauli(i, j) in [1, 2]
            z2z[i, j] = tableau.z_output_pauli(i, j) in [2, 3]

        If bit_packed=True then:
            x2x.dtype = np.uint8
            x2z.dtype = np.uint8
            z2x.dtype = np.uint8
            z2z.dtype = np.uint8
            x_signs.dtype = np.uint8
            z_signs.dtype = np.uint8
            x2x.shape = (len(tableau), math.ceil(len(tableau) / 8))
            x2z.shape = (len(tableau), math.ceil(len(tableau) / 8))
            z2x.shape = (len(tableau), math.ceil(len(tableau) / 8))
            z2z.shape = (len(tableau), math.ceil(len(tableau) / 8))
            x_signs.shape = math.ceil(len(tableau) / 8)
            z_signs.shape = math.ceil(len(tableau) / 8)
            (x2x[i, j // 8] >> (j % 8)) & 1 = tableau.x_output_pauli(i, j) in [1, 2]
            (x2z[i, j // 8] >> (j % 8)) & 1 = tableau.x_output_pauli(i, j) in [2, 3]
            (z2x[i, j // 8] >> (j % 8)) & 1 = tableau.z_output_pauli(i, j) in [1, 2]
            (z2z[i, j // 8] >> (j % 8)) & 1 = tableau.z_output_pauli(i, j) in [2, 3]

    Examples:
        >>> import stim
        >>> cnot = stim.Tableau.from_named_gate("CNOT")
        >>> print(repr(cnot))
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+XX"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+ZZ"),
            ],
        )
        >>> x2x, x2z, z2x, z2z, x_signs, z_signs = cnot.to_numpy()
        >>> x2x
        array([[ True,  True],
               [False,  True]])
        >>> x2z
        array([[False, False],
               [False, False]])
        >>> z2x
        array([[False, False],
               [False, False]])
        >>> z2z
        array([[ True, False],
               [ True,  True]])
        >>> x_signs
        array([False, False])
        >>> z_signs
        array([False, False])

        >>> t = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("-Y_ZY"),
        ...         stim.PauliString("-Y_YZ"),
        ...         stim.PauliString("-XXX_"),
        ...         stim.PauliString("+ZYX_"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("-_ZZX"),
        ...         stim.PauliString("+YZXZ"),
        ...         stim.PauliString("+XZ_X"),
        ...         stim.PauliString("-YYXX"),
        ...     ],
        ... )

        >>> x2x, x2z, z2x, z2z, x_signs, z_signs = t.to_numpy()
        >>> x2x
        array([[ True, False, False,  True],
               [ True, False,  True, False],
               [ True,  True,  True, False],
               [False,  True,  True, False]])
        >>> x2z
        array([[ True, False,  True,  True],
               [ True, False,  True,  True],
               [False, False, False, False],
               [ True,  True, False, False]])
        >>> z2x
        array([[False, False, False,  True],
               [ True, False,  True, False],
               [ True, False, False,  True],
               [ True,  True,  True,  True]])
        >>> z2z
        array([[False,  True,  True, False],
               [ True,  True, False,  True],
               [False,  True, False, False],
               [ True,  True, False, False]])
        >>> x_signs
        array([ True,  True,  True, False])
        >>> z_signs
        array([ True, False, False,  True])

        >>> x2x, x2z, z2x, z2z, x_signs, z_signs = t.to_numpy(bit_packed=True)
        >>> x2x
        array([[9],
               [5],
               [7],
               [6]], dtype=uint8)
        >>> x2z
        array([[13],
               [13],
               [ 0],
               [ 3]], dtype=uint8)
        >>> z2x
        array([[ 8],
               [ 5],
               [ 9],
               [15]], dtype=uint8)
        >>> z2z
        array([[ 6],
               [11],
               [ 2],
               [ 3]], dtype=uint8)
        >>> x_signs
        array([7], dtype=uint8)
        >>> z_signs
        array([9], dtype=uint8)
    """
```

<a name="stim.Tableau.to_pauli_string"></a>
```python
# stim.Tableau.to_pauli_string

# (in class stim.Tableau)
def to_pauli_string(
    self,
) -> stim.PauliString:
    """Return a Pauli string equivalent to the tableau.

    If the tableau is equivalent to a pauli product, creates
    an equivalent pauli string. If not, then an error is raised.

    Returns:
        The created pauli string

    Raises:
        ValueError: The Tableau isn't equivalent to a Pauli product.

    Example:
        >>> import stim
        >>> t = (stim.Tableau.from_named_gate("Z") +
        ...      stim.Tableau.from_named_gate("Y") +
        ...      stim.Tableau.from_named_gate("I") +
        ...      stim.Tableau.from_named_gate("X"))
        >>> print(t)
        +-xz-xz-xz-xz-
        | -+ -- ++ +-
        | XZ __ __ __
        | __ XZ __ __
        | __ __ XZ __
        | __ __ __ XZ
        >>> print(t.to_pauli_string())
        +ZY_X
    """
```

<a name="stim.Tableau.to_state_vector"></a>
```python
# stim.Tableau.to_state_vector

# (in class stim.Tableau)
def to_state_vector(
    self,
    *,
    endian: str = 'little',
) -> np.ndarray[np.complex64]:
    """Returns the state vector produced by applying the tableau to the |0..0> state.

    This function takes O(n * 2**n) time and O(2**n) space, where n is the number of
    qubits. The computation is done by initialization a random state vector and
    iteratively projecting it into the +1 eigenspace of each stabilizer of the
    state. The state is then canonicalized so that zero values are actually exactly
    0, and so that the first non-zero entry is positive.

    Args:
        endian:
            "little" (default): state vector is in little endian order, where higher
                index qubits correspond to larger changes in the state index.
            "big": state vector is in big endian order, where higher index qubits
                correspond to smaller changes in the state index.

    Returns:
        A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes.

        If the result is in little endian order then the amplitude at offset
        b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
        computational basis state where the qubit with index 0 is storing the bit
        b_0, the qubit with index 1 is storing the bit b_1, etc.

        If the result is in big endian order then the amplitude at offset
        b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
        computational basis state where the qubit with index 0 is storing the bit
        b_{n-1}, the qubit with index 1 is storing the bit b_{n-2}, etc.

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> i2 = stim.Tableau.from_named_gate('I')
        >>> x = stim.Tableau.from_named_gate('X')
        >>> h = stim.Tableau.from_named_gate('H')

        >>> (x + i2).to_state_vector(endian='little')
        array([0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j], dtype=complex64)

        >>> (i2 + x).to_state_vector(endian='little')
        array([0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j], dtype=complex64)

        >>> (i2 + x).to_state_vector(endian='big')
        array([0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j], dtype=complex64)

        >>> (h + h).to_state_vector(endian='little')
        array([0.5+0.j, 0.5+0.j, 0.5+0.j, 0.5+0.j], dtype=complex64)
    """
```

<a name="stim.Tableau.to_unitary_matrix"></a>
```python
# stim.Tableau.to_unitary_matrix

# (in class stim.Tableau)
def to_unitary_matrix(
    self,
    *,
    endian: str,
) -> np.ndarray[np.complex64]:
    """Converts the tableau into a unitary matrix.

    For an n-qubit tableau, this method performs O(n 4^n) work. It uses the state
    channel duality to transform the tableau into a list of stabilizers, then
    generates a random state vector and projects it into the +1 eigenspace of each
    stabilizer.

    Note that tableaus don't have a defined global phase, so the result's global
    phase may be different from what you expect. For example, the square of
    SQRT_X's unitary might equal -X instead of +X.

    Args:
        endian:
            "little": The first qubit is the least significant (corresponds
                to an offset of 1 in the state vector).
            "big": The first qubit is the most significant (corresponds
                to an offset of 2**(n - 1) in the state vector).

    Returns:
        A numpy array with dtype=np.complex64 and
        shape=(1 << len(tableau), 1 << len(tableau)).

    Example:
        >>> import stim
        >>> cnot = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("XX"),
        ...         stim.PauliString("_X"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("Z_"),
        ...         stim.PauliString("ZZ"),
        ...     ],
        ... )
        >>> cnot.to_unitary_matrix(endian='big')
        array([[1.+0.j, 0.+0.j, 0.+0.j, 0.+0.j],
               [0.+0.j, 1.+0.j, 0.+0.j, 0.+0.j],
               [0.+0.j, 0.+0.j, 0.+0.j, 1.+0.j],
               [0.+0.j, 0.+0.j, 1.+0.j, 0.+0.j]], dtype=complex64)
    """
```

<a name="stim.Tableau.x_output"></a>
```python
# stim.Tableau.x_output

# (in class stim.Tableau)
def x_output(
    self,
    target: int,
) -> stim.PauliString:
    """Returns the result of conjugating a Pauli X by the tableau's Clifford operation.

    Args:
        target: The qubit targeted by the Pauli X operation.

    Examples:
        >>> import stim
        >>> h = stim.Tableau.from_named_gate("H")
        >>> h.x_output(0)
        stim.PauliString("+Z")

        >>> cnot = stim.Tableau.from_named_gate("CNOT")
        >>> cnot.x_output(0)
        stim.PauliString("+XX")
        >>> cnot.x_output(1)
        stim.PauliString("+_X")
    """
```

<a name="stim.Tableau.x_output_pauli"></a>
```python
# stim.Tableau.x_output_pauli

# (in class stim.Tableau)
def x_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.x_output(input_index)[output_index]`

    Args:
        input_index: Identifies the tableau column (the qubit of the input X
            generator).
        output_index: Identifies the tableau row (the output qubit).

    Returns:
        An integer identifying Pauli at the given location in the tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... )
        >>> t.x_output_pauli(0, 0)
        2
        >>> t.x_output_pauli(0, 1)
        0
        >>> t.x_output_pauli(1, 0)
        2
        >>> t.x_output_pauli(1, 1)
        3
    """
```

<a name="stim.Tableau.x_sign"></a>
```python
# stim.Tableau.x_sign

# (in class stim.Tableau)
def x_sign(
    self,
    target: int,
) -> int:
    """Returns just the sign of the result of conjugating an X generator.

    This operation runs in constant time.

    Args:
        target: The qubit the X generator applies to.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_named_gate("S_DAG").x_sign(0)
        -1
        >>> stim.Tableau.from_named_gate("S").x_sign(0)
        1
    """
```

<a name="stim.Tableau.y_output"></a>
```python
# stim.Tableau.y_output

# (in class stim.Tableau)
def y_output(
    self,
    target: int,
) -> stim.PauliString:
    """Returns the result of conjugating a Pauli Y by the tableau's Clifford operation.

    Args:
        target: The qubit targeted by the Pauli Y operation.

    Examples:
        >>> import stim
        >>> h = stim.Tableau.from_named_gate("H")
        >>> h.y_output(0)
        stim.PauliString("-Y")

        >>> cnot = stim.Tableau.from_named_gate("CNOT")
        >>> cnot.y_output(0)
        stim.PauliString("+YX")
        >>> cnot.y_output(1)
        stim.PauliString("+ZY")
    """
```

<a name="stim.Tableau.y_output_pauli"></a>
```python
# stim.Tableau.y_output_pauli

# (in class stim.Tableau)
def y_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.y_output(input_index)[output_index]`

    Args:
        input_index: Identifies the tableau column (the qubit of the input Y
            generator).
        output_index: Identifies the tableau row (the output qubit).

    Returns:
        An integer identifying Pauli at the given location in the tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... )
        >>> t.y_output_pauli(0, 0)
        1
        >>> t.y_output_pauli(0, 1)
        2
        >>> t.y_output_pauli(1, 0)
        0
        >>> t.y_output_pauli(1, 1)
        2
    """
```

<a name="stim.Tableau.y_sign"></a>
```python
# stim.Tableau.y_sign

# (in class stim.Tableau)
def y_sign(
    self,
    target: int,
) -> int:
    """Returns just the sign of the result of conjugating a Y generator.

    Unlike x_sign and z_sign, this operation runs in linear time.
    The Y generator has to be computed by multiplying the X and Z
    outputs and the sign depends on all terms.

    Args:
        target: The qubit the Y generator applies to.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_named_gate("S_DAG").y_sign(0)
        1
        >>> stim.Tableau.from_named_gate("S").y_sign(0)
        -1
    """
```

<a name="stim.Tableau.z_output"></a>
```python
# stim.Tableau.z_output

# (in class stim.Tableau)
def z_output(
    self,
    target: int,
) -> stim.PauliString:
    """Returns the result of conjugating a Pauli Z by the tableau's Clifford operation.

    Args:
        target: The qubit targeted by the Pauli Z operation.

    Examples:
        >>> import stim
        >>> h = stim.Tableau.from_named_gate("H")
        >>> h.z_output(0)
        stim.PauliString("+X")

        >>> cnot = stim.Tableau.from_named_gate("CNOT")
        >>> cnot.z_output(0)
        stim.PauliString("+Z_")
        >>> cnot.z_output(1)
        stim.PauliString("+ZZ")
    """
```

<a name="stim.Tableau.z_output_pauli"></a>
```python
# stim.Tableau.z_output_pauli

# (in class stim.Tableau)
def z_output_pauli(
    self,
    input_index: int,
    output_index: int,
) -> int:
    """Constant-time version of `tableau.z_output(input_index)[output_index]`

    Args:
        input_index: Identifies the tableau column (the qubit of the input Z
            generator).
        output_index: Identifies the tableau row (the output qubit).

    Returns:
        An integer identifying Pauli at the given location in the tableau:

            0: I
            1: X
            2: Y
            3: Z

    Examples:
        >>> import stim

        >>> t = stim.Tableau.from_conjugated_generators(
        ...     xs=[stim.PauliString("-Y_"), stim.PauliString("+YZ")],
        ...     zs=[stim.PauliString("-ZY"), stim.PauliString("+YX")],
        ... )
        >>> t.z_output_pauli(0, 0)
        3
        >>> t.z_output_pauli(0, 1)
        2
        >>> t.z_output_pauli(1, 0)
        2
        >>> t.z_output_pauli(1, 1)
        1
    """
```

<a name="stim.Tableau.z_sign"></a>
```python
# stim.Tableau.z_sign

# (in class stim.Tableau)
def z_sign(
    self,
    target: int,
) -> int:
    """Returns just the sign of the result of conjugating a Z generator.

    This operation runs in constant time.

    Args:
        target: The qubit the Z generator applies to.

    Examples:
        >>> import stim
        >>> stim.Tableau.from_named_gate("SQRT_X_DAG").z_sign(0)
        1
        >>> stim.Tableau.from_named_gate("SQRT_X").z_sign(0)
        -1
    """
```

<a name="stim.TableauIterator"></a>
```python
# stim.TableauIterator

# (at top-level in the stim module)
class TableauIterator:
    """Iterates over all stabilizer tableaus of a specified size.

    Examples:
        >>> import stim
        >>> tableau_iterator = stim.Tableau.iter_all(1)
        >>> n = 0
        >>> for single_qubit_clifford in tableau_iterator:
        ...     n += 1
        >>> n
        24
    """
```

<a name="stim.TableauIterator.__iter__"></a>
```python
# stim.TableauIterator.__iter__

# (in class stim.TableauIterator)
def __iter__(
    self,
) -> stim.TableauIterator:
    """Returns an independent copy of the tableau iterator.

    Since for-loops and loop-comprehensions call `iter` on things they
    iterate, this effectively allows the iterator to be iterated
    multiple times.
    """
```

<a name="stim.TableauIterator.__next__"></a>
```python
# stim.TableauIterator.__next__

# (in class stim.TableauIterator)
def __next__(
    self,
) -> stim.Tableau:
    """Returns the next iterated tableau.
    """
```

<a name="stim.TableauSimulator"></a>
```python
# stim.TableauSimulator

# (at top-level in the stim module)
class TableauSimulator:
    """A stabilizer circuit simulator that tracks an inverse stabilizer tableau.

    Supports interactive usage, where gates and measurements are applied on demand.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> if s.measure(0):
        ...     s.h(1)
        ...     s.cnot(1, 2)
        >>> s.measure(1) == s.measure(2)
        True

        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.cnot(0, 1)
        >>> s.current_inverse_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+ZX"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+X_"),
                stim.PauliString("+XZ"),
            ],
        )
    """
```

<a name="stim.TableauSimulator.__init__"></a>
```python
# stim.TableauSimulator.__init__

# (in class stim.TableauSimulator)
def __init__(
    self,
    *,
    seed: Optional[int] = None,
) -> None:
    """Initializes a stim.TableauSimulator.

    Args:
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng is seeded from system entropy.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how the
            circuit is executed. For example, reordering whether a reset on one
            qubit happens before or after a reset on another qubit can result in
            different measurement results being observed starting from the same
            seed.

    Returns:
        An initialized stim.TableauSimulator.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator(seed=0)
        >>> s2 = stim.TableauSimulator(seed=0)
        >>> s.h(0)
        >>> s2.h(0)
        >>> s.measure(0) == s2.measure(0)
        True
    """
```

<a name="stim.TableauSimulator.c_xyz"></a>
```python
# stim.TableauSimulator.c_xyz

# (in class stim.TableauSimulator)
def c_xyz(
    self,
    *targets,
) -> None:
    """Applies a C_XYZ gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.c_zyx"></a>
```python
# stim.TableauSimulator.c_zyx

# (in class stim.TableauSimulator)
def c_zyx(
    self,
    *targets,
) -> None:
    """Applies a C_ZYX gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.canonical_stabilizers"></a>
```python
# stim.TableauSimulator.canonical_stabilizers

# (in class stim.TableauSimulator)
def canonical_stabilizers(
    self,
) -> List[stim.PauliString]:
    """Returns a standardized list of the simulator's current stabilizer generators.

    Two simulators have the same canonical stabilizers if and only if their current
    quantum state is equal (and tracking the same number of qubits).

    The canonical form is computed as follows:

        1. Get a list of stabilizers using the `z_output`s of
            `simulator.current_inverse_tableau()**-1`.
        2. Perform Gaussian elimination on each generator g.
            2a) The generators are considered in order X0, Z0, X1, Z1, X2, Z2, etc.
            2b) Pick any stabilizer that uses the generator g. If there are none,
                go to the next g.
            2c) Multiply that stabilizer into all other stabilizers that use the
                generator g.
            2d) Swap that stabilizer with the stabilizer at position `next_output`
                then increment `next_output`.

    Returns:
        A List[stim.PauliString] of the simulator's state's stabilizers.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.cnot(0, 1)
        >>> s.x(2)
        >>> for e in s.canonical_stabilizers():
        ...     print(repr(e))
        stim.PauliString("+XX_")
        stim.PauliString("+ZZ_")
        stim.PauliString("-__Z")

        >>> # Scramble the stabilizers then check the canonical form is unchanged.
        >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
        >>> s.cnot(0, 1)
        >>> s.cz(0, 2)
        >>> s.s(0, 2)
        >>> s.cy(2, 1)
        >>> s.set_inverse_tableau(s.current_inverse_tableau()**-1)
        >>> for e in s.canonical_stabilizers():
        ...     print(repr(e))
        stim.PauliString("+XX_")
        stim.PauliString("+ZZ_")
        stim.PauliString("-__Z")
    """
```

<a name="stim.TableauSimulator.cnot"></a>
```python
# stim.TableauSimulator.cnot

# (in class stim.TableauSimulator)
def cnot(
    self,
    *targets,
) -> None:
    """Applies a controlled X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.copy"></a>
```python
# stim.TableauSimulator.copy

# (in class stim.TableauSimulator)
def copy(
    self,
    *,
    copy_rng: bool = False,
    seed: Optional[int] = None,
) -> stim.TableauSimulator:
    """Returns a simulator with the same internal state, except perhaps its prng.

    Args:
        copy_rng: By default, new simulator's prng is reinitialized with a random
            seed. However, one can set this argument to True in order to have the
            prng state copied together with the rest of the original simulator's
            state. Consequently, in this case the two simulators will produce the
            same measurement outcomes for the same quantum circuits.  If both seed
            and copy_rng are set, an exception is raised. Defaults to False.
        seed: PARTIALLY determines simulation results by deterministically seeding
            the random number generator.

            Must be None or an integer in range(2**64).

            Defaults to None. When None, the prng state is either copied from the
            original simulator or reseeded from system entropy, depending on the
            copy_rng argument.

            When set to an integer, making the exact same series calls on the exact
            same machine with the exact same version of Stim will produce the exact
            same simulation results.

            CAUTION: simulation results *WILL NOT* be consistent between versions of
            Stim. This restriction is present to make it possible to have future
            optimizations to the random sampling, and is enforced by introducing
            intentional differences in the seeding strategy from version to version.

            CAUTION: simulation results *MAY NOT* be consistent across machines that
            differ in the width of supported SIMD instructions. For example, using
            the same seed on a machine that supports AVX instructions and one that
            only supports SSE instructions may produce different simulation results.

            CAUTION: simulation results *MAY NOT* be consistent if you vary how the
            circuit is executed. For example, reordering whether a reset on one
            qubit happens before or after a reset on another qubit can result in
            different measurement results being observed starting from the same
            seed.

    Examples:
        >>> import stim

        >>> s1 = stim.TableauSimulator()
        >>> s1.set_inverse_tableau(stim.Tableau.random(1))
        >>> s2 = s1.copy()
        >>> s2 is s1
        False
        >>> s2.current_inverse_tableau() == s1.current_inverse_tableau()
        True

        >>> s1 = stim.TableauSimulator()
        >>> s2 = s1.copy(copy_rng=True)
        >>> s1.h(0)
        >>> s2.h(0)
        >>> assert s1.measure(0) == s2.measure(0)

        >>> s = stim.TableauSimulator()
        >>> def brute_force_post_select(qubit, desired_result):
        ...     global s
        ...     while True:
        ...         s2 = s.copy()
        ...         if s2.measure(qubit) == desired_result:
        ...             s = s2
        ...             break
        >>> s.h(0)
        >>> brute_force_post_select(qubit=0, desired_result=True)
        >>> s.measure(0)
        True
    """
```

<a name="stim.TableauSimulator.current_inverse_tableau"></a>
```python
# stim.TableauSimulator.current_inverse_tableau

# (in class stim.TableauSimulator)
def current_inverse_tableau(
    self,
) -> stim.Tableau:
    """Returns a copy of the internal state of the simulator as a stim.Tableau.

    Returns:
        A stim.Tableau copy of the simulator's state.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.current_inverse_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+X"),
            ],
        )
        >>> s.cnot(0, 1)
        >>> s.current_inverse_tableau()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+ZX"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+X_"),
                stim.PauliString("+XZ"),
            ],
        )
    """
```

<a name="stim.TableauSimulator.current_measurement_record"></a>
```python
# stim.TableauSimulator.current_measurement_record

# (in class stim.TableauSimulator)
def current_measurement_record(
    self,
) -> List[bool]:
    """Returns a copy of the record of all measurements performed by the simulator.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.current_measurement_record()
        []
        >>> s.measure(0)
        False
        >>> s.x(0)
        >>> s.measure(0)
        True
        >>> s.current_measurement_record()
        [False, True]
        >>> s.do(stim.Circuit("M 0"))
        >>> s.current_measurement_record()
        [False, True, True]

    Returns:
        A list of booleans containing the result of every measurement performed by
        the simulator so far.
    """
```

<a name="stim.TableauSimulator.cx"></a>
```python
# stim.TableauSimulator.cx

# (in class stim.TableauSimulator)
def cx(
    self,
    *targets,
) -> None:
    """Applies a controlled X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.cy"></a>
```python
# stim.TableauSimulator.cy

# (in class stim.TableauSimulator)
def cy(
    self,
    *targets,
) -> None:
    """Applies a controlled Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.cz"></a>
```python
# stim.TableauSimulator.cz

# (in class stim.TableauSimulator)
def cz(
    self,
    *targets,
) -> None:
    """Applies a controlled Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.depolarize1"></a>
```python
# stim.TableauSimulator.depolarize1

# (in class stim.TableauSimulator)
def depolarize1(
    self,
    *targets: int,
    p: float,
):
    """Probabilistically applies single-qubit depolarization to targets.

    Args:
        *targets: The indices of the qubits to target with the noise.
        p: The chance of the error being applied,
            independently, to each qubit.
    """
```

<a name="stim.TableauSimulator.depolarize2"></a>
```python
# stim.TableauSimulator.depolarize2

# (in class stim.TableauSimulator)
def depolarize2(
    self,
    *targets: int,
    p: float,
):
    """Probabilistically applies two-qubit depolarization to targets.

    Args:
        *targets: The indices of the qubits to target with the noise.
            The pairs of qubits are formed by
            zip(targets[::1], targets[1::2]).
        p: The chance of the error being applied,
            independently, to each qubit pair.
    """
```

<a name="stim.TableauSimulator.do"></a>
```python
# stim.TableauSimulator.do

# (in class stim.TableauSimulator)
def do(
    self,
    circuit_or_pauli_string: Union[stim.Circuit, stim.PauliString, stim.CircuitInstruction, stim.CircuitRepeatBlock],
) -> None:
    """Applies a circuit or pauli string to the simulator's state.

    Args:
        circuit_or_pauli_string: A stim.Circuit, stim.PauliString,
            stim.CircuitInstruction, or stim.CircuitRepeatBlock
            with operations to apply to the simulator's state.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.do(stim.Circuit('''
        ...     X 0
        ...     M 0
        ... '''))
        >>> s.current_measurement_record()
        [True]

        >>> s = stim.TableauSimulator()
        >>> s.do(stim.PauliString("IXYZ"))
        >>> s.measure_many(0, 1, 2, 3)
        [False, True, True, False]
    """
```

<a name="stim.TableauSimulator.do_circuit"></a>
```python
# stim.TableauSimulator.do_circuit

# (in class stim.TableauSimulator)
def do_circuit(
    self,
    circuit: stim.Circuit,
) -> None:
    """Applies a circuit to the simulator's state.

    Args:
        circuit: A stim.Circuit containing operations to apply.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.do_circuit(stim.Circuit('''
        ...     X 0
        ...     M 0
        ... '''))
        >>> s.current_measurement_record()
        [True]
    """
```

<a name="stim.TableauSimulator.do_pauli_string"></a>
```python
# stim.TableauSimulator.do_pauli_string

# (in class stim.TableauSimulator)
def do_pauli_string(
    self,
    pauli_string: stim.PauliString,
) -> None:
    """Applies the paulis from a pauli string to the simulator's state.

    Args:
        pauli_string: A stim.PauliString containing Paulis to apply.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.do_pauli_string(stim.PauliString("IXYZ"))
        >>> s.measure_many(0, 1, 2, 3)
        [False, True, True, False]
    """
```

<a name="stim.TableauSimulator.do_tableau"></a>
```python
# stim.TableauSimulator.do_tableau

# (in class stim.TableauSimulator)
def do_tableau(
    self,
    tableau: stim.Tableau,
    targets: List[int],
) -> None:
    """Applies a custom tableau operation to qubits in the simulator.

    Note that this method has to compute the inverse of the tableau, because the
    simulator's internal state is an inverse tableau.

    Args:
        tableau: A stim.Tableau representing the Clifford operation to apply.
        targets: The indices of the qubits to operate on.

    Examples:
        >>> import stim
        >>> sim = stim.TableauSimulator()
        >>> sim.h(1)
        >>> sim.h_yz(2)
        >>> [str(sim.peek_bloch(k)) for k in range(4)]
        ['+Z', '+X', '+Y', '+Z']
        >>> rot3 = stim.Tableau.from_conjugated_generators(
        ...     xs=[
        ...         stim.PauliString("_X_"),
        ...         stim.PauliString("__X"),
        ...         stim.PauliString("X__"),
        ...     ],
        ...     zs=[
        ...         stim.PauliString("_Z_"),
        ...         stim.PauliString("__Z"),
        ...         stim.PauliString("Z__"),
        ...     ],
        ... )

        >>> sim.do_tableau(rot3, [1, 2, 3])
        >>> [str(sim.peek_bloch(k)) for k in range(4)]
        ['+Z', '+Z', '+X', '+Y']

        >>> sim.do_tableau(rot3, [1, 2, 3])
        >>> [str(sim.peek_bloch(k)) for k in range(4)]
        ['+Z', '+Y', '+Z', '+X']
    """
```

<a name="stim.TableauSimulator.h"></a>
```python
# stim.TableauSimulator.h

# (in class stim.TableauSimulator)
def h(
    self,
    *targets,
) -> None:
    """Applies a Hadamard gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.h_xy"></a>
```python
# stim.TableauSimulator.h_xy

# (in class stim.TableauSimulator)
def h_xy(
    self,
    *targets,
) -> None:
    """Applies an operation that swaps the X and Y axes to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.h_xz"></a>
```python
# stim.TableauSimulator.h_xz

# (in class stim.TableauSimulator)
def h_xz(
    self,
    *targets,
) -> None:
    """Applies a Hadamard gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.h_yz"></a>
```python
# stim.TableauSimulator.h_yz

# (in class stim.TableauSimulator)
def h_yz(
    self,
    *targets,
) -> None:
    """Applies an operation that swaps the Y and Z axes to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.iswap"></a>
```python
# stim.TableauSimulator.iswap

# (in class stim.TableauSimulator)
def iswap(
    self,
    *targets,
) -> None:
    """Applies an ISWAP gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.iswap_dag"></a>
```python
# stim.TableauSimulator.iswap_dag

# (in class stim.TableauSimulator)
def iswap_dag(
    self,
    *targets,
) -> None:
    """Applies an ISWAP_DAG gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.measure"></a>
```python
# stim.TableauSimulator.measure

# (in class stim.TableauSimulator)
def measure(
    self,
    target: int,
) -> bool:
    """Measures a single qubit.

    Unlike the other methods on TableauSimulator, this one does not broadcast
    over multiple targets. This is to avoid returning a list, which would
    create a pitfall where typing `if sim.measure(qubit)` would be a bug.

    To measure multiple qubits, use `TableauSimulator.measure_many`.

    Args:
        target: The index of the qubit to measure.

    Returns:
        The measurement result as a bool.
    """
```

<a name="stim.TableauSimulator.measure_kickback"></a>
```python
# stim.TableauSimulator.measure_kickback

# (in class stim.TableauSimulator)
def measure_kickback(
    self,
    target: int,
) -> tuple:
    """Measures a qubit and returns the result as well as its Pauli kickback (if any).

    The "Pauli kickback" of a stabilizer circuit measurement is a set of Pauli
    operations that flip the post-measurement system state between the two possible
    post-measurement states. For example, consider measuring one of the qubits in
    the state |00>+|11> in the Z basis. If the measurement result is False, then the
    system projects into the state |00>. If the measurement result is True, then the
    system projects into the state |11>. Applying a Pauli X operation to both qubits
    flips between |00> and |11>. Therefore the Pauli kickback of the measurement is
    `stim.PauliString("XX")`. Note that there are often many possible equivalent
    Pauli kickbacks. For example, if in the previous example there was a third qubit
    in the |0> state, then both `stim.PauliString("XX_")` and
    `stim.PauliString("XXZ")` are valid kickbacks.

    Measurements with deterministic results don't have a Pauli kickback.

    Args:
        target: The index of the qubit to measure.

    Returns:
        A (result, kickback) tuple.
        The result is a bool containing the measurement's output.
        The kickback is either None (meaning the measurement was deterministic) or a
        stim.PauliString (meaning the measurement was random, and the operations in
        the Pauli string flip between the two possible post-measurement states).

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()

        >>> s.measure_kickback(0)
        (False, None)

        >>> s.h(0)
        >>> s.measure_kickback(0)[1]
        stim.PauliString("+X")

        >>> def pseudo_post_select(qubit, desired_result):
        ...     m, kick = s.measure_kickback(qubit)
        ...     if m != desired_result:
        ...         if kick is None:
        ...             raise ValueError("Post-selected the impossible!")
        ...         s.do(kick)
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.cnot(0, 1)
        >>> s.cnot(0, 2)
        >>> pseudo_post_select(qubit=2, desired_result=True)
        >>> s.measure_many(0, 1, 2)
        [True, True, True]
    """
```

<a name="stim.TableauSimulator.measure_many"></a>
```python
# stim.TableauSimulator.measure_many

# (in class stim.TableauSimulator)
def measure_many(
    self,
    *targets,
) -> List[bool]:
    """Measures multiple qubits.

    Args:
        *targets: The indices of the qubits to measure.

    Returns:
        The measurement results as a list of bools.
    """
```

<a name="stim.TableauSimulator.measure_observable"></a>
```python
# stim.TableauSimulator.measure_observable

# (in class stim.TableauSimulator)
def measure_observable(
    self,
    observable: stim.PauliString,
    *,
    flip_probability: float = 0.0,
) -> bool:
    """Measures an pauli string observable, as if by an MPP instruction.

    Args:
        observable: The observable to measure, specified as a stim.PauliString.
        flip_probability: Probability of the recorded measurement result being
            flipped.

    Returns:
        The result of the measurement.

        The result is also recorded into the measurement record.

    Raises:
        ValueError: The given pauli string isn't Hermitian, or the given probability
            isn't a valid probability.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.cnot(0, 1)

        >>> s.measure_observable(stim.PauliString("XX"))
        False

        >>> s.measure_observable(stim.PauliString("YY"))
        True

        >>> s.measure_observable(stim.PauliString("-ZZ"))
        True
    """
```

<a name="stim.TableauSimulator.num_qubits"></a>
```python
# stim.TableauSimulator.num_qubits

# (in class stim.TableauSimulator)
@property
def num_qubits(
    self,
) -> int:
    """Returns the number of qubits currently being tracked by the simulator.

    Note that the number of qubits being tracked will implicitly increase if qubits
    beyond the current limit are touched. Untracked qubits are always assumed to be
    in the |0> state.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.num_qubits
        0
        >>> s.h(2)
        >>> s.num_qubits
        3
    """
```

<a name="stim.TableauSimulator.peek_bloch"></a>
```python
# stim.TableauSimulator.peek_bloch

# (in class stim.TableauSimulator)
def peek_bloch(
    self,
    target: int,
) -> stim.PauliString:
    """Returns the state of the qubit as a single-qubit stim.PauliString stabilizer.

    This is a non-physical operation. It reports information about the qubit without
    disturbing it.

    Args:
        target: The qubit to peek at.

    Returns:
        stim.PauliString("I"):
            The qubit is entangled. Its bloch vector is x=y=z=0.
        stim.PauliString("+Z"):
            The qubit is in the |0> state. Its bloch vector is z=+1, x=y=0.
        stim.PauliString("-Z"):
            The qubit is in the |1> state. Its bloch vector is z=-1, x=y=0.
        stim.PauliString("+Y"):
            The qubit is in the |i> state. Its bloch vector is y=+1, x=z=0.
        stim.PauliString("-Y"):
            The qubit is in the |-i> state. Its bloch vector is y=-1, x=z=0.
        stim.PauliString("+X"):
            The qubit is in the |+> state. Its bloch vector is x=+1, y=z=0.
        stim.PauliString("-X"):
            The qubit is in the |-> state. Its bloch vector is x=-1, y=z=0.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.peek_bloch(0)
        stim.PauliString("+Z")
        >>> s.x(0)
        >>> s.peek_bloch(0)
        stim.PauliString("-Z")
        >>> s.h(0)
        >>> s.peek_bloch(0)
        stim.PauliString("-X")
        >>> s.sqrt_x(1)
        >>> s.peek_bloch(1)
        stim.PauliString("-Y")
        >>> s.cz(0, 1)
        >>> s.peek_bloch(0)
        stim.PauliString("+_")
    """
```

<a name="stim.TableauSimulator.peek_observable_expectation"></a>
```python
# stim.TableauSimulator.peek_observable_expectation

# (in class stim.TableauSimulator)
def peek_observable_expectation(
    self,
    observable: stim.PauliString,
) -> int:
    """Determines the expected value of an observable.

    Because the simulator's state is always a stabilizer state, the expectation will
    always be exactly -1, 0, or +1.

    This is a non-physical operation.
    It reports information about the quantum state without disturbing it.

    Args:
        observable: The observable to determine the expected value of.
            This observable must have a real sign, not an imaginary sign.

    Returns:
        +1: Observable will be deterministically false when measured.
        -1: Observable will be deterministically true when measured.
        0: Observable will be random when measured.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.peek_observable_expectation(stim.PauliString("+Z"))
        1
        >>> s.peek_observable_expectation(stim.PauliString("+X"))
        0
        >>> s.peek_observable_expectation(stim.PauliString("-Z"))
        -1

        >>> s.do(stim.Circuit('''
        ...     H 0
        ...     CNOT 0 1
        ... '''))
        >>> queries = ['XX', 'YY', 'ZZ', '-ZZ', 'ZI', 'II', 'IIZ']
        >>> for q in queries:
        ...     print(q, s.peek_observable_expectation(stim.PauliString(q)))
        XX 1
        YY -1
        ZZ 1
        -ZZ -1
        ZI 0
        II 1
        IIZ 1
    """
```

<a name="stim.TableauSimulator.peek_x"></a>
```python
# stim.TableauSimulator.peek_x

# (in class stim.TableauSimulator)
def peek_x(
    self,
    target: int,
) -> int:
    """Returns the expected value of a qubit's X observable.

    Because the simulator's state is always a stabilizer state, the expectation will
    always be exactly -1, 0, or +1.

    This is a non-physical operation.
    It reports information about the quantum state without disturbing it.

    Args:
        target: The qubit to analyze.

    Returns:
        +1: Qubit is in the |+> state.
        -1: Qubit is in the |-> state.
        0: Qubit is in some other state.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.reset_z(0)
        >>> s.peek_x(0)
        0
        >>> s.reset_x(0)
        >>> s.peek_x(0)
        1
        >>> s.z(0)
        >>> s.peek_x(0)
        -1
    """
```

<a name="stim.TableauSimulator.peek_y"></a>
```python
# stim.TableauSimulator.peek_y

# (in class stim.TableauSimulator)
def peek_y(
    self,
    target: int,
) -> int:
    """Returns the expected value of a qubit's Y observable.

    Because the simulator's state is always a stabilizer state, the expectation will
    always be exactly -1, 0, or +1.

    This is a non-physical operation.
    It reports information about the quantum state without disturbing it.

    Args:
        target: The qubit to analyze.

    Returns:
        +1: Qubit is in the |i> state.
        -1: Qubit is in the |-i> state.
        0: Qubit is in some other state.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.reset_z(0)
        >>> s.peek_y(0)
        0
        >>> s.reset_y(0)
        >>> s.peek_y(0)
        1
        >>> s.z(0)
        >>> s.peek_y(0)
        -1
    """
```

<a name="stim.TableauSimulator.peek_z"></a>
```python
# stim.TableauSimulator.peek_z

# (in class stim.TableauSimulator)
def peek_z(
    self,
    target: int,
) -> int:
    """Returns the expected value of a qubit's Z observable.

    Because the simulator's state is always a stabilizer state, the expectation will
    always be exactly -1, 0, or +1.

    This is a non-physical operation.
    It reports information about the quantum state without disturbing it.

    Args:
        target: The qubit to analyze.

    Returns:
        +1: Qubit is in the |0> state.
        -1: Qubit is in the |1> state.
        0: Qubit is in some other state.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.reset_x(0)
        >>> s.peek_z(0)
        0
        >>> s.reset_z(0)
        >>> s.peek_z(0)
        1
        >>> s.x(0)
        >>> s.peek_z(0)
        -1
    """
```

<a name="stim.TableauSimulator.postselect_observable"></a>
```python
# stim.TableauSimulator.postselect_observable

# (in class stim.TableauSimulator)
def postselect_observable(
    self,
    observable: stim.PauliString,
    *,
    desired_value: bool = False,
) -> None:
    """Projects into a desired observable, or raises an exception if it was impossible.

    Postselecting an observable forces it to collapse to a specific eigenstate,
    as if it was measured and that state was the result of the measurement.

    Args:
        observable: The observable to postselect, specified as a pauli string.
            The pauli string's sign must be -1 or +1 (not -i or +i).
        desired_value:
            False (default): Postselect into the +1 eigenstate of the observable.
            True: Postselect into the -1 eigenstate of the observable.

    Raises:
        ValueError:
            The given observable had an imaginary sign.
            OR
            The postselection was impossible. The observable was in the opposite
            eigenstate, so measuring it would never ever return the desired result.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.postselect_observable(stim.PauliString("+XX"))
        >>> s.postselect_observable(stim.PauliString("+ZZ"))
        >>> s.peek_observable_expectation(stim.PauliString("+YY"))
        -1
    """
```

<a name="stim.TableauSimulator.postselect_x"></a>
```python
# stim.TableauSimulator.postselect_x

# (in class stim.TableauSimulator)
def postselect_x(
    self,
    targets: Union[int, Iterable[int]],
    *,
    desired_value: bool,
) -> None:
    """Postselects qubits in the X basis, or raises an exception.

    Postselecting a qubit forces it to collapse to a specific state, as
    if it was measured and that state was the result of the measurement.

    Args:
        targets: The qubit index or indices to postselect.
        desired_value:
            False: postselect targets into the |+> state.
            True: postselect targets into the |-> state.

    Raises:
        ValueError:
            The postselection failed. One of the qubits was in a state
            orthogonal to the desired state, so it was literally
            impossible for a measurement of the qubit to return the
            desired result.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.peek_x(0)
        0
        >>> s.postselect_x(0, desired_value=False)
        >>> s.peek_x(0)
        1
        >>> s.h(0)
        >>> s.peek_x(0)
        0
        >>> s.postselect_x(0, desired_value=True)
        >>> s.peek_x(0)
        -1
    """
```

<a name="stim.TableauSimulator.postselect_y"></a>
```python
# stim.TableauSimulator.postselect_y

# (in class stim.TableauSimulator)
def postselect_y(
    self,
    targets: Union[int, Iterable[int]],
    *,
    desired_value: bool,
) -> None:
    """Postselects qubits in the Y basis, or raises an exception.

    Postselecting a qubit forces it to collapse to a specific state, as
    if it was measured and that state was the result of the measurement.

    Args:
        targets: The qubit index or indices to postselect.
        desired_value:
            False: postselect targets into the |i> state.
            True: postselect targets into the |-i> state.

    Raises:
        ValueError:
            The postselection failed. One of the qubits was in a state
            orthogonal to the desired state, so it was literally
            impossible for a measurement of the qubit to return the
            desired result.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.peek_y(0)
        0
        >>> s.postselect_y(0, desired_value=False)
        >>> s.peek_y(0)
        1
        >>> s.reset_x(0)
        >>> s.peek_y(0)
        0
        >>> s.postselect_y(0, desired_value=True)
        >>> s.peek_y(0)
        -1
    """
```

<a name="stim.TableauSimulator.postselect_z"></a>
```python
# stim.TableauSimulator.postselect_z

# (in class stim.TableauSimulator)
def postselect_z(
    self,
    targets: Union[int, Iterable[int]],
    *,
    desired_value: bool,
) -> None:
    """Postselects qubits in the Z basis, or raises an exception.

    Postselecting a qubit forces it to collapse to a specific state, as if it was
    measured and that state was the result of the measurement.

    Args:
        targets: The qubit index or indices to postselect.
        desired_value:
            False: postselect targets into the |0> state.
            True: postselect targets into the |1> state.

    Raises:
        ValueError:
            The postselection failed. One of the qubits was in a state
            orthogonal to the desired state, so it was literally
            impossible for a measurement of the qubit to return the
            desired result.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.peek_z(0)
        0
        >>> s.postselect_z(0, desired_value=False)
        >>> s.peek_z(0)
        1
        >>> s.h(0)
        >>> s.peek_z(0)
        0
        >>> s.postselect_z(0, desired_value=True)
        >>> s.peek_z(0)
        -1
    """
```

<a name="stim.TableauSimulator.reset"></a>
```python
# stim.TableauSimulator.reset

# (in class stim.TableauSimulator)
def reset(
    self,
    *targets,
) -> None:
    """Resets qubits to the |0> state.

    Args:
        *targets: The indices of the qubits to reset.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.x(0)
        >>> s.reset(0)
        >>> s.peek_bloch(0)
        stim.PauliString("+Z")
    """
```

<a name="stim.TableauSimulator.reset_x"></a>
```python
# stim.TableauSimulator.reset_x

# (in class stim.TableauSimulator)
def reset_x(
    self,
    *targets,
) -> None:
    """Resets qubits to the |+> state.

    Args:
        *targets: The indices of the qubits to reset.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.reset_x(0)
        >>> s.peek_bloch(0)
        stim.PauliString("+X")
    """
```

<a name="stim.TableauSimulator.reset_y"></a>
```python
# stim.TableauSimulator.reset_y

# (in class stim.TableauSimulator)
def reset_y(
    self,
    *targets,
) -> None:
    """Resets qubits to the |i> state.

    Args:
        *targets: The indices of the qubits to reset.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.reset_y(0)
        >>> s.peek_bloch(0)
        stim.PauliString("+Y")
    """
```

<a name="stim.TableauSimulator.reset_z"></a>
```python
# stim.TableauSimulator.reset_z

# (in class stim.TableauSimulator)
def reset_z(
    self,
    *targets,
) -> None:
    """Resets qubits to the |0> state.

    Args:
        *targets: The indices of the qubits to reset.

    Example:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> s.h(0)
        >>> s.reset_z(0)
        >>> s.peek_bloch(0)
        stim.PauliString("+Z")
    """
```

<a name="stim.TableauSimulator.s"></a>
```python
# stim.TableauSimulator.s

# (in class stim.TableauSimulator)
def s(
    self,
    *targets,
) -> None:
    """Applies a SQRT_Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.s_dag"></a>
```python
# stim.TableauSimulator.s_dag

# (in class stim.TableauSimulator)
def s_dag(
    self,
    *targets,
) -> None:
    """Applies a SQRT_Z_DAG gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.set_inverse_tableau"></a>
```python
# stim.TableauSimulator.set_inverse_tableau

# (in class stim.TableauSimulator)
def set_inverse_tableau(
    self,
    new_inverse_tableau: stim.Tableau,
) -> None:
    """Overwrites the simulator's internal state with the given inverse tableau.

    The inverse tableau specifies how Pauli product observables of qubits at the
    current time transform into equivalent Pauli product observables at the
    beginning of time, when all qubits were in the |0> state. For example, if the Z
    observable on qubit 5 maps to a product of Z observables at the start of time
    then a Z basis measurement on qubit 5 will be deterministic and equal to the
    sign of the product. Whereas if it mapped to a product of observables including
    an X or a Y then the Z basis measurement would be random.

    Any qubits not within the length of the tableau are implicitly in the |0> state.

    Args:
        new_inverse_tableau: The tableau to overwrite the internal state with.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> t = stim.Tableau.random(4)
        >>> s.set_inverse_tableau(t)
        >>> s.current_inverse_tableau() == t
        True
    """
```

<a name="stim.TableauSimulator.set_num_qubits"></a>
```python
# stim.TableauSimulator.set_num_qubits

# (in class stim.TableauSimulator)
def set_num_qubits(
    self,
    new_num_qubits: int,
) -> None:
    """Resizes the simulator's internal state.

    This forces the simulator's internal state to track exactly the qubits whose
    indices are in `range(new_num_qubits)`.

    Note that untracked qubits are always assumed to be in the |0> state. Therefore,
    calling this method will effectively force any qubit whose index is outside
    `range(new_num_qubits)` to be reset to |0>.

    Note that this method does not prevent future operations from implicitly
    expanding the size of the tracked state (e.g. setting the number of qubits to 5
    will not prevent a Hadamard from then being applied to qubit 100, increasing the
    number of qubits back to 101).

    Args:
        new_num_qubits: The length of the range of qubits the internal simulator
            should be tracking.

    Examples:
        >>> import stim
        >>> s = stim.TableauSimulator()
        >>> len(s.current_inverse_tableau())
        0

        >>> s.set_num_qubits(5)
        >>> len(s.current_inverse_tableau())
        5

        >>> s.x(0, 1, 2, 3)
        >>> s.set_num_qubits(2)
        >>> s.measure_many(0, 1, 2, 3)
        [True, True, False, False]
    """
```

<a name="stim.TableauSimulator.set_state_from_stabilizers"></a>
```python
# stim.TableauSimulator.set_state_from_stabilizers

# (in class stim.TableauSimulator)
def set_state_from_stabilizers(
    self,
    stabilizers: Iterable[stim.PauliString],
    *,
    allow_redundant: bool = False,
    allow_underconstrained: bool = False,
) -> None:
    """Sets the tableau simulator's state to a state satisfying the given stabilizers.

    The old quantum state is completely overwritten, even if the new state is
    underconstrained by the given stabilizers. The number of qubits is changed to
    exactly match the number of qubits in the longest given stabilizer.

    Args:
        stabilizers: A list of `stim.PauliString`s specifying the stabilizers that
            the new state must have. It is permitted for stabilizers to have
            different lengths. All stabilizers are padded up to the length of the
            longest stabilizer by appending identity terms.
        allow_redundant: Defaults to False. If set to False, then the given
            stabilizers must all be independent. If any one of them is a product of
            the others (including the empty product), an exception will be raised.
            If set to True, then redundant stabilizers are simply ignored.
        allow_underconstrained: Defaults to False. If set to False, then the given
            stabilizers must form a complete set of generators. They must exactly
            specify the desired stabilizer state, with no degrees of freedom left
            over. For an n-qubit state there must be n independent stabilizers. If
            set to True, then there can be leftover degrees of freedom which can be
            set arbitrarily.

    Returns:
        Nothing. Mutates the states of the simulator to match the desired
        stabilizers.

        Guarantees that self.current_inverse_tableau().inverse_z_output(k) will be
        equal to the k'th independent stabilizer from the `stabilizers` argument.

    Raises:
        ValueError:
            A stabilizer is redundant but allow_redundant=True wasn't set.
            OR
            The given stabilizers are contradictory (e.g. "+Z" and "-Z" both
            specified).
            OR
            The given stabilizers anticommute (e.g. "+Z" and "+X" both specified).
            OR
            The stabilizers left behind a degree of freedom but
            allow_underconstrained=True wasn't set.
            OR
            A stabilizer has an imaginary sign (i or -i).

    Examples:

        >>> import stim
        >>> tab_sim = stim.TableauSimulator()
        >>> tab_sim.set_state_from_stabilizers([
        ...     stim.PauliString("XX"),
        ...     stim.PauliString("ZZ"),
        ... ])
        >>> tab_sim.current_inverse_tableau().inverse()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+XX"),
                stim.PauliString("+ZZ"),
            ],
        )

        >>> tab_sim.set_state_from_stabilizers([
        ...     stim.PauliString("XX_"),
        ...     stim.PauliString("ZZ_"),
        ...     stim.PauliString("-YY_"),
        ...     stim.PauliString(""),
        ... ], allow_underconstrained=True, allow_redundant=True)
        >>> tab_sim.current_inverse_tableau().inverse()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z__"),
                stim.PauliString("+_X_"),
                stim.PauliString("+__X"),
            ],
            zs=[
                stim.PauliString("+XX_"),
                stim.PauliString("+ZZ_"),
                stim.PauliString("+__Z"),
            ],
        )
    """
```

<a name="stim.TableauSimulator.set_state_from_state_vector"></a>
```python
# stim.TableauSimulator.set_state_from_state_vector

# (in class stim.TableauSimulator)
def set_state_from_state_vector(
    self,
    state_vector: Iterable[float],
    *,
    endian: str,
) -> None:
    """Sets the simulator's state to a superposition specified by an amplitude vector.

    Args:
        state_vector: A list of complex amplitudes specifying a superposition. The
            vector must correspond to a state that is reachable using Clifford
            operations, and must be normalized (i.e. it must be a unit vector).
        endian:
            "little": state vector is in little endian order, where higher index
                qubits correspond to larger changes in the state index.
            "big": state vector is in big endian order, where higher index qubits
                correspond to smaller changes in the state index.

    Returns:
        Nothing. Mutates the states of the simulator to match the desired state.

    Raises:
        ValueError:
            The given state vector isn't a list of complex values specifying a
            stabilizer state.
            OR
            The given endian value isn't 'little' or 'big'.

    Examples:

        >>> import stim
        >>> tab_sim = stim.TableauSimulator()
        >>> tab_sim.set_state_from_state_vector([
        ...     0.5**0.5,
        ...     0.5**0.5 * 1j,
        ... ], endian='little')
        >>> tab_sim.current_inverse_tableau().inverse()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z"),
            ],
            zs=[
                stim.PauliString("+Y"),
            ],
        )
        >>> tab_sim.set_state_from_state_vector([
        ...     0.5**0.5,
        ...     0,
        ...     0,
        ...     0.5**0.5,
        ... ], endian='little')
        >>> tab_sim.current_inverse_tableau().inverse()
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("+Z_"),
                stim.PauliString("+_X"),
            ],
            zs=[
                stim.PauliString("+XX"),
                stim.PauliString("+ZZ"),
            ],
        )
    """
```

<a name="stim.TableauSimulator.sqrt_x"></a>
```python
# stim.TableauSimulator.sqrt_x

# (in class stim.TableauSimulator)
def sqrt_x(
    self,
    *targets,
) -> None:
    """Applies a SQRT_X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.sqrt_x_dag"></a>
```python
# stim.TableauSimulator.sqrt_x_dag

# (in class stim.TableauSimulator)
def sqrt_x_dag(
    self,
    *targets,
) -> None:
    """Applies a SQRT_X_DAG gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.sqrt_y"></a>
```python
# stim.TableauSimulator.sqrt_y

# (in class stim.TableauSimulator)
def sqrt_y(
    self,
    *targets,
) -> None:
    """Applies a SQRT_Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.sqrt_y_dag"></a>
```python
# stim.TableauSimulator.sqrt_y_dag

# (in class stim.TableauSimulator)
def sqrt_y_dag(
    self,
    *targets,
) -> None:
    """Applies a SQRT_Y_DAG gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.state_vector"></a>
```python
# stim.TableauSimulator.state_vector

# (in class stim.TableauSimulator)
def state_vector(
    self,
    *,
    endian: str = 'little',
) -> np.ndarray[np.complex64]:
    """Returns a wavefunction for the simulator's current state.

    This function takes O(n * 2**n) time and O(2**n) space, where n is the number of
    qubits. The computation is done by initialization a random state vector and
    iteratively projecting it into the +1 eigenspace of each stabilizer of the
    state. The state is then canonicalized so that zero values are actually exactly
    0, and so that the first non-zero entry is positive.

    Args:
        endian:
            "little" (default): state vector is in little endian order, where higher
                index qubits correspond to larger changes in the state index.
            "big": state vector is in big endian order, where higher index qubits
                correspond to smaller changes in the state index.

    Returns:
        A `numpy.ndarray[numpy.complex64]` of computational basis amplitudes.

        If the result is in little endian order then the amplitude at offset
        b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
        computational basis state where the qubit with index 0 is storing the bit
        b_0, the qubit with index 1 is storing the bit b_1, etc.

        If the result is in big endian order then the amplitude at offset
        b_0 + b_1*2 + b_2*4 + ... + b_{n-1}*2^{n-1} is the amplitude for the
        computational basis state where the qubit with index 0 is storing the bit
        b_{n-1}, the qubit with index 1 is storing the bit b_{n-2}, etc.

    Examples:
        >>> import stim
        >>> import numpy as np
        >>> s = stim.TableauSimulator()
        >>> s.x(2)
        >>> list(s.state_vector(endian='little'))
        [0j, 0j, 0j, 0j, (1+0j), 0j, 0j, 0j]

        >>> list(s.state_vector(endian='big'))
        [0j, (1+0j), 0j, 0j, 0j, 0j, 0j, 0j]

        >>> s.sqrt_x(1, 2)
        >>> list(s.state_vector())
        [(0.5+0j), 0j, -0.5j, 0j, 0.5j, 0j, (0.5+0j), 0j]
    """
```

<a name="stim.TableauSimulator.swap"></a>
```python
# stim.TableauSimulator.swap

# (in class stim.TableauSimulator)
def swap(
    self,
    *targets,
) -> None:
    """Applies a swap gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.x"></a>
```python
# stim.TableauSimulator.x

# (in class stim.TableauSimulator)
def x(
    self,
    *targets,
) -> None:
    """Applies a Pauli X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.x_error"></a>
```python
# stim.TableauSimulator.x_error

# (in class stim.TableauSimulator)
def x_error(
    self,
    *targets: int,
    p: float,
):
    """Probabilistically applies X errors to targets.

    Args:
        *targets: The indices of the qubits to target with the noise.
        p: The chance of the X error being applied,
            independently, to each qubit.
    """
```

<a name="stim.TableauSimulator.xcx"></a>
```python
# stim.TableauSimulator.xcx

# (in class stim.TableauSimulator)
def xcx(
    self,
    *targets,
) -> None:
    """Applies an X-controlled X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.xcy"></a>
```python
# stim.TableauSimulator.xcy

# (in class stim.TableauSimulator)
def xcy(
    self,
    *targets,
) -> None:
    """Applies an X-controlled Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.xcz"></a>
```python
# stim.TableauSimulator.xcz

# (in class stim.TableauSimulator)
def xcz(
    self,
    *targets,
) -> None:
    """Applies an X-controlled Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.y"></a>
```python
# stim.TableauSimulator.y

# (in class stim.TableauSimulator)
def y(
    self,
    *targets,
) -> None:
    """Applies a Pauli Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.y_error"></a>
```python
# stim.TableauSimulator.y_error

# (in class stim.TableauSimulator)
def y_error(
    self,
    *targets: int,
    p: float,
):
    """Probabilistically applies Y errors to targets.

    Args:
        *targets: The indices of the qubits to target with the noise.
        p: The chance of the Y error being applied,
            independently, to each qubit.
    """
```

<a name="stim.TableauSimulator.ycx"></a>
```python
# stim.TableauSimulator.ycx

# (in class stim.TableauSimulator)
def ycx(
    self,
    *targets,
) -> None:
    """Applies a Y-controlled X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.ycy"></a>
```python
# stim.TableauSimulator.ycy

# (in class stim.TableauSimulator)
def ycy(
    self,
    *targets,
) -> None:
    """Applies a Y-controlled Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.ycz"></a>
```python
# stim.TableauSimulator.ycz

# (in class stim.TableauSimulator)
def ycz(
    self,
    *targets,
) -> None:
    """Applies a Y-controlled Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.z"></a>
```python
# stim.TableauSimulator.z

# (in class stim.TableauSimulator)
def z(
    self,
    *targets,
) -> None:
    """Applies a Pauli Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
    """
```

<a name="stim.TableauSimulator.z_error"></a>
```python
# stim.TableauSimulator.z_error

# (in class stim.TableauSimulator)
def y_error(
    self,
    *targets: int,
    p: float,
):
    """Probabilistically applies Z errors to targets.

    Args:
        *targets: The indices of the qubits to target with the noise.
        p: The chance of the Z error being applied,
            independently, to each qubit.
    """
```

<a name="stim.TableauSimulator.zcx"></a>
```python
# stim.TableauSimulator.zcx

# (in class stim.TableauSimulator)
def zcx(
    self,
    *targets,
) -> None:
    """Applies a controlled X gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.zcy"></a>
```python
# stim.TableauSimulator.zcy

# (in class stim.TableauSimulator)
def zcy(
    self,
    *targets,
) -> None:
    """Applies a controlled Y gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.TableauSimulator.zcz"></a>
```python
# stim.TableauSimulator.zcz

# (in class stim.TableauSimulator)
def zcz(
    self,
    *targets,
) -> None:
    """Applies a controlled Z gate to the simulator's state.

    Args:
        *targets: The indices of the qubits to target with the gate.
            Applies the gate to the first two targets, then the next two targets,
            and so forth. There must be an even number of targets.
    """
```

<a name="stim.gate_data"></a>
```python
# stim.gate_data

# (at top-level in the stim module)
@overload
def gate_data(
    name: str,
) -> stim.GateData:
    pass
@overload
def gate_data(
) -> Dict[str, stim.GateData]:
    pass
def gate_data(
    name: Optional[str] = None,
) -> Union[str, Dict[str, stim.GateData]]:
    """Returns gate data for the given named gate, or all gates.

    Examples:
        >>> import stim
        >>> stim.gate_data('cnot').aliases
        ['CNOT', 'CX', 'ZCX']
        >>> stim.gate_data('cnot').is_two_qubit_gate
        True
        >>> gate_dict = stim.gate_data()
        >>> len(gate_dict) > 50
        True
        >>> gate_dict['MX'].produces_measurements
        True
    """
```

<a name="stim.main"></a>
```python
# stim.main

# (at top-level in the stim module)
def main(
    *,
    command_line_args: List[str],
) -> int:
    """Runs the command line tool version of stim on the given arguments.

    Note that by default any input will be read from stdin, any output
    will print to stdout (as opposed to being intercepted). For most
    commands, you can use arguments like `--out` to write to a file
    instead of stdout and `--in` to read from a file instead of stdin.

    Returns:
        An exit code (0 means success, not zero means failure).

    Raises:
        A large variety of errors, depending on what you are doing and
        how it failed! Beware that many errors are caught by the main
        method itself and printed to stderr, with the only indication
        that something went wrong being the return code.

    Example:
        >>> import stim
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = f'{d}/tmp.out'
        ...     return_code = stim.main(command_line_args=[
        ...         "gen",
        ...         "--code=repetition_code",
        ...         "--task=memory",
        ...         "--rounds=1000",
        ...         "--distance=2",
        ...         "--out",
        ...         path,
        ...     ])
        ...     assert return_code == 0
        ...     with open(path) as f:
        ...         print(f.read(), end='')
        # Generated repetition_code circuit.
        # task: memory
        # rounds: 1000
        # distance: 2
        # before_round_data_depolarization: 0
        # before_measure_flip_probability: 0
        # after_reset_flip_probability: 0
        # after_clifford_depolarization: 0
        # layout:
        # L0 Z1 d2
        # Legend:
        #     d# = data qubit
        #     L# = data qubit with logical observable crossing
        #     Z# = measurement qubit
        R 0 1 2
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        MR 1
        DETECTOR(1, 0) rec[-1]
        REPEAT 999 {
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            MR 1
            SHIFT_COORDS(0, 1)
            DETECTOR(1, 0) rec[-1] rec[-2]
        }
        M 0 2
        DETECTOR(1, 1) rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    """
```

<a name="stim.read_shot_data_file"></a>
```python
# stim.read_shot_data_file

# (at top-level in the stim module)
@overload
def read_shot_data_file(
    *,
    path: Union[str, pathlib.Path],
    format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'],
    bit_packed: bool = False,
    num_measurements: int = 0,
    num_detectors: int = 0,
    num_observables: int = 0,
) -> np.ndarray:
    pass
@overload
def read_shot_data_file(
    *,
    path: Union[str, pathlib.Path],
    format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'],
    bit_packed: bool = False,
    num_measurements: int = 0,
    num_detectors: int = 0,
    num_observables: int = 0,
    separate_observables: 'Literal[True]',
) -> Tuple[np.ndarray, np.ndarray]:
    pass
def read_shot_data_file(
    *,
    path: Union[str, pathlib.Path],
    format: Union[str, 'Literal["01", "b8", "r8", "ptb64", "hits", "dets"]'],
    bit_packed: bool = False,
    num_measurements: int = 0,
    num_detectors: int = 0,
    num_observables: int = 0,
    separate_observables: bool = False,
) -> Union[Tuple[np.ndarray, np.ndarray], np.ndarray]:
    """Reads shot data, such as measurement samples, from a file.

    Args:
        path: The path to the file to read the data from.
        format: The format that the data is stored in, such as 'b8'.
            See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        bit_packed: Defaults to false. Determines whether the result is a bool_
            numpy array with one bit per byte, or a uint8 numpy array with 8 bits
            per byte.
        num_measurements: How many measurements there are per shot.
        num_detectors: How many detectors there are per shot.
        num_observables: How many observables there are per shot.
            Note that this only refers to observables *stored in the file*, not to
            observables from the original circuit that was sampled.
        separate_observables: When set to True, the result is a tuple of two arrays,
            one containing the detection event data and the other containing the
            observable data, instead of a single array.

    Returns:
        If separate_observables=True:
            A tuple (dets, obs) of numpy arrays containing the loaded data.

            If bit_packed=False:
                dets.dtype = np.bool_
                dets.shape = (num_shots, num_measurements + num_detectors)
                det bit b from shot s is at dets[s, b]
                obs.dtype = np.bool_
                obs.shape = (num_shots, num_observables)
                obs bit b from shot s is at dets[s, b]
            If bit_packed=True:
                dets.dtype = np.uint8
                dets.shape = (num_shots, math.ceil(
                    (num_measurements + num_detectors) / 8))
                obs.dtype = np.uint8
                obs.shape = (num_shots, math.ceil(num_observables / 8))
                det bit b from shot s is at dets[s, b // 8] & (1 << (b % 8))
                obs bit b from shot s is at obs[s, b // 8] & (1 << (b % 8))

        If separate_observables=False:
            A numpy array containing the loaded data.

            If bit_packed=False:
                dtype = np.bool_
                shape = (num_shots,
                         num_measurements + num_detectors + num_observables)
                bit b from shot s is at result[s, b]
            If bit_packed=True:
                dtype = np.uint8
                shape = (num_shots, math.ceil(
                    (num_measurements + num_detectors + num_observables) / 8))
                bit b from shot s is at result[s, b // 8] & (1 << (b % 8))

    Examples:
        >>> import stim
        >>> import pathlib
        >>> import tempfile
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = pathlib.Path(d) / 'shots'
        ...     with open(path, 'w') as f:
        ...         print("0000", file=f)
        ...         print("0101", file=f)
        ...
        ...     read = stim.read_shot_data_file(
        ...         path=str(path),
        ...         format='01',
        ...         num_measurements=4)
        >>> read
        array([[False, False, False, False],
               [False,  True, False,  True]])
    """
```

<a name="stim.target_combiner"></a>
```python
# stim.target_combiner

# (at top-level in the stim module)
def target_combiner(
) -> stim.GateTarget:
    """Returns a target combiner that can be used to build Pauli products.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("MPP", [
        ...     stim.target_x(2),
        ...     stim.target_combiner(),
        ...     stim.target_y(3),
        ...     stim.target_combiner(),
        ...     stim.target_z(5),
        ... ])
        >>> circuit
        stim.Circuit('''
            MPP X2*Y3*Z5
        ''')
    """
```

<a name="stim.target_inv"></a>
```python
# stim.target_inv

# (at top-level in the stim module)
def target_inv(
    qubit_index: Union[int, stim.GateTarget],
) -> stim.GateTarget:
    """Returns a target flagged as inverted.

    Inverted targets are used to indicate measurement results should be flipped.

    Args:
        qubit_index: The underlying qubit index of the inverted target.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("M", [2, stim.target_inv(3)])
        >>> circuit
        stim.Circuit('''
            M 2 !3
        ''')

    For example, the '!1' in 'M 0 !1 2' is qubit 1 flagged as inverted,
    meaning the measurement result from qubit 1 should be inverted when reported.
    """
```

<a name="stim.target_logical_observable_id"></a>
```python
# stim.target_logical_observable_id

# (at top-level in the stim module)
def target_logical_observable_id(
    index: int,
) -> stim.DemTarget:
    """Returns a logical observable id identifying a frame change.

    Args:
        index: The index of the observable.

    Returns:
        The logical observable target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_logical_observable_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) L13
        ''')
    """
```

<a name="stim.target_rec"></a>
```python
# stim.target_rec

# (at top-level in the stim module)
def target_rec(
    lookback_index: int,
) -> stim.GateTarget:
    """Returns a measurement record target with the given lookback.

    Measurement record targets are used to refer back to the measurement record;
    the list of measurements that have been performed so far. Measurement record
    targets always specify an index relative to the *end* of the measurement record.
    The latest measurement is `stim.target_rec(-1)`, the next most recent
    measurement is `stim.target_rec(-2)`, and so forth. Indexing is done this way
    in order to make it possible to write loops.

    Args:
        lookback_index: A negative integer indicating how far to look back, relative
            to the end of the measurement record.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("M", [5, 7, 11])
        >>> circuit.append("CX", [stim.target_rec(-2), 3])
        >>> circuit
        stim.Circuit('''
            M 5 7 11
            CX rec[-2] 3
        ''')
    """
```

<a name="stim.target_relative_detector_id"></a>
```python
# stim.target_relative_detector_id

# (at top-level in the stim module)
def target_relative_detector_id(
    index: int,
) -> stim.DemTarget:
    """Returns a relative detector id (e.g. "D5" in a .dem file).

    Args:
        index: The index of the detector, relative to the current detector offset.

    Returns:
        The relative detector target.

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_relative_detector_id(13)
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D13
        ''')
    """
```

<a name="stim.target_separator"></a>
```python
# stim.target_separator

# (at top-level in the stim module)
def target_separator(
) -> stim.DemTarget:
    """Returns a target separator (e.g. "^" in a .dem file).

    Examples:
        >>> import stim
        >>> m = stim.DetectorErrorModel()
        >>> m.append("error", 0.25, [
        ...     stim.target_relative_detector_id(1),
        ...     stim.target_separator(),
        ...     stim.target_relative_detector_id(2),
        ... ])
        >>> print(repr(m))
        stim.DetectorErrorModel('''
            error(0.25) D1 ^ D2
        ''')
    """
```

<a name="stim.target_sweep_bit"></a>
```python
# stim.target_sweep_bit

# (at top-level in the stim module)
def target_sweep_bit(
    sweep_bit_index: int,
) -> stim.GateTarget:
    """Returns a sweep bit target that can be passed into `stim.Circuit.append`.

    Args:
        sweep_bit_index: The index of the sweep bit to target.

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("CX", [stim.target_sweep_bit(2), 5])
        >>> circuit
        stim.Circuit('''
            CX sweep[2] 5
        ''')
    """
```

<a name="stim.target_x"></a>
```python
# stim.target_x

# (at top-level in the stim module)
def target_x(
    qubit_index: Union[int, stim.GateTarget],
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a Pauli X target that can be passed into `stim.Circuit.append`.

    Args:
        qubit_index: The qubit that the Pauli applies to.
        invert: Defaults to False. If True, the target is inverted (indicating
            that, for example, measurement results should be inverted).

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("MPP", [
        ...     stim.target_x(2),
        ...     stim.target_combiner(),
        ...     stim.target_y(3, invert=True),
        ...     stim.target_combiner(),
        ...     stim.target_z(5),
        ... ])
        >>> circuit
        stim.Circuit('''
            MPP X2*!Y3*Z5
        ''')
    """
```

<a name="stim.target_y"></a>
```python
# stim.target_y

# (at top-level in the stim module)
def target_y(
    qubit_index: Union[int, stim.GateTarget],
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a Pauli Y target that can be passed into `stim.Circuit.append`.

    Args:
        qubit_index: The qubit that the Pauli applies to.
        invert: Defaults to False. If True, the target is inverted (indicating
            that, for example, measurement results should be inverted).

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("MPP", [
        ...     stim.target_x(2),
        ...     stim.target_combiner(),
        ...     stim.target_y(3, invert=True),
        ...     stim.target_combiner(),
        ...     stim.target_z(5),
        ... ])
        >>> circuit
        stim.Circuit('''
            MPP X2*!Y3*Z5
        ''')
    """
```

<a name="stim.target_z"></a>
```python
# stim.target_z

# (at top-level in the stim module)
def target_z(
    qubit_index: Union[int, stim.GateTarget],
    invert: bool = False,
) -> stim.GateTarget:
    """Returns a Pauli Z target that can be passed into `stim.Circuit.append`.

    Args:
        qubit_index: The qubit that the Pauli applies to.
        invert: Defaults to False. If True, the target is inverted (indicating
            that, for example, measurement results should be inverted).

    Examples:
        >>> import stim
        >>> circuit = stim.Circuit()
        >>> circuit.append("MPP", [
        ...     stim.target_x(2),
        ...     stim.target_combiner(),
        ...     stim.target_y(3, invert=True),
        ...     stim.target_combiner(),
        ...     stim.target_z(5),
        ... ])
        >>> circuit
        stim.Circuit('''
            MPP X2*!Y3*Z5
        ''')
    """
```

<a name="stim.write_shot_data_file"></a>
```python
# stim.write_shot_data_file

# (at top-level in the stim module)
def write_shot_data_file(
    *,
    data: np.ndarray,
    path: Union[str, pathlib.Path],
    format: str,
    num_measurements: int = 0,
    num_detectors: int = 0,
    num_observables: int = 0,
) -> None:
    """Writes shot data, such as measurement samples, to a file.

    Args:
        data: The data to write to the file. This must be a numpy array. The dtype
            of the array determines whether or not the data is bit packed, and the
            shape must match the bits per shot.

            dtype=np.bool_: Not bit packed. Shape must be
                (num_shots, num_measurements + num_detectors + num_observables).
            dtype=np.uint8: Yes bit packed. Shape must be
                (num_shots, math.ceil(
                    (num_measurements + num_detectors + num_observables) / 8)).
        path: The path to the file to write the data to.
        format: The format that the data is stored in, such as 'b8'.
            See https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
        num_measurements: How many measurements there are per shot.
        num_detectors: How many detectors there are per shot.
        num_observables: How many observables there are per shot.
            Note that this only refers to observables *in the given shot data*, not
            to observables from the original circuit that was sampled.

    Examples:
        >>> import stim
        >>> import pathlib
        >>> import tempfile
        >>> import numpy as np
        >>> with tempfile.TemporaryDirectory() as d:
        ...     path = pathlib.Path(d) / 'shots'
        ...     shot_data = np.array([
        ...         [0, 1, 0],
        ...         [0, 1, 1],
        ...     ], dtype=np.bool_)
        ...
        ...     stim.write_shot_data_file(
        ...         path=str(path),
        ...         data=shot_data,
        ...         format='01',
        ...         num_measurements=3)
        ...
        ...     with open(path) as f:
        ...         written = f.read()
        >>> written
        '010\n011\n'
    """
```
