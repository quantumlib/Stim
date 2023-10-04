from setuptools import find_packages, setup

with open('README.md', encoding='UTF-8') as f:
  long_description = f.read()

__version__ = '0.1.0'

setup(
    name='olssco',
    version=__version__,
    author='Daniel Bochen Tan',
    author_email='tbcdebug@outlook.com',
    url='https://github.com/quantumlib/stim/',
    license='Apache 2',
    packages=find_packages(),
    description='A Compiler for Lattice Surgery Subroutines',
    long_description=long_description,
    long_description_content_type='text/markdown',
    python_requires='>=3.6.0',
    data_files=['README.md'],
    install_requires=[
        'z3-solver==4.12.1.0',
        'stim',
        'networkx',
    ],
    tests_require=['pytest', 'python3-distutils'],
)
