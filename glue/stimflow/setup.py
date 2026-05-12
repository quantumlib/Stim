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

with open('README.md', encoding='UTF-8') as f:
    long_description = f.read()
with open('requirements.txt', encoding='UTF-8') as f:
    requirements = f.read().splitlines()

__version__ = '1.16.dev0'

setup(
    name='stimflow',
    version=__version__,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/quantumlib/stim',
    license='Apache 2',
    packages=['stimflow'],
    package_dir={'': 'src'},
    description='A library for creating quantum error correction circuits.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    python_requires='>=3.6.0',
    data_files=['README.md', 'requirements.txt'],
    install_requires=requirements,
    tests_require=['pytest', 'python3-distutils'],
)
