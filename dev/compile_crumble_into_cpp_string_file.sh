#!/bin/bash
set -e

# Get to this script's git repo root.
cd "$( dirname "${BASH_SOURCE[0]}" )"
cd "$(git rev-parse --show-toplevel)"

echo '#include "stim/diagram/crumble_data.h"'
echo '';
echo 'std::string stim_draw_internal::make_crumble_html() {'
echo '    std::string result;'
dev/compile_crumble_into_single_html_page.sh | python -c '
import sys
for line in sys.stdin:
    for k in range(0, len(line), 1024):
        part = line[k:k+1024]
        print(f"""    result.append(R"CRUMBLE_PART({part})CRUMBLE_PART");""")';
echo '    return result;'
echo '}'
