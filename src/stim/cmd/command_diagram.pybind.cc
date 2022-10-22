// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/cmd/command_diagram.pybind.h"

#include "stim/cmd/command_help.h"
#include "stim/diagram/base64.h"
#include "stim/diagram/detector_slice/detector_slice_set.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/diagram/timeline/timeline_3d_drawer.h"
#include "stim/diagram/timeline/timeline_ascii_drawer.h"
#include "stim/diagram/timeline/timeline_svg_drawer.h"
#include "stim/diagram/graph/match_graph_3d_drawer.h"
#include "stim/diagram/graph/match_graph_svg_drawer.h"

using namespace stim;
using namespace stim_pybind;
using namespace stim_draw_internal;

pybind11::class_<DiagramHelper> stim_pybind::pybind_diagram(pybind11::module &m) {
    auto c = pybind11::class_<DiagramHelper>(
        m,
        "_DiagramHelper",
        clean_doc_string(u8R"DOC(
            A helper class for displaying diagrams in IPython notebooks.

            To write the diagram's contents to a file (for example, to write an
            SVG image to an SVG file), use `print(diagram, file=file)`.
        )DOC")
            .data());

    return c;
}

void write_html_viewer_for_gltf_data(const std::string &gltf_data, std::ostream &out) {
    out << R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
</head>
<body>
  <a download="model.gltf" id="stim-3d-viewer-download-link" href="data:text/plain;base64,)HTML";
    write_data_as_base64_to(gltf_data.data(), gltf_data.size(), out);
    out << R"HTML(">Download 3D Model as .GLTF File</a>
  <br>Mouse Wheel = Zoom. Left Drag = Orbit. Right Drag = Strafe.
  <div style="border: 1px dashed gray; margin-bottom: 50px; width: 300px; height: 300px; resize: both; overflow: hidden">
    <div id="stim-3d-viewer-scene-container" style="width: 100%; height: 100%;">JavaScript Blocked?</div>
  </div>

  <script type="module">
    /// BEGIN TERRIBLE HACK.
    /// Get the object by ID then change the ID.
    /// This is a workaround for https://github.com/jupyter/notebook/issues/6598
    let container = document.getElementById("stim-3d-viewer-scene-container");
    container.id = "stim-3d-viewer-scene-container-USED";
    let downloadLink = document.getElementById("stim-3d-viewer-download-link");
    downloadLink.id = "stim-3d-viewer-download-link-USED";
    /// END TERRIBLE HACK.

    container.textContent = "Loading viewer...";

    /// BEGIN TERRIBLE HACK.
    /// This a workaround for https://github.com/jupyter/notebook/issues/6597
    ///
    /// What this SHOULD be is:
    ///
    /// import {Box3, Scene, Color, PerspectiveCamera, WebGLRenderer, DirectionalLight} from "three";
    /// import {OrbitControls} from "three-orbitcontrols";
    /// import {GLTFLoader} from "three-gltf-loader";
    ///
    /// assuming the following import map exists:
    ///
    /// with import map:
    ///   {
    ///     "imports": {
    ///       "three": "https://unpkg.com/three@0.138.0/build/three.module.js",
    ///       "three-orbitcontrols": "https://unpkg.com/three@0.138.0/examples/jsm/controls/OrbitControls.js",
    ///       "three-gltf-loader": "https://unpkg.com/three@0.138.0/examples/jsm/loaders/GLTFLoader.js"
    ///     }
    ///   }
    import {
        WebGLRenderer,Scene,EventDispatcher,MOUSE,Quaternion,Spherical,TOUCH,Vector2,Vector3,AnimationClip,Bone,Box3,BufferAttribute,BufferGeometry,ClampToEdgeWrapping,Color,DirectionalLight,DoubleSide,FileLoader,FrontSide,Group,ImageBitmapLoader,InterleavedBuffer,InterleavedBufferAttribute,Interpolant,InterpolateDiscrete,InterpolateLinear,Line,LineBasicMaterial,LineLoop,LineSegments,LinearFilter,LinearMipmapLinearFilter,LinearMipmapNearestFilter,Loader,LoaderUtils,Material,MathUtils,Matrix4,Mesh,MeshBasicMaterial,MeshPhysicalMaterial,MeshStandardMaterial,MirroredRepeatWrapping,NearestFilter,NearestMipmapLinearFilter,NearestMipmapNearestFilter,NumberKeyframeTrack,Object3D,OrthographicCamera,PerspectiveCamera,PointLight,Points,PointsMaterial,PropertyBinding,QuaternionKeyframeTrack,RepeatWrapping,Skeleton,SkinnedMesh,Sphere,SpotLight,TangentSpaceNormalMap,Texture,TextureLoader,TriangleFanDrawMode,TriangleStripDrawMode,VectorKeyframeTrack,sRGBEncoding
    } from "https://unpkg.com/three@0.138.0/build/three.module.js";
    async function workaround(result, url) {
        let fetched = await fetch(url);
        let content = await (await fetched.blob()).text();
        let strip_module = content.split("} from 'three';")[1].split("export {")[0];
        let wrap_function = "(() => {" + strip_module + "\nreturn " + result + ";\n})()";
        return eval(wrap_function);
    }
    let OrbitControls = await workaround("OrbitControls", "https://unpkg.com/three@0.138.0/examples/jsm/controls/OrbitControls.js");
    let GLTFLoader = await workaround("GLTFLoader", "https://unpkg.com/three@0.138.0/examples/jsm/loaders/GLTFLoader.js");
    ///
    /// END TERRIBLE HACK.
    ///

    container.textContent = "Loading model...";
    let modelDataUri = downloadLink.href;
    let gltf = await new GLTFLoader().loadAsync(modelDataUri);
    container.textContent = "Loading scene...";

    // Create the scene, adding lighting for the loaded objects.
    let scene = new Scene();
    scene.background = new Color("white");
    let mainLight = new DirectionalLight(0xffffff, 5);
    mainLight.position.set(1, 1, 0);
    let backLight = new DirectionalLight(0xffffff, 4);
    backLight.position.set(-1, -1, 0);
    scene.add(mainLight, backLight);
    scene.add(gltf.scene);

    // Point the camera at the center, far enough back to see everything.
    let camera = new PerspectiveCamera(35, container.clientWidth / container.clientHeight, 0.1, 100000);
    let controls = new OrbitControls(camera, container);
    let bounds = new Box3().setFromObject(scene);
    controls.target.set(
        (bounds.min.x + bounds.max.x) * 0.5,
        (bounds.min.y + bounds.max.y) * 0.5,
        (bounds.min.z + bounds.max.z) * 0.5,
    );
    let dx = bounds.min.x + bounds.max.x;
    let dy = bounds.min.y + bounds.max.y;
    let dz = bounds.min.z + bounds.max.z;
    let diag = Math.sqrt(dx*dx + dy*dy + dz*dz);
    camera.position.set(diag*0.3, diag*0.4, -diag*1.8);
    controls.update();

    // Set up rendering.
    let renderer = new WebGLRenderer({ antialias: true });
    container.textContent = "";
    renderer.setSize(container.clientWidth, container.clientHeight);
    renderer.setPixelRatio(window.devicePixelRatio);
    renderer.physicallyCorrectLights = true;
    container.appendChild(renderer.domElement);

    // Render whenever any important changes have occurred.
    requestAnimationFrame(() => renderer.render(scene, camera));
    new ResizeObserver(() => {
      camera.aspect = container.clientWidth / container.clientHeight;
      camera.updateProjectionMatrix();
      renderer.setSize(container.clientWidth, container.clientHeight);
      renderer.render(scene, camera);
    }).observe(container);
    controls.addEventListener("change", () => {
        renderer.render(scene, camera);
    })
  </script>
</body>
)HTML";
}

