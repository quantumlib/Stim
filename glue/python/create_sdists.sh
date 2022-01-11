#!/bin/bash

#########################################################
# Sets version numbers and produces python distributions
#########################################################
# Example usage (from repo root):
#
# ./glue/python/create_sdists.sh VERSION_STRING
#########################################################

set -e

if [ -z "$1" ]; then
  echo "Provide a version argument like '1.2.0' or '1.2.dev0'."
  exit 1
fi

TMP_DIR="$(mktemp -d)"
function cleanup {
  rm  -r "${TMP_DIR}"
}
trap cleanup EXIT

python -m venv "${TMP_DIR}"
source "${TMP_DIR}/bin/activate"
cd "$(git rev-parse --show-toplevel)"

sed "s/version.*=.*'.*'/version = '$1'/g" -i setup.py
sed "s/version.*=.*'.*'/version = '$1'/g" -i glue/cirq/setup.py
sed "s/version.*=.*'.*'/version = '$1'/g" -i glue/zx/setup.py

python -m pip install pybind11
python setup.py sdist
cd glue/cirq
python setup.py sdist
cd ../..
cp glue/cirq/dist/* dist/
