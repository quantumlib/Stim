#include "stim/diagram/detector_slice/detector_slice_animation.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string_view>
#include <vector>

#include "stim/diagram/base64.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

namespace {

constexpr size_t MIN_BOUNDARY_SAMPLES = 64;
constexpr size_t MAX_BOUNDARY_SAMPLES = 256;

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

}  // namespace

std::vector<DetectorSliceAnimationTransition> stim_draw_internal::make_detector_slice_animation_transitions(
    const DetectorSliceAnimationFrame &source, const DetectorSliceAnimationFrame &target) {
    std::set<uint64_t> ids;
    for (const auto &entry : source.detector_regions) {
        ids.insert(entry.first);
    }
    for (const auto &entry : target.detector_regions) {
        ids.insert(entry.first);
    }

    std::vector<DetectorSliceAnimationTransition> result;
    for (uint64_t id : ids) {
        auto s = source.detector_regions.find(id);
        auto t = target.detector_regions.find(id);
        bool has_source = s != source.detector_regions.end();
        bool has_target = t != target.detector_regions.end();
        bool geometry_changed = !has_source || !has_target || !same_path(s->second.path, t->second.path);
        bool style_changed = has_source && has_target && s->second.style != t->second.style;
        if (!geometry_changed && !style_changed) {
            continue;
        }

        std::vector<Coord<2>> source_points;
        std::vector<Coord<2>> target_points;
        if (geometry_changed) {
            size_t count =
                std::max(has_source ? sample_count(s->second.path) : 0, has_target ? sample_count(t->second.path) : 0);
            source_points = has_source ? sample_path(s->second.path, count) : sample_path(t->second.path, count);
            target_points = has_target ? sample_path(t->second.path, count) : sample_path(s->second.path, count);
            if (!has_source) {
                source_points = collapsed(target_points);
            } else if (!has_target) {
                target_points = collapsed(source_points);
            } else {
                target_points = aligned_to(source_points, target_points);
            }
        }
        result.push_back({
            id,
            has_source,
            has_target,
            geometry_changed,
            style_changed,
            std::move(source_points),
            std::move(target_points),
        });
    }
    return result;
}

std::vector<DetectorSliceAnimationFrame> stim_draw_internal::make_detector_slice_animation_frames(
    const Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    SpanRef<const CoordFilter> det_coord_filter) {
    std::vector<DetectorSliceAnimationFrame> result;
    uint64_t circuit_num_ticks = circuit.count_ticks();
    if (tick_slice_start > circuit_num_ticks) {
        return result;
    }
    tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start + 1);
    if (!circuit.operations.empty() && circuit.operations.back().gate_type == GateType::TICK) {
        tick_slice_num = std::min(tick_slice_num, circuit_num_ticks - tick_slice_start);
    }

    result.reserve(tick_slice_num);
    for (uint64_t k = 0; k < tick_slice_num; k++) {
        DetectorSliceAnimationFrame frame{tick_slice_start + k, {}, {}};
        DetectorSliceSvgMetadata metadata;
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(
            circuit,
            out,
            frame.tick,
            1,
            DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE,
            det_coord_filter,
            0,
            &metadata);
        for (auto &region : metadata.regions) {
            uint64_t detector_id = region.detector_id;
            frame.detector_regions.insert({detector_id, std::move(region)});
        }
        frame.svg = out.str();
        result.push_back(std::move(frame));
    }

    return result;
}

