#include "stim/diagram/detector_slice/detector_slice_animation.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "stim/diagram/base64.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

namespace {

constexpr size_t MIN_BOUNDARY_SAMPLES = 64;
constexpr size_t MAX_BOUNDARY_SAMPLES = 256;
constexpr size_t ESTIMATED_TRANSITION_BYTES = 1400;

enum class DetectorSliceSvgPathKind {
    CIRCLE,
    PATH,
};

struct DetectorSliceSvgPathSegment {
    bool is_line;
    Coord<2> control1;
    Coord<2> control2;
    Coord<2> end;
};

struct DetectorSliceSvgPath {
    DetectorSliceSvgPathKind kind;
    Coord<2> start;
    float radius;
    std::vector<DetectorSliceSvgPathSegment> segments;
};

struct DiagramTimelineSvgFrame {
    uint64_t tick;
    std::string svg;
    struct DetectorRegion {
        DetectorSliceSvgPath path;
        std::string style;
    };
    std::map<uint64_t, DetectorRegion> detector_regions;
};

struct StringBuffer : std::streambuf {
    std::string data;

    explicit StringBuffer(size_t capacity) {
        data.reserve(capacity);
    }

    std::streamsize xsputn(const char *s, std::streamsize n) override {
        data.append(s, n);
        return n;
    }

    int overflow(int c) override {
        if (c != traits_type::eof()) {
            data.push_back((char)c);
        }
        return traits_type::not_eof(c);
    }
};

std::string_view attribute(std::string_view tag, std::string_view name) {
    size_t start = tag.find(name);
    while (start != std::string_view::npos) {
        size_t equals = start + name.size();
        if (equals + 1 < tag.size() && tag[equals] == '=' && tag[equals + 1] == '"' &&
            (start == 0 || std::isspace((uint8_t)tag[start - 1]))) {
            start = equals + 2;
            size_t end = tag.find('"', start);
            if (end != std::string_view::npos) {
                return tag.substr(start, end - start);
            }
        }
        start = tag.find(name, start + 1);
    }
    return {};
}

float parse_float(std::string_view text) {
    std::string copy(text);
    char *end;
    float result = strtof(copy.c_str(), &end);
    if (end != copy.c_str() + copy.size()) {
        throw std::invalid_argument("Malformed number in detector slice SVG.");
    }
    return result;
}

struct SvgPathReader {
    std::string text;
    const char *p;

    explicit SvgPathReader(std::string_view text) : text(text), p(this->text.c_str()) {
    }

    void skip_separators() {
        while (*p == ' ' || *p == ',') {
            p++;
        }
    }

    float read_float() {
        skip_separators();
        char *end;
        float result = strtof(p, &end);
        if (end == p) {
            throw std::invalid_argument("Malformed detector slice SVG path.");
        }
        p = end;
        return result;
    }

