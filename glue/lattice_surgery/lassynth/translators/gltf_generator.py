"""generating a 3D modelling file in gltf format from our LaSRe."""

import json
from typing import Any, Mapping, Optional, Sequence, Tuple

# constants
SQ2 = 0.707106769085  # square root of 2
THICKNESS = 0.01  # half separation of front and back sides of each face
AXESTHICKNESS = 0.1


def float_to_little_endian_hex(f):
    from struct import pack

    # Pack the float into a binary string using the little-endian format
    binary_data = pack("<f", f)

    # Convert the binary string to a hexadecimal string
    hex_string = binary_data.hex()

    return hex_string


def hex_to_bin(s):
    from codecs import encode, decode

    s = encode(decode(s, "hex"), "base64").decode()
    s = str(s).replace("\n", "")
    return "data:application/octet-stream;base64," + s


def base_gen(
    tubelen: float = 2.0,
) -> Mapping[str, Any]:
    """generate basic gltf contents, i.e., independent from the LaS

    Args:
        tubelen (float, optional): ratio of the length of the pipe with respect
            to the length of a cube. Defaults to 2.0.

    Returns:
        Mapping[str, Any]: gltf with everything here.
    """
    # floats as hex, little endian
    floats = {
        "0": "00000000",
        "1": "0000803F",
        "-1": "000080BF",
        "0.5": "0000003F",
        "0.45": "6666E63E",
    }
    floats["tube"] = float_to_little_endian_hex(tubelen)
    floats["+SQ2"] = float_to_little_endian_hex(SQ2)
    floats["-SQ2"] = float_to_little_endian_hex(-SQ2)
    floats["+T"] = float_to_little_endian_hex(THICKNESS)
    floats["-T"] = float_to_little_endian_hex(-THICKNESS)
    floats["1-T"] = float_to_little_endian_hex(1 - THICKNESS)
    floats["T-1"] = float_to_little_endian_hex(THICKNESS - 1)
    floats["0.5+T"] = float_to_little_endian_hex(0.5 + THICKNESS)
    floats["0.5-T"] = float_to_little_endian_hex(0.5 - THICKNESS)

    # integers as hex
    ints = ["0000", "0100", "0200", "0300", "0400", "0500", "0600", "0700"]

    gltf = {
        "asset": {"generator": "LaSRe CodeGen by Daniel Bochen Tan", "version": "2.0"},
        "scene": 0,
        "scenes": [{"name": "Scene", "nodes": [0]}],
        "nodes": [{"name": "Lattice Surgery Subroutine", "children": []}],
    }
    gltf["accessors"] = []
    gltf["buffers"] = []
    gltf["bufferViews"] = []

    # materials are the colors. baseColorFactor is (R, G, B, alpha)
    gltf["materials"] = [
        {
            "name": "0-blue",
            "pbrMetallicRoughness": {"baseColorFactor": [0, 0, 1, 1]},
            "doubleSided": False,
        },
        {
            "name": "1-red",
            "pbrMetallicRoughness": {"baseColorFactor": [1, 0, 0, 1]},
            "doubleSided": False,
        },
        {
            "name": "2-green",
            "pbrMetallicRoughness": {"baseColorFactor": [0, 1, 0, 1]},
            "doubleSided": False,
        },
        {
            "name": "3-gray",
            "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 1]},
            "doubleSided": False,
        },
        {
            "name": "4-cyan.3",
            "pbrMetallicRoughness": {"baseColorFactor": [0, 1, 1, 0.3]},
            "doubleSided": False,
            "alphaMode": "BLEND",
        },
        {
            "name": "5-black",
            "pbrMetallicRoughness": {"baseColorFactor": [0, 0, 0, 1]},
            "doubleSided": False,
        },
        {
            "name": "6-yellow",
            "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 0, 1]},
            "doubleSided": False,
        },
        {
            "name": "7-white",
            "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1]},
            "doubleSided": False,
        },
    ]

    # for a 3D coordinate (X,Y,Z), the convention of VEC3 in GLTF is (X,Z,-Y)
    # below are the data we store into the embedded binary in the GLTF.
    # For each data, we create a buffer, there is one and only one bufferView
    # for this buffer, and there is one and only one accessor for this
    # bufferView. This is for simplicity. So in what follows, we always gather
    # the string corresponding to the data, whether they're a list of floats or
    # a list of integers. Then, we append a buffer, a bufferView, and an
    # accessor to the GLTF. This part is quite machinary.

    # GLTF itself support doubleside color in materials, but this can lead to
    # problems when converting to other formats. So, for each face of a cube or
    # a pipe, we will make it two sides, front and back. The POSITION of
    # vertices of these two are shifted on the Z axis by 2*THICKNESS. Since we
    # need their color to both facing outside, the back side require opposite
    # NORMAL vectors, and the index order needs to be reversed. We begin with
    # definition for the front sides.

    # 0, positions of square: [(+T,+T,-T),(1-T,+T,-T),(+T,1-T,-T),(1-T,1-T,-T)]
    s = (
        floats["+T"]
        + floats["-T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["-T"]
        + floats["-T"]
        + floats["+T"]
        + floats["-T"]
        + floats["T-1"]
        + floats["1-T"]
        + floats["-T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 0, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 0,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, -THICKNESS, -THICKNESS],
            "min": [THICKNESS, -THICKNESS, THICKNESS - 1],
        }
    )

    # 1, positions of rectangle: [(0,0,-T),(L,0,-T),(0,1,-T),(L,1,-T)]
    s = (
        floats["0"]
        + floats["-T"]
        + floats["0"]
        + floats["tube"]
        + floats["-T"]
        + floats["0"]
        + floats["0"]
        + floats["-T"]
        + floats["-1"]
        + floats["tube"]
        + floats["-T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 1, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 1,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [tubelen, -THICKNESS, 0],
            "min": [0, -THICKNESS, -1],
        }
    )

    # 2, normals of rect/sqr: (0,0,-1)*4
    s = (
        floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 2, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 2, "componentType": 5126, "type": "VEC3", "count": 4}
    )

    # 3, vertices of rect/sqr: [1,0,3, 3,0,2]
    s = ints[1] + ints[0] + ints[3] + ints[3] + ints[0] + ints[2]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 3, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 3, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 4, positions of tilted rect: [(0,0,1/2+T),(1/2,0,+T),(0,1,1/2+T),(1/2,1,+T)]
    s = (
        floats["0"]
        + floats["0.5+T"]
        + floats["0"]
        + floats["0.5"]
        + floats["+T"]
        + floats["0"]
        + floats["0"]
        + floats["0.5+T"]
        + floats["-1"]
        + floats["0.5"]
        + floats["+T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 4, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 4,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [0.5, 0.5 + THICKNESS, 0],
            "min": [0, THICKNESS, -1],
        }
    )

    # 5, normals of tilted rect: (-sqrt(2)/2, 0, -sqrt(2)/2)*4
    s = (
        floats["-SQ2"]
        + floats["-SQ2"]
        + floats["0"]
        + floats["-SQ2"]
        + floats["-SQ2"]
        + floats["0"]
        + floats["-SQ2"]
        + floats["-SQ2"]
        + floats["0"]
        + floats["-SQ2"]
        + floats["-SQ2"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 5, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 5, "componentType": 5126, "type": "VEC3", "count": 4}
    )

    # 6, positions of Hadamard rectangle: [(0,0,-T),(15/32L,0,-T),(15/32L,1,-T),
    # (15/32L,1,-T),(17/32L,0,-T),(17/32L,1,-T),(L,0,-T),(L,1,-T)]
    floats["left"] = float_to_little_endian_hex(tubelen * 15 / 32)
    floats["right"] = float_to_little_endian_hex(tubelen * 17 / 32)
    s = (
        floats["0"]
        + floats["-T"]
        + floats["0"]
        + floats["left"]
        + floats["-T"]
        + floats["0"]
        + floats["0"]
        + floats["-T"]
        + floats["-1"]
        + floats["left"]
        + floats["-T"]
        + floats["-1"]
        + floats["right"]
        + floats["-T"]
        + floats["0"]
        + floats["right"]
        + floats["-T"]
        + floats["-1"]
        + floats["tube"]
        + floats["-T"]
        + floats["0"]
        + floats["tube"]
        + floats["-T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 96, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 6, "byteLength": 96, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 6,
            "componentType": 5126,
            "type": "VEC3",
            "count": 8,
            "max": [tubelen, -THICKNESS, 0],
            "min": [0, -THICKNESS, -1],
        }
    )

    # 7, normals of Hadamard rect (0,0,-1)*8
    s = (
        floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
        + floats["0"]
        + floats["-1"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 96, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 7, "byteLength": 96, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 7, "componentType": 5126, "type": "VEC3", "count": 8}
    )

    # 8, vertices of middle rect in Hadamard rect: [4,1,5, 5,1,3]
    s = ints[4] + ints[1] + ints[5] + ints[5] + ints[1] + ints[3]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 8, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 8, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 9, vertices of upper rect in Hadamard rect: [6,4,7, 7,4,5]
    s = ints[6] + ints[4] + ints[7] + ints[7] + ints[4] + ints[5]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 9, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 9, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 10, vertices of lines around a face: [0,1, 0,2, 2,3, 3,1]
    # GLTF supports drawing lines, but there may be a problem converting to
    # other formats. We have thus not used these data.
    s = ints[0] + ints[1] + ints[0] + ints[2] + ints[2] + ints[3] + ints[3] + ints[1]
    gltf["buffers"].append({"byteLength": 16, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 10, "byteLength": 16, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 10, "componentType": 5123, "type": "SCALAR", "count": 8}
    )

    # 11, positions of half-distance rectangle: [(0,0,-T),(0.45,0,-T),(0,1,-T),(0.45,1,-T)]
    s = (
        floats["0"]
        + floats["-T"]
        + floats["0"]
        + floats["0.45"]
        + floats["-T"]
        + floats["0"]
        + floats["0"]
        + floats["-T"]
        + floats["-1"]
        + floats["0.45"]
        + floats["-T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 11, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 11,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [0.45, -THICKNESS, 0],
            "min": [0, -THICKNESS, -1],
        }
    )

    # 12, backside, positions of square: [(+T,+T,+T),(1-T,+T,+T),(+T,1-T,+T),(1-T,1-T,+T)]
    s = (
        floats["+T"]
        + floats["+T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["+T"]
        + floats["-T"]
        + floats["+T"]
        + floats["+T"]
        + floats["T-1"]
        + floats["1-T"]
        + floats["+T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 12, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 12,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, THICKNESS, -THICKNESS],
            "min": [THICKNESS, THICKNESS, THICKNESS - 1],
        }
    )

    # 13, backside, normals of rect/sqr: (0,0,1)*4
    s = (
        floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 13, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 13, "componentType": 5126, "type": "VEC3", "count": 4}
    )

    # For the cubes, we want to draw black lines around its boundaries to help
    # people identify them visually. However, drawing lines in GLTF may become
    # a problem when converting to other formats. Here, we define super thin
    # rectangles at the boundaries of squares which will be seen as lines.

    # 14, frontside, positions of edge 0: [(+T,0,-T),(1-T,0,-T),(+T,+T,-T),(1-T,+T,-T)]
    s = (
        floats["+T"]
        + floats["-T"]
        + floats["0"]
        + floats["1-T"]
        + floats["-T"]
        + floats["0"]
        + floats["+T"]
        + floats["-T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["-T"]
        + floats["-T"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 14, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 14,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, -THICKNESS, 0],
            "min": [THICKNESS, -THICKNESS, -THICKNESS],
        }
    )

    # 15, frontside, positions of edge 1: [(1-T,+T,-T),(1,+T,-T),(1-T,1-T,-T),(1,1-T,-T)]
    s = (
        floats["1-T"]
        + floats["-T"]
        + floats["-T"]
        + floats["1"]
        + floats["-T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["-T"]
        + floats["T-1"]
        + floats["1"]
        + floats["-T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 15, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 15,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1, -THICKNESS, -THICKNESS],
            "min": [1 - THICKNESS, -THICKNESS, THICKNESS - 1],
        }
    )

    # 16, frontside, positions of edge 2: [(0,+T,-T),(+T,+T,-T),(0,1-T,-T),(+T,1-T,-T)]
    s = (
        floats["0"]
        + floats["-T"]
        + floats["-T"]
        + floats["+T"]
        + floats["-T"]
        + floats["-T"]
        + floats["0"]
        + floats["-T"]
        + floats["T-1"]
        + floats["+T"]
        + floats["-T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 16, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 16,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [THICKNESS, -THICKNESS, -THICKNESS],
            "min": [0, -THICKNESS, THICKNESS - 1],
        }
    )

    # 17, frontside, positions of edge 3: [(+T,1-T,-T),(1-T,1-T,-T),(+T,1,-T),(1-T,1,-T)]
    s = (
        floats["+T"]
        + floats["-T"]
        + floats["T-1"]
        + floats["1-T"]
        + floats["-T"]
        + floats["T-1"]
        + floats["+T"]
        + floats["-T"]
        + floats["-1"]
        + floats["1-T"]
        + floats["-T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 17, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 17,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, -THICKNESS, THICKNESS - 1],
            "min": [THICKNESS, -THICKNESS, -1],
        }
    )

    # 18, backside, positions of edge 0: [(+T,0,+T),(1-T,0,+T),(+T,+T,+T),(1-T,+T,+T)]
    s = (
        floats["+T"]
        + floats["+T"]
        + floats["0"]
        + floats["1-T"]
        + floats["+T"]
        + floats["0"]
        + floats["+T"]
        + floats["+T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["+T"]
        + floats["-T"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 18, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 18,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, THICKNESS, 0],
            "min": [THICKNESS, THICKNESS, -THICKNESS],
        }
    )

    # 19, backside, positions of edge 1: [(1-T,+T,+T),(1,+T,+T),(1-T,1-T,+T),(1,1-T,+T)]
    s = (
        floats["1-T"]
        + floats["+T"]
        + floats["-T"]
        + floats["1"]
        + floats["+T"]
        + floats["-T"]
        + floats["1-T"]
        + floats["+T"]
        + floats["T-1"]
        + floats["1"]
        + floats["+T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 19, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 19,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1, THICKNESS, -THICKNESS],
            "min": [1 - THICKNESS, THICKNESS, THICKNESS - 1],
        }
    )

    # 20, backside, positions of edge 2: [(0,+T,+T),(+T,+T,+T),(0,1-T,+T),(+T,1-T,+T)]
    s = (
        floats["0"]
        + floats["+T"]
        + floats["-T"]
        + floats["+T"]
        + floats["+T"]
        + floats["-T"]
        + floats["0"]
        + floats["+T"]
        + floats["T-1"]
        + floats["+T"]
        + floats["+T"]
        + floats["T-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 20, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 20,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [THICKNESS, THICKNESS, -THICKNESS],
            "min": [0, THICKNESS, THICKNESS - 1],
        }
    )

    # 21, backside, positions of edge 3: [(+T,1-T,+T),(1-T,1-T,+T),(+T,1,+T),(1-T,1,+T)]
    s = (
        floats["+T"]
        + floats["+T"]
        + floats["T-1"]
        + floats["1-T"]
        + floats["+T"]
        + floats["T-1"]
        + floats["+T"]
        + floats["+T"]
        + floats["-1"]
        + floats["1-T"]
        + floats["+T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 21, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 21,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [1 - THICKNESS, THICKNESS, THICKNESS - 1],
            "min": [THICKNESS, THICKNESS, -1],
        }
    )

    # 22, backside vertices of rect/sqr: [1,3,0, 3,2,0]
    s = ints[1] + ints[3] + ints[0] + ints[3] + ints[2] + ints[0]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 22, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 22, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 23, backside, positions of rectangle: [(0,0,+T),(L,0,+T),(0,1,+T),(L,1,+T)]
    s = (
        floats["0"]
        + floats["+T"]
        + floats["0"]
        + floats["tube"]
        + floats["+T"]
        + floats["0"]
        + floats["0"]
        + floats["+T"]
        + floats["-1"]
        + floats["tube"]
        + floats["+T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 23, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 23,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [tubelen, THICKNESS, 0],
            "min": [0, THICKNESS, -1],
        }
    )

    # 24, backside, positions of half-distance rectangle: [(0,0,+T),(0.45,0,+T),(0,1,+T),(0.45,1,+T)]
    s = (
        floats["0"]
        + floats["+T"]
        + floats["0"]
        + floats["0.45"]
        + floats["+T"]
        + floats["0"]
        + floats["0"]
        + floats["+T"]
        + floats["-1"]
        + floats["0.45"]
        + floats["+T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 24, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 24,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [0.45, THICKNESS, 0],
            "min": [0, THICKNESS, -1],
        }
    )

    # 25, backside, positions of Hadamard rectangle: [(0,0,+T),(15/32L,0,+T),(15/32L,1,+T),
    # (15/32L,1,+T),(17/32L,0,+T),(17/32L,1,+T),(L,0,+T),(L,1,+T)]
    floats["left"] = float_to_little_endian_hex(tubelen * 15 / 32)
    floats["right"] = float_to_little_endian_hex(tubelen * 17 / 32)
    s = (
        floats["0"]
        + floats["+T"]
        + floats["0"]
        + floats["left"]
        + floats["+T"]
        + floats["0"]
        + floats["0"]
        + floats["+T"]
        + floats["-1"]
        + floats["left"]
        + floats["+T"]
        + floats["-1"]
        + floats["right"]
        + floats["+T"]
        + floats["0"]
        + floats["right"]
        + floats["+T"]
        + floats["-1"]
        + floats["tube"]
        + floats["+T"]
        + floats["0"]
        + floats["tube"]
        + floats["+T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 96, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 25, "byteLength": 96, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 25,
            "componentType": 5126,
            "type": "VEC3",
            "count": 8,
            "max": [tubelen, THICKNESS, 0],
            "min": [0, THICKNESS, -1],
        }
    )

    # 26, backside, normals of Hadamard rect (0,0,1)*8
    s = (
        floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
        + floats["0"]
        + floats["1"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 96, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 26, "byteLength": 96, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 26, "componentType": 5126, "type": "VEC3", "count": 8}
    )

    # 27, backside, vertices of middle rect in Hadamard rect: [4,5,1, 5,3,1]
    s = ints[4] + ints[5] + ints[1] + ints[5] + ints[3] + ints[1]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 27, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 27, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 28, backside, vertices of upper rect in Hadamard rect: [6,7,4, 7,5,4]
    s = ints[6] + ints[7] + ints[4] + ints[7] + ints[5] + ints[4]
    gltf["buffers"].append({"byteLength": 12, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 28, "byteLength": 12, "byteOffset": 0, "target": 34963}
    )
    gltf["accessors"].append(
        {"bufferView": 28, "componentType": 5123, "type": "SCALAR", "count": 6}
    )

    # 29, backside, positions of tilted rect: [(0,0,1/2+T),(1/2,0,+T),(0,1,1/2+T),(1/2,1,+T)]
    s = (
        floats["0"]
        + floats["0.5-T"]
        + floats["0"]
        + floats["0.5"]
        + floats["-T"]
        + floats["0"]
        + floats["0"]
        + floats["0.5-T"]
        + floats["-1"]
        + floats["0.5"]
        + floats["-T"]
        + floats["-1"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 29, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {
            "bufferView": 29,
            "componentType": 5126,
            "type": "VEC3",
            "count": 4,
            "max": [0.5, 0.5 - THICKNESS, 0],
            "min": [0, -THICKNESS, -1],
        }
    )

    # 30, backside, normals of tilted rect: (sqrt(2)/2, 0, sqrt(2)/2)*4
    s = (
        floats["+SQ2"]
        + floats["+SQ2"]
        + floats["0"]
        + floats["+SQ2"]
        + floats["+SQ2"]
        + floats["0"]
        + floats["+SQ2"]
        + floats["+SQ2"]
        + floats["0"]
        + floats["+SQ2"]
        + floats["+SQ2"]
        + floats["0"]
    )
    gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
    gltf["bufferViews"].append(
        {"buffer": 30, "byteLength": 48, "byteOffset": 0, "target": 34962}
    )
    gltf["accessors"].append(
        {"bufferView": 30, "componentType": 5126, "type": "VEC3", "count": 4}
    )

    # finished creating the binary

    # Now we create meshes. These are the real constructors of the 3D diagram.
    # a mesh can contain multiple primitives. A primitive can be defined by a
    # set of vertices POSITION, their NORMAL vectors, the order of going around
    # these vertices, and the color (material) for the triangles defined by
    # going around the vertices. `mode:4` means color these triangles.
    gltf["meshes"] = [
        {
            "name": "0-square-blue",
            "primitives": [
                # front side
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 0,
                    "mode": 4,
                },
                # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 0,
                    "mode": 4,
                },
                # front side edge 0
                {
                    "attributes": {"NORMAL": 2, "POSITION": 14},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 1
                {
                    "attributes": {"NORMAL": 2, "POSITION": 15},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 2
                {
                    "attributes": {"NORMAL": 2, "POSITION": 16},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 3
                {
                    "attributes": {"NORMAL": 2, "POSITION": 17},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 0
                {
                    "attributes": {"NORMAL": 13, "POSITION": 18},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 1
                {
                    "attributes": {"NORMAL": 13, "POSITION": 19},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 2
                {
                    "attributes": {"NORMAL": 13, "POSITION": 20},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 3
                {
                    "attributes": {"NORMAL": 13, "POSITION": 21},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "1-square-red",
            "primitives": [
                # front side
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 1,
                    "mode": 4,
                },
                # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 1,
                    "mode": 4,
                },
                # front side edge 0
                {
                    "attributes": {"NORMAL": 2, "POSITION": 14},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 1
                {
                    "attributes": {"NORMAL": 2, "POSITION": 15},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 2
                {
                    "attributes": {"NORMAL": 2, "POSITION": 16},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # front side edge 3
                {
                    "attributes": {"NORMAL": 2, "POSITION": 17},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 0
                {
                    "attributes": {"NORMAL": 13, "POSITION": 18},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 1
                {
                    "attributes": {"NORMAL": 13, "POSITION": 19},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 2
                {
                    "attributes": {"NORMAL": 13, "POSITION": 20},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
                # back side edge 3
                {
                    "attributes": {"NORMAL": 13, "POSITION": 21},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "2-square-gray",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 3,
                    "mode": 4,
                },
                # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 3,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "3-square-green",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 2,
                    "mode": 4,
                },  # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 2,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "4-rectangle-blue",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 1},
                    "indices": 3,
                    "material": 0,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 13, "POSITION": 23},
                    "indices": 22,
                    "material": 0,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "5-rectangle-red",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 1},
                    "indices": 3,
                    "material": 1,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 13, "POSITION": 23},
                    "indices": 22,
                    "material": 1,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "6-rectangle-gray",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 1},
                    "indices": 3,
                    "material": 3,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 13, "POSITION": 23},
                    "indices": 22,
                    "material": 3,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "7-rectangle-red/yellow/blue",
            "primitives": [
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 3,
                    "material": 1,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 8,
                    "material": 6,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 9,
                    "material": 0,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 22,
                    "material": 1,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 27,
                    "material": 6,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 28,
                    "material": 0,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "8-rectangle-blue/yellow/red",
            "primitives": [
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 3,
                    "material": 0,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 8,
                    "material": 6,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 7, "POSITION": 6},
                    "indices": 9,
                    "material": 1,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 22,
                    "material": 0,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 27,
                    "material": 6,
                    "mode": 4,
                },
                {
                    "attributes": {"NORMAL": 26, "POSITION": 25},
                    "indices": 28,
                    "material": 1,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "9-square-cyan.3",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 4,
                    "mode": 4,
                },  # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 4,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "10-rectangle-cyan.3",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 1},
                    "indices": 3,
                    "material": 4,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 13, "POSITION": 23},
                    "indices": 22,
                    "material": 4,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "11-tilted-cyan.3",
            "primitives": [
                {
                    "attributes": {"NORMAL": 5, "POSITION": 4},
                    "indices": 3,
                    "material": 4,
                    "mode": 4,
                },
                # backside
                {
                    "attributes": {"NORMAL": 30, "POSITION": 29},
                    "indices": 22,
                    "material": 4,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "12-half-distance-rectangle-green",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 11},
                    "indices": 3,
                    "material": 2,
                    "mode": 4,
                },
                # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 24},
                    "indices": 22,
                    "material": 2,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "13-square-black",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 0},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },  # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 12},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
            ],
        },
        {
            "name": "14-half-distance-rectangle-black",
            "primitives": [
                {
                    "attributes": {"NORMAL": 2, "POSITION": 11},
                    "indices": 3,
                    "material": 5,
                    "mode": 4,
                },
                # back side
                {
                    "attributes": {"NORMAL": 13, "POSITION": 24},
                    "indices": 22,
                    "material": 5,
                    "mode": 4,
                },
            ],
        },
    ]

    return gltf


def axes_gen(
    SEP: float, max_i: int, max_j: int, max_k: int
) -> Sequence[Mapping[str, Any]]:
    rectangles = []

    # I axis, red
    rectangles += [
        {
            "name": f"axisI:-K",
            "mesh": 5,
            "translation": [-0.5, -0.5, 0.5],
            "scale": [SEP * max_i / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisI:+K",
            "mesh": 5,
            "translation": [-0.5, -0.5 + AXESTHICKNESS, 0.5],
            "scale": [SEP * max_i / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisI:-J",
            "mesh": 5,
            "translation": [-0.5, -0.5, 0.5],
            "rotation": [SQ2, 0, 0, SQ2],
            "scale": [SEP * max_i / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisI:+J",
            "mesh": 5,
            "translation": [-0.5, -0.5, 0.5 - AXESTHICKNESS],
            "rotation": [SQ2, 0, 0, SQ2],
            "scale": [SEP * max_i / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
    ]

    # J axis, green
    rectangles += [
        {
            "name": f"axisJ:-K",
            "rotation": [0, SQ2, 0, SQ2],
            "translation": [-0.5 + AXESTHICKNESS, -0.5, 0.5],
            "mesh": 3,
            "scale": [SEP * max_j, AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisJ:+K",
            "rotation": [0, SQ2, 0, SQ2],
            "translation": [
                -0.5 + AXESTHICKNESS,
                -0.5 + AXESTHICKNESS,
                0.5,
            ],
            "mesh": 3,
            "scale": [SEP * max_j, AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisJ:-I",
            "rotation": [0.5, 0.5, -0.5, 0.5],
            "translation": [-0.5, -0.5, 0.5],
            "mesh": 3,
            "scale": [SEP * max_j, AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisJ:+I",
            "rotation": [0.5, 0.5, -0.5, 0.5],
            "translation": [-0.5 + AXESTHICKNESS, -0.5, 0.5],
            "mesh": 3,
            "scale": [SEP * max_j, AXESTHICKNESS, AXESTHICKNESS],
        },
    ]

    # K axis, blue
    rectangles += [
        {
            "name": f"axisK:-I",
            "mesh": 4,
            "rotation": [0, 0, SQ2, SQ2],
            "translation": [-0.5, -0.5 + AXESTHICKNESS, 0.5],
            "scale": [SEP * max_k / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisK:+I",
            "mesh": 4,
            "rotation": [0, 0, SQ2, SQ2],
            "translation": [-0.5 + AXESTHICKNESS, -0.5 + AXESTHICKNESS, 0.5],
            "scale": [SEP * max_k / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisK:-J",
            "mesh": 4,
            "rotation": [0.5, 0.5, 0.5, 0.5],
            "translation": [-0.5 + AXESTHICKNESS, -0.5 + AXESTHICKNESS, 0.5],
            "scale": [SEP * max_k / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
        {
            "name": f"axisK:+J",
            "mesh": 4,
            "rotation": [0.5, 0.5, 0.5, 0.5],
            "translation": [
                -0.5 + AXESTHICKNESS,
                -0.5 + AXESTHICKNESS,
                0.5 - AXESTHICKNESS,
            ],
            "scale": [SEP * max_k / (SEP - 1), AXESTHICKNESS, AXESTHICKNESS],
        },
    ]

    return rectangles


def tube_gen(
    SEP: float,
    loc: Tuple[int, int, int],
    dir: str,
    color: int,
    stabilizer: int,
    corr: Tuple[int, int],
    noColor: bool,
    rm_dir: str,
) -> Sequence[Mapping[str, Any]]:
    """compute the GLTF nodes for a pipe. This can include its four faces and
    correlation surface inside, minus the face to remove specified by rm_dir.

    Args:
        SEP (float): the distance, e.g., from I-pipe(i,j,k) to I-pipe(i+1,j,k).
        loc (Tuple[int, int, int]): 3D coordinate of the pipe.
        dir (str): direction of the pipe, "I", "J", or "K".
        color (int): color variable of the pipe, can be -1(unknown), 0, or 1.
        stabilizer (int): index of the stabilizer.
        corr (Tuple[int, int]): two bits for two possible corr surface inside.
        noColor (bool): K-pipe are not colored if this is True.
        rm_dir (str): the direction of face to remove. if a stabilier is shown.

    Returns:
        Sequence[Mapping[str, Any]]: list of constructed GLTF nodes, typically
            4 or 5 contiguous nodes in the list corredpond to one pipe.
    """
    rectangles = []
    if dir == "I":
        rectangles = [
            {
                "name": f"edgeI{loc}:-K",
                "mesh": 4 if color else 5,
                "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
            },
            {
                "name": f"edgeI{loc}:+K",
                "mesh": 4 if color else 5,
                "translation": [1 + SEP * loc[0], 1 + SEP * loc[2], -SEP * loc[1]],
            },
            {
                "name": f"edgeI{loc}:-J",
                "mesh": 5 if color else 4,
                "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
                "rotation": [SQ2, 0, 0, SQ2],
            },
            {
                "name": f"edgeI{loc}:+J",
                "mesh": 5 if color else 4,
                "translation": [1 + SEP * loc[0], SEP * loc[2], -1 - SEP * loc[1]],
                "rotation": [SQ2, 0, 0, SQ2],
            },
        ]
        if corr[0]:
            rectangles.append(
                {
                    "name": f"edgeI{loc}:CorrIJ",
                    "mesh": 10,
                    "translation": [
                        1 + SEP * loc[0],
                        0.5 + SEP * loc[2],
                        -SEP * loc[1],
                    ],
                }
            )
        if corr[1]:
            rectangles.append(
                {
                    "name": f"edgeI{loc}:CorrIK",
                    "mesh": 10,
                    "translation": [
                        1 + SEP * loc[0],
                        SEP * loc[2],
                        -0.5 - SEP * loc[1],
                    ],
                    "rotation": [SQ2, 0, 0, SQ2],
                }
            )
    elif dir == "J":
        rectangles = [
            {
                "name": f"edgeJ{loc}:-K",
                "rotation": [0, SQ2, 0, SQ2],
                "translation": [1 + SEP * loc[0], SEP * loc[2], -1 - SEP * loc[1]],
                "mesh": 5 if color else 4,
            },
            {
                "name": f"edgeJ{loc}:+K",
                "rotation": [0, SQ2, 0, SQ2],
                "translation": [
                    1 + SEP * loc[0],
                    1 + SEP * loc[2],
                    -1 - SEP * loc[1],
                ],
                "mesh": 5 if color else 4,
            },
            {
                "name": f"edgeJ{loc}:-I",
                "rotation": [0.5, 0.5, -0.5, 0.5],
                "translation": [SEP * loc[0], SEP * loc[2], -1 - SEP * loc[1]],
                "mesh": 4 if color else 5,
            },
            {
                "name": f"edgeJ{loc}:+I",
                "rotation": [0.5, 0.5, -0.5, 0.5],
                "translation": [1 + SEP * loc[0], SEP * loc[2], -1 - SEP * loc[1]],
                "mesh": 4 if color else 5,
            },
        ]
        if corr[0]:
            rectangles.append(
                {
                    "name": f"edgeJ{loc}:CorrJK",
                    "mesh": 10,
                    "rotation": [0.5, 0.5, -0.5, 0.5],
                    "translation": [
                        0.5 + SEP * loc[0],
                        SEP * loc[2],
                        -1 - SEP * loc[1],
                    ],
                }
            )
        if corr[1]:
            rectangles.append(
                {
                    "name": f"edgeJ{loc}:CorrJI",
                    "mesh": 10,
                    "rotation": [0, SQ2, 0, SQ2],
                    "translation": [
                        1 + SEP * loc[0],
                        0.5 + SEP * loc[2],
                        -1 - SEP * loc[1],
                    ],
                }
            )

    elif dir == "K":
        colorKM = color // 7
        colorKP = color % 7
        rectangles = [
            {
                "name": f"edgeJ{loc}:-I",
                "mesh": 6,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [SEP * loc[0], 1 + SEP * loc[2], -SEP * loc[1]],
            },
            {
                "name": f"edgeJ{loc}:+I",
                "mesh": 6,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [1 + SEP * loc[0], 1 + SEP * loc[2], -SEP * loc[1]],
            },
            {
                "name": f"edgeK{loc}:-J",
                "mesh": 6,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [1 + SEP * loc[0], 1 + SEP * loc[2], -SEP * loc[1]],
            },
            {
                "name": f"edgeJ{loc}:+J",
                "mesh": 6,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [
                    1 + SEP * loc[0],
                    1 + SEP * loc[2],
                    -1 - SEP * loc[1],
                ],
            },
        ]
        if not noColor:
            if colorKM == 0 and colorKP == 0:
                rectangles[0]["mesh"] = 4
                rectangles[1]["mesh"] = 4
                rectangles[2]["mesh"] = 5
                rectangles[3]["mesh"] = 5
            if colorKM == 1 and colorKP == 1:
                rectangles[0]["mesh"] = 5
                rectangles[1]["mesh"] = 5
                rectangles[2]["mesh"] = 4
                rectangles[3]["mesh"] = 4
            if colorKM == 1 and colorKP == 0:
                rectangles[0]["mesh"] = 7
                rectangles[1]["mesh"] = 7
                rectangles[2]["mesh"] = 8
                rectangles[3]["mesh"] = 8
            if colorKM == 0 and colorKP == 1:
                rectangles[0]["mesh"] = 8
                rectangles[1]["mesh"] = 8
                rectangles[2]["mesh"] = 7
                rectangles[3]["mesh"] = 7

        if corr[0]:
            rectangles.append(
                {
                    "name": f"edgeK{loc}:CorrKI",
                    "mesh": 10,
                    "rotation": [0.5, 0.5, 0.5, 0.5],
                    "translation": [
                        1 + SEP * loc[0],
                        1 + SEP * loc[2],
                        -0.5 - SEP * loc[1],
                    ],
                }
            )
        if corr[1]:
            rectangles.append(
                {
                    "name": f"edgeK{loc}:CorrKJ",
                    "mesh": 10,
                    "rotation": [0, 0, SQ2, SQ2],
                    "translation": [
                        0.5 + SEP * loc[0],
                        1 + SEP * loc[2],
                        -SEP * loc[1],
                    ],
                }
            )

    rectangles = [rect for rect in rectangles if rm_dir not in rect["name"]]
    if stabilizer == -1:
        rectangles = [rect for rect in rectangles if "Corr" not in rect["name"]]
    return rectangles


def cube_gen(
    SEP: float,
    loc: Tuple[int, int, int],
    exists: Mapping[str, int],
    colors: Mapping[str, int],
    stabilizer: int,
    corr: Mapping[str, Tuple[int, int]],
    noColor: bool,
    rm_dir: str,
) -> Sequence[Mapping[str, Any]]:
    """compute the GLTF nodes for a cube. This can include its faces and
    correlation surface inside, minus the face to remove specified by rm_dir.

    Args:
        SEP (float): the distance, e.g., from cube(i,j,k) to cube(i+1,j,k).
        loc (Tuple[int, int, int]): 3D coordinate of the pipe.
        exists (Mapping[str, int]): whether there is a pipe in the 6 directions
            to this cube. (+|-)(I|J|K).
        colors (Mapping[str, int]): color variable of the pipe, can be
            -1(unknown), 0, or 1.
        stabilizer (int): index of the stabilizer.
        corr (Mapping[str, Tuple[int, int]]): two bits for two possible
            correlation surface inside a pipe. These info for all 6 pipes.
        noColor (bool): K-pipe are not colored if this is True.
        rm_dir (str): the direction of face to remove. if a stabilier is shown.

    Returns:
        Sequence[Mapping[str, Any]]: list of constructed GLTF nodes.
    """
    squares = []
    for face in ["-K", "+K"]:
        if exists[face] == 0:
            squares.append(
                {
                    "name": f"spider{loc}:{face}",
                    "mesh": 2,
                    "translation": [
                        SEP * loc[0],
                        (1 if face == "+K" else 0) + SEP * loc[2],
                        -SEP * loc[1],
                    ],
                }
            )
            for dir in ["+I", "-I", "+J", "-J"]:
                if exists[dir]:
                    if dir == "+I" or dir == "-I":
                        if colors[dir] == 1:
                            squares[-1]["mesh"] = 0
                        else:
                            squares[-1]["mesh"] = 1
                    else:
                        if colors[dir] == 0:
                            squares[-1]["mesh"] = 0
                        else:
                            squares[-1]["mesh"] = 1
                    break
    for face in ["-I", "+I"]:
        if exists[face] == 0:
            squares.append(
                {
                    "name": f"spider{loc}:{face}",
                    "mesh": 2,
                    "translation": [
                        (1 if face == "+I" else 0) + SEP * loc[0],
                        SEP * loc[2],
                        -SEP * loc[1],
                    ],
                    "rotation": [0, 0, SQ2, SQ2],
                }
            )
            for dir in ["+J", "-J", "+K", "-K"]:
                if exists[dir]:
                    if dir == "+J" or dir == "-J":
                        if colors[dir] == 1:
                            squares[-1]["mesh"] = 0
                        else:
                            squares[-1]["mesh"] = 1
                    elif not noColor:
                        if colors[dir] == 1:
                            squares[-1]["mesh"] = 1
                        elif colors[dir] == 0:
                            squares[-1]["mesh"] = 0
    for face in ["-J", "+J"]:
        if exists[face] == 0:
            squares.append(
                {
                    "name": f"spider{loc}:{face}",
                    "mesh": 2,
                    "translation": [
                        1 + SEP * loc[0],
                        SEP * loc[2],
                        (-1 if face == "+J" else 0) - SEP * loc[1],
                    ],
                    "rotation": [0.5, 0.5, 0.5, 0.5],
                }
            )
            for dir in ["+I", "-I", "+K", "-K"]:
                if exists[dir]:
                    if dir == "+I" or dir == "-I":
                        if colors[dir] == 0:
                            squares[-1]["mesh"] = 0
                        else:
                            squares[-1]["mesh"] = 1
                    elif not noColor:
                        if colors[dir] == 1:
                            squares[-1]["mesh"] = 0
                        elif colors[dir] == 0:
                            squares[-1]["mesh"] = 1

    degree = sum([v for (k, v) in exists.items()])
    normal = {"I": 0, "J": 0, "K": 0}
    if exists["-I"] == 0 and exists["+I"] == 0:
        normal["I"] = 1
    if exists["-J"] == 0 and exists["+J"] == 0:
        normal["J"] = 1
    if exists["-K"] == 0 and exists["+K"] == 0:
        normal["K"] = 1
    if degree > 1:
        if (
            exists["-I"]
            and exists["+I"]
            and exists["-J"] == 0
            and exists["+J"] == 0
            and exists["-K"] == 0
            and exists["+K"] == 0
        ):
            if corr["-I"][0]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            SEP * loc[0],
                            0.5 + SEP * loc[2],
                            -SEP * loc[1],
                        ],
                    }
                )
            if corr["-I"][1]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            1 + SEP * loc[0],
                            SEP * loc[2],
                            -0.5 - SEP * loc[1],
                        ],
                        "rotation": [0.5, 0.5, 0.5, 0.5],
                    }
                )
        elif (
            exists["-I"] == 0
            and exists["+I"] == 0
            and exists["-J"]
            and exists["+J"]
            and exists["-K"] == 0
            and exists["+K"] == 0
        ):
            if corr["-J"][0]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            0.5 + SEP * loc[0],
                            SEP * loc[2],
                            -SEP * loc[1],
                        ],
                        "rotation": [0, 0, SQ2, SQ2],
                    }
                )
            if corr["-J"][1]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            SEP * loc[0],
                            0.5 + SEP * loc[2],
                            -SEP * loc[1],
                        ],
                    }
                )
        elif (
            exists["-I"] == 0
            and exists["+I"] == 0
            and exists["-J"] == 0
            and exists["+J"] == 0
            and exists["-K"]
            and exists["+K"]
        ):
            if corr["-K"][0]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            1 + SEP * loc[0],
                            SEP * loc[2],
                            -0.5 - SEP * loc[1],
                        ],
                        "rotation": [0.5, 0.5, 0.5, 0.5],
                    }
                )
            if corr["-K"][1]:
                squares.append(
                    {
                        "name": f"spider{loc}:Corr",
                        "mesh": 9,
                        "translation": [
                            0.5 + SEP * loc[0],
                            SEP * loc[2],
                            -SEP * loc[1],
                        ],
                        "rotation": [0, 0, SQ2, SQ2],
                    }
                )
        else:
            if normal["I"]:
                if corr["-J"][0] or corr["+J"][0] or corr["-K"][1] or corr["+K"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                0.5 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )

                if corr["-J"][1] and corr["+J"][1] and corr["-K"][0] and corr["+K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                SEP * loc[2],
                                -1 - SEP * loc[1],
                            ],
                            "rotation": [0, -SQ2, 0, SQ2],
                        }
                    )
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0, -SQ2, 0, SQ2],
                        }
                    )
                elif corr["-J"][1] and corr["+J"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                        }
                    )
                elif corr["-K"][0] and corr["+K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0.5, 0.5, 0.5, 0.5],
                        }
                    )
                elif corr["-J"][1] and corr["-K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, SQ2, 0, SQ2],
                        }
                    )
                elif corr["+J"][1] and corr["+K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                1 + SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0, SQ2, 0, SQ2],
                        }
                    )
                elif corr["+J"][1] and corr["-K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                SEP * loc[2],
                                -1 - SEP * loc[1],
                            ],
                            "rotation": [0, -SQ2, 0, SQ2],
                        }
                    )
                elif corr["-J"][1] and corr["+K"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0, -SQ2, 0, SQ2],
                        }
                    )
            elif normal["J"]:
                if corr["-K"][0] or corr["+K"][0] or corr["-I"][1] or corr["+I"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0.5, 0.5, 0.5, 0.5],
                        }
                    )

                if corr["-K"][1] and corr["+K"][1] and corr["-I"][0] and corr["+I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
                elif corr["-K"][1] and corr["+K"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                0.5 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
                elif corr["-I"][0] and corr["+I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                        }
                    )
                elif corr["-K"][1] and corr["-I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
                        }
                    )
                elif corr["+K"][1] and corr["+I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                        }
                    )
                elif corr["+K"][1] and corr["-I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
                elif corr["-K"][1] and corr["+I"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
            else:
                if corr["-I"][0] or corr["+I"][0] or corr["-J"][1] or corr["+J"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                SEP * loc[0],
                                0.5 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                        }
                    )

                if corr["-I"][1] and corr["+I"][1] and corr["-J"][0] and corr["+J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [SQ2, 0, 0, SQ2],
                        }
                    )
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                SEP * loc[2],
                                -1 - SEP * loc[1],
                            ],
                            "rotation": [SQ2, 0, 0, SQ2],
                        }
                    )
                elif corr["-I"][1] and corr["+I"][1]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                1 + SEP * loc[0],
                                SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [0.5, 0.5, 0.5, 0.5],
                        }
                    )
                elif corr["-J"][0] and corr["+J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 9,
                            "translation": [
                                0.5 + SEP * loc[0],
                                SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [0, 0, SQ2, SQ2],
                        }
                    )
                elif corr["-I"][1] and corr["-J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                1 + SEP * loc[2],
                                -SEP * loc[1],
                            ],
                            "rotation": [-SQ2, 0, 0, SQ2],
                        }
                    )
                elif corr["+I"][1] and corr["+J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                1 + SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [-SQ2, 0, 0, SQ2],
                        }
                    )
                elif corr["+I"][1] and corr["-J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                0.5 + SEP * loc[0],
                                SEP * loc[2],
                                -0.5 - SEP * loc[1],
                            ],
                            "rotation": [SQ2, 0, 0, SQ2],
                        }
                    )
                elif corr["-I"][1] and corr["+J"][0]:
                    squares.append(
                        {
                            "name": f"spider{loc}:Corr",
                            "mesh": 11,
                            "translation": [
                                SEP * loc[0],
                                SEP * loc[2],
                                -1 - SEP * loc[1],
                            ],
                            "rotation": [SQ2, 0, 0, SQ2],
                        }
                    )

    squares = [sqar for sqar in squares if rm_dir not in sqar["name"]]
    if stabilizer == -1:
        squares = [sqar for sqar in squares if "Corr" not in sqar["name"]]
    return squares