void stim_pybind::pybind_diagram_methods(pybind11::module &m, pybind11::class_<DiagramHelper> &c) {
    c.def("_repr_html_", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type == DIAGRAM_TYPE_TEXT) {
            return pybind11::cast("<pre>" + self.content + "</pre>");
        }
        if (self.type == DIAGRAM_TYPE_SVG) {
            std::stringstream out;
            out << R"HTML(<div style="border: 1px dashed gray; margin-bottom: 50px; width: 300px; resize: both; overflow: hidden">)HTML";
            out << R"HTML(<img style="max-width: 100%; max-height: 100%" src="data:image/svg+xml;base64,)HTML";
            write_data_as_base64_to(self.content.data(), self.content.size(), out);
            out << R"HTML("/></div>)HTML";
            return pybind11::cast(out.str());
        }
        if (self.type == DIAGRAM_TYPE_GLTF) {
            std::stringstream out;
            write_html_viewer_for_gltf_data(self.content, out);
            return pybind11::cast(out.str());
        }
        return pybind11::none();
    });
    c.def("_repr_svg_", [](const DiagramHelper &self) -> pybind11::object {
        if (self.type != DIAGRAM_TYPE_SVG) {
            return pybind11::none();
        }
        return pybind11::cast(self.content);
    });
    c.def("_repr_pretty_", [](const DiagramHelper &self, pybind11::object p, pybind11::object cycle) -> void {
        pybind11::getattr(p, "text")(self.content);
    });
    c.def("__str__", [](const DiagramHelper &self) {
        return self.content;
    });
}

DiagramHelper stim_pybind::dem_diagram(const DetectorErrorModel &dem, const std::string &type) {
    if (type == "match-graph-svg") {
        std::stringstream out;
        dem_match_graph_to_svg_diagram_write_to(dem, out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "match-graph-3d") {
        std::stringstream out;
        dem_match_graph_to_basic_3d_diagram(dem).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out.str()};
    } else {
        throw std::invalid_argument("Unrecognized diagram type: " + type);
    }
}
DiagramHelper stim_pybind::circuit_diagram(
    const Circuit &circuit, const std::string &type, const pybind11::object &tick) {
    if (type == "timeline-text") {
        if (!tick.is_none()) {
            throw std::invalid_argument("`tick` isn't used with type='timeline-text'");
        }
        std::stringstream out;
        out << DiagramTimelineAsciiDrawer::make_diagram(circuit);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "timeline-svg") {
        if (!tick.is_none()) {
            throw std::invalid_argument("`tick` isn't used with type='timeline-svg'");
        }
        std::stringstream out;
        DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "timeline-3d") {
        std::stringstream out;
        DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit).to_gltf_scene().to_json().write(out);
        return DiagramHelper{DIAGRAM_TYPE_GLTF, out.str()};
    } else if (type == "detector-slice-text") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-text'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_tick(circuit, pybind11::cast<uint64_t>(tick)).write_text_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_TEXT, out.str()};
    } else if (type == "detector-slice-svg") {
        if (tick.is_none()) {
            throw std::invalid_argument("You must specify the tick= argument when using type='detector-slice-svg'");
        }
        std::stringstream out;
        DetectorSliceSet::from_circuit_tick(circuit, pybind11::cast<uint64_t>(tick)).write_svg_diagram_to(out);
        return DiagramHelper{DIAGRAM_TYPE_SVG, out.str()};
    } else if (type == "match-graph-svg" || type == "match-graph-3d") {
        auto dem = ErrorAnalyzer::circuit_to_detector_error_model(
            circuit, true, true, false, 1, true, false);
        return dem_diagram(dem, type);
    } else {
        throw std::invalid_argument("Unrecognized diagram type: " + type);
    }
}
