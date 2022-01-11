#!/bin/bash

#########################################################
# Sets version numbers to a date-based dev version.
#########################################################
# Example usage (from repo root):
#
# ./glue/python/overwrite_dev_version_with_date.sh
#########################################################
echo STEP 0
false

set -e

echo STEP 1
# Get to the repo root.
cd "$(dirname "${BASH_SOURCE[0]}")"
echo STEP 2
cd "$(git rev-parse --show-toplevel)"
echo STEP 3

# Generate dev version starting from major.minor version.
# (Requires the existing version to have a 'dev' suffix.)
# (Uses the timestamp of the HEAD commit, to ensure consistency when run multiple times.)
echo STEP 4
readonly MAJOR_MINOR_VERSION="$(cat setup.py | sed -n "s/version.*=.*'\(.*\)\.dev.*'/\1/p")"
echo STEP 5 ${MAJOR_MINOR_VERSION}
readonly TIMESTAMP="$(git show -s --format=%ct HEAD)"
echo STEP 6 ${TIMESTAMP}
readonly DEV_VERSION="${MAJOR_MINOR_VERSION}.dev${TIMESTAMP}"

# Overwrite existing versions.
echo STEP 7 ${DEV_VERSION}
declare -a setup_file_paths=("setup.py" "glue/cirq/setup.py" "glue/zx/setup.py")
for setup_file_path in "${setup_file_paths[@]}"; do
  sed -e "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g" > "${setup_file_path}_tmp" < "${setup_file_path}"
  mv "${setup_file_path}_tmp" "${setup_file_path}"
done
echo STEP 8
