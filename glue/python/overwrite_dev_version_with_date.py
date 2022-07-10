#!/usr/bin/env python3

#########################################################
# Sets version numbers to a date-based dev version.
#
# Does nothing if not on a dev version.
#########################################################
# Example usage (from repo root):
#
# ./glue/python/overwrite_dev_version_with_date.sh
#########################################################

import os
import pathlib
import re
import subprocess


def main():
    os.chdir(pathlib.Path(__file__).parent)
    os.chdir(subprocess.check_output(["git", "rev-parse", "--show-toplevel"]).decode().strip())

    # Generate dev version starting from major.minor version.
    # (Requires the existing version to have a 'dev' suffix.)
    # (Uses the timestamp of the HEAD commit, to ensure consistency when run multiple times.)
    with open('setup.py') as f:
        maj_min_version_line, = [line for line in f.read().splitlines() if re.match("^version = '[^']+'", line)]
        maj_version, min_version, patch = maj_min_version_line.split()[-1].strip("'").split('.')
        if 'dev' not in patch:
            return  # Do nothing for non-dev versions.
    timestamp = subprocess.check_output(['git', 'show', '-s', '--format=%ct', 'HEAD']).decode().strip()
    new_version = f"{maj_version}.{min_version}.dev{timestamp}"

    # Overwrite existing versions.
    package_setup_files = [
        "setup.py",
        "glue/cirq/setup.py",
        "glue/zx/setup.py",
        "glue/sample/setup.py",
    ]
    for path in package_setup_files:
        with open(path) as f:
            content = f.read()
        assert maj_min_version_line in content
        content = content.replace(maj_min_version_line, f"version = '{new_version}'")
        with open(path, 'w') as f:
            print(content, file=f, end='')


if __name__ == '__main__':
    main()