def special_gen(
    SEP: float,
    loc: Tuple[int, int, int],
    exists: Mapping[str, int],
    type: str,
    stabilizer: int,
    rm_dir: str,
) -> Sequence[Mapping[str, Any]]:
    """compute the GLTF nodes for special cubes. Currently Ycube and Tinjection

    Args:
        SEP (float): the distance, e.g., from cube(i,j,k) to cube(i+1,j,k).
        loc (Tuple[int, int, int]): 3D coordinate of the pipe.
        exists (Mapping[str, int]): whether there is a pipe in the 6 directions
            to this cube. (+|-)(I|J|K).
        stabilizer (int): index of the stabilizer.
        noColor (bool): K-pipe are not colored if this is True.
        rm_dir (str): the direction of face to remove. if a stabilier is shown.

    Returns:
        Sequence[Mapping[str, Any]]: list of constructed GLTF nodes.
    """
    if type == "Y":
        square_mesh = 3
        half_dist_mesh = 12
    elif type == "T":
        square_mesh = 13
        half_dist_mesh = 14
    else:
        square_mesh = -1
        half_dist_mesh = -1

    shapes = []
    if exists["+K"]:
        # need connect to top
        shapes.append(
            {
                "name": f"spider{loc}:top:-K",
                "mesh": square_mesh,
                "translation": [
                    SEP * loc[0],
                    0.55 + SEP * loc[2],
                    -SEP * loc[1],
                ],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:top:-I",
                "mesh": half_dist_mesh,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:top:+I",
                "mesh": half_dist_mesh,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [1 + SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:top:-J",
                "mesh": half_dist_mesh,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [1 + SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:top:+J",
                "mesh": half_dist_mesh,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [
                    1 + SEP * loc[0],
                    0.55 + SEP * loc[2],
                    -SEP * loc[1] - 1,
                ],
            }
        )

    if exists["-K"]:
        # need connect to bottom
        shapes.append(
            {
                "name": f"spider{loc}:bottom:+K",
                "mesh": square_mesh,
                "translation": [
                    SEP * loc[0],
                    0.45 + SEP * loc[2],
                    -SEP * loc[1],
                ],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:bottom:-I",
                "mesh": half_dist_mesh,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:bottom:+I",
                "mesh": half_dist_mesh,
                "rotation": [0, 0, SQ2, SQ2],
                "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:bottom:-J",
                "mesh": half_dist_mesh,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
            }
        )
        shapes.append(
            {
                "name": f"spider{loc}:bottom:+J",
                "mesh": half_dist_mesh,
                "rotation": [0.5, 0.5, 0.5, 0.5],
                "translation": [
                    1 + SEP * loc[0],
                    SEP * loc[2],
                    -SEP * loc[1] - 1,
                ],
            }
        )

    shapes = [shp for shp in shapes if rm_dir not in shp["name"]]
    return shapes


def gltf_generator(
    lasre: Mapping[str, Any],
    stabilizer: int = -1,
    tube_len: float = 2.0,
    no_color_z: bool = False,
    attach_axes: bool = False,
    rm_dir: Optional[str] = None,
) -> Mapping[str, Any]:
    """generate gltf in a dict and write to a json file with extension .gltf

    Args:
        lasre (Mapping[str, Any]): LaSRe of the LaS.
        stabilizer (int, optional): index of the stabilizer. The correlation
            surfaces corresponding to it will be drawn. Defaults to -1, which
            means do not draw any correlation surfaces.
        tube_len (float, optional): ratio of the length of the pipes compared
            to the length of the cubes. Defaults to 2.0.
        no_color_z (bool, optional): do not color the Z-pipes. Defaults to
            False, which means by default Z-pipes are colored.
        attach_axes (bool, optional): attach an IJK axes. Defaults to False.
        rm_dir (str, optional): the direction of faces to remove to reveal
            the correlation surfaces. Defaults to None.

    Raises:
        ValueError: rm_dir is not any one of :(+|-)(I|J|K)
        ValueError: the index of stabilizer is not -1 nor in [0, n_stabilizer)

    Returns:
        Mapping[str, Any]: the constructed gltf in a dict.
    """
    s, tubelen, noColor = (
        stabilizer,
        tube_len,
        no_color_z,
    )
    if rm_dir is None:
        rm_dir = ":II"
    elif rm_dir not in [":+I", ":-I", ":+J", ":-J", ":+K", ":-K"]:
        raise ValueError("rm_dir is not one of :+I, :-I, :+J, :-J, :+K, :-K")

    gltf = base_gen(tubelen)

    i_bound = lasre["n_i"]
    j_bound = lasre["n_j"]
    k_bound = lasre["n_k"]
    NodeY = lasre["NodeY"]
    ExistI = lasre["ExistI"]
    ColorI = lasre["ColorI"]
    ExistJ = lasre["ExistJ"]
    ColorJ = lasre["ColorJ"]
    ExistK = lasre["ExistK"]
    if "CorrIJ" in lasre:
        CorrIJ = lasre["CorrIJ"]
        CorrIK = lasre["CorrIK"]
        CorrJI = lasre["CorrJI"]
        CorrJK = lasre["CorrJK"]
        CorrKI = lasre["CorrKI"]
        CorrKJ = lasre["CorrKJ"]
        s_bound = len(CorrIJ)
    if "ColorKP" not in lasre:
        ColorKP = [
            [[-1 for _ in range(k_bound)] for _ in range(j_bound)]
            for _ in range(i_bound)
        ]
    else:
        ColorKP = lasre["ColorKP"]
    if "ColorKM" not in lasre:
        ColorKM = [
            [[-1 for _ in range(k_bound)] for _ in range(j_bound)]
            for _ in range(i_bound)
        ]
    else:
        ColorKM = lasre["ColorKM"]
    port_cubes = lasre["port_cubes"]
    t_injections = (
        lasre["optional"]["t_injections"] if "t_injections" in lasre["optional"] else []
    )

    if s < -1 or (s_bound > 0 and s not in range(-1, s_bound)):
        raise ValueError(f"No such stabilizer index {s}.")

    for i in range(i_bound):
        for j in range(j_bound):
            for k in range(k_bound):
                if ExistI[i][j][k]:
                    gltf["nodes"] += tube_gen(
                        tubelen + 1.0,
                        (i, j, k),
                        "I",
                        ColorI[i][j][k],
                        s,
                        (CorrIJ[s][i][j][k], CorrIK[s][i][j][k]) if s_bound else (0, 0),
                        noColor,
                        rm_dir,
                    )
                if ExistJ[i][j][k]:
                    gltf["nodes"] += tube_gen(
                        tubelen + 1.0,
                        (i, j, k),
                        "J",
                        ColorJ[i][j][k],
                        s,
                        (CorrJK[s][i][j][k], CorrJI[s][i][j][k]) if s_bound else (0, 0),
                        noColor,
                        rm_dir,
                    )
                if ExistK[i][j][k]:
                    gltf["nodes"] += tube_gen(
                        tubelen + 1.0,
                        (i, j, k),
                        "K",
                        7 * ColorKM[i][j][k] + ColorKP[i][j][k],
                        s,
                        (CorrKI[s][i][j][k], CorrKJ[s][i][j][k]) if s_bound else (0, 0),
                        noColor,
                        rm_dir,
                    )

    for i in range(i_bound):
        for j in range(j_bound):
            for k in range(k_bound):
                exists = {"-I": 0, "+I": 0, "-K": 0, "+K": 0, "-J": 0, "+J": 0}
                colors = {}
                corr = {
                    "-I": (0, 0),
                    "+I": (0, 0),
                    "-J": (0, 0),
                    "+J": (0, 0),
                    "-K": (0, 0),
                    "+K": (0, 0),
                }
                if i > 0 and ExistI[i - 1][j][k]:
                    exists["-I"] = 1
                    colors["-I"] = ColorI[i - 1][j][k]
                    corr["-I"] = (
                        (CorrIJ[s][i - 1][j][k], CorrIK[s][i - 1][j][k])
                        if s_bound
                        else (0, 0)
                    )
                if ExistI[i][j][k]:
                    exists["+I"] = 1
                    colors["+I"] = ColorI[i][j][k]
                    corr["+I"] = (
                        (CorrIJ[s][i][j][k], CorrIK[s][i][j][k]) if s_bound else (0, 0)
                    )
                if j > 0 and ExistJ[i][j - 1][k]:
                    exists["-J"] = 1
                    colors["-J"] = ColorJ[i][j - 1][k]
                    corr["-J"] = (
                        (CorrJK[s][i][j - 1][k], CorrJI[s][i][j - 1][k])
                        if s_bound
                        else (0, 0)
                    )
                if ExistJ[i][j][k]:
                    exists["+J"] = 1
                    colors["+J"] = ColorJ[i][j][k]
                    corr["+J"] = (
                        (CorrJK[s][i][j][k], CorrJI[s][i][j][k]) if s_bound else (0, 0)
                    )
                if k > 0 and ExistK[i][j][k - 1]:
                    exists["-K"] = 1
                    colors["-K"] = ColorKP[i][j][k - 1]
                    corr["-K"] = (
                        (CorrKI[s][i][j][k - 1], CorrKJ[s][i][j][k - 1])
                        if s_bound
                        else (0, 0)
                    )
                if ExistK[i][j][k]:
                    exists["+K"] = 1
                    colors["+K"] = ColorKM[i][j][k]
                    corr["+K"] = (
                        (CorrKI[s][i][j][k], CorrKJ[s][i][j][k]) if s_bound else (0, 0)
                    )
                if sum([v for (k, v) in exists.items()]) > 0:
                    if (i, j, k) not in port_cubes:
                        if NodeY[i][j][k]:
                            gltf["nodes"] += special_gen(
                                tubelen + 1.0,
                                (i, j, k),
                                exists,
                                "Y",
                                s,
                                rm_dir,
                            )
                        else:
                            gltf["nodes"] += cube_gen(
                                tubelen + 1.0,
                                (i, j, k),
                                exists,
                                colors,
                                s,
                                corr,
                                noColor,
                                rm_dir,
                            )
                    elif [i, j, k] in t_injections:
                        gltf["nodes"] += special_gen(
                            tubelen + 1.0,
                            (i, j, k),
                            exists,
                            "T",
                            s,
                            rm_dir,
                        )

    if attach_axes:
        gltf["nodes"] += axes_gen(tube_len + 1.0, i_bound, j_bound, k_bound)

    gltf["nodes"][0]["children"] = list(range(1, len(gltf["nodes"])))

    return gltf
