#include "stim/diagram/crumble.h"

#include "stim/diagram/crumble_data.h"

using namespace stim;
using namespace stim_draw_internal;

void stim_draw_internal::write_crumble_html_with_preloaded_circuit(const Circuit &circuit, std::ostream &out) {
    auto html = make_crumble_html();
    const char *indicator = "[[[DEFAULT_CIRCUIT_CONTENT_LITERAL]]]";
    size_t start = html.find(indicator);
    out << html.substr(0, start);
    out << circuit;
    out << html.substr(start + strlen(indicator));
}
