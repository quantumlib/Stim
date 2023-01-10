#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

{
    echo '#include "stim/diagram/crumble_data.h"';
    echo '';
    echo 'std::string stim_draw_internal::make_crumble_html() {';
    echo '    return R"CRUMBLE_HTML(';
    cat glue/crumble/main.html | grep -v "^<script";
    echo "<script>";
    rollup glue/crumble/main.js | uglifyjs -c -m --mangle-props --toplevel;
    echo "</script>";
    echo '    )CRUMBLE_HTML";';
    echo '}';
} > src/stim/diagram/crumble_data.cc
