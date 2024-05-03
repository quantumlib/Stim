#ifndef _STIM_IO_STIM_DATA_FORMATS_H
#define _STIM_IO_STIM_DATA_FORMATS_H

#include <map>
#include <string>

namespace stim {

enum class SampleFormat {
    SAMPLE_FORMAT_01,
    SAMPLE_FORMAT_B8,
    SAMPLE_FORMAT_PTB64,
    SAMPLE_FORMAT_HITS,
    SAMPLE_FORMAT_R8,
    SAMPLE_FORMAT_DETS,
};

struct FileFormatData {
    const char *name;
    SampleFormat id;
    const char *help;
    const char *help_python_save;
    const char *help_python_parse;
};

const std::map<std::string_view, FileFormatData> &format_name_to_enum_map();

}  // namespace stim

#endif
