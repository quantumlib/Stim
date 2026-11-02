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


def test_tableau_sampler_basic():
    sampler = stim.TableauSampler(4, seed=12345)
    t = sampler.next_tableau()
    assert isinstance(t, stim.Tableau)
    assert len(t) == 4


def test_tableau_sampler_seed_deterministic():
    s1 = stim.TableauSampler(4, seed=42)
    s2 = stim.TableauSampler(4, seed=42)
    for _ in range(10):
        assert s1.next_tableau() == s2.next_tableau()


def test_tableau_sampler_different_seeds_diverge():
    s1 = stim.TableauSampler(4, seed=42)
    s2 = stim.TableauSampler(4, seed=99)
    assert s1.next_tableau() != s2.next_tableau()


def test_tableau_sampler_successive_calls_differ():
    sampler = stim.TableauSampler(5, seed=1)
    t1 = sampler.next_tableau()
    t2 = sampler.next_tableau()
    assert t1 != t2


def test_tableau_sampler_no_seed():
    s1 = stim.TableauSampler(4)
    s2 = stim.TableauSampler(4)
    assert s1.next_tableau() != s2.next_tableau()


def test_tableau_sampler_repr():
    sampler = stim.TableauSampler(3, seed=7)
    r = repr(sampler)
    assert "stim.TableauSampler" in r
    assert "3" in r


def test_tableau_sampler_num_qubits_consistent():
    sampler = stim.TableauSampler(3, seed=123)
    for _ in range(5):
        assert len(sampler.next_tableau()) == 3
