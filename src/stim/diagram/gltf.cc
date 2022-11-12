#include "stim/diagram/gltf.h"

#include <iostream>

using namespace stim;
using namespace stim_draw_internal;

void GltfScene::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "scenes",
        [&]() {
            return _to_json_local();
        },
        (uintptr_t)this);
    for (auto &node : nodes) {
        node->visit(callback);
    }
}

JsonObj GltfScene::_to_json_local() const {
    std::vector<JsonObj> scene_nodes_json;
    for (const auto &n : nodes) {
        scene_nodes_json.push_back(n->id.index);
    }
    return std::map<std::string, JsonObj>{
        {"nodes", std::move(scene_nodes_json)},
    };
}

JsonObj GltfScene::to_json() {
    // Clear indices.
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        item_id.index = SIZE_MAX;
    });

    // Re-index.
    std::map<std::string, size_t> counts;
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        auto &c = counts[type];
        if (item_id.index == SIZE_MAX || item_id.index == c) {
            item_id.index = c;
            c++;
        } else if (item_id.index > c) {
            throw std::invalid_argument("out of order");
        }
    });

    std::map<std::string, JsonObj> result{
        {"scene", 0},
        {"asset",
         std::map<std::string, JsonObj>{
             {"version", "2.0"},
         }},
    };
    for (const auto &r : counts) {
        result.insert({r.first, std::vector<JsonObj>{}});
    }
    visit([&](GltfId &item_id, const char *type, const std::function<JsonObj(void)> &to_json, uintptr_t abs_id) {
        auto &list = result.at(type).arr;
        if (item_id.index == list.size()) {
            list.push_back(to_json());
        }
    });

    return result;
}

void GltfSampler::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "samplers",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
}

JsonObj GltfSampler::to_json() const {
    return std::map<std::string, JsonObj>{
        {"magFilter", magFilter},
        {"minFilter", minFilter},
        {"wrapS", wrapS},
        {"wrapT", wrapT},
    };
}

void GltfImage::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "images",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
}

JsonObj GltfImage::to_json() const {
    return std::map<std::string, JsonObj>{
        {"name", id.name},
        {"uri", uri},
    };
}

void GltfTexture::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "textures",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
    sampler->visit(callback);
    source->visit(callback);
}

JsonObj GltfTexture::to_json() const {
    return std::map<std::string, JsonObj>{
        {"name", id.name},
        {"sampler", 0},
        {"source", 0},
    };
}

void GltfMaterial::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "materials",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
    if (texture) {
        texture->visit(callback);
    }
}

JsonObj GltfMaterial::to_json() const {
    JsonObj result = std::map<std::string, JsonObj>{
        {"name", id.name},
        {"pbrMetallicRoughness",
         std::map<std::string, JsonObj>{
             {"baseColorFactor",
              std::vector<JsonObj>{
                  base_color_factor_rgba[0],
                  base_color_factor_rgba[1],
                  base_color_factor_rgba[2],
                  base_color_factor_rgba[3],
              }},
             {"metallicFactor", metallic_factor},
             {"roughnessFactor", roughness_factor}}},
        {"doubleSided", double_sided},
    };
    if (texture) {
        result.map.at("pbrMetallicRoughness")
            .map.insert(
                {"baseColorTexture",
                 std::map<std::string, JsonObj>{
                     {"index", texture->id.index},
                     {"texCoord", 0},
                 }});
    }
    return result;
}

void GltfPrimitive::visit(const gltf_visit_callback &callback) {
    position_buffer->visit(callback);
    if (tex_coords_buffer) {
        tex_coords_buffer->visit(callback);
    }
    material->visit(callback);
}

JsonObj GltfPrimitive::to_json() const {
    std::map<std::string, JsonObj> attributes;
    attributes.insert({"POSITION", position_buffer->id.index});
    if (tex_coords_buffer) {
        attributes.insert({"TEXCOORD_0", tex_coords_buffer->id.index});
    }
    return std::map<std::string, JsonObj>{
        // Note: validator says "name" not expected for primitives.
        // {"name", id.name},
        {"attributes", std::move(attributes)},
        {"material", material->id.index},
        {"mode", element_type},
    };
}

std::shared_ptr<GltfMesh> GltfMesh::from_singleton_primitive(std::shared_ptr<GltfPrimitive> primitive) {
    return std::shared_ptr<GltfMesh>(new GltfMesh{
        {"mesh_" + primitive->id.name},
        {primitive},
    });
}

void GltfMesh::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "meshes",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
    for (auto &p : primitives) {
        p->visit(callback);
    }
}

JsonObj GltfMesh::to_json() const {
    std::vector<JsonObj> json_primitives;
    for (const auto &p : primitives) {
        json_primitives.push_back(p->to_json());
    }
    return std::map<std::string, JsonObj>{
        {"primitives", std::move(json_primitives)},
    };
}

void GltfNode::visit(const gltf_visit_callback &callback) {
    callback(
        id,
        "nodes",
        [&]() {
            return to_json();
        },
        (uintptr_t)this);
    if (mesh) {
        mesh->visit(callback);
    }
}

JsonObj GltfNode::to_json() const {
    return std::map<std::string, JsonObj>{
        {"mesh", mesh->id.index},
        {"translation", (std::vector<JsonObj>{translation.xyz[0], translation.xyz[1], translation.xyz[2]})},
    };
}

void stim_draw_internal::write_html_viewer_for_gltf_data(const std::string &gltf_data, std::ostream &out) {
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
  <div style="border: 1px dashed gray; margin-bottom: 50px; width: 512px; height: 512px; resize: both; overflow: hidden">
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

    try {
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
      let mid = new Vector3(
          (bounds.min.x + bounds.max.x) * 0.5,
          (bounds.min.y + bounds.max.y) * 0.5,
          (bounds.min.z + bounds.max.z) * 0.5,
      );
      let boxPoints = [];
      for (let dx of [0, 0.5, 1]) {
          for (let dy of [0, 0.5, 1]) {
              for (let dz of [0, 0.5, 1]) {
                  boxPoints.push(new Vector3(
                      bounds.min.x + (bounds.max.x - bounds.min.x) * dx,
                      bounds.min.y + (bounds.max.y - bounds.min.y) * dy,
                      bounds.min.z + (bounds.max.z - bounds.min.z) * dz,
                  ));
              }
          }
      }
      let isInView = p => {
          p = new Vector3(p.x, p.y, p.z);
          p.project(camera);
          return Math.abs(p.x) < 1 && Math.abs(p.y) < 1 && p.z >= 0 && p.z < 1;
      };
      let unit = new Vector3(0.3, 0.4, -1.8);
      unit.normalize();
      let setCameraDistance = d => {
          controls.target.copy(mid);
          camera.position.copy(mid);
          camera.position.addScaledVector(unit, d);
          controls.update();
          return boxPoints.every(isInView);
      };

      let maxDistance = 1;
      for (let k = 0; k < 20; k++) {
          if (setCameraDistance(maxDistance)) {
              break;
          }
          maxDistance *= 2;
      }
      let minDistance = maxDistance;
      for (let k = 0; k < 20; k++) {
          minDistance /= 2;
          if (!setCameraDistance(minDistance)) {
              break;
          }
      }
      for (let k = 0; k < 20; k++) {
          let mid = (minDistance + maxDistance) / 2;
          if (setCameraDistance(mid)) {
              maxDistance = mid;
          } else {
              minDistance = mid;
          }
      }
      setCameraDistance(maxDistance);

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
    } catch (ex) {
      container.textContent = "Failed to show model. " + ex;
      console.error(ex);
    }
  </script>
</body>
)HTML";
}
