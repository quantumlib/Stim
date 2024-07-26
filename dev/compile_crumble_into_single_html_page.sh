#!/bin/bash
set -e

cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

cat glue/crumble/crumble.html | grep -v "^<script";
echo "<script>";
# HACK: this temp file is to work around https://github.com/rollup/rollup/issues/5097
rollup glue/crumble/main.js --silent > tmp_crumble.tmp
uglifyjs -c -m --mangle-props --toplevel < tmp_crumble.tmp;
rm tmp_crumble.tmp
echo "</script>";
