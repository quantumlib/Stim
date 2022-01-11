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
# (Uses the timestamp of the HEAD commit, to ensure consistency when run multiple times.)
readonly MAJOR_MINOR_VERSION="$(cat setup.py | sed -n "s/version.*=.*'\(.*\)\.dev.*'/\1/p")"
readonly TIMESTAMP="$(date -d @"$(git show -s --format=%ct HEAD)" "+%Y%m%d%H%M%S")"
readonly DEV_VERSION="${MAJOR_MINOR_VERSION}.dev${TIMESTAMP}"

# Overwrite existing versions.
ls .
echo STARTING SED
cat setup.py | wc
echo STARTING SED
sed -i setup.py -e "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g"
ls glue
ls glue/cirq
echo STARTING SED GLUE CIRQ
cat glue/cirq/setup.py | wc
echo STARTING SED GLUE CIRQ
sed -i glue/cirq/setup.py -e "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g"
ls glue/zx
echo STARTING SED GLUE ZX
cat glue/zx/setup.py | wc
echo STARTING SED GLUE ZX
sed -i glue/zx/setup.py -e "s/version.*=.*'.*'/version = '${DEV_VERSION}'/g"
