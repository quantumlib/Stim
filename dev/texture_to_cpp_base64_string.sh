#!/bin/bash
set -e

#########################################################################
# Transforms binary data into an array of string literals containing the
# base 64 data encoding the data. The strings are split into an array
# to avoid exceeding C++'s maximum string literal length.
#########################################################################

echo '#include "stim/diagram/gate_data_3d_texture_data.h"'
echo ''
echo 'std::string stim_draw_internal::make_gate_3d_texture_data_uri() {'
echo '    std::string result;'
echo '    result.append("data:image/png;base64,");'
base64 -w 1024 | sed 's/.*/    result.append("\0");/g'
echo '    return result;'
echo '}'
