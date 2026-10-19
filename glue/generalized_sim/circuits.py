import stim
import numpy as np
from panqec.codes import StabilizerCode


def generate_circuit_measure_all(
    code: StabilizerCode, 
    n_rounds=1, 
    p_phys=0.1, 
    p_meas=0.1,
    noise_model="pheno",
    replace_T_with_S=False,
    y_state_init=True
) -> stim.Circuit:
    """
    Generate a circuit for the given stabilizer code.
    
    Args:
        code (StabilizerCode): The stabilizer code to use.
        n_rounds (int): The number of rounds of error correction.
        p_phys (float): The physical error rate.
        p_meas (float): The measurement error rate.
        noise_model (str): The noise model to use. Can be "pheno" or "full".
        replace_T_with_S (bool): If True, replace T gates with S gates.
    Returns:
        stim.Circuit: The generated circuit.
    """

    if noise_model not in ["pheno", "full"]:
        raise ValueError("noise_model must be 'pheno' or 'full'")
    
    circuit = stim.Circuit()
    data_qubit_indices = list(range(code.n))
    data_qubit_indices_part1 = [1, 3, 5, 6]
    data_qubit_indices_part2 = [0, 2, 4]
    n_z = code.Hz.shape[0]
    z_stabilizer_indices = list(range(code.n, code.n + n_z))
    x_stabilizer_indices = list(
        range(code.n + n_z, code.n + code.n_stabilizers)
    )
    h_meas_index = code.n + code.n_stabilizers

    if y_state_init:
        # Initialize the data qubits in the Y state
        circuit.append("H", data_qubit_indices)
        circuit.append("S", data_qubit_indices_part1)
        circuit.append("S_DAG", data_qubit_indices_part2)

    def h_measurement(noisy=True):
        if replace_T_with_S:
            circuit.append("SQRT_Y_DAG", data_qubit_indices)
        else:
            circuit.append("I", data_qubit_indices, tag="Tdag")

        for data_qubit in data_qubit_indices:
            circuit.append("CNOT", [data_qubit, h_meas_index])

        if replace_T_with_S:
            circuit.append("SQRT_Y", data_qubit_indices)
        else:
            circuit.append("I", data_qubit_indices, tag="T")

        if noisy:
            circuit.append("X_ERROR", [h_meas_index], p_meas)
        circuit.append("MR", [h_meas_index], tag='H')
            
    
    def x_syndrome_extraction(noisy=True):
        circuit.append("H", x_stabilizer_indices)
        for x_stab in range(code.Hx.shape[0]):
            qubits = code.Hx[x_stab].nonzero()[1]
            for qubit in qubits:
                circuit.append("CNOT", [code.n + n_z + x_stab, qubit])
        circuit.append("H", x_stabilizer_indices)
        if noisy:
            circuit.append("X_ERROR", x_stabilizer_indices, p_meas)

    def z_syndrome_extraction(noisy=True):
        for z_stab in range(n_z):
            qubits = code.Hz[z_stab].nonzero()[1]
            for qubit in qubits:
                circuit.append("CNOT", [qubit, code.n + z_stab])
        if noisy:
            circuit.append("X_ERROR", z_stabilizer_indices, p_meas)

    for i_round in range(n_rounds):
        noisy = (i_round != n_rounds - 1)
        
        circuit.append("DEPOLARIZE1", data_qubit_indices, p_phys)
        x_syndrome_extraction(noisy)
        if i_round > 0 or y_state_init:
            z_syndrome_extraction(noisy)

        circuit.append("MR", z_stabilizer_indices + x_stabilizer_indices)
        circuit.append("DEPOLARIZE1", data_qubit_indices, p_phys)

        # if i_round == 0:
        #     circuit.append("CNOT", [1, 3])
        #     circuit.append("CNOT", [5, 3])
        #     circuit.append("I", [3], tag="T")
        #     circuit.append("CNOT", [5, 3])
        #     circuit.append("CNOT", [1, 3])
        h_measurement(noisy)

    return circuit


