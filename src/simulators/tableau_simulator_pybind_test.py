# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import pytest
import stim


def test_basic():
    s = stim.TableauSimulator()
    assert s.measure(0) is False
    assert s.measure(0) is False
    s.x(0)
    assert s.measure(0) is True
    assert s.measure(0) is True
    s.reset(0)
    assert s.measure(0) is False
    s.h(0)
    s.h(0)
    s.sqrt_x(1)
    s.sqrt_x(1)
    assert s.measure_many(0, 1) == [False, True]


def test_access_tableau():
    s = stim.TableauSimulator()
    assert s.current_inverse_tableau() == stim.Tableau(0)

    s.h(0)
    assert s.current_inverse_tableau() == stim.Tableau.from_named_gate("H")

    s.h(0)
    assert s.current_inverse_tableau() == stim.Tableau(1)

    s.h(1)
    s.h(1)
    assert s.current_inverse_tableau() == stim.Tableau(2)

    s.h(2)
    assert s.current_inverse_tableau() == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("X__"),
            stim.PauliString("_X_"),
            stim.PauliString("__Z"),
        ],
        zs=[
            stim.PauliString("Z__"),
            stim.PauliString("_Z_"),
            stim.PauliString("__X"),
        ],
    )


@pytest.mark.parametrize("name", [
    "x",
    "y",
    "z",
    "h",
    "h_xy",
    "h_yz",
    "sqrt_x",
    "sqrt_x_dag",
    "sqrt_y",
    "sqrt_y_dag",
    "s",
    "s_dag",
    "swap",
    "iswap",
    "iswap_dag",
    "xcx",
    "xcy",
    "xcz",
    "ycx",
    "ycy",
    "ycz",
    "cnot",
    "cy",
    "cz",
])
def test_gates_present(name: str):
    t = stim.Tableau.from_named_gate(name)
    n = len(t)
    s1 = stim.TableauSimulator()
    s2 = stim.TableauSimulator()
    for k in range(n):
        s1.h(k)
        s2.h(k)
        s1.cnot(k, k + n)
        s2.cnot(k, k + n)
    getattr(s1, name)(*range(n))
    s2.do(stim.Circuit(f"{name} " + " ".join(str(e) for e in range(n))))
    assert s1.current_inverse_tableau() == s2.current_inverse_tableau()
