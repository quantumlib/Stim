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

import stim
import pytest


def test_init_and_equality():
    r = stim.CircuitRepeatBlock(500, stim.Circuit("X 0"))
    assert r.repeat_count == 500
    assert r.body_copy() == stim.Circuit("X 0")
    assert stim.CircuitRepeatBlock(500, stim.Circuit("X 0")) == stim.CircuitRepeatBlock(500, stim.Circuit("X 0"))
    assert stim.CircuitRepeatBlock(500, stim.Circuit("X 0")) != stim.CircuitRepeatBlock(500, stim.Circuit())
    assert stim.CircuitRepeatBlock(500, stim.Circuit("X 0")) != stim.CircuitRepeatBlock(101, stim.Circuit("X 0"))
    assert not (stim.CircuitRepeatBlock(500, stim.Circuit("X 0")) == stim.CircuitRepeatBlock(500, stim.Circuit()))
    assert not (stim.CircuitRepeatBlock(500, stim.Circuit("X 0")) != stim.CircuitRepeatBlock(500, stim.Circuit("X 0")))
    r2 = stim.CircuitRepeatBlock(repeat_count=500, body=stim.Circuit("X 0"))
    assert r == r2

    with pytest.raises(ValueError, match="repeat 0"):
        stim.CircuitRepeatBlock(0, stim.Circuit())


@pytest.mark.parametrize("value", [
    stim.CircuitRepeatBlock(500, stim.Circuit("X 0")),
    stim.CircuitRepeatBlock(1, stim.Circuit("X 0\nREPEAT 100 {\nH 1\n}\n")),
])
def test_repr(value):
    assert eval(repr(value), {'stim': stim}) == value
    assert repr(eval(repr(value), {'stim': stim})) == repr(value)
