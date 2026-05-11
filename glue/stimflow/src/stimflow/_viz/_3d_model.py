from __future__ import annotations

import base64
import collections
from collections.abc import Iterable, Sequence
from typing import Any, cast

import numpy as np

from stimflow._viz._3d_model_text_texture import make_text_texture_data_uri
from stimflow._viz._3d_model_viewer import Viewable3dModelGLTF


class TextData:
    def __init__(
        self,
        *,
        text: str,
        start: Sequence[float],
        forward: Sequence[float],
        up: Sequence[float],
        mirror_backside: bool = True,
    ):
        """Describes a rectangle showing text.

        Args:
            text: The text to draw in the rectangle.
            start: The 3d point where the rectangle and text starts.
                This is the `bottom_left` of the rectangle, in 3d.
            forward: The 3d direction along which the text grows as the message gets longer.
                This is the `bottom_right - bottom_left` of the rectangle, in 3d.
                The length of this vector is ignored.
            up: A 3d direction along which the text is oriented.
                This is the `top_left - bottom_left` of the rectangle, in 3d.
                The length of this vector is ignored.
                Should be perpendicular to `forward`.
            mirror_backside: Determines whether or not the text on the back of the rectangle
                is mirrored (making it readable) or not (keeping the forward direction consistent).
                Defaults to True (readable on both sides).
        """
        self.text = text
        self.start = np.array(start, dtype=np.float32)
        self.forward = np.array(forward, dtype=np.float32)
        self.up = np.array(up, dtype=np.float32)
        self.mirror_backside = mirror_backside


class TriangleData:
    def __init__(self, *, rgba: tuple[float, float, float, float], triangle_list: np.ndarray):
        """Triangles with associated color information.

        Args:
            rgba: Red, green, blue, and alpha data to associate with all the triangles.
                Each value should range from 0 to 1.
                (The alpha data is ignored in most viewers, but needed by the 3d model format.)
            triangle_list: A 3d float32 numpy array with shape == (*, 3, 3).
                Axis 0 is the triangle axis (each entry is a triangle).
                Axis 1 is the ABC vertex axis (each entry is a vertex from the triangle).
                Axis 2 is the XYZ coordinate axis (each entry is a coordinate from the vertex).
        """
        assert (
            len(triangle_list.shape) == 3
            and triangle_list.shape[1] == 3
            and triangle_list.shape[2] == 3
            and triangle_list.dtype == np.float32
        )
        assert len(rgba) == 4
        assert triangle_list.shape[0] > 0
        self.rgba: tuple[float, float, float, float] = cast(Any, tuple(rgba))
        self.triangle_list: np.ndarray = triangle_list

    @staticmethod
    def rect(
        *,
        rgba: tuple[float, float, float, float],
        origin: Iterable[float],
        d1: Iterable[float],
        d2: Iterable[float],
    ) -> TriangleData:
        """Creates a pair of triangles forming a rectangle.

        Args:
            rgba: Color of the rectangle.
            origin: Bottom-left corner of the rectangle.
            d1: The right - left displacement.
            d2: The top - bottom displacement.
        """
        origin = np.array(origin, dtype=np.float32)
        d1 = np.array(d1, dtype=np.float32)
        d2 = np.array(d2, dtype=np.float32)
        p1 = origin + d1
        p2 = origin + d2
        return TriangleData(
            rgba=rgba,
            triangle_list=np.array([
                [origin, p1, p2],
                [p2, p1, p1 + d2],
            ], dtype=np.float32)
        )

    @staticmethod
    def fused(data: Iterable[TriangleData]) -> list[TriangleData]:
        """Attempts to combine triangle data instances into fewer instances."""
        groups = collections.defaultdict(list)
        for e in data:
            groups[e.rgba].append(e)
        result = []
        for rgba, group in groups.items():
            if len(group) == 1:
                result.append(group[0])
            else:
                result.append(
                    TriangleData(
                        rgba=rgba,
                        triangle_list=np.concatenate([e.triangle_list for e in group], axis=0),
                    )
                )
        return result


