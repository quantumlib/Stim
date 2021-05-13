test("tableau_simulator.constructor", ({stim, assert}) => {
    let s = new stim.TableauSimulator();
    assert(s.current_inverse_tableau().isEqualTo(new stim.Tableau(0)));
});

test("tableau_simulator.set_inverse_tableau", ({stim, assert}) => {
    let s = new stim.TableauSimulator();
    let t = new stim.Tableau(10);
    s.set_inverse_tableau(t);
    assert(s.current_inverse_tableau().isEqualTo(t));
});

test("tableau_simulator.measure", ({stim, assert}) => {
    let s = new stim.TableauSimulator();
    assert(s.measure(0) === false);
    assert(s.measure(1) === false);
    assert(s.measure(2) === false);
    s.X(0);
    s.X(2);
    assert(s.measure(0) === true);
    assert(s.measure(1) === false);
    assert(s.measure(2) === true);

    assert(s.measure(500) === false);
});

test("tableau_simulator.gate_methods_vs_do_tableau", ({stim, assert}) => {
    for (let gate of ["X", "Y", "Z", "H", "CNOT", "CY", "CZ"]) {
        let t = stim.Tableau.from_named_gate(gate);
        let n = t.length;
        let s = new stim.TableauSimulator();
        for (let k = 0; k < n; k++) {
            s.H(k);
            s.CNOT(k, k + n);
        }
        let s2 = s.copy();
        if (n === 1) {
            s[gate](2);
            s2.do_tableau(t, [2]);
        } else {
            s[gate](2, 3);
            s2.do_tableau(t, [2, 3]);
        }
        assert(s.current_inverse_tableau().isEqualTo(s2.current_inverse_tableau()));
    }
});

test("tableau_simulator.do_tableau", ({stim, assert}) => {
    let a = new stim.TableauSimulator();
    a.do_tableau(stim.Tableau.from_named_gate("X"), [0]);
    assert(a.measure(0) === true);
    assert(a.measure(1) === false);
    assert(a.measure(2) === false);
    a.do_tableau(stim.Tableau.from_named_gate("CNOT"), [0, 2]);
    assert(a.measure(0) === true);
    assert(a.measure(1) === false);
    assert(a.measure(2) === true);
});

test("tableau_simulator.copy", ({stim, assert}) => {
    let a = new stim.TableauSimulator();
    a.X(2);
    let b = a.copy();
    assert(a.current_inverse_tableau().isEqualTo(b.current_inverse_tableau()));
    a.Y(1);
    assert(!a.current_inverse_tableau().isEqualTo(b.current_inverse_tableau()));
});

test("tableau_simulator.do_circuit", ({stim, assert}) => {
    let a = new stim.TableauSimulator();
    a.do_circuit(new stim.Circuit(`
        X 0 2
        SWAP 2 3
    `));
    assert(a.measure(0) === true);
    assert(a.measure(1) === false);
    assert(a.measure(2) === false);
    assert(a.measure(3) === true);
});

test("tableau_simulator.do_pauli_string", ({stim, assert}) => {
    let a = new stim.TableauSimulator();
    a.do_circuit(new stim.Circuit(`
        H_XZ 4 5 6 7
        H_YZ 8 9 10 11
    `));
    a.do_pauli_string(new stim.PauliString("_XYZ_XYZ_XYZ"));
    a.do_circuit(new stim.Circuit(`
        H_XZ 4 5 6 7
        H_YZ 8 9 10 11
    `));
    assert(a.measure(0) === false);
    assert(a.measure(1) === true);
    assert(a.measure(2) === true);
    assert(a.measure(3) === false);

    assert(a.measure(4) === false);
    assert(a.measure(5) === false);
    assert(a.measure(6) === true);
    assert(a.measure(7) === true);

    assert(a.measure(8) === false);
    assert(a.measure(9) === true);
    assert(a.measure(10) === false);
    assert(a.measure(11) === true);
});

test("tableau_simulator.measure_kickback", ({stim, assert}) => {
    let a = new stim.TableauSimulator();
    a.do_circuit(new stim.Circuit(`
        H 0
        CNOT 0 1 0 3
    `));
    let v1 = a.measure_kickback(1);
    let v2 = a.measure_kickback(3);
    assert(v1.kickback.isEqualTo(new stim.PauliString('+XX_X')));
    assert(v2.kickback === undefined);
    assert(v1.result === v2.result);
});

test("tableau_simulator.collision", ({stim, assert}) => {
    let s = new stim.TableauSimulator();
    try {
        s.CNOT(0, 0);
        assert(false);
    } catch {
    }
    try {
        s.SWAP(2, 2);
        assert(false);
    } catch {
    }
    s.SWAP(0, 1);
});