    Coord<2> read_coord() {
        return {read_float(), read_float()};
    }
};

DetectorSliceSvgPath parse_shape(std::string_view tag) {
    if (tag.substr(0, 7) == "<circle") {
        return {
            DetectorSliceSvgPathKind::CIRCLE,
            {parse_float(attribute(tag, "cx")), parse_float(attribute(tag, "cy"))},
            parse_float(attribute(tag, "r")),
            {},
        };
    }

    SvgPathReader reader(attribute(tag, "d"));
    reader.skip_separators();
    if (*reader.p++ != 'M') {
        throw std::invalid_argument("Malformed detector slice SVG path.");
    }
    DetectorSliceSvgPath result{DetectorSliceSvgPathKind::PATH, reader.read_coord(), 0, {}};
    while (true) {
        reader.skip_separators();
        char command = *reader.p++;
        if (command == 0 || command == 'Z') {
            break;
        }
        if (command == 'L') {
            result.segments.push_back({true, {}, {}, reader.read_coord()});
        } else if (command == 'C') {
            auto control1 = reader.read_coord();
            auto control2 = reader.read_coord();
            result.segments.push_back({false, control1, control2, reader.read_coord()});
        } else {
            throw std::invalid_argument("Unsupported detector slice SVG path command.");
        }
    }
    return result;
}

std::string style_signature(std::string_view group, std::string_view shape_tag) {
    std::stringstream out;
    out << attribute(shape_tag, "fill") << ':' << attribute(shape_tag, "fill-opacity");
    size_t start = 0;
    while ((start = group.find("<circle", start)) != std::string_view::npos) {
        size_t end = group.find('>', start);
        if (end == std::string_view::npos) {
            break;
        }
        auto tag = group.substr(start, end - start + 1);
        auto fill = attribute(tag, "fill");
        if (fill.find("grad") != std::string_view::npos) {
            out << ':' << attribute(tag, "cx") << ',' << attribute(tag, "cy") << ',' << attribute(tag, "r") << ','
                << fill;
        }
        start = end + 1;
    }
    return out.str();
}

DiagramTimelineSvgFrame parse_frame(uint64_t tick, std::string svg) {
    DiagramTimelineSvgFrame result{tick, std::move(svg), {}};
    size_t start = 0;
    while ((start = result.svg.find("<g id=\"slice:", start)) != std::string::npos) {
        size_t id_start = start + strlen("<g id=\"slice:");
        size_t id_end = id_start;
        while (id_end < result.svg.size() && std::isdigit((uint8_t)result.svg[id_end])) {
            id_end++;
        }
        size_t group_end = result.svg.find("</g>", id_end);
        if (group_end == std::string::npos) {
            throw std::invalid_argument("Malformed detector slice SVG group.");
        }
        if (id_end > id_start && id_end < result.svg.size() && result.svg[id_end] == ':') {
            uint64_t id = std::stoull(result.svg.substr(id_start, id_end - id_start));
            auto group = std::string_view(result.svg).substr(start, group_end + 4 - start);
            size_t path_start = group.find("<path");
            size_t circle_start = group.find("<circle");
            size_t shape_start = std::min(path_start, circle_start);
            if (path_start == std::string_view::npos) {
                shape_start = circle_start;
            } else if (circle_start == std::string_view::npos) {
                shape_start = path_start;
            }
            size_t shape_end = group.find('>', shape_start);
            if (shape_start == std::string_view::npos || shape_end == std::string_view::npos) {
                throw std::invalid_argument("Missing detector slice SVG shape.");
            }
            auto shape_tag = group.substr(shape_start, shape_end - shape_start + 1);
            auto inserted = result.detector_regions.insert(
                {id, {parse_shape(shape_tag), style_signature(group, shape_tag)}});
            if (!inserted.second) {
                throw std::invalid_argument("Duplicate detector slice SVG group.");
            }
        }
        start = group_end + 4;
    }
    return result;
}

std::vector<DiagramTimelineSvgFrame> make_frames(
    const Circuit &circuit, uint64_t tick_slice_start, uint64_t tick_slice_num, SpanRef<const CoordFilter> filter) {
    uint64_t circuit_num_ticks = circuit.count_ticks();
    if (tick_slice_start > circuit_num_ticks) {
        return {};
    }
    tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start + 1);
    if (!circuit.operations.empty() && circuit.operations.back().gate_type == GateType::TICK) {
        tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start);
    }

    std::vector<DiagramTimelineSvgFrame> result;
    result.reserve(tick_slice_num);
    for (uint64_t k = 0; k < tick_slice_num; k++) {
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit,
            out,
            tick_slice_start + k,
            1,
            DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE,
            filter);
        result.push_back(parse_frame(tick_slice_start + k, out.str()));
    }
    return result;
}

bool same_path(const DetectorSliceSvgPath &a, const DetectorSliceSvgPath &b) {
    if (a.kind != b.kind || a.start != b.start || a.radius != b.radius || a.segments.size() != b.segments.size()) {
        return false;
    }
    for (size_t k = 0; k < a.segments.size(); k++) {
        const auto &x = a.segments[k];
        const auto &y = b.segments[k];
        if (x.is_line != y.is_line || x.control1 != y.control1 || x.control2 != y.control2 || x.end != y.end) {
            return false;
        }
    }
    return true;
}

