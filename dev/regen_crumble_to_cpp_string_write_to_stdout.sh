#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

{
    echo '#include "stim/diagram/crumble_data.h"';
    echo '';
    echo 'std::string stim_draw_internal::make_crumble_html() {';
    echo '    std::string result;'
    {
        cat glue/crumble/crumble.html | grep -v "^<script";
        echo "<script>";
        # HACK: this temp file is to work around https://github.com/rollup/rollup/issues/5097
        rollup glue/crumble/main.js > tmp_crumble.tmp
        uglifyjs -c -m --mangle-props --toplevel < tmp_crumble.tmp;
        rm tmp_crumble.tmp
        echo "</script>";
    } | python -c '
import sys
for line in sys.stdin:
    for k in range(0, len(line), 1024):
        part = line[k:k+1024]
        print(f"""    result.append(R"CRUMBLE_PART({part})CRUMBLE_PART");""")';
    echo '    return result;'
    echo '}';
}