class LineData:
    def __init__(self, *, rgba: tuple[float, float, float, float], edge_list: np.ndarray):
        """Lines with associated color information.

        Args:
            rgba: Red, green, blue, and alpha data to associate with all the lines.
                Each value should range from 0 to 1.
                (The alpha data is ignored in most viewers, but needed by the 3d model format.)
            edge_list: A 3d float32 numpy array with shape == (*, 2, 3).
                Axis 0 is the triangle axis (each entry is a triangle).
                Axis 1 is the AB vertex axis (each entry is a vertex from the edge).
                Axis 2 is the XYZ coordinate axis (each entry is a coordinate from the vertex).
        """
        assert (
            len(edge_list.shape) == 3
            and edge_list.shape[1] == 2
            and edge_list.shape[2] == 3
            and edge_list.dtype == np.float32
        )
        assert len(rgba) == 4
        assert edge_list.shape[0] > 0
        self.rgba: tuple[float, float, float, float] = cast(Any, tuple(rgba))
        self.edge_list: np.ndarray = edge_list

    @staticmethod
    def fused(data: Iterable[LineData]) -> list[LineData]:
        """Attempts to combine line data instances into fewer instances."""
        groups = collections.defaultdict(list)
        for e in data:
            groups[e.rgba].append(e)
        result = []
        for rgba, group in groups.items():
            if len(group) == 1:
                result.append(group[0])
            else:
                result.append(
                    LineData(
                        rgba=rgba, edge_list=np.concatenate([e.edge_list for e in group], axis=0)
                    )
                )
        return result