Coord<2> cubic_point(Coord<2> a, Coord<2> b, Coord<2> c, Coord<2> d, float t) {
    float s = 1 - t;
    return a * (s * s * s) + b * (3 * s * s * t) + c * (3 * s * t * t) + d * (t * t * t);
}

size_t sample_count(const DetectorSliceSvgPath &path) {
    size_t requested = std::max(MIN_BOUNDARY_SAMPLES, path.segments.size() * 4);
    size_t result = MIN_BOUNDARY_SAMPLES;
    while (result < requested && result < MAX_BOUNDARY_SAMPLES) {
        result *= 2;
    }
    return result;
}

std::vector<Coord<2>> sample_path(const DetectorSliceSvgPath &path, size_t count) {
    std::vector<Coord<2>> result(count);
    if (path.kind == DetectorSliceSvgPathKind::CIRCLE) {
        for (size_t k = 0; k < result.size(); k++) {
            float angle = 2 * 3.14159265358979323846f * k / result.size();
            result[k] = path.start + Coord<2>{cosf(angle), sinf(angle)} * path.radius;
        }
        return result;
    }

    std::vector<Coord<2>> polyline{path.start};
    auto start = path.start;
    for (const auto &segment : path.segments) {
        size_t steps = segment.is_line ? 1 : 16;
        for (size_t k = 1; k <= steps; k++) {
            float t = (float)k / steps;
            polyline.push_back(
                segment.is_line ? segment.end : cubic_point(start, segment.control1, segment.control2, segment.end, t));
        }
        start = segment.end;
    }

    std::vector<float> cumulative(polyline.size());
    for (size_t k = 1; k < polyline.size(); k++) {
        cumulative[k] = cumulative[k - 1] + (polyline[k] - polyline[k - 1]).norm();
    }
    float total = cumulative.back();
    size_t segment = 1;
    for (size_t k = 0; k < result.size(); k++) {
        float distance = total * k / result.size();
        while (segment + 1 < cumulative.size() && cumulative[segment] < distance) {
            segment++;
        }
        float length = cumulative[segment] - cumulative[segment - 1];
        float t = length == 0 ? 0 : (distance - cumulative[segment - 1]) / length;
        result[k] = polyline[segment - 1] + (polyline[segment] - polyline[segment - 1]) * t;
    }
    return result;
}

std::vector<Coord<2>> aligned_to(const std::vector<Coord<2>> &source, const std::vector<Coord<2>> &target) {
    float best_error = INFINITY;
    size_t best_shift = 0;
    bool best_reversed = false;
    for (size_t reversed = 0; reversed < 2; reversed++) {
        for (size_t shift = 0; shift < target.size(); shift++) {
            float error = 0;
            for (size_t k = 0; k < source.size(); k++) {
                size_t j = reversed ? (shift + target.size() - k) % target.size() : (shift + k) % target.size();
                float dx = source[k].xyz[0] - target[j].xyz[0];
                float dy = source[k].xyz[1] - target[j].xyz[1];
                error += dx * dx + dy * dy;
                if (error >= best_error) {
                    break;
                }
            }
            if (error < best_error) {
                best_error = error;
                best_shift = shift;
                best_reversed = reversed;
            }
        }
    }
    std::vector<Coord<2>> result(target.size());
    for (size_t k = 0; k < target.size(); k++) {
        size_t j = best_reversed ? (best_shift + target.size() - k) % target.size() : (best_shift + k) % target.size();
        result[k] = target[j];
    }
    return result;
}

std::vector<Coord<2>> collapsed(const std::vector<Coord<2>> &points) {
    Coord<2> center{};
    float area_twice = 0;
    for (size_t k = 0; k < points.size(); k++) {
        const auto &a = points[k];
        const auto &b = points[(k + 1) % points.size()];
        float cross = a.xyz[0] * b.xyz[1] - b.xyz[0] * a.xyz[1];
        center += (a + b) * cross;
        area_twice += cross;
    }
    if (std::abs(area_twice) > 1e-6f) {
        center /= 3 * area_twice;
    } else {
        center = {};
        for (const auto &point : points) {
            center += point;
        }
        center /= points.size();
    }
    return std::vector<Coord<2>>(points.size(), center);
}