namespace {

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

void write_transitions(std::vector<DetectorSliceAnimationFrame> &frames, std::ostream &out) {
    out << '[';
    for (size_t k = 1; k < frames.size(); k++) {
        if (k > 1) {
            out << ',';
        }
        auto transitions = make_detector_slice_animation_transitions(frames[k - 1], frames[k]);
        out << '[';
        for (size_t j = 0; j < transitions.size(); j++) {
            if (j) {
                out << ',';
            }
            const auto &transition = transitions[j];
            uint8_t flags = transition.has_source | (transition.has_target << 1) | (transition.geometry_changed << 2) |
                            (transition.style_changed << 3);
            out << "[\"" << transition.detector_id << "\"," << (uint32_t)flags << ',' << transition.source_points.size()
                << ',';
            if (transition.geometry_changed) {
                write_points(transition.source_points, transition.target_points, out);
            } else {
                out << "\"\"";
            }
            out << ']';
        }
        out << ']';
        std::map<uint64_t, DetectorSliceSvgRegion>().swap(frames[k - 1].detector_regions);
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

void write_animation_frames(
    std::vector<DetectorSliceAnimationFrame> frames, uint64_t tick_slice_start, std::ostream &out) {
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
if (window.frameElement) window.frameElement.style.height = '820px';
const frames = )HTML";
    out << '[';
    for (size_t k = 0; k < frames.size(); k++) {
        if (k) {
            out << ',';
        }
        write_json_string(out, frames[k].svg);
        std::string().swap(frames[k].svg);
    }
    out << R"HTML(];
const transitions = )HTML";
    write_transitions(frames, out);
    out << R"HTML(;
const firstTick = )HTML"
        << (frames.empty() ? tick_slice_start : frames.front().tick) << R"HTML(;
const stage = document.getElementById('stage'), seek = document.getElementById('seek'),
    label = document.getElementById('label'), play = document.getElementById('play'),
    speed = document.getElementById('speed');
const parser = new DOMParser(), NS = 'http://www.w3.org/2000/svg', secondsPerTick = .55;
const templates = new Map;
let position = 0, playing = frames.length > 1, lastTime = 0, intervalIndex = -1, records = [],
    preparedInterval = null, intervalWork = null;
seek.min = 0;
seek.max = Math.max(0, frames.length - 1);
seek.value = 0;
play.disabled = frames.length < 2;

function frameTemplate(k) {
    let svg = templates.get(k);
    if (!svg) {
        svg = parser.parseFromString(frames[k], 'image/svg+xml').documentElement;
        templates.set(k, svg)
    }
    return svg
}

function parseFrame(k) {
    return frameTemplate(k).cloneNode(true)
}

function preloadFrame(k) {
    if (k >= frames.length || templates.has(k)) return;
    const work = () => frameTemplate(k);
    if (window.requestIdleCallback) requestIdleCallback(work);
    else setTimeout(work)
}

function runIdle(work) {
    if (window.requestIdleCallback) requestIdleCallback(work);
    else setTimeout(() => work({timeRemaining: () => 4}))
}

function detectorNumber(id) {
    const m = /^slice:(\d+):/.exec(id);
    return m ? m[1] : null
}

function detectorMap(svg) {
    const result = new Map;
    for (const g of svg.querySelectorAll('g[id^="slice:"]')) {
        const d = detectorNumber(g.id);
        if (d !== null) result.set(d, g)
    }
    return result
}

function outline(group) {
    for (let k = group.children.length - 1; k >= 0; k--) {
        const e = group.children[k];
        if ((e.tagName === 'path' || e.tagName === 'circle') &&
            e.getAttribute('stroke') === 'black' && e.getAttribute('fill') === 'none')
            return e
    }
    return null
}

function fillOpacity(group) {
    for (const e of group.querySelectorAll('path,circle'))
        if (e.getAttribute('stroke') === 'none' && e.getAttribute('fill') !== 'none') {
            const value = Number(e.getAttribute('fill-opacity') || 1);
            if (Number.isFinite(value)) return value
        }
    return 1
}

function decodePoints(text) {
    const raw = atob(text), bytes = new Uint8Array(raw.length);
    for (let k = 0; k < raw.length; k++) bytes[k] = raw.charCodeAt(k);
    return new Float32Array(bytes.buffer)
}

function pathData(a, b, t) {
    let d = 'M';
    for (let k = 0; k < a.length / 2; k++) {
        if (k) d += 'L';
        d += (a[2 * k] + (b[2 * k] - a[2 * k]) * t).toFixed(4) + ',' +
             (a[2 * k + 1] + (b[2 * k + 1] - a[2 * k + 1]) * t).toFixed(4)
    }
    return d + 'Z'
}

function replaceShape(element) {
    if (!element) return null;
    const path = document.createElementNS(NS, 'path');
    for (const attr of element.attributes) {
        if (attr.name !== 'd' && attr.name !== 'cx' && attr.name !== 'cy' && attr.name !== 'r')
            path.setAttribute(attr.name, attr.value)
    }
    element.replaceWith(path);
    return path
}

function shapeNode(group, id) {
    const shapes = [], first = group.firstElementChild;
    if (first && (first.tagName === 'path' || first.tagName === 'circle')) shapes.push(first);
    for (const child of [...group.children])
        if (child.tagName === 'clipPath' && child.firstElementChild) shapes.push(child.firstElementChild);
    const edge = outline(group);
    if (edge && !shapes.includes(edge)) shapes.push(edge);
    const master = replaceShape(shapes[0]);
    master.id = id;
    for (let k = 1; k < shapes.length; k++) {
        const use = document.createElementNS(NS, 'use');
        for (const attr of shapes[k].attributes)
            if (!['d', 'cx', 'cy', 'r'].includes(attr.name)) use.setAttribute(attr.name, attr.value);
        use.setAttribute('href', '#' + id);
        shapes[k].replaceWith(use)
    }
    return master
}

function uniqueIds(group, suffix) {
    const remap = new Map, elements = [group, ...group.querySelectorAll('[id]')];
    for (const e of elements) {
        if (!e.id) continue;
        const old = e.id, neu = old + suffix;
        remap.set(old, neu);
        e.id = neu
    }
    for (const e of [group, ...group.querySelectorAll('*')])
        for (const a of [...e.attributes])
            for (const [old, neu] of remap)
                e.setAttribute(a.name, a.value.replace('url(#' + old + ')', 'url(#' + neu + ')'))
}

function mountExact(k) {
    intervalIndex = -1;
    records = [];
    stage.replaceChildren(parseFrame(k));
}

function beginInterval(k) {
    const source = frameTemplate(k), svg = parseFrame(k + 1), sm = detectorMap(source), mm = detectorMap(svg);
    return {k, svg, sm, mm, sourceOrder: [...sm.keys()], index: 0, records: []};
}

function addIntervalRecord(work, item) {
    const [id, flags, count, encoded] = item, sg = work.sm.get(id), mg = work.mm.get(id);
    let sourceGroup = null, targetGroup = mg, nodes = [], sourceLayer = null, targetLayer = null,
        targetFillOpacity = 1;
    const hasSource = !!(flags & 1), hasTarget = !!(flags & 2), geometryChanged = !!(flags & 4),
        styleChanged = !!(flags & 8);
    if (styleChanged) {
        sourceGroup = sg.cloneNode(true);
        uniqueIds(sourceGroup, '-source-' + work.k + '-' + id);
        targetFillOpacity = fillOpacity(targetGroup);
        const sourceOutline = outline(sourceGroup), targetOutline = outline(targetGroup),
            edge = targetOutline.cloneNode(true), wrapper = document.createElementNS(NS, 'g');
        sourceOutline.setAttribute('opacity', '0');
        targetOutline.setAttribute('opacity', '0');
        targetGroup.replaceWith(wrapper);
        sourceLayer = document.createElementNS(NS, 'g');
        targetLayer = document.createElementNS(NS, 'g');
        sourceLayer.append(sourceGroup);
        targetLayer.append(targetGroup);
        wrapper.append(sourceLayer, targetLayer, edge);
        if (geometryChanged) nodes.push(replaceShape(edge));
    } else if (!hasTarget) {
        sourceGroup = sg.cloneNode(true);
        uniqueIds(sourceGroup, '-dying-' + work.k + '-' + id);
        let anchor = work.svg.querySelector('#qubit_dots');
        for (let j = work.sourceOrder.indexOf(id) + 1; j < work.sourceOrder.length; j++)
            if (work.mm.has(work.sourceOrder[j])) {
                anchor = work.mm.get(work.sourceOrder[j]);
                break
            }
        work.svg.insertBefore(sourceGroup, anchor);
        targetGroup = null
    }
    if (geometryChanged) {
        if (targetGroup) nodes.push(shapeNode(targetGroup, 'shape-' + work.k + '-' + id + '-target'));
        if (sourceGroup) nodes.push(shapeNode(sourceGroup, 'shape-' + work.k + '-' + id + '-source'))
    }
    const points = geometryChanged ? decodePoints(encoded) : null,
        a = points ? points.subarray(0, count * 2) : null,
        b = points ? points.subarray(count * 2) : null;
    work.records.push({
        a, b, nodes, sourceGroup, targetGroup, sourceLayer, targetLayer, targetFillOpacity,
        born: !hasSource, died: !hasTarget, styleChanged, geometryChanged
    });
}

function finishInterval(work) {
    templates.delete(work.k);
    delete work.sm;
    delete work.mm;
    delete work.sourceOrder;
    delete work.index;
    return work
}

function buildInterval(k) {
    const work = beginInterval(k);
    while (work.index < transitions[k].length) addIntervalRecord(work, transitions[k][work.index++]);
    return finishInterval(work)
}

function preloadInterval(k) {
    if (k >= transitions.length || (preparedInterval && preparedInterval.k === k) ||
        (intervalWork && intervalWork.k === k)) return;
    const token = {k};
    intervalWork = token;
    const step = deadline => {
        if (intervalWork !== token) return;
        if (!token.svg) Object.assign(token, beginInterval(k));
        let first = true;
        while (token.index < transitions[k].length && (first || deadline.timeRemaining() > 2)) {
            addIntervalRecord(token, transitions[k][token.index++]);
            first = false
        }
        if (token.index === transitions[k].length) {
            preparedInterval = finishInterval(token);
            intervalWork = null;
        } else runIdle(step);
    };
    runIdle(step);
}

function prepareInterval(k) {
    const ready = preparedInterval && preparedInterval.k === k ? preparedInterval : buildInterval(k);
    preparedInterval = null;
    intervalWork = null;
    stage.replaceChildren(ready.svg);
    records = ready.records;
    intervalIndex = k;
    const next = k + 1 < transitions.length ? k + 1 : 0;
    for (const tick of templates.keys())
        if (tick < next || tick > next + 1) templates.delete(tick);
    preloadFrame(next);
    preloadFrame(next + 1);
    preloadInterval(next);
}

function renderInterval(k, t) {
    if (intervalIndex !== k) prepareInterval(k);
    const eased = t * t * (3 - 2 * t);
    for (const r of records) {
        if (r.geometryChanged) {
            const d = pathData(r.a, r.b, eased);
            for (const node of r.nodes) node.setAttribute('d', d)
        }
        if (r.born) r.targetGroup.style.opacity = eased;
        else if (r.died) r.sourceGroup.style.opacity = 1 - eased;
        else if (r.styleChanged) {
            const denominator = 1 - eased * r.targetFillOpacity;
            r.sourceLayer.style.opacity = eased >= 1 ? 0 : (1 - eased) / Math.max(denominator, 1e-9);
            r.targetLayer.style.opacity = eased
        }
    }
}

function show(value) {
    position = Math.max(0, Math.min(frames.length - 1, value));
    const nearest = Math.round(position);
    if (Math.abs(position - nearest) < 1e-7) mountExact(nearest);
    else renderInterval(Math.floor(position), position - Math.floor(position));
    seek.value = position;
    label.value = 'diagram tick ' + (firstTick + position).toFixed(3)
}

function animate(now) {
    if (!lastTime) lastTime = now;
    if (playing && frames.length > 1) {
        position += (now - lastTime) / 1000 / secondsPerTick * Number(speed.value);
        if (position >= frames.length - 1) position %= frames.length - 1;
        show(position)
    }
    lastTime = now;
    requestAnimationFrame(animate)
}

play.onclick = () => {
    playing = !playing;
    play.textContent = playing ? 'Pause' : 'Play';
    lastTime = performance.now()
};
seek.oninput = () => {
    playing = false;
    play.textContent = 'Play';
    show(Number(seek.value))
};
if (frames.length) {
    show(0);
    if (transitions.length) preparedInterval = buildInterval(0)
} else {
    stage.textContent = 'No diagram ticks in the requested range';
    play.textContent = 'Play'
}
requestAnimationFrame(animate);
</script>)HTML";
}

}  // namespace

std::string stim_draw_internal::make_detector_slice_animation_html(
    const Circuit &circuit,
    uint64_t tick_slice_start,
    uint64_t tick_slice_num,
    SpanRef<const CoordFilter> det_coord_filter) {
    std::stringstream out;
    write_animation_frames(
        make_detector_slice_animation_frames(circuit, tick_slice_start, tick_slice_num, det_coord_filter),
        tick_slice_start,
        out);
    return out.str();
}
