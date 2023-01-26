# Crumble

Crumble is an in-development tool for exploring and inspecting 2D stabilizer circuits, with a
focus on quantum error correction.
In particular, crumble automates the process of propagating and verifying the
flow of Pauli products across the layers of the circuit.

**Crumble is still being prototyped and developed.
Crumble is not polished.
Crumble is not stable.**

## Index

- [Using Crumble](#using-crumble)
    - [Loading and Saving Circuits](#loading-saving)
    - [Keyboard Controls](#keyboard-commands)
    - [Mouse Controls](#mouse-commands)
- [Building Crumble](#building-crumble)
- [Testing Crumble](#testing-crumble)

<a name="using-crumble"></a>
# Using Crumble

There are two core pieces to using crumble effectively:
(1) editing the circuit
and (2) propagating Paulis.

Editing the circuit is done by selecting qubits with the mouse and hitting
keyboard keys to place gates.
The layers of the circuit can be navigated using the Q/E keys.

Propagating Paulis is done by placing *markers* to indicate where to add terms.
Each marker has a type (X, Y, or Z) and an index (0-9) indicating which indexed
Pauli product the marker is modifying.
For example, to place a Z term after a reset gate into the Pauli product with
index 1, select the reset gate and press `Z`+`1`. This introduces a Z term into
the Pauli product, and advancing through the circuit will show how the now
non-empty Pauli product changes as it is rewritten by the circuit's stabilizer
operations.

The simplest way to use markers is as a method for seeing how errors propagate
through the circuit.
But the far more useful case is for seeing how *knowledge*
propagates through the circuit.
After a qubit is reset, its Z observable is in a known state.
As the circuit executes, this piece of knowledge is transformed into different
forms; it may later correspond to knowing a multi-qubit Pauli product or to
knowing what the result of a measurement must be.
Pauli propagation shows how these pieces of knowledge move
through the circuit.

Of particular interest is finding small sets of resets that match small sets of
nearby measurements (i.e. resets that prepare knowledge predicting the parity
of a set of measurements).
These local reset-vs-measurement tautologies correspond to detectors, which can
be used to correct errors.
The path that the Pauli product takes, starting from the various resets and
terminating on the various measurements, forms the detecting region of the
detector.

<a name="loading-saving"></a>
## Loading and Saving Circuits

- **Bookmarking**:
As crumble runs, it constantly updates the web page's address so that it encodes
the current circuit.
For example, for an empty circuit, the URL will end with `#circuit=` whereas
a circuit with a single Hadamard gate would end with `#circuit=Q(0,0)0;H_0`.
Thus, the current circuit can be saved by bookmarking the page and loaded again
later by opening the bookmark.
- **Importing and Exporting**: In the top left of the page, there is a button
"Show Import/Export".
Clicking this button will reveal a large textbox containing the current circuit
encoded as a Stim circuit.
To save the circuit, copy this text and write it to a text file on your computer.
The saved circuit can later be loaded by copying the file's contents to your
clipboard, pasting over the contents of the textbox, and hitting the
"↓ Import from Stim Circuit ↓" button below the textbox.
The import/export text box can be hidden by clicking the "Show Import/Export"
button (now labelled "Hide Import/Export") again.

<a name="keyboard-commands"></a>
## Keyboard Controls

**Markings**

- `[XYZ]+[0-9]`: Place Pauli propagation markers at current selection.
    The X, Y, or Z determines the basis of the marker.
    The 1, 2, 3, or etc determines which of the tracked Pauli products is affected by the marker.
    For example, `X+2` will place X type markers that multiply an X dependence into Pauli product #2.
    The XYZ can be omitted, in which case the basis will be inferred based on the selected gates (for example, an RX gate implies X basis).
- `spacebar`: Clear all Pauli propagation markers at current selection.
- `P`: Add a background polygon with corners at the current selection.
    The color of the polygon is affected by modifier keys: X, Y, Z, alt, shift.

**Editing**

- `escape`: Unselect. Set current selection to the empty set.
- `delete`: Delete gates at current selection.
- `backspace`: Delete gates at current selection.
- `ctrl+delete`: Delete current circuit layer.
- `ctrl+backspace`: Delete current circuit layer.
- `ctrl+insert`: Insert empty layer at current circuit layer, pushing current circuit layer ahead in time.
- `ctrl+z`: Undo
- `ctrl+y`: Redo
- `ctrl+shift+z`: Redo
- `ctrl+c`: Copy selection to clipboard.
- `ctrl+v`: Past clipboard contents at current selection.
- `ctrl+x`: Cut selection to clipboard.
- `t`: Rotate circuit 45 degrees clockwise.
- `shift+t`: Rotate circuit 45 degrees counter-clockwise.

**Single Qubit Gates**

Note: use `shift` to get the inverse of a gate.

- `h`: Overwrite selection with `H` gate
- `s`: Overwrite selection with `SQRT_Z` gate
- `r`: Overwrite selection with `R` gate
- `m`: Overwrite selection with `M` gate
- `h+z` or `h+x+y`: Overwrite selection with `H_XY` gate
- `h+x` or `h+y+z`: Overwrite selection with `H_YZ` gate
- `s+x`: Overwrite selection with `SQRT_X` gate
- `s+y`: Overwrite selection with `SQRT_Y` gate
- `r+x`: Overwrite selection with `RX` gate
- `r+y`: Overwrite selection with `RY` gate
- `m+x`: Overwrite selection with `MX` gate
- `m+y`: Overwrite selection with `MY` gate
- `m+r+x`: Overwrite selection with `MRX` gate
- `m+r+y`: Overwrite selection with `MRY` gate
- `m+r`: Overwrite selection with `MR` gate
- `f`: Overwrite selection with `C_XYZ` gate

**Two Qubit Gates**

Note: when a single qubit is selected and a two qubit gate is placed, the gate
spans between the selected qubit and *the qubit the mouse is hovering over*.
When multiple qubits are selected, the difference between the topmost leftmost
qubit and *the qubit the mouse is hovering over* is computed, and then each selected qubit
targets its position offset by that difference.

Note: use `shift` to get the inverse of a gate.

- `w`: Overwrite selection with `SWAP` gate targeting mouse
- `w+i`: Overwrite selection with `ISWAP` gate targeting mouse
- `w+x`: Overwrite selection with `CXSWAP` gate targeting mouse
- `c+x`: Overwrite selection with `CX` gate targeting mouse
- `c+y`: Overwrite selection with `CY` gate targeting mouse
- `c+z`: Overwrite selection with `CZ` gate targeting mouse
- `c+x+y`: Overwrite selection with `XCY` gate targeting mouse
- `alt+c+x`: Overwrite selection with `XCX` gate targeting mouse
- `alt+c+y`: Overwrite selection with `YCY` gate targeting mouse
- `c+s+x`: Overwrite selection with `SQRT_XX` gate targeting mouse
- `c+s+y`: Overwrite selection with `SQRT_YY` gate targeting mouse
- `c+s+z`: Overwrite selection with `SQRT_ZZ` gate targeting mouse
- `c+m+x`: Overwrite selection with `MPP X*X` gate targeting mouse
- `c+m+y`: Overwrite selection with `MPP Y*Y` gate targeting mouse
- `c+m+z`: Overwrite selection with `MPP Z*Z` gate targeting mouse

**Keyboard Buttons as Gate Adjectives**

Roughly speaking, the "keyboard language" for gates used by Crumble has the following "adjectives":

- `x` means "X basis"
- `y` means "Y basis"
- `z` means "Z basis"
- `shift` means "inverse"
- `c` means "controlled gate" or more generally "two qubit variant of gate"
- `s` means "square root"
- `m` means "measure"
- `r` means "reset"
- `w` means "swap"
- `alt` means "no not that one, a different one"

Here are some examples:

- `r+x` is the **X basis variant** of the **reset operation** (i.e. the gate `RX 0`).
- `m+r` is the **measure** (m) and **reset** (r) operation (i.e. the gate `MR 0`).
- `m+r+y` is the **Y basis variant** of the **measure** (m) and **reset** (r) operation (i.e. the gate `MRY 0`).
- `c+m+x` is the **two qubit variant** (c)
of **measurement** (m)
in the **X basis** (x) (i.e. the gate `MPP X1*X2`).
- `shift+c+s+y` is the **inverse** (shift)
of the **two qubit variant** (c)
of the **square root** (s)
of the **Y gate** (y) (i.e. the gate `SQRT_YY_DAG 1 2`).

<a name="mouse-commands"></a>
### Mouse Controls

Note: to `BoxSelect` means to press down the left mouse button, drag the mouse
while holding the button down to outline a rectangular region, and then release
the mouse button.

Note: using `shift` modifies how the mouse updates the current selection. When
`shift` is being held and selection S1 would be replaced by selection S2, the new
selection will instead be set to the union of S1 and S2.

Note: using `ctrl` modifies how the mouse updates the current selection. When
`ctrl` is being held and selection S1 would be replaced by selection S2, the new
selection will instead be set to the symmetric difference of S1 and S2.

- `LeftClick`: Set current selection to clicked qubit (or to nothing, if clicking empty space).
- `BoxSelect`: Set current selection to boxed area.
- `Alt+BoxSelect`: Set current selection to boxed area, but only including the
qubits with the same parity as the initially clicked qubit at the start of the
box selection action. The specific parity being used depends on context. For
example, when selecting a column of qubits, the row parity is used. When
selecting a 2d region, the subgrid parity is used.

<a name="building-crumble"></a>
# Building Crumble

Crumble's source code can be served directly by a webserver.
For example, serve crumble using python's built-in web server:

```bash
# from the root of Stim's git repository:
python -m http.server --directory glue/crumble
```

then open [http://localhost:8000/crumble.html](http://localhost:8000/crumble.html) in a web browser.

A single-page version of crumble can be created using rollup-js and uglify-js:

```base
# npm install -g rollup uglify-js

# from the root of Stim's git repository:
{
    cat glue/crumble/crumble.html | grep -v "^<script";
    echo "<script>";
    rollup glue/crumble/main.js | uglifyjs -c -m --mangle-props --toplevel;
    echo "</script>";
} > crumble_single_page.html
```

<a name="testing-crumble"></a>
# Testing Crumble

Crumble's unit tests can be executed by opening the page `test/test.html` in a
web browser:

```bash
# from the root of Stim's git repository:
python -m http.server --directory glue/crumble &
firefox localhost:8000/test/test.html

# if page says "All X tests passed" then tests passed.
# see the browser console for a log of tests that were run
```

Unit tests can also be run headless using NodeJS.
Unit tests for functionality such as canvas drawing will be skipped when running
headless:

```
# from the root of Stim's git repository:

node glue/crumble/run_tests_headless.js
# will end with 'all tests passed' if all tests passed
```
