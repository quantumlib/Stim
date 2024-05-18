from setuptools import find_packages, setup

with open('README.md', encoding='UTF-8') as f:
    long_description = f.read()

__version__ = '0.1.0'

setup(
    name='LaSsynth',
    version=__version__,
    author='',
    author_email='',
    url='',
    license='Apache 2',
    packages=find_packages(),
    description='Lattice Surgery Subroutine Synthesizer',
    long_description=long_description,
    long_description_content_type='text/markdown',
    python_requires='>=3.6.0',
    data_files=['README.md'],
    install_requires=[
        'z3-solver==4.12.1.0',
        'stim',
        'networkx',
        'ipykernel',
    ],
    tests_require=['pytest', 'python3-distutils'],
)
