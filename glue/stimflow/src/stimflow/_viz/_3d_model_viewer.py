from __future__ import annotations

import base64

import pygltflib

from stimflow._core import str_html


class Viewable3dModelGLTF(pygltflib.GLTF2):
    """A pygltflib.GLTF2 augmented with the ability to create a simple 3d viewer for the model."""

    def html_viewer(self) -> str_html:
        """Returns an HTML document that embeds the 3d model within a 3d viewer."""
        return html_viewer_for_gltf_model(self)

    def _repr_html_(self) -> str:
        """This method causes Jupyter notebooks to show the model using an inline HTML viewer."""
        return self.html_viewer()


def html_viewer_for_gltf_model(model: pygltflib.GLTF2) -> str_html:
    model_bytes = b"".join(model.save_to_bytes())

    model_data_uri = f"""data:text/plain;base64,{base64.b64encode(model_bytes).decode()}"""

    return str_html(
        r'''<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8" />
</head>
<body>
  <a download="model.gltf" id="stim-3d-viewer-download-link" href="'''
        + model_data_uri
        + r"""">Download 3D Model as .GLTF File</a>
  <br>Mouse Wheel = Zoom. Left Drag = Orbit. Right Drag = Strafe.
  <div id="stim-outer-container", style="border: 1px dashed gray; margin-bottom: 50px; width: 100%; height: 600px; resize: both; overflow: hidden">
    <div id="stim-3d-viewer-scene-container" style="width: 100%; height: 100%;">JavaScript Blocked?</div>
  </div>

  <script type="module">
    /// BEGIN TERRIBLE HACK.
    /// Get the object by ID then change the ID.
    /// This is a workaround for https://github.com/jupyter/notebook/issues/6598
    let outerContainer = document.getElementById("stim-outer-container");
    let container = document.getElementById("stim-3d-viewer-scene-container");
    container.id = "stim-3d-viewer-scene-container-USED";
    outerContainer.id = "stim-outer-container-USED";
    outerContainer.style.height = `${window.innerHeight-100}px`;
    let downloadLink = document.getElementById("stim-3d-viewer-download-link");
    downloadLink.id = "stim-3d-viewer-download-link-USED";
    /// END TERRIBLE HACK.

    container.textContent = "Loading viewer...";

    /// BEGIN TERRIBLE HACK.
    /// This a workaround for https://github.com/jupyter/notebook/issues/6597
    ///
    /// What this SHOULD be is:
    ///
    /// import {Box3, Scene, Color, OrthographicCamera, PerspectiveCamera, WebGLRenderer, DirectionalLight} from "three";
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
        AmbientLight,WebGLRenderer,Scene,EventDispatcher,MOUSE,Quaternion,Spherical,TOUCH,Vector2,Vector3,AnimationClip,Bone,Box3,BufferAttribute,BufferGeometry,ClampToEdgeWrapping,Color,DirectionalLight,DoubleSide,FileLoader,FrontSide,Group,ImageBitmapLoader,InterleavedBuffer,InterleavedBufferAttribute,Interpolant,InterpolateDiscrete,InterpolateLinear,Line,LineBasicMaterial,LineLoop,LineSegments,LinearFilter,LinearMipmapLinearFilter,LinearMipmapNearestFilter,Loader,LoaderUtils,Material,MathUtils,Matrix4,Mesh,MeshBasicMaterial,MeshPhysicalMaterial,MeshStandardMaterial,MirroredRepeatWrapping,NearestFilter,NearestMipmapLinearFilter,NearestMipmapNearestFilter,NumberKeyframeTrack,Object3D,OrthographicCamera,PerspectiveCamera,PointLight,Points,PointsMaterial,PropertyBinding,QuaternionKeyframeTrack,RepeatWrapping,Skeleton,SkinnedMesh,Sphere,SpotLight,TangentSpaceNormalMap,Texture,TextureLoader,TriangleFanDrawMode,TriangleStripDrawMode,VectorKeyframeTrack,sRGBEncoding
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
      mainLight.position.set(1, 2, 3);
      let backLight = new DirectionalLight(0xffffff, 4);
      backLight.position.set(-1, -2, -4);
      let ambientLight =  new AmbientLight(0xffffff, 1);
      scene.add(mainLight);
      scene.add(backLight);
      scene.add(ambientLight);
      scene.add(gltf.scene);

      // Point the camera at the center, far enough back to see everything.
      let bounds = new Box3().setFromObject(scene);
      let w = container.clientWidth;
      let h = container.clientHeight;
      let camera = new OrthographicCamera(-w/2, w/2, h/2, -h/2, 0.1, 100000);
      let controls = new OrbitControls(camera, container);
      let mid = new Vector3(
          (bounds.min.x + bounds.max.x) * 0.5,
          (bounds.min.y + bounds.max.y) * 0.5,
          (bounds.min.z + bounds.max.z) * 0.5,
      );
      let max_dx = bounds.max.x - bounds.min.x;
      let max_dy = bounds.max.y - bounds.min.y;
      let max_dz = bounds.max.z - bounds.min.z;
      let max_d = Math.sqrt(max_dx*max_dx + max_dy*max_dy + max_dz*max_dz);
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
          return Math.abs(p.x) < 1 && Math.abs(p.y) < 1;
      };
      let unit = new Vector3(0.3, 0.4, -1.8);
      unit.normalize();
      let setCameraDistance = d => {
          controls.target.copy(mid);
          camera.position.copy(mid);
          camera.position.addScaledVector(unit, max_d);
          camera.zoom = 1/d;
          camera.updateProjectionMatrix();
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
        let w = container.clientWidth;
        let h = container.clientHeight;
        camera.left = -w/2;
        camera.right = w/2;
        camera.top = h/2;
        camera.bottom = -h/2;
        camera.updateProjectionMatrix();
        controls.update();
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
    """  # noqa: E501
    )