def make_3d_model(
    elements: Iterable[TriangleData | LineData | TextData],
) -> Viewable3dModelGLTF:
    """Creates a 3d model containing the elements.

    Args:
        elements: A list of objects to include in the model. The list can include triangles
            (TriangleData), lines (LineData), and text (TextData).

    Returns:
        The 3d model, as a `stimflow.gltf_model`.

        `stimflow.gltf_model` inherits from `pygltflib.GLTF2` but adds a `_repr_html_` class
        (creating a 3d viewer in Jupyter notebooks) and a `write_viewer_to` method for
        saving a standalone HTML viewer.
    """
    import pygltflib

    triangles: list[TriangleData] = []
    lines: list[LineData] = []
    texts: list[TextData] = []
    for item in elements:
        if isinstance(item, TriangleData):
            triangles.append(item)
        elif isinstance(item, LineData):
            lines.append(item)
        elif isinstance(item, TextData):
            texts.append(item)
        else:
            raise NotImplementedError(f'make_gltf_model {item=}')

    triangles = TriangleData.fused(triangles)
    lines = LineData.fused(lines)

    gltf = Viewable3dModelGLTF()
    gltf.asset = {"version": "2.0"}

    def add_obj_index(x: list[Any], v: Any):
        x.append(v)
        return len(x) - 1

    list_coords_text_triangles: list[np.ndarray] = []
    list_uv_coords_text_triangles: list[list[float]] = []
    for t in texts:
        a = t.start
        b = a + t.forward * 0.5 * len(t.text)
        c = a + t.up
        d = b + t.up
        lines.append(
            LineData(
                rgba=(0, 0, 0, 1), edge_list=np.array([[a, b], [a, c], [b, d], [c, d]], dtype=np.float32)
            )
        )
        for direction in [-1, +1]:
            a = t.start.copy()
            if direction == -1:
                a += t.forward * 0.5 * len(t.text)
            seq = t.text
            if direction == -1 and not t.mirror_backside:
                seq = seq[::-1]
            for char in seq:
                b = a + t.forward * 0.5 * direction
                c = a + t.up
                d = b + t.up
                o = ord(char)
                if o >= 128:
                    o = 0
                u0: float = o % 16.0
                v0: float = o // 16
                u1: float = u0 + 1
                v1: float = v0 + 1
                u0 /= 16
                u1 /= 16
                v0 /= 8
                v1 /= 8
                ua = [u0, v1]
                ub = [u1, v1]
                uc = [u0, v0]
                ud = [u1, v0]
                list_coords_text_triangles.extend([a, b, c, c, b, d])
                if direction == -1 and not t.mirror_backside:
                    list_uv_coords_text_triangles.extend([ub, ua, ud, ud, ua, uc])
                else:
                    list_uv_coords_text_triangles.extend([ua, ub, uc, uc, ub, ud])
                a = b

    lines = LineData.fused(lines)

    coords_tri = (
        np.array([])
        if not triangles
        else np.concatenate([data.triangle_list for data in triangles], axis=0)
    )
    coords_edg = (
        np.array([])
        if not lines
        else np.concatenate([data.edge_list for data in lines], axis=0)
    )
    coords_text_triangles = np.array(list_coords_text_triangles, dtype=np.float32)
    uv_coords_text_triangles = np.array(list_uv_coords_text_triangles, dtype=np.float32)

    coord_data = (
        coords_tri.tobytes()
        + coords_edg.tobytes()
        + coords_text_triangles.tobytes()
        + uv_coords_text_triangles.tobytes()
    )
    buffer_bytes_b64 = base64.b64encode(coord_data).decode()
    shared_buffer_index = add_obj_index(
        gltf.buffers,
        pygltflib.Buffer(
            uri=f"data:application/octet-stream;base64,{buffer_bytes_b64}",
            byteLength=len(coord_data),
        ),
    )

    byte_offset = 0
    mesh0 = pygltflib.Mesh()
    for tri_data in triangles:
        material_index = add_obj_index(
            gltf.materials,
            pygltflib.Material(
                pbrMetallicRoughness=pygltflib.PbrMetallicRoughness(
                    baseColorFactor=list(tri_data.rgba), roughnessFactor=0.8, metallicFactor=0.3
                ),
                emissiveFactor=None,
                doubleSided=True,
            ),
        )
        byte_length = tri_data.triangle_list.shape[0] * 3 * 3 * 4
        buffer_view_index = add_obj_index(
            gltf.bufferViews,
            pygltflib.BufferView(
                buffer=shared_buffer_index,
                byteOffset=byte_offset,
                byteLength=byte_length,
                target=pygltflib.ARRAY_BUFFER,
            ),
        )
        byte_offset += byte_length
        accessor_index = add_obj_index(
            gltf.accessors,
            pygltflib.Accessor(
                bufferView=buffer_view_index,
                byteOffset=0,
                componentType=pygltflib.FLOAT,
                count=tri_data.triangle_list.shape[0] * 3,
                type=pygltflib.VEC3,
                max=[float(e) for e in np.max(tri_data.triangle_list, axis=(0, 1))],
                min=[float(e) for e in np.min(tri_data.triangle_list, axis=(0, 1))],
            ),
        )
        mesh0.primitives.append(
            pygltflib.Primitive(
                material=material_index,
                mode=pygltflib.TRIANGLES,
                attributes=pygltflib.Attributes(POSITION=accessor_index, TEXCOORD_0=accessor_index),
            )
        )
    for line_data in lines:
        material_index = add_obj_index(
            gltf.materials,
            pygltflib.Material(
                pbrMetallicRoughness=pygltflib.PbrMetallicRoughness(
                    baseColorFactor=list(line_data.rgba), roughnessFactor=0.8, metallicFactor=0.3
                )
            ),
        )

        byte_length = line_data.edge_list.shape[0] * 3 * 2 * 4
        buffer_view_index = add_obj_index(
            gltf.bufferViews,
            pygltflib.BufferView(
                buffer=shared_buffer_index,
                byteOffset=byte_offset,
                byteLength=byte_length,
                target=pygltflib.ARRAY_BUFFER,
            ),
        )
        byte_offset += byte_length
        accessor_index = add_obj_index(
            gltf.accessors,
            pygltflib.Accessor(
                bufferView=buffer_view_index,
                byteOffset=0,
                componentType=pygltflib.FLOAT,
                count=line_data.edge_list.shape[0] * 2,
                type=pygltflib.VEC3,
                max=[float(e) for e in np.max(line_data.edge_list, axis=(0, 1))],
                min=[float(e) for e in np.min(line_data.edge_list, axis=(0, 1))],
            ),
        )
        mesh0.primitives.append(
            pygltflib.Primitive(
                material=material_index,
                mode=pygltflib.LINES,
                attributes=pygltflib.Attributes(POSITION=accessor_index),
            )
        )

    if texts:
        text_image_index = add_obj_index(
            gltf.images, pygltflib.Image(uri=make_text_texture_data_uri())
        )
        text_sampler_index = add_obj_index(
            gltf.samplers,
            pygltflib.Sampler(
                magFilter=pygltflib.NEAREST_MIPMAP_NEAREST,
                minFilter=pygltflib.NEAREST_MIPMAP_NEAREST,
                wrapS=pygltflib.CLAMP_TO_EDGE,
                wrapT=pygltflib.CLAMP_TO_EDGE,
            ),
        )
        text_texture_index = add_obj_index(
            gltf.textures, pygltflib.Texture(sampler=text_sampler_index, source=text_image_index)
        )
        text_material_index = add_obj_index(
            gltf.materials,
            pygltflib.Material(
                pbrMetallicRoughness=pygltflib.PbrMetallicRoughness(
                    metallicFactor=0.1,
                    roughnessFactor=0.9,
                    baseColorTexture=pygltflib.TextureInfo(index=text_texture_index),
                ),
                doubleSided=False,
            ),
        )
        byte_length = coords_text_triangles.shape[0] * 3 * 4
        buffer_view_index = add_obj_index(
            gltf.bufferViews,
            pygltflib.BufferView(
                buffer=shared_buffer_index,
                byteOffset=byte_offset,
                byteLength=byte_length,
                target=pygltflib.ARRAY_BUFFER,
            ),
        )
        byte_offset += byte_length
        accessor_index = add_obj_index(
            gltf.accessors,
            pygltflib.Accessor(
                bufferView=buffer_view_index,
                byteOffset=0,
                componentType=pygltflib.FLOAT,
                count=coords_text_triangles.shape[0],
                type=pygltflib.VEC3,
                max=[float(e) for e in np.max(coords_text_triangles, axis=0)],
                min=[float(e) for e in np.min(coords_text_triangles, axis=0)],
            ),
        )

        byte_length = uv_coords_text_triangles.shape[0] * 2 * 4
        buffer_view_index_2 = add_obj_index(
            gltf.bufferViews,
            pygltflib.BufferView(
                buffer=shared_buffer_index,
                byteOffset=byte_offset,
                byteLength=byte_length,
                target=pygltflib.ARRAY_BUFFER,
            ),
        )
        byte_offset += byte_length
        accessor_index_2 = add_obj_index(
            gltf.accessors,
            pygltflib.Accessor(
                bufferView=buffer_view_index_2,
                byteOffset=0,
                componentType=pygltflib.FLOAT,
                count=uv_coords_text_triangles.shape[0],
                type=pygltflib.VEC2,
                max=[float(e) for e in np.max(uv_coords_text_triangles, axis=0)],
                min=[float(e) for e in np.min(uv_coords_text_triangles, axis=0)],
            ),
        )
        mesh0.primitives.append(
            pygltflib.Primitive(
                material=text_material_index,
                mode=pygltflib.TRIANGLES,
                attributes=pygltflib.Attributes(
                    POSITION=accessor_index, TEXCOORD_0=accessor_index_2
                ),
            )
        )

    mesh0_index = add_obj_index(gltf.meshes, mesh0)
    node0 = pygltflib.Node(mesh=mesh0_index)
    node0_index = add_obj_index(gltf.nodes, node0)
    gltf.scenes.append(pygltflib.Scene(nodes=[node0_index]))

    return gltf
