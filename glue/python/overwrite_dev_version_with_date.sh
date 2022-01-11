#!/bin/bash

#########################################################
# Sets version numbers to a date-based dev version.
#########################################################
# Example usage (from repo root):
#
# ./glue/python/overwrite_dev_version_with_date.sh
#########################################################

set -e

# Get to the repo root.
cd "$(dirname "${BASH_SOURCE[0]}")"
cd "$(git rev-parse --show-toplevel)"

# Generate dev version starting from major.minor version.
# (Requires the existing version to have a 'dev' suffix.)
# (Uses the timestamp of the HEAD commit, to ensure consistency when run multiple times.)
readonly MAJOR_MINOR_VERSION="$(cat setup.py | sed -n "s/version.*=.*'\(.*\)\.dev.*'/\1/p")"
readonly TIMESTAMP="$(git show -s --format=%ct HEAD)"
readonly DEV_VERSION="${MAJOR_MINOR_VERSION}.dev${TIMESTAMP}"

# Overwrite existing versions.
declare -a setup_file_paths=("setup.py" "glue/cirq/setup.py" "glue/zx/setup.py")
for setup_file_path in "${setup_file_paths[@]}"; do
  sed -e "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g" > "${setup_file_path}_tmp" < "${setup_file_path}"
  mv "${setup_file_path}_tmp" "${setup_file_path}"
done
