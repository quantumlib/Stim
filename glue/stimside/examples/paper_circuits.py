"""Circuits of "Erasure surface code circuit without mid-circuit erasure checks".

Builds the static (rotated) and moonwalking surface code memory circuits of
M. Pavlovich, I. Rojkov, C. Wang and S. Puri (arXiv:2607.29443) with stimside
leakage tags, for the examples in this folder (``paper_circuit`` at the end).

The layout classes below are copied unchanged from the paper's code,
https://github.com/magzpavz/surface-code-leakage-erasure (files
``surface_code.py`` and ``walking_surface_code.py``), under its MIT license:

    MIT License

    Copyright (c) 2026 Margaret Pavlovich

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
"""

# ---------------------------------------------------------------------------
# surface_code.py (surface-code-leakage-erasure, MIT license)
# ---------------------------------------------------------------------------
# This file defines the surface code layout, including helper classes
# SurfaceCodeLayout is very slow but only need to do it once per distance

class Coord(tuple):
    # qubit or plaquette coordinate
    def __new__(cls, x, y=None):
        # Accept either Coord(x, y) or Coord((x, y)) to be pickle-friendly.
        if y is None:
            try:
                x0, y0 = x
            except Exception as e:
                raise TypeError("Coord requires two values (x, y) or a single iterable of length 2") from e
        else:
            x0, y0 = x, y
        return super().__new__(cls, (x0, y0))

    def __repr__(self):
        return f"Coord({self[0]}, {self[1]})"

    def __add__(self, other):
        if isinstance(other, Coord):
            return Coord(self[0] + other[0], self[1] + other[1])
        else:
            raise TypeError(f"Unsupported operand type(s) {type(self)} and {type(other)} for +")

    def __sub__(self, other):
        if isinstance(other, Coord):
            return Coord(self[0] - other[0], self[1] - other[1])
        else:
            raise TypeError(f"Unsupported operand type(s) {type(self)} and {type(other)} for -")

    def __mul__(self, other):
        if isinstance(other, int):
            return Coord(self[0]*other, self[1]*other)
        else:
            raise TypeError(f"Unsupported operand type(s) {type(self)} and {type(other)} for *")

    def __rmul__(self, other):
        return self * other

    def __lt__(self, other):
        return (self[0] < other[0]) or (self[0] == other[0] and self[1] < other[1])

    def isinside(self, top_left: 'Coord', bottom_right: 'Coord') -> bool:
        return (top_left[0] <= self[0] <= bottom_right[0]) and (top_left[1] <= self[1] <= bottom_right[1])

class Qubit:
    def __init__(self, index: int, coord: Coord):
        self.index = index
        self.coord = coord

    def __eq__(self, other):
        return (type(self) == type(other)) and (self.index == other.index) and (self.coord == other.coord)

    def __hash__(self):
        # Hash must be consistent with __eq__. Use the immutable attributes.
        return hash((self.index, self.coord))

    def __lt__(self, other):
        return self.index < other.index

    def __repr__(self):
        return f"Qubit(index={self.index}, coord={self.coord})"

class DataQubit(Qubit):
    def __init__(self, index: int, coord: Coord):
        super().__init__(index, coord)
        self.ordered_plaquettes: list[Plaquette | None] = [None, None, None, None]

    def __repr__(self):
        return f"DataQubit(index={self.index}, coord={self.coord})"

class Plaquette:
    def __init__(self, s_type: str, ordered_data_qubits: list[Qubit | None], ancilla: Qubit, coord: Coord):
        assert s_type in ['X','Z']
        self.type = s_type
        self.ordered_data_qubits = ordered_data_qubits
        self.ancilla = ancilla
        self.index = ancilla.index
        self.coord = coord

    def __eq__(self, other):
        # Compare plaquette type, ordered data qubit indexes (or None), ancilla index, and coord.
        # We avoid comparing objects directly to prevent recursive equality issues.
        def dq_key(dq):
            return None if dq is None else dq.index

        return (
            type(self) == type(other) and
            self.type == other.type and
            [dq_key(dq) for dq in self.ordered_data_qubits] == [dq_key(dq) for dq in other.ordered_data_qubits] and
            self.ancilla.index == other.ancilla.index and
            self.index == other.index and
            self.coord == other.coord
        )

    def __hash__(self):
        # Build a tuple of immutable keys consistent with __eq__.
        # Deliberately hash the plaquette type as a bool rather than the string
        # 'X'/'Z': Python randomizes string hashing per process, which would make
        # iteration order of any set of Plaquettes differ between runs, and hence
        # detector numbering non-reproducible. All other components are ints.
        dq_indexes = tuple(None if dq is None else dq.index for dq in self.ordered_data_qubits)
        return hash((self.type == 'X', dq_indexes, self.ancilla.index, self.index, self.coord))

    def __lt__(self, other):
        return (self.type < other.type) or (self.type == other.type and self.index < other.index)

    def __repr__(self):
        return f"Plaquette(type={self.type}, ancilla={self.ancilla.index}, coord={self.coord})"

