test("tableau.constructor<int>", ({stim, assert}) => {
    let p = new stim.Tableau(0);
    assert(p.length === 0);

    p = new stim.Tableau(1);
    assert(p.length === 1);
    assert(p.x_output(0).toString() === "+X");
    assert(p.y_output(0).toString() === "+Y");
    assert(p.z_output(0).toString() === "+Z");

    p = new stim.Tableau(8);
    assert(p.length === 8);
    assert(p.x_output(5).toString() === "+_____X__");
    assert(p.y_output(5).toString() === "+_____Y__");
    assert(p.z_output(5).toString() === "+_____Z__");
});

test("tableau.from_conjugated_generators_xs_zs", ({stim, assert}) => {
    assert(new stim.Tableau(0).isEqualTo(stim.Tableau.from_conjugated_generators_xs_zs([], [])));
    assert(new stim.Tableau(1).isEqualTo(stim.Tableau.from_conjugated_generators_xs_zs(
        [new stim.PauliString("+X")], [new stim.PauliString("+Z")]
    )));
    let t = stim.Tableau.from_conjugated_generators_xs_zs(
        [
            new stim.PauliString("+XX"),
            new stim.PauliString("+_X"),
        ],
        [
            new stim.PauliString("+Z_"),
            new stim.PauliString("+ZZ"),
        ],
    );
    assert(t.toString().trim() === `
+-xz-xz-
| ++ ++
| XZ _Z
| X_ XZ
`.trim());
});

test("tableau.from_named_gate_vs_output", ({stim, assert}) => {
    assert(new stim.Tableau(3).toString().trim() === `
+-xz-xz-xz-
| ++ ++ ++
| XZ __ __
| __ XZ __
| __ __ XZ
`.trim());

    assert(stim.Tableau.from_named_gate("H").toString().trim() === `
+-xz-
| ++
| ZX
`.trim());

    assert(stim.Tableau.from_named_gate("X").toString().trim() === `
+-xz-
| +-
| XZ
`.trim());

    let cnot = stim.Tableau.from_named_gate("CNOT");
    assert(cnot.toString().trim() === `
+-xz-xz-
| ++ ++
| XZ _Z
| X_ XZ
`.trim());
    assert(cnot.x_output(0).toString() === "+XX");
    assert(cnot.x_output(1).toString() === "+_X");
    assert(cnot.y_output(0).toString() === "+YX");
    assert(cnot.y_output(1).toString() === "+ZY");
    assert(cnot.z_output(0).toString() === "+Z_");
    assert(cnot.z_output(1).toString() === "+ZZ");
});

test("tableau.random", ({stim, assert}) => {
    assert(!stim.Tableau.random(10).isEqualTo(stim.Tableau.random(10)));
});

test("tableau.equality", ({stim, assert}) => {
    let h = stim.Tableau.from_named_gate("H");
    let x = stim.Tableau.from_named_gate("X");
    let id = stim.Tableau.from_named_gate("I");
    let cnot = stim.Tableau.from_named_gate("CNOT");
    assert(h.isEqualTo(h));
    assert(h.isEqualTo(stim.Tableau.from_named_gate("H")));
    assert(!h.isEqualTo(x));
    assert(!id.isEqualTo(x));
    assert(id.isEqualTo(id));
    assert(x.isEqualTo(x));
    assert(cnot.isEqualTo(cnot));
    assert(!x.isEqualTo(cnot));
    assert(new stim.Tableau(2).isEqualTo(new stim.Tableau(2)));
    assert(!new stim.Tableau(3).isEqualTo(new stim.Tableau(2)));
});
