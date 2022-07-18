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
    requirements = [line.split()[0] for line in f.read().splitlines()]

version = '1.10.dev0'

setup(
    name='sinter',
    version=version,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    license='Apache 2',
    packages=['sinter'],
    package_dir={'': 'src'},
    description='Samples stim circuits and decodes them using pymatching.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    python_requires='>=3.7.0',
    data_files=['README.md', 'requirements.txt', 'readme_example_plot.png'],
    install_requires=requirements,
    tests_require=['pytest'],
    entry_points={
        'console_scripts': ['sinter=sinter._main:main'],
    },
)
