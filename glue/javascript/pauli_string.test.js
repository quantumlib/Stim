test("pauli_string.constructor<int>", ({stim, assert}) => {
    let p = new stim.PauliString(0);
    assert(p.length === 0);
    assert(p.sign === +1);
    assert(p.toString() === "+");

    p = new stim.PauliString(1);
    assert(p.length === 1);
    assert(p.sign === +1);
    assert(p.pauli(0) === 0);
    assert(p.toString() === "+_");

    p = new stim.PauliString(2);
    assert(p.length === 2);
    assert(p.sign === +1);
    assert(p.pauli(0) === 0);
    assert(p.pauli(1) === 0);
    assert(p.toString() === "+__");

    p = new stim.PauliString(1000);
    assert(p.length === 1000);
    assert(p.sign === +1);
    assert(p.pauli(0) === 0);
    assert(p.pauli(1) === 0);
    assert(p.pauli(999) === 0);
    assert(p.toString() === "+" + "_".repeat(1000));
});

test("pauli_string.constructor<str>", ({stim, assert}) => {
    let p = new stim.PauliString("");
    assert(p.length === 0);
    assert(p.sign === +1);
    assert(p.toString() === "+");

    p = new stim.PauliString("-");
    assert(p.length === 0);
    assert(p.sign === -1);
    assert(p.toString() === "-");

    p = new stim.PauliString("-IXYZ");
    assert(p.length === 4);
    assert(p.sign === -1);
    assert(p.pauli(0) === 0);
    assert(p.pauli(1) === 1);
    assert(p.pauli(2) === 2);
    assert(p.pauli(3) === 3);
    assert(p.toString() === "-_XYZ");

    try {
        new stim.PauliString([]);
        assert(false);
    } catch {
    }
});

test("pauli_string.equality", ({stim, assert}) => {
    let ps = k => new stim.PauliString(k);
    assert(ps("+").isEqualTo(ps("+")));
    assert(ps("XX").isEqualTo(ps("XX")));
    assert(!ps("XX").isEqualTo(ps("XY")));
    assert(!ps("XX").isEqualTo(ps("XXX")));
    assert(!ps("__").isEqualTo(ps("___")));
    assert(!ps("XX").isEqualTo(ps("ZX")));
    assert(!ps("XX").isEqualTo(ps("__")));
    assert(!ps("+").isEqualTo(ps("-")));
});

test("pauli_string.random", ({stim, assert}) => {
    assert(!stim.PauliString.random(100).isEqualTo(stim.PauliString.random(100)));
});

test("pauli_string.times", ({stim, assert}) => {
    let ps = k => new stim.PauliString(k);
    assert(ps("XX").times(ps("YY")).isEqualTo(ps("-ZZ")));
    assert(ps("XY").times(ps("YX")).isEqualTo(ps("+ZZ")));
    assert(ps("ZZ").times(ps("YY")).isEqualTo(ps("-XX")));
});

test("pauli_string.commutes", ({stim, assert}) => {
    let ps = k => new stim.PauliString(k);
    assert(ps('+').commutes(ps('+')));
    assert(ps('+XX').commutes(ps('+YY')));
    assert(ps('+XX').commutes(ps('-YY')));
    assert(ps('+XX').commutes(ps('+ZZ')));
    assert(!ps('+XX').commutes(ps('+XZ')));
    assert(!ps('+XX').commutes(ps('+XZ')));
});

test("pauli_string.neg", ({stim, assert}) => {
    let ps = k => new stim.PauliString(k);
    assert(ps('+').neg().isEqualTo(ps('-')));
    assert(ps('-').neg().isEqualTo(ps('+')));
    assert(ps('+XYZ').neg().isEqualTo(ps('-XYZ')));
});
