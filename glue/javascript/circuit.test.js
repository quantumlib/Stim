test("circuit.constructor", ({stim, assert}) => {
    let c = new stim.Circuit();
    assert(c.toString() === '');

    c = new stim.Circuit('H 0\n   H 1  # comment');
    assert(c.toString() === 'H 0 1');
});

test("circuit.repeated", ({stim, assert}) => {
    let c = new stim.Circuit('H 0\nCNOT 0 1');
    assert(c.repeated(0).isEqualTo(new stim.Circuit()));
    assert(c.repeated(1).isEqualTo(c));
    assert(!c.repeated(5).isEqualTo(c));

    assert(c.repeated(5).toString().trim() === `
REPEAT 5 {
    H 0
    CX 0 1
}
    `.trim());
});

test("circuit.copy", ({stim, assert}) => {
    let c = new stim.Circuit(`
        REPEAT 2 {
            H 0
            CNOT 0 1
        }
    `);
    let c2 = c.copy();
    assert(c.isEqualTo(c2));
    c.append_operation("X", [0], []);
    assert(!c.isEqualTo(c2));
});

test("circuit.append_operation", ({stim, assert}) => {
    let c = new stim.Circuit();
    c.append_operation("H", [0, 1], []);
    c.append_operation("X_ERROR", [0, 1, 2], [0.25]);
    assert(c.toString().trim() === `
H 0 1
X_ERROR(0.25) 0 1 2
    `.trim());
});

test("circuit.append_from_stim_program_text", ({stim, assert}) => {
    let c = new stim.Circuit();
    let c2 = new stim.Circuit("H 0\nCNOT 0 1");
    c.append_from_stim_program_text("H 0\nCNOT 0 1");
    assert(c.isEqualTo(c2));
});

test("circuit.isEqualTo", ({stim, assert}) => {
    assert(new stim.Circuit().isEqualTo(new stim.Circuit()));
    assert(new stim.Circuit('').isEqualTo(new stim.Circuit()));
    assert(!new stim.Circuit('H 0').isEqualTo(new stim.Circuit()));
    assert(!new stim.Circuit('H 0').isEqualTo(new stim.Circuit('X 0')));
    assert(!new stim.Circuit('H 0').isEqualTo(new stim.Circuit('H 1')));
    assert(new stim.Circuit('H 1').isEqualTo(new stim.Circuit('H 1')));
    assert(!new stim.Circuit('H 1').isEqualTo(new stim.Circuit(`
        REPEAT 1 {
            H 1
        }
    `)));
});