void write_points(const std::vector<Coord<2>> &source, const std::vector<Coord<2>> &target, std::ostream &out) {
    std::string bytes;
    bytes.reserve((source.size() + target.size()) * 2 * sizeof(float));
    for (const auto *points : {&source, &target}) {
        for (const auto &point : *points) {
            for (float value : point.xyz) {
                uint32_t bits;
                memcpy(&bits, &value, sizeof(bits));
                for (size_t k = 0; k < sizeof(bits); k++) {
                    bytes.push_back((char)(bits >> (8 * k)));
                }
            }
        }
    }
    out << '"';
    write_data_as_base64_to(bytes, out);
    out << '"';
}

void write_transitions(std::vector<DiagramTimelineSvgFrame> &frames, std::ostream &out) {
    out << '[';
    for (size_t k = 1; k < frames.size(); k++) {
        if (k > 1) {
            out << ',';
        }
        out << '[';
        bool first = true;
        const auto &source = frames[k - 1].detector_regions;
        const auto &target = frames[k].detector_regions;
        std::set<uint64_t> ids;
        for (const auto &entry : source)
            ids.insert(entry.first);
        for (const auto &entry : target)
            ids.insert(entry.first);
        for (uint64_t id : ids) {
            auto s = source.find(id);
            auto t = target.find(id);
            bool has_source = s != source.end();
            bool has_target = t != target.end();
            bool geometry_changed = !has_source || !has_target || !same_path(s->second.path, t->second.path);
            bool style_changed = has_source && has_target && s->second.style != t->second.style;
            if (!geometry_changed && !style_changed) {
                continue;
            }
            if (!first) {
                out << ',';
            }
            first = false;
            uint8_t flags = has_source | (has_target << 1) | (geometry_changed << 2) | (style_changed << 3);
            size_t count = 0;
            if (geometry_changed) {
                count = std::max(
                    has_source ? sample_count(s->second.path) : 0, has_target ? sample_count(t->second.path) : 0);
            }
            out << "[\"" << id << "\"," << (uint32_t)flags << ',' << count << ',';
            if (geometry_changed) {
                auto a = has_source ? sample_path(s->second.path, count) : sample_path(t->second.path, count);
                auto b = has_target ? sample_path(t->second.path, count) : sample_path(s->second.path, count);
                if (!has_source) {
                    a = collapsed(b);
                } else if (!has_target) {
                    b = collapsed(a);
                } else {
                    b = aligned_to(a, b);
                }
                write_points(a, b, out);
            } else {
                out << "\"\"";
            }
            out << ']';
        }
        out << ']';
        std::map<uint64_t, DiagramTimelineSvgFrame::DetectorRegion>().swap(frames[k - 1].detector_regions);
    }
    out << ']';
}

void write_json_string(std::ostream &out, std::string_view text) {
    out << '"';
    for (uint8_t c : text) {
        switch (c) {
            case '"':
                out << "\\\"";
                break;
            case '\\':
                out << "\\\\";
                break;
            case '\b':
                out << "\\b";
                break;
            case '\f':
                out << "\\f";
                break;
            case '\n':
                out << "\\n";
                break;
            case '\r':
                out << "\\r";
                break;
            case '\t':
                out << "\\t";
                break;
            case '<':
                out << "\\u003c";
                break;
            case '>':
                out << "\\u003e";
                break;
            case '&':
                out << "\\u0026";
                break;
            default:
                if (c < 0x20) {
                    out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (uint32_t)c << std::dec;
                } else {
                    out << (char)c;
                }
        }
    }
    out << '"';
}

