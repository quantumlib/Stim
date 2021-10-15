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

from setuptools import setup

with open('README.md') as f:
    long_description = f.read()

version = '1.6.0'

setup(
    name='stimcirq',
    version=version,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/quantumlib/stim',
    license='Apache 2',
    packages=['stimcirq'],
    description='Implements a cirq.Sampler backed by stim.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    python_requires='>=3.6.0',
    data_files=['README.md'],
    install_requires=['stim', 'cirq-core'],
    tests_require=['pytest', 'python3-distutils'],
)
