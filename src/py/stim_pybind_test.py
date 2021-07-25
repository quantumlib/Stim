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
import re


def test_version():
    assert re.match(r"^\d\.\d+", stim.__version__)


def test_targets():
    assert stim.target_x(5) & 0xFFFF == 5
    assert stim.target_y(5) == (stim.target_x(5) | stim.target_z(5))
    assert stim.target_z(5) & 0xFFFF == 5
    assert stim.target_inv(5) & 0xFFFF == 5
    assert stim.target_rec(-5) & 0xFFFF == 5
    assert stim.target_separator() & 0xFFFF == 0
    assert stim.target_combiner() & 0xFFFF == 0