void write_animation_frames(std::vector<DiagramTimelineSvgFrame> frames, uint64_t tick_slice_start, std::ostream &out) {
    out << R"HTML(<!doctype html>
<meta charset="utf-8">
<style>
html,body{height:100%;margin:0}body{font:14px system-ui,sans-serif;display:flex;flex-direction:column;background:white;color:#222}
#stage{min-height:0;flex:1;display:grid;place-items:center;overflow:hidden}#stage svg{width:100%;height:100%;display:block}
#controls{display:flex;align-items:center;gap:9px;padding:8px 10px;border-top:1px solid #ddd;background:#fafafa}
button,select{font:inherit}button{min-width:4.5em}input{min-width:80px;flex:1}output{min-width:9em;text-align:right;font-variant-numeric:tabular-nums}
</style>
<div id="stage"></div>
<div id="controls"><button id="play">Pause</button><input id="seek" type="range" step="0.001"><select id="speed"><option value="2">2x</option><option value="1" selected>1x</option><option value="0.5">0.5x</option><option value="0.25">0.25x</option></select><output id="label"></output></div>
<script>
if(window.frameElement)window.frameElement.style.height='820px';
const frames=)HTML";
    out << '[';
    for (size_t k = 0; k < frames.size(); k++) {
        if (k) {
            out << ',';
        }
        write_json_string(out, frames[k].svg);
        std::string().swap(frames[k].svg);
    }
    out << R"HTML(];
const transitions=)HTML";
    write_transitions(frames, out);
    out << R"HTML(;
const firstTick=)HTML"
        << (frames.empty() ? tick_slice_start : frames.front().tick) << R"HTML(;
const stage=document.getElementById('stage'),seek=document.getElementById('seek'),label=document.getElementById('label'),play=document.getElementById('play'),speed=document.getElementById('speed');
const parser=new DOMParser(),NS='http://www.w3.org/2000/svg',secondsPerTick=.55;
const templates=new Map;
let position=0,playing=frames.length>1,lastTime=0,intervalIndex=-1,records=[],preparedInterval=null,intervalWork=null;
seek.min=0;seek.max=Math.max(0,frames.length-1);seek.value=0;play.disabled=frames.length<2;

