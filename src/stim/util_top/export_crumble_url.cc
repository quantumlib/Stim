#include "stim/util_top/export_crumble_url.h"

using namespace stim;

std::string stim::export_crumble_url(const Circuit &circuit) {
    auto s = circuit.str();
    std::string_view s_view = s;
    std::vector<std::pair<std::string_view, std::string_view>> replace_rules{
        {"QUBIT_COORDS", "Q"},
        {", ", ","},
        {") ", ")"},
        {"    ", ""},
        {" ", "_"},
        {"\n", ";"},
    };
    std::stringstream out;
    out << "https://algassert.com/crumble#circuit=";
    for (size_t k = 0; k < s.size(); k++) {
        std::string_view v = s_view.substr(k);
        bool matched = false;
        for (auto [src, dst] : replace_rules) {
            if (v.starts_with(src)) {
                out << dst;
                k += src.size() - 1;
                matched = true;
                break;
            }
        }
        if (!matched) {
            out << s[k];
        }
    }
    return out.str();
}