def generate_circuit_cn(
    code: StabilizerCode, 
    n_rounds=1, 
    p_phys=0.1, 
    p_meas=0.1,
    noise_model="pheno",
    replace_T_with_S=False,
) -> stim.Circuit:
    """
    Generate the Chamberland-Noh H state preparation circuit
    
    Args:
        code (StabilizerCode): The stabilizer code to use.
        n_rounds (int): The number of rounds of error correction.
        p_phys (float): The physical error rate.
        p_meas (float): The measurement error rate.
        noise_model (str): The noise model to use. Can be "pheno" or "full".
        replace_T_with_S (bool): If True, replace T gates with S gates.
    Returns:
        stim.Circuit: The generated circuit.
    """

    if noise_model not in ["pheno", "full"]:
        raise ValueError("noise_model must be 'pheno' or 'full'")
    
    circuit = stim.Circuit()
    data_qubit_indices = list(range(code.n))
    n_z = code.Hz.shape[0]
    n_x = code.Hx.shape[0]
    z_stabilizer_indices = list(range(code.n, code.n + n_z))
    x_stabilizer_indices = list(
        range(code.n + n_z, code.n + code.n_stabilizers)
    )
    weight2_x_meas_index = code.n + code.n_stabilizers
    weight2_z_meas_index = code.n + code.n_stabilizers + 1
    h_meas_index = code.n + code.n_stabilizers + 2

    def h_measurement(noisy=True):
        if replace_T_with_S:
            circuit.append("SQRT_Y_DAG", data_qubit_indices)
        else:
            circuit.append("I", data_qubit_indices, tag="Tdag")

        for data_qubit in data_qubit_indices:
            circuit.append("CNOT", [data_qubit, h_meas_index])

        if replace_T_with_S:
            circuit.append("SQRT_Y", data_qubit_indices)
        else:
            circuit.append("I", data_qubit_indices, tag="T")

        if noisy:
            circuit.append("X_ERROR", [h_meas_index], p_meas)
        circuit.append("MR", [h_meas_index], tag='H')
            
    
    def x_syndrome_extraction(noisy=True, subindices=None):
        if subindices is None:
            subindices = range(n_x)
        circuit.append("H", x_stabilizer_indices)
        for x_stab in subindices:
            qubits = code.Hx[x_stab].nonzero()[1]
            for qubit in qubits:
                circuit.append("CNOT", [code.n + n_z + x_stab, qubit])
        circuit.append("H", x_stabilizer_indices)
        if noisy:
            circuit.append("X_ERROR", x_stabilizer_indices, p_meas)

    def z_syndrome_extraction(noisy=True, subindices=None):
        if subindices is None:
            subindices = range(n_z)
            
        for z_stab in subindices:
            qubits = code.Hz[z_stab].nonzero()[1]
            for qubit in qubits:
                circuit.append("CNOT", [qubit, code.n + z_stab])
        if noisy:
            circuit.append("X_ERROR", z_stabilizer_indices, p_meas)
        
    def weight_2_syndrome_extraction(noisy=True, x_only=False):
        # Extracts the weight-2 boundary stabilizers
        data_qubits = [2, 3]
        circuit.append("H", [weight2_x_meas_index])
        for qubit in data_qubits:
            circuit.append("CNOT", [weight2_x_meas_index, qubit])
        circuit.append("H", [weight2_x_meas_index])

        if not x_only:
            for qubit in data_qubits:
                circuit.append("CNOT", [qubit, weight2_z_meas_index])

            if noisy:
                circuit.append("X_ERROR", [weight2_x_meas_index, weight2_z_meas_index], p_meas)        

    noisy = False
    circuit.append("I", 5, tag="T")
    x_syndrome_extraction(noisy, subindices=[0, 2])
    weight_2_syndrome_extraction(noisy, x_only=True)
    circuit.append("TICK")
    circuit.append("MR", [
        code.n + n_z + 0, code.n + n_z + 2, weight2_x_meas_index
    ])
    x_syndrome_extraction(noisy, subindices=[1])
    z_syndrome_extraction(noisy, subindices=[1])
    circuit.append("MR", [code.n + 1, code.n + n_z + 1])

    for i_round in range(n_rounds):
        noisy = (i_round != n_rounds - 1)

        circuit.append("DEPOLARIZE1", data_qubit_indices, p_phys)
        h_measurement(noisy)
        
        circuit.append("DEPOLARIZE1", data_qubit_indices, p_phys)
        x_syndrome_extraction(noisy)
        z_syndrome_extraction(noisy)

        circuit.append("MR", z_stabilizer_indices + x_stabilizer_indices)

    return circuit