class SurfaceCodeLayout:
    ### constructs an object which contains all the information about the surface
    # code layout, including the plaquette types and gate order
    Z_DELTAS = (Coord(1,1), Coord(-1,1), Coord(1,-1), Coord(-1,-1))
    X_DELTAS = (Coord(1,1), Coord(1,-1), Coord(-1,1), Coord(-1,-1))

    def __init__(self, d):
        d = int(d)
        self.d: int = d
        self.define_layout()

    def __eq__(self, other):
        return (type(self) == type(other) and self.d == other.d and
                self.data_qubits == other.data_qubits and
                self.qubits == other.qubits and
                self.plaquettes == other.plaquettes)

    def define_layout(self):
        ## write qubits and coord_to_qubit dictionary
        data_qubits, dq_coords = self.layout_data_qubits()
        ancillas, aq_coords = self.layout_ancillas(qubit_index=len(data_qubits))

        self.data_qubits = frozenset(data_qubits)
        self.qubits = frozenset(data_qubits + ancillas)
        self.coord_to_qubit = {**dq_coords, **aq_coords}
        self.index_to_qubit = {q.index: q for q in self.qubits}

        ## define plaquettes including gate orders
        z_plaqs, x_plaqs, coord_to_plaq, index_to_plaq = self.layout_plaquettes()

        self.plaquettes = frozenset(z_plaqs + x_plaqs)
        self.z_plaquettes = frozenset(z_plaqs)
        self.x_plaquettes = frozenset(x_plaqs)

        self.coord_to_plaquette = coord_to_plaq
        self.index_to_plaquette = index_to_plaq

        ## define logical observables
        z_logical, x_logical = self.get_logicals()
        self.z_logical = frozenset(z_logical)
        self.x_logical = frozenset(x_logical)

    def layout_data_qubits(self, qubit_index = 0, top_left = Coord(1,1)):
        ## define the data qubits in a grid
        d = self.d
        qubits : list[Qubit] = []
        coord_to_qubit: dict[Coord, Qubit]= {}
        for i in range(d):
            for j in range(d):
                coord = top_left + Coord(2*i, 2*j)
                qubit = DataQubit(qubit_index, coord)
                qubits.append(qubit)
                coord_to_qubit[coord] = qubit
                qubit_index += 1

        return qubits, coord_to_qubit

    def layout_ancillas(self, qubit_index: int):
        ## define the ancilla qubits
        d = self.d
        coord_list = []

        # the bulk ancillas
        top_left = Coord(2,2)
        for i in range(d-1):
            for j in range(d-1):
                coord_list.append(top_left + Coord(2*i, 2*j))

        # the edge ancillas
        top_left = Coord(0,2)
        top_right = Coord(d*2-2, 0)
        bottom_left = Coord(2, d*2)
        bottom_right = Coord(d*2, d*2-2)
        starts = [top_left, top_right, bottom_left, bottom_right]
        directions = [Coord(0,4), Coord(-4,0), Coord(4,0), Coord(0,-4)]
        for i in range(4):
            direction = directions[i]
            if (d%2==0) and ((i==1) or (i==2)):
                ran = (d-1)//2
                start = starts[i] + Coord(direction[0]//2, direction[1]//2)
            else:
                ran = d//2
                start = starts[i]
            for j in range(ran):
                coord_list.append(start + direction*j)

        coord_list.sort()
        qubits = [Qubit(qubit_index + i, coord) for i, coord in enumerate(coord_list)]
        coord_to_qubit = {coord: qubits[i] for i, coord in enumerate(coord_list)}

        return qubits, coord_to_qubit

    def layout_plaquettes(self, top_left = Coord(0,0)):
        d = self.d
        ## make plaquettes and coord_to_plaquette dictionary
        z_plaquettes: list[Plaquette] = []
        x_plaquettes: list[Plaquette] = []
        coord_to_plaquette: dict[Coord, Plaquette] = {}
        index_to_plaquette: dict[int, Plaquette] = {}

        z_deltas = self.Z_DELTAS
        x_deltas = self.X_DELTAS
        for i in range(d+1):
            for j in range(d+1):
                coord = top_left + Coord(j*2, i*2)
                if coord in self.coord_to_qubit:
                    s_type = 'Z' if ((i+j)%2)==0 else 'X'
                    ancilla = self.coord_to_qubit[coord]
                    deltas = z_deltas if s_type == 'Z' else x_deltas
                    ordered_data_qubits = [self.coord_to_qubit.get(coord+delta) for delta in deltas]
                    plaquette = Plaquette(s_type, ordered_data_qubits, ancilla, coord)
                    for (t, dq) in enumerate(ordered_data_qubits):
                        if dq is not None and isinstance(dq, DataQubit):
                            dq.ordered_plaquettes[t] = plaquette
                    coord_to_plaquette[coord] = plaquette
                    index_to_plaquette[ancilla.index] = plaquette
                    if s_type == 'Z':
                        z_plaquettes.append(plaquette)
                    else:
                        x_plaquettes.append(plaquette)

        return z_plaquettes, x_plaquettes, coord_to_plaquette, index_to_plaquette

    def get_logicals(self):
        z_logical = []
        x_logical = []
        for qubit in self.data_qubits:
            if qubit.coord[0] == 1:
                z_logical.append(qubit)
            if qubit.coord[1] == 1:
                x_logical.append(qubit)

        return z_logical, x_logical

    def __repr__(self):
        return f"SurfaceCodeLayout(d={self.d})"

# ---------------------------------------------------------------------------
# walking_surface_code.py (surface-code-leakage-erasure, MIT license)
# ---------------------------------------------------------------------------

class WalkingSurfaceCodeLayout(SurfaceCodeLayout):
    """Layout for the walking and moonwalking surface codes.

    `swap_time` selects which of the two circuits the paper describes:
    `"late"` is the conventional **walking** surface code, `"early"` is the
    **moonwalking** surface code. 
    """

    def __init__(self, d, swap_time="late"):
        if d % 2 == 0:
            raise ValueError("Distance d must be odd for walking surface code.")
        if swap_time not in ["early", "late"]:
            raise ValueError("`swap_time` must be 'early' or 'late'.")
        self.d: int = d
        self.swap_time: str = swap_time
        self.define_layout(swap_time)

    def define_layout(self, swap_time):
        ## write qubits and coord_to_qubit dictionary
        dq1, dq1_coords = self.layout_data_qubits(qubit_index=0, top_left=Coord(1,1))
        dq2, dq2_coords = self.layout_data_qubits(qubit_index=len(dq1), top_left=Coord(2,2))
        ancillas, aq_coords = self.layout_edge_ancillas()

        if swap_time == "late":
            self.data_qubits = [frozenset(dq1), frozenset(dq2)]
        else:
            self.data_qubits = [frozenset(dq2), frozenset(dq1)]
        self.qubits = frozenset(dq1 + dq2 + ancillas)

        self.coord_to_qubit = {**dq1_coords, **dq2_coords, **aq_coords}
        self.index_to_qubit = {q.index: q for q in self.qubits}

        ## define plaquettes including gate orders
        self.layout_plaquettes()

        ## define logical observables
        z_logical, x_logical = self.get_logicals()
        self.z_logical = z_logical
        self.x_logical = x_logical

        ## define detectors
        self.define_detectors()

    def layout_edge_ancillas(self):
        ## for walking surface code, only have true "ancillas" on the edges
        d = self.d
        coord_list = []

        top_left = Coord(0,2)
        top_right = Coord(d*2-2, 0)
        bottom_left = Coord(2, d*2) + Coord(1,1)
        bottom_right = Coord(d*2, d*2-2) + Coord(1,1)
        starts = [top_left, top_right, bottom_left, bottom_right]
        directions = [Coord(0,4), Coord(-4,0), Coord(4,0), Coord(0,-4)]
        for i in range(4):
            direction = directions[i]
            if (d%2==0) and ((i==1) or (i==2)):
                ran = (d-1)//2
                start = starts[i] + Coord(direction[0]//2, direction[1]//2)
            else:
                ran = d//2
                start = starts[i]
            for j in range(ran):
                coord_list.append(start + direction*j)

        coord_list.sort()
        qubits = [Qubit(2*d**2 + i, coord) for i, coord in enumerate(coord_list)]
        coord_to_qubit = {coord: qubits[i] for i, coord in enumerate(coord_list)}

        return qubits, coord_to_qubit

    def layout_plaquettes(self):

        # initialize empty lists
        self.plaquettes = [] # list of sets, one per round, of all plaquettes

        self.bulk_plaquettes = []
        self.initial_plaquettes = []
        self.leading_plaquettes = []
        self.trailing_plaquettes = []

        self.z_plaquettes = []
        self.x_plaquettes = []

        self.reset_indexes = [] # list of lists, of qubits to be initialized at the start of round
        self.rx_indexes = [] # list of lists, of qubits to be initialized in + state
        self.measure_indexes = [] # list of lists, of qubits which are measured at end of round
        self.mx_indexes = [] # list of lists, of qubits tob be measured in X basis

        self.layout_bulk_plaquettes()
        self.layout_edge_plaquettes()
        if self.swap_time == "late":
            self.define_resets_measurements_late_swap()
        else:  # swap_time == "early"
            self.define_resets_measurements_early_swap()

    def layout_bulk_plaquettes(self):
        ## lay out plaquettes in the bulk
        d = self.d
        coord_to_plaquette: dict[Coord, Plaquette] = {}
        index_to_plaquette: dict[int, Plaquette] = {}

        z_deltas = self.Z_DELTAS  #[Coord(1,1), Coord(-1,1), Coord(1,-1), Coord(-1,-1)]
        x_deltas = self.X_DELTAS  #[Coord(1,1), Coord(1,-1), Coord(-1,1), Coord(-1,-1)]

        top_left_list = [Coord(2,2), Coord(3,3)]
        if self.swap_time == "early":
            top_left_list = top_left_list[::-1]

        for (rnd, top_left) in enumerate(top_left_list):
            z_plaquettes: list[Plaquette] = []
            x_plaquettes: list[Plaquette] = []
            for i in range(d-1):
                for j in range(d-1):
                    coord = top_left + Coord(j*2, i*2)
                    s_type = 'Z' if ((i+j)%2)==0 else 'X'
                    ancilla = self.coord_to_qubit[coord]
                    deltas = z_deltas if s_type == 'Z' else x_deltas
                    ordered_data_qubits = [self.coord_to_qubit.get(coord+delta) for delta in deltas]
                    plaquette = Plaquette(s_type, ordered_data_qubits, ancilla, coord)
                    for (t, dq) in enumerate(ordered_data_qubits):
                        if dq is not None:
                            dq.ordered_plaquettes[t] = plaquette
                    coord_to_plaquette[coord] = plaquette
                    index_to_plaquette[ancilla.index] = plaquette
                    if s_type == 'Z':
                        z_plaquettes.append(plaquette)
                    else:
                        x_plaquettes.append(plaquette)

            self.plaquettes.append(set(z_plaquettes + x_plaquettes))
            self.bulk_plaquettes.append(frozenset(z_plaquettes + x_plaquettes))
            self.z_plaquettes.append(set(z_plaquettes))
            self.x_plaquettes.append(set(x_plaquettes))

            # reverse gate order for next round
            z_deltas = z_deltas[::-1]
            x_deltas = x_deltas[::-1]

        self.coord_to_plaquette = coord_to_plaquette
        self.index_to_plaquette = index_to_plaquette

    def layout_edge_plaquettes(self):
        ## lay out plaquettes on the edges
        d = self.d
        swap_time = self.swap_time

        coord_to_plaquette: dict[Coord, Plaquette] = {}
        index_to_plaquette: dict[int, Plaquette] = {}

        z_deltas = self.Z_DELTAS
        x_deltas = self.X_DELTAS

        leading_plaquettes: list[list[Plaquette]] = [[],[]]
        trailing_plaquettes: list[list[Plaquette]] = [[],[]]

        ## regular edge plaquettes
        start_left_or_top = [Coord(0,2), Coord(4,0)]
        start_right_or_bottom = [Coord(2*d,4), Coord(2,2*d)]
        steps = [Coord(0,4), Coord(4,0)]

        for (stype, start1, start2, step) in zip(['X','Z'], start_left_or_top, start_right_or_bottom, steps):
            for rnd in range(2):
                for start in [start1, start2]:
                    if swap_time == "early":
                        start += Coord(1,1)
                    deltas = x_deltas if stype == 'X' else z_deltas
                    plaq_list = self._one_edge_plaquettes(stype, rnd, start, step, deltas)

                    if start.isinside(Coord(1,1), Coord(d*2,d*2)) and swap_time == "late":
                        leading_plaquettes[rnd].extend(plaq_list)
                    elif not start.isinside(Coord(1,1), Coord(d*2,d*2)) and swap_time == "early":
                        leading_plaquettes[rnd].extend(plaq_list)
                    else:
                        trailing_plaquettes[rnd].extend(plaq_list)

                # update for second round
                if swap_time == "late":
                    start1 += Coord(1,1)
                    start2 += Coord(1,1)
                else:  # swap_time == "early"
                    start1 -= Coord(1,1)
                    start2 -= Coord(1,1)
                x_deltas = x_deltas[::-1]
                z_deltas = z_deltas[::-1]

        # initial plaquettes exclude special leakage-removal plaquettes
        self.initial_plaquettes = [sorted(self.z_plaquettes[0]), sorted(self.x_plaquettes[0])]

        ## special leakage-removal plaquettes
        leakage_plaquettes = []
        # if swap_time == "late":
        start_zs = [Coord(2*d, 2), Coord(1,1)]
        start_xs = [Coord(4, 2*d), Coord(3,1)]
        delta = Coord(-1,-1)  # delta to data qubit leakage is removed from
        t = 3  # what time "swap" happens at
        if swap_time == "early":
            start_zs = start_zs[::-1]
            start_xs = start_xs[::-1]
            t = 0
            delta = -1*delta

        for (rnd, start_z, start_x) in zip([0,1], start_zs, start_xs):
            for (stype, start, step) in zip(['Z','X'], [start_z, start_x], steps):
                plaq_list = []
                for i in range((d+1)//2):
                    coord = start + step*i
                    if not coord.isinside(Coord(1,1), Coord(d*2,d*2)):
                        break

                    ancilla = self.coord_to_qubit[coord]
                    ordered_data_qubits = [None, None, None, None]
                    ordered_data_qubits[t] = self.coord_to_qubit.get(coord+delta)
                    plaquette = Plaquette(stype, ordered_data_qubits, ancilla, coord)
                    ordered_data_qubits[t].ordered_plaquettes[t] = plaquette
                    plaq_list.append(plaquette)

                    coord_to_plaquette[coord] = plaquette
                    index_to_plaquette[ancilla.index] = plaquette

                self.plaquettes[rnd] |= set(plaq_list)
                if stype == 'Z':
                    self.z_plaquettes[rnd] |= set(plaq_list)
                else:
                    self.x_plaquettes[rnd] |= set(plaq_list)

                leakage_plaquettes.extend(plaq_list)
                if swap_time == "late":
                    leading_plaquettes[rnd].extend(plaq_list)
                # else:  # swap_time == "early"
                #     trailing_plaquettes[rnd].extend(plaq_list)

            delta = -1*delta

        self.trailing_plaquettes = [frozenset(plaq_list) for plaq_list in trailing_plaquettes]
        self.leading_plaquettes = [frozenset(plaq_list) for plaq_list in leading_plaquettes]
        self.leakage_plaquettes = frozenset(leakage_plaquettes)
        self.coord_to_plaquette.update(coord_to_plaquette)
        self.index_to_plaquette.update(index_to_plaquette)

    def _one_edge_plaquettes(self, stype, rnd, start, step, deltas):
        plaq_list = []

        for i in range(self.d//2):
            coord = start + step*i
            ancilla = self.coord_to_qubit[coord]

            ordered_data_qubits = []
            for delta in deltas:
                dq = self.coord_to_qubit.get(coord+delta)
                if dq in self.data_qubits[rnd]:
                    ordered_data_qubits.append(dq)
                else:
                    ordered_data_qubits.append(None)
            plaquette = Plaquette(stype, ordered_data_qubits, ancilla, coord)
            for (t, dq) in enumerate(ordered_data_qubits):
                if dq is not None:
                    dq.ordered_plaquettes[t] = plaquette

            plaq_list.append(plaquette)

        self.coord_to_plaquette.update({plaquette.coord: plaquette for plaquette in plaq_list})
        self.index_to_plaquette.update({plaquette.index: plaquette for plaquette in plaq_list})

        self.plaquettes[rnd] |= set(plaq_list)
        if stype == 'Z':
            self.z_plaquettes[rnd] |= set(plaq_list)
        else:
            self.x_plaquettes[rnd] |= set(plaq_list)

        return plaq_list

    def define_resets_measurements_late_swap(self):
        """ Define which qubits are reset and measured in each round for the late walking surface code """
        for rnd in range(2):
            reset_list = []
            rx_list = []
            measure_list = []
            mx_list = []

            for plaquette in self.plaquettes[rnd]:

                reset_list.append(plaquette.ancilla.index)
                if plaquette.type == 'X' and plaquette not in self.leakage_plaquettes:
                    # leakage plaquettes have opposite initialization from expected
                    rx_list.append(plaquette.ancilla.index)
                elif plaquette.type == 'Z' and plaquette in self.leakage_plaquettes:
                    # leakage plaquettes have opposite initialization from expected
                    rx_list.append(plaquette.ancilla.index)

                last_dq = plaquette.ordered_data_qubits[-1]
                if last_dq is not None:
                    measure_list.append(last_dq.index)
                    if plaquette.type == 'X':
                        mx_list.append(last_dq.index)

            for plaquette in sorted(self.trailing_plaquettes[rnd]):
                measure_list.append(plaquette.ancilla.index)
                if plaquette.type == 'X':
                    mx_list.append(plaquette.ancilla.index)

            self.reset_indexes.append(sorted(reset_list))
            self.rx_indexes.append(sorted(rx_list))
            self.measure_indexes.append(sorted(measure_list))
            self.mx_indexes.append(sorted(mx_list))

    def define_resets_measurements_early_swap(self):
        """ Define which qubits are reset and measured in each round for the early walking surface code """
        for rnd in range(2):
            reset_list = []
            rx_list = []
            measure_list = []
            mx_list = []

            for plaquette in self.plaquettes[rnd]:

                first_dq = plaquette.ordered_data_qubits[0]
                if first_dq is not None:
                    reset_list.append(first_dq.index)
                    if plaquette.type == 'X':
                        rx_list.append(first_dq.index)

                measure_list.append(plaquette.ancilla.index)
                if plaquette.type == 'X' and plaquette not in self.leakage_plaquettes:
                    mx_list.append(plaquette.ancilla.index)
                elif plaquette.type == 'Z' and plaquette in self.leakage_plaquettes:
                    mx_list.append(plaquette.ancilla.index)

            for plaquette in sorted(self.leading_plaquettes[rnd]):
                reset_list.append(plaquette.ancilla.index)
                if plaquette.type == 'X':
                    rx_list.append(plaquette.ancilla.index)


            self.reset_indexes.append(sorted(reset_list))
            self.rx_indexes.append(sorted(rx_list))
            self.measure_indexes.append(sorted(measure_list))
            self.mx_indexes.append(sorted(mx_list))

    def define_detectors(self):

        self.detectors = [[], []]
        self.define_bulk_detectors()

        if self.swap_time == "late":
            self.initial_detectors = self.initial_detectors_late_swap()
            self.final_detectors = self.final_detectors_late_swap()
            self.edge_detectors_late_swap()
        else:  # swap_time == "early"
            self.initial_detectors = self.initial_detectors_early_swap()
            self.final_detectors = self.final_detectors_early_swap()
            self.edge_detectors_early_swap()

    def initial_detectors_late_swap(self):
        initial_detectors = []
        for plaq_list in self.initial_plaquettes:
            detector_qubits = []
            for plaquette in plaq_list:
                coord = plaquette.coord
                qubit = self.coord_to_qubit.get(coord + Coord(-1,-1)) or plaquette.ancilla
                detector_qubits.append((qubit.index,))
            initial_detectors.append(detector_qubits)

        return initial_detectors

    def initial_detectors_early_swap(self):
        # Z type
        initial_detectors_z = []
        for plaquette in self.initial_plaquettes[0]:
            if plaquette.coord[1] == 1:
                # top edge: almonds parallel to patch
                q0 = plaquette.ancilla.index
                q1 = self.coord_to_qubit[plaquette.coord + Coord(-2,0)].index
                initial_detectors_z.append((q0, q1))
            elif plaquette.coord[1] == 3:
                # top row of bulk: almonds sticking out of patch
                q0 = plaquette.ancilla.index
                q1 = self.coord_to_qubit[plaquette.coord + Coord(0,-2)].index
                initial_detectors_z.append((q0, q1))
            else:  # rest are just single qubit detectors on ancilla
                initial_detectors_z.append((plaquette.ancilla.index,))

        # X type
        initial_detectors_x = []
        for plaquette in self.initial_plaquettes[1]:
            if plaquette.coord[0] == 1:
                # left edge: almonds parallel to patch
                q0 = plaquette.ancilla.index
                q1 = self.coord_to_qubit[plaquette.coord + Coord(0,-2)].index
                initial_detectors_x.append((q0, q1))
            elif plaquette.coord[0] == 3:
                # left row of bulk: almonds sticking out of patch
                q0 = plaquette.ancilla.index
                q1 = self.coord_to_qubit[plaquette.coord + Coord(-2,0)].index
                initial_detectors_x.append((q0, q1))
            else:  # rest are just single qubit detectors on ancilla
                initial_detectors_x.append((plaquette.ancilla.index,))

        return [initial_detectors_z, initial_detectors_x]

    def define_bulk_detectors(self):
        detector_qubit_pairs_rounds = []
        d = self.d
        swap_time = self.swap_time

        step = Coord(1,1)
        if swap_time == "early":
            step = -1*step

        for rnd in range(2):
            detector_qubit_tuples = []
            for plaq in sorted(self.bulk_plaquettes[rnd]):
                qubit_list = []
                q0 = plaq.ordered_data_qubits[0] if swap_time == "late" else plaq.ancilla
                qubit_list.append(q0.index)
                coord = q0.coord
                other_coord = coord + step
                qubit_list.append(self.coord_to_qubit.get(other_coord).index)
                if swap_time == "early":
                    extra_qubit = self.extra_bulk_detector_qubit(rnd, plaq, other_coord)
                    if extra_qubit is not None:
                        qubit_list.append(extra_qubit)

                detector_qubit_tuples.append(tuple(qubit_list))

            # reverse order because measure detector initialized in round 0 in round 1
            self.detectors[1-rnd].extend(detector_qubit_tuples)
            step = -1*step  # update for odd round

    def extra_bulk_detector_qubit(self, rnd, plaq, other_coord):
        d = self.d
        z_delta = (-1)**(rnd) * Coord(0,2)
        x_delta = (-1)**(rnd) *  Coord(2,0)

        # in early walking surface code, some bulk detectors have an extra qubit
        if plaq.type == 'Z' and (other_coord[1] == 2*(d-1) or other_coord[1] == 3):
            return self.coord_to_qubit.get(other_coord + z_delta).index
        elif plaq.type == 'X' and (other_coord[0] == 2*(d-1) or other_coord[0] == 3):
            return self.coord_to_qubit.get(other_coord + x_delta).index

        return None

    def edge_detectors_late_swap(self):
        # detectors which start as "almonds" along leading edge end as single qubit detectors on trailing edge
        for rnd in range(2):
            detector_qubit_tuples = []
            for plaquette in sorted(self.trailing_plaquettes[rnd]):
                detector_qubit_tuples.append((plaquette.ancilla.index,))

            self.detectors[rnd].extend(detector_qubit_tuples)

        # detectors which start as single qubits on trailing edge have three qubits which participate in final detector
        step = Coord(1,1)
        for rnd in range(2):
            detector_qubit_tuples = []
            for plaquette in sorted(self.trailing_plaquettes[rnd]):
                coord = plaquette.coord
                detector_qubit_tuples.append(
                    tuple(self.coord_to_qubit[coord + i*step].index for i in range(3)))

            self.detectors[1-rnd].extend(detector_qubit_tuples)
            step = -1*step  # update for odd round

    def edge_detectors_early_swap(self):
        # detectors which start as single qubits on leading edge have one qubit in first round and end as "almonds" along trailing edge
        for rnd in range(2):
            detector_qubit_tuples = []
            for plaquette in sorted(self.leading_plaquettes[rnd]):
                qubit_list = [plaquette.ancilla.index]
                qubit_list.extend(qubit.index for qubit in plaquette.ordered_data_qubits if qubit is not None)
                detector_qubit_tuples.append(tuple(qubit_list))

            self.detectors[1-rnd].extend(detector_qubit_tuples)

        # detectors which start as single qubits on trailing edge have one qubit in first round and one qubit in second round
        for rnd in range(2):
            detector_qubit_tuples = []
            for plaquette in sorted(self.trailing_plaquettes[rnd]):
                q0 = plaquette.ancilla
                q1_idx = q0.ordered_plaquettes[-1].ancilla.index
                detector_qubit_tuples.append((q0.index, q1_idx))

            self.detectors[1-rnd].extend(detector_qubit_tuples)

    def final_detectors_late_swap(self):
        final_detectors_z = []
        final_detectors_x = []

        # bulk detectors
        steps = self.Z_DELTAS  # order doesn't matter here
        for plaq in sorted(self.bulk_plaquettes[1]):
            qlist = []
            coord = plaq.coord + Coord(-1,-1)
            qlist.append(self.coord_to_qubit[coord].index)
            for step in steps:
                qlist.append(self.coord_to_qubit[coord + step].index)
            if plaq.type == 'Z':
                final_detectors_z.append(tuple(qlist))
            else:
                final_detectors_x.append(tuple(qlist))

        # leading edge
        plaq_list = sorted(
            set(plaq for plaq_list in self.initial_plaquettes for plaq in plaq_list)
            & set(self.trailing_plaquettes[0]))
        for plaq in plaq_list:
            qlist = [q.index for q in plaq.ordered_data_qubits if q is not None]
            if plaq.type == 'Z':
                final_detectors_z.append(tuple(qlist))
            else:
                final_detectors_x.append(tuple(qlist))

        # trailing edge
        step = Coord(-1,-1)
        for plaq0 in sorted(self.trailing_plaquettes[1]):
            coord = plaq0.coord
            q0 = self.coord_to_qubit[coord + step]
            qlist = [q0.index]
            for plaq1 in q0.ordered_plaquettes:
                if plaq1 is None:
                    continue
                qlist.append(plaq1.ancilla.index)
            if plaq0.type == 'Z':
                final_detectors_z.append(tuple(qlist))
            else:
                final_detectors_x.append(tuple(qlist))

        return [final_detectors_z, final_detectors_x]

    def final_detectors_early_swap(self):
        final_detectors_z = []
        final_detectors_x = []

        for plaq in sorted(self.plaquettes[1] - self.leakage_plaquettes):
            qlist = [q.index for q in plaq.ordered_data_qubits if q is not None]
            qlist.append(plaq.ancilla.index)
            if plaq.type == 'Z':
                final_detectors_z.append(tuple(qlist))
            else:
                final_detectors_x.append(tuple(qlist))

        return [final_detectors_z, final_detectors_x]

    def get_logicals(self):
        # there are some qubits where we need full meas histories and some we need only last meas
        # this is because of virtual classical-control swap in the late walking surface code. Not necessary in early walking surface code!
        d = self.d
        swap_time = self.swap_time

        z_logical_last = []
        x_logical_last = []
        z_logical_all = []
        x_logical_all = []

        if swap_time == "late":
            # qubits where all measurements are needed
            step = Coord(-1,-1)
            for plaq in sorted(self.z_plaquettes[0]):
                coord = plaq.coord
                if coord[0] == 2:
                    z_logical_all.append(plaq.ancilla.index)
                    z_logical_all.append(self.coord_to_qubit[coord + step].index)
            for plaq in sorted(self.x_plaquettes[0]):
                coord = plaq.coord
                if coord[1] == 4:
                    x_logical_all.append(plaq.ancilla.index)
                    x_logical_all.append(self.coord_to_qubit[coord + step].index)

        # qubits where only last measurement is needed
        dq_list = self.data_qubits[0] if swap_time == "late" else self.data_qubits[1]
        for qubit in dq_list:
            if (qubit.coord[0] == 1) and (qubit.index not in z_logical_all):
                z_logical_last.append(qubit.index)
            if (qubit.coord[1] == 3) and (qubit.index not in x_logical_all):
                x_logical_last.append(qubit.index)

        return ([tuple(z_logical_last), tuple(z_logical_all)],
                [tuple(x_logical_last), tuple(x_logical_all)])


# ---------------------------------------------------------------------------
# Circuit emission with stimside leakage tags (not from the reference repo).
#
# The circuits follow surface-code-leakage-erasure's CircuitBuilder with its
# defaults for the paper's Fig. 3 noise model: basis Z, Pauli noise only after
# CX gates (``DEPOLARIZE2(p_pauli)``), leakage only before CX gates (one of the
# two qubits, ``p_leak`` per gate), and the skip-gate leaked-qubit effect, which
# stimside's ``LeakageUint8`` / ``LeakageUint8Coset`` op handlers implement with
# their default ``unconditional_condition_on_U=True`` (untagged gates and noise
# skip pairs with a leaked qubit). An erasure check is an
# ``MPAD[LEAKAGE_MEASUREMENT]`` flag followed by unleaking (which depolarizes).
# ---------------------------------------------------------------------------

import stim  # noqa: E402

KINDS = ("static", "moonwalking")
EC_SCHEDULES = (4, 8)


def _cnot_pair(plaquette, t, reverse=False):
    dq = plaquette.ordered_data_qubits[t]
    if dq is None:
        return []
    pair = [plaquette.ancilla.index, dq.index] if plaquette.type == "X" else [dq.index, plaquette.ancilla.index]
    return pair[::-1] if reverse else pair


class _Round:
    """The qubits of one syndrome extraction round (one per round parity)."""

    def __init__(self, reset, rx, cnots, mx, measure, ec8):
        self.reset, self.rx, self.cnots, self.mx, self.measure, self.ec8 = (
            reset, rx, cnots, mx, measure, ec8)


def _static_rounds(layout):
    x = sorted(layout.x_plaquettes)
    z = sorted(layout.z_plaquettes)
    cnots = [[q for p in x + z for q in _cnot_pair(p, t)] for t in range(4)]
    x_idx = [p.ancilla.index for p in x]
    z_idx = [p.ancilla.index for p in z]
    measure = sorted(x_idx + z_idx)
    # EC schedule 8: a leakage-SWAP moves each data qubit's leak to the ancilla
    # of the plaquette whose first CX it is in, or to an extra erasure ancilla
    # (a qubit outside the stim circuit that is only ever erasure checked).
    swaps = {}
    for p in x + z:
        dq = p.ordered_data_qubits[0]
        if dq is not None:
            swaps[dq.index] = p.ancilla.index
    n = len(layout.qubits)
    rest = sorted(q.index for q in layout.data_qubits if q.index not in swaps)
    swaps.update({q: n + i for i, q in enumerate(rest)})
    ec8 = measure + [n + i for i in range(len(rest))]
    rnd = _Round(x_idx + z_idx, x_idx, cnots, x_idx, measure, ec8)
    return [rnd, rnd], sorted(swaps.items())


def _moonwalking_rounds(layout):
    rounds = []
    for r in range(2):
        plaqs = sorted(layout.x_plaquettes[r]) + sorted(layout.z_plaquettes[r])
        cnots = [[q for p in plaqs for q in _cnot_pair(p, t, reverse=(t == 0))] for t in range(4)]
        measure = sorted(layout.measure_indexes[r])
        rounds.append(_Round(layout.reset_indexes[r], layout.rx_indexes[r], cnots,
                             layout.mx_indexes[r], measure, measure))
    return rounds


class _Records:
    """Measurement record bookkeeping (``rec[-k]`` targets of each qubit's results)."""

    def __init__(self):
        self.history = {}
        self.count = 0

    def add(self, qubits):
        for q in qubits:
            self.history.setdefault(q, []).append(self.count)
            self.count += 1

    def add_anonymous(self, num):
        self.count += num

    def last(self, q):
        return stim.target_rec(self.history[q][-1] - self.count)

    def every(self, q):
        return [stim.target_rec(k - self.count) for k in self.history[q]]


def paper_circuit(
    kind: str,
    distance: int,
    rounds: int,
    p_pauli: float,
    p_leak: float,
    ec_sched: int = 8,
) -> stim.Circuit:
    """A memory-Z circuit of the paper with stimside leakage tags.

    Args:
        kind: ``"static"`` (the rotated surface code) or ``"moonwalking"``.
        distance: code distance (odd for moonwalking).
        rounds: syndrome extraction rounds (even for moonwalking; the paper
            uses ``3 * distance + 1``).
        p_pauli: ``DEPOLARIZE2`` strength after each CX.
        p_leak: probability that one qubit (either, with equal odds) of a CX
            leaks just before it.
        ec_sched: 4 checks every qubit after the 4th CX layer of each round; 8
            checks only the qubits measured at the end of the round (for the
            static code, each round starts with a leakage-SWAP of every data
            qubit's leak onto an ancilla). Data qubits are also checked at the
            final readout.
    """
    if kind not in KINDS:
        raise ValueError(f"kind must be one of {KINDS}, got {kind!r}.")
    if ec_sched not in EC_SCHEDULES:
        raise ValueError(f"ec_sched must be one of {EC_SCHEDULES}, got {ec_sched!r}.")
    if kind == "static":
        layout = SurfaceCodeLayout(distance)
        spec, swaps = _static_rounds(layout)
        data = sorted(q.index for q in layout.data_qubits)
    else:
        if rounds % 2:
            raise ValueError("The moonwalking circuit needs an even number of rounds.")
        layout = WalkingSurfaceCodeLayout(distance, swap_time="early")
        spec, swaps = _moonwalking_rounds(layout), []
        data = sorted(q.index for q in layout.data_qubits[1])
    qubits = sorted(layout.qubits)
    all_qubits = [q.index for q in qubits]
    leak = f"LEAKAGE_TRANSITION_2: ({p_leak / 2}, U_U-->2_U) ({p_leak / 2}, U_U-->U_2)"
    rec = _Records()
    c = stim.Circuit()

    def erasure_check(targets):
        c.append("MPAD", [0] * len(targets), tag="LEAKAGE_MEASUREMENT: (1.0, 2): " + " ".join(map(str, targets)))
        c.append("I_ERROR", targets, tag="LEAKAGE_TRANSITION_1: (1.0, 2-->U)")
        rec.add_anonymous(len(targets))

    for q in qubits:
        c.append("QUBIT_COORDS", [q.index], [q.coord[0], q.coord[1]])
    c.append("R", data)
    for r in range(rounds):
        s = spec[r % 2]
        if ec_sched == 8 and swaps and r > 0 and p_leak > 0:
            c.append("II_ERROR", [q for pair in swaps for q in pair], tag="LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)")
        c.append("R", s.reset)
        if s.rx:
            c.append("H", s.rx)
        c.append("TICK")
        for t in range(4):
            if p_leak > 0:
                c.append("II_ERROR", s.cnots[t], tag=leak)
            c.append("CX", s.cnots[t])
            if p_pauli > 0:
                c.append("DEPOLARIZE2", s.cnots[t], [p_pauli])
            c.append("TICK")
        if p_leak > 0:
            erasure_check(all_qubits if ec_sched == 4 else s.ec8)
        if s.mx:
            c.append("H", s.mx)
        c.append("M", s.measure)
        rec.add(s.measure)
        c.append("TICK")
        if kind == "static":
            plaqs = sorted(layout.z_plaquettes) if r == 0 else sorted(layout.x_plaquettes) + sorted(layout.z_plaquettes)
            for p in plaqs:
                targets = [rec.last(p.index)]
                if r > 0:
                    targets.append(stim.target_rec(rec.history[p.index][-2] - rec.count))
                c.append("DETECTOR", targets)
        else:
            dets = layout.initial_detectors[0] if r == 0 else layout.detectors[r % 2]
            for qs in dets:
                c.append("DETECTOR", [rec.last(q) for q in qs])
    if p_leak > 0:
        erasure_check(data)
    c.append("M", data)
    rec.add(data)
    if kind == "static":
        for p in sorted(layout.z_plaquettes):
            qs = [p.index] + [dq.index for dq in p.ordered_data_qubits if dq is not None]
            c.append("DETECTOR", [rec.last(q) for q in qs])
        c.append("OBSERVABLE_INCLUDE", [rec.last(q.index) for q in sorted(layout.z_logical)], [0])
    else:
        for qs in layout.final_detectors[0]:
            c.append("DETECTOR", [rec.last(q) for q in qs])
        last, every = layout.z_logical
        c.append("OBSERVABLE_INCLUDE", [rec.last(q) for q in last] + [t for q in every for t in rec.every(q)], [0])
    return c
