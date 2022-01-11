#!/bin/bash

#########################################################
# Sets version numbers to a date-based dev version.
#########################################################
# Example usage (from repo root):
#
# ./glue/python/overwrite_dev_version_with_date.sh
#########################################################

set -e
set -x

# Get to the repo root.
cd "$(dirname "${BASH_SOURCE[0]}")"
cd "$(git rev-parse --show-toplevel)"

# Generate dev version starting from major.minor version.
# (Requires the existing version to have a 'dev' suffix.)
readonly MAJOR_MINOR_VERSION="$(cat setup.py | sed -n "s/version.*=.*'\(.*\)\.dev.*'/\1/p")"
readonly DEV_VERSION="${MAJOR_MINOR_VERSION}.dev$(date "+%Y%m%d%H%M%S")"

# Overwrite existing versions.
ls .
sed "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g" -i setup.py
ls glue
ls glue/cirq
sed "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g" -i glue/cirq/setup.py
ls glue/zx
sed "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g" -i glue/zx/setup.py