function frameTemplate(k){let svg=templates.get(k);if(!svg){svg=parser.parseFromString(frames[k],'image/svg+xml').documentElement;templates.set(k,svg)}return svg}
function parseFrame(k){return frameTemplate(k).cloneNode(true)}
function preloadFrame(k){if(k>=frames.length||templates.has(k))return;const work=()=>frameTemplate(k);if(window.requestIdleCallback)requestIdleCallback(work);else setTimeout(work)}
function runIdle(work){if(window.requestIdleCallback)requestIdleCallback(work);else setTimeout(()=>work({timeRemaining:()=>4}))}
function detectorNumber(id){const m=/^slice:(\d+):/.exec(id);return m?m[1]:null}
function detectorMap(svg){const result=new Map;for(const g of svg.querySelectorAll('g[id^="slice:"]')){const d=detectorNumber(g.id);if(d!==null)result.set(d,g)}return result}
function outline(group){for(let k=group.children.length-1;k>=0;k--){const e=group.children[k];if((e.tagName==='path'||e.tagName==='circle')&&e.getAttribute('stroke')==='black'&&e.getAttribute('fill')==='none')return e}return null}
function fillOpacity(group){for(const e of group.querySelectorAll('path,circle'))if(e.getAttribute('stroke')==='none'&&e.getAttribute('fill')!=='none'){const value=Number(e.getAttribute('fill-opacity')||1);if(Number.isFinite(value))return value}return 1}
function decodePoints(text){const raw=atob(text),bytes=new Uint8Array(raw.length);for(let k=0;k<raw.length;k++)bytes[k]=raw.charCodeAt(k);return new Float32Array(bytes.buffer)}
function pathData(a,b,t){let d='M';for(let k=0;k<a.length/2;k++){if(k)d+='L';d+=(a[2*k]+(b[2*k]-a[2*k])*t).toFixed(4)+','+(a[2*k+1]+(b[2*k+1]-a[2*k+1])*t).toFixed(4)}return d+'Z'}
function replaceShape(element){if(!element)return null;const path=document.createElementNS(NS,'path');for(const attr of element.attributes){if(attr.name!=='d'&&attr.name!=='cx'&&attr.name!=='cy'&&attr.name!=='r')path.setAttribute(attr.name,attr.value)}element.replaceWith(path);return path}
function shapeNode(group,id){const shapes=[],first=group.firstElementChild;if(first&&(first.tagName==='path'||first.tagName==='circle'))shapes.push(first);for(const child of [...group.children])if(child.tagName==='clipPath'&&child.firstElementChild)shapes.push(child.firstElementChild);const edge=outline(group);if(edge&&!shapes.includes(edge))shapes.push(edge);const master=replaceShape(shapes[0]);master.id=id;for(let k=1;k<shapes.length;k++){const use=document.createElementNS(NS,'use');for(const attr of shapes[k].attributes)if(!['d','cx','cy','r'].includes(attr.name))use.setAttribute(attr.name,attr.value);use.setAttribute('href','#'+id);shapes[k].replaceWith(use)}return master}
function uniqueIds(group,suffix){const remap=new Map,elements=[group,...group.querySelectorAll('[id]')];for(const e of elements){if(!e.id)continue;const old=e.id,neu=old+suffix;remap.set(old,neu);e.id=neu}for(const e of [group,...group.querySelectorAll('*')])for(const a of [...e.attributes])for(const [old,neu] of remap)e.setAttribute(a.name,a.value.replace('url(#'+old+')','url(#'+neu+')'))}
function mountExact(k){intervalIndex=-1;records=[];stage.replaceChildren(parseFrame(k));}
function beginInterval(k){
  const source=frameTemplate(k),svg=parseFrame(k+1),sm=detectorMap(source),mm=detectorMap(svg);
  return {k,svg,sm,mm,sourceOrder:[...sm.keys()],index:0,records:[]};
}
function addIntervalRecord(work,item){
    const [id,flags,count,encoded]=item,sg=work.sm.get(id),mg=work.mm.get(id);let sourceGroup=null,targetGroup=mg,nodes=[],sourceLayer=null,targetLayer=null,targetFillOpacity=1;
    const hasSource=!!(flags&1),hasTarget=!!(flags&2),geometryChanged=!!(flags&4),styleChanged=!!(flags&8);
    if(styleChanged){
      sourceGroup=sg.cloneNode(true);uniqueIds(sourceGroup,'-source-'+work.k+'-'+id);targetFillOpacity=fillOpacity(targetGroup);
      const sourceOutline=outline(sourceGroup),targetOutline=outline(targetGroup),edge=targetOutline.cloneNode(true),wrapper=document.createElementNS(NS,'g');
      sourceOutline.setAttribute('opacity','0');targetOutline.setAttribute('opacity','0');targetGroup.replaceWith(wrapper);
      sourceLayer=document.createElementNS(NS,'g');targetLayer=document.createElementNS(NS,'g');sourceLayer.append(sourceGroup);targetLayer.append(targetGroup);wrapper.append(sourceLayer,targetLayer,edge);
      if(geometryChanged)nodes.push(replaceShape(edge));
    }
    else if(!hasTarget){sourceGroup=sg.cloneNode(true);uniqueIds(sourceGroup,'-dying-'+work.k+'-'+id);let anchor=work.svg.querySelector('#qubit_dots');for(let j=work.sourceOrder.indexOf(id)+1;j<work.sourceOrder.length;j++)if(work.mm.has(work.sourceOrder[j])){anchor=work.mm.get(work.sourceOrder[j]);break}work.svg.insertBefore(sourceGroup,anchor);targetGroup=null}
    if(geometryChanged){if(targetGroup)nodes.push(shapeNode(targetGroup,'shape-'+work.k+'-'+id+'-target'));if(sourceGroup)nodes.push(shapeNode(sourceGroup,'shape-'+work.k+'-'+id+'-source'))}
    const points=geometryChanged?decodePoints(encoded):null,a=points?points.subarray(0,count*2):null,b=points?points.subarray(count*2):null;
    work.records.push({a,b,nodes,sourceGroup,targetGroup,sourceLayer,targetLayer,targetFillOpacity,born:!hasSource,died:!hasTarget,styleChanged,geometryChanged});
}
function finishInterval(work){templates.delete(work.k);delete work.sm;delete work.mm;delete work.sourceOrder;delete work.index;return work}
function buildInterval(k){const work=beginInterval(k);while(work.index<transitions[k].length)addIntervalRecord(work,transitions[k][work.index++]);return finishInterval(work)}
function preloadInterval(k){
  if(k>=transitions.length||(preparedInterval&&preparedInterval.k===k)||(intervalWork&&intervalWork.k===k))return;
  const token={k};intervalWork=token;
  const step=deadline=>{
    if(intervalWork!==token)return;
    if(!token.svg)Object.assign(token,beginInterval(k));
    let first=true;
    while(token.index<transitions[k].length&&(first||deadline.timeRemaining()>2)){addIntervalRecord(token,transitions[k][token.index++]);first=false}
    if(token.index===transitions[k].length){
      preparedInterval=finishInterval(token);intervalWork=null;
    }else runIdle(step);
  };
  runIdle(step);
}
function prepareInterval(k){
  const ready=preparedInterval&&preparedInterval.k===k?preparedInterval:buildInterval(k);
  preparedInterval=null;intervalWork=null;stage.replaceChildren(ready.svg);records=ready.records;intervalIndex=k;
  const next=k+1<transitions.length?k+1:0;
  for(const tick of templates.keys())if(tick<next||tick>next+1)templates.delete(tick);
  preloadFrame(next);preloadFrame(next+1);preloadInterval(next);
}
function renderInterval(k,t){if(intervalIndex!==k)prepareInterval(k);const eased=t*t*(3-2*t);for(const r of records){if(r.geometryChanged){const d=pathData(r.a,r.b,eased);for(const node of r.nodes)node.setAttribute('d',d)}if(r.born)r.targetGroup.style.opacity=eased;else if(r.died)r.sourceGroup.style.opacity=1-eased;else if(r.styleChanged){const denominator=1-eased*r.targetFillOpacity;r.sourceLayer.style.opacity=eased>=1?0:(1-eased)/Math.max(denominator,1e-9);r.targetLayer.style.opacity=eased}}}
function show(value){position=Math.max(0,Math.min(frames.length-1,value));const nearest=Math.round(position);if(Math.abs(position-nearest)<1e-7)mountExact(nearest);else renderInterval(Math.floor(position),position-Math.floor(position));seek.value=position;label.value='diagram tick '+(firstTick+position).toFixed(3)}
function animate(now){if(!lastTime)lastTime=now;if(playing&&frames.length>1){position+=(now-lastTime)/1000/secondsPerTick*Number(speed.value);if(position>=frames.length-1)position%=frames.length-1;show(position)}lastTime=now;requestAnimationFrame(animate)}
play.onclick=()=>{playing=!playing;play.textContent=playing?'Pause':'Play';lastTime=performance.now()};seek.oninput=()=>{playing=false;play.textContent='Play';show(Number(seek.value))};
if(frames.length){show(0);if(transitions.length)preparedInterval=buildInterval(0)}else{stage.textContent='No diagram ticks in the requested range';play.textContent='Play'}requestAnimationFrame(animate);
</script>)HTML";
}

}  // namespace

std::string stim_draw_internal::make_detector_slice_animation_html(
    const Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    SpanRef<const CoordFilter> det_coord_filter) {
    auto frames = make_frames(circuit, tick_slice_start, tick_slice_num, det_coord_filter);
    size_t capacity = 4096;
    for (const auto &frame : frames) {
        capacity += frame.svg.size() + frame.detector_regions.size() * ESTIMATED_TRANSITION_BYTES;
    }
    StringBuffer buffer(capacity);
    std::ostream out(&buffer);
    write_animation_frames(std::move(frames), tick_slice_start, out);
    return std::move(buffer.data);
}
