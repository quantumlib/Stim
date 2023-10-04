"""A one-line summary of the module or program, terminated by a period.

Leave one blank line.  The rest of this docstring should contain an
overall description of the module or program.  Optionally, it may also
contain a brief description of exported classes and functions and/or usage
examples.

Typical usage example:

  foo = ClassFoo()
  bar = foo.FunctionBar()
"""

import json
from typing import Any, Mapping, Optional, Sequence

SQ2 = 0.707106769085
BASE_GLTF = {
    "asset": {"generator": "LaSIR CodeGen", "version": "2.0"},
    "scene": 0,
    "scenes": [{"name": "Scene", "nodes": [0]}],
    "nodes": [{"name": "try", "children": []}],
}


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
    gltf: dict, tubelen: float = 2.0, outputBase=False, base_file: str = None
):
  # little endian
  floats = {
      "0": "00000000",
      "1": "0000803F",
      "-1": "000080BF",
      "0.5": "0000003F",
      "0.45": "6666E63E",
  }
  floats["tube"] = float_to_little_endian_hex(tubelen)
  floats["SQ2"] = float_to_little_endian_hex(SQ2)
  floats["-SQ2"] = float_to_little_endian_hex(-SQ2)
  ints = ["0000", "0100", "0200", "0300", "0400", "0500", "0600", "0700"]

  gltf["accessors"] = []
  gltf["buffers"] = []
  gltf["bufferViews"] = []
  gltf["materials"] = [
      {
          "name": "0-blue",
          "pbrMetallicRoughness": {"baseColorFactor": [0, 0, 1, 1]},
          "doubleSided": True,
      },
      {
          "name": "1-red",
          "pbrMetallicRoughness": {"baseColorFactor": [1, 0, 0, 1]},
          "doubleSided": True,
      },
      {
          "name": "2-green",
          "pbrMetallicRoughness": {"baseColorFactor": [0, 1, 0, 1]},
          "doubleSided": True,
      },
      {
          "name": "3-gray",
          "pbrMetallicRoughness": {"baseColorFactor": [0.5, 0.5, 0.5, 1]},
          "doubleSided": True,
      },
      {
          "name": "4-cyan.3",
          "pbrMetallicRoughness": {"baseColorFactor": [0, 1, 1, 0.3]},
          "doubleSided": True,
          "alphaMode": "BLEND",
      },
      {
          "name": "5-black",
          "pbrMetallicRoughness": {"baseColorFactor": [0, 0, 0, 1]},
          "doubleSided": True,
      },
      {
          "name": "6-yellow",
          "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 0, 1]},
          "doubleSided": True,
      },
      {
          "name": "7-white",
          "pbrMetallicRoughness": {"baseColorFactor": [1, 1, 1, 1]},
          "doubleSided": True,
      },
  ]

  # convention of VEC3: (X,Z,-Y)

  # 0, positions of square: [(0,0,0),(1,0,0),(0,1,0),(1,1,0)]
  s = (
      floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["1"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["-1"]
      + floats["1"]
      + floats["0"]
      + floats["-1"]
  )
  gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 0, "byteLength": 48, "byteOffset": 0, "target": 34962}
  )
  gltf["accessors"].append({
      "bufferView": 0,
      "componentType": 5126,
      "type": "VEC3",
      "count": 4,
      "max": [1, 0, 0],
      "min": [0, 0, -1],
  })

  # 1, positions of rectangle: [(0,0,0),(L,0,0),(0,1,0),(L,1,0)]
  s = (
      floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["tube"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["-1"]
      + floats["tube"]
      + floats["0"]
      + floats["-1"]
  )
  gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 1, "byteLength": 48, "byteOffset": 0, "target": 34962}
  )
  gltf["accessors"].append({
      "bufferView": 1,
      "componentType": 5126,
      "type": "VEC3",
      "count": 4,
      "max": [tubelen, 0, 0],
      "min": [0, 0, -1],
  })

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

  # 4, positions of tilted rect: [(0,0,1/2),(1/2,0,0),(0,1,1/2),(1/2,1,0)]
  s = (
      floats["0"]
      + floats["0.5"]
      + floats["0"]
      + floats["0.5"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0.5"]
      + floats["-1"]
      + floats["0.5"]
      + floats["0"]
      + floats["-1"]
  )
  gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 4, "byteLength": 48, "byteOffset": 0, "target": 34962}
  )
  gltf["accessors"].append({
      "bufferView": 4,
      "componentType": 5126,
      "type": "VEC3",
      "count": 4,
      "max": [0.5, 0.5, 0],
      "min": [0, 0, -1],
  })

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

  # 6, positions of Hadamard rectangle: [(0,0,0),(15/32L,0,0),(15/32L,1,0),
  # (15/32L,1,0),(17/32L,0,0),(17/32L,1,0),(L,0,0),(L,1,0)]
  floats["left"] = float_to_little_endian_hex(tubelen * 15 / 32)
  floats["right"] = float_to_little_endian_hex(tubelen * 17 / 32)
  s = (
      floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["left"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["-1"]
      + floats["left"]
      + floats["0"]
      + floats["-1"]
      + floats["right"]
      + floats["0"]
      + floats["0"]
      + floats["right"]
      + floats["0"]
      + floats["-1"]
      + floats["tube"]
      + floats["0"]
      + floats["0"]
      + floats["tube"]
      + floats["0"]
      + floats["-1"]
  )
  gltf["buffers"].append({"byteLength": 96, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 6, "byteLength": 96, "byteOffset": 0, "target": 34962}
  )
  gltf["accessors"].append({
      "bufferView": 6,
      "componentType": 5126,
      "type": "VEC3",
      "count": 8,
      "max": [tubelen, 0, 0],
      "min": [0, 0, -1],
  })

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
  s = (
      ints[0]
      + ints[1]
      + ints[0]
      + ints[2]
      + ints[2]
      + ints[3]
      + ints[3]
      + ints[1]
  )
  gltf["buffers"].append({"byteLength": 16, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 10, "byteLength": 16, "byteOffset": 0, "target": 34963}
  )
  gltf["accessors"].append(
      {"bufferView": 10, "componentType": 5123, "type": "SCALAR", "count": 8}
  )

  # 11, positions of half-distance rectangle: [(0,0,0),(0.45,0,0),(0,1,0),(0.45,1,0)]
  s = (
      floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0.45"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["0"]
      + floats["-1"]
      + floats["0.45"]
      + floats["0"]
      + floats["-1"]
  )
  gltf["buffers"].append({"byteLength": 48, "uri": hex_to_bin(s)})
  gltf["bufferViews"].append(
      {"buffer": 11, "byteLength": 48, "byteOffset": 0, "target": 34962}
  )
  gltf["accessors"].append({
      "bufferView": 11,
      "componentType": 5126,
      "type": "VEC3",
      "count": 4,
      "max": [0.45, 0, 0],
      "min": [0, 0, -1],
  })

  gltf["meshes"] = [
      {
          "name": "0-square-blue",
          "primitives": [
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 3,
                  "material": 0,
                  "mode": 4,
              },
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
              },
          ],
      },
      {
          "name": "1-square-red",
          "primitives": [
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 3,
                  "material": 1,
                  "mode": 4,
              },
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
              },
          ],
      },
      {
          "name": "2-square-gray",
          "primitives": [{
              "attributes": {"NORMAL": 2, "POSITION": 0},
              "indices": 3,
              "material": 3,
              "mode": 4,
          }],
      },
      {
          "name": "3-square-green",
          "primitives": [{
              "attributes": {"NORMAL": 2, "POSITION": 0},
              "indices": 3,
              "material": 2,
              "mode": 4,
          }],
      },
      {
          "name": "4-rectangle-blue",
          "primitives": [{
              "attributes": {"NORMAL": 2, "POSITION": 1},
              "indices": 3,
              "material": 0,
              "mode": 4,
          }],
      },
      {
          "name": "5-rectangle-red",
          "primitives": [{
              "attributes": {"NORMAL": 2, "POSITION": 1},
              "indices": 3,
              "material": 1,
              "mode": 4,
          }],
      },
      {
          "name": "6-rectangle-gray",
          "primitives": [{
              "attributes": {"NORMAL": 2, "POSITION": 1},
              "indices": 3,
              "material": 3,
              "mode": 4,
          }],
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
              },
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
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
              {
                  "attributes": {"NORMAL": 2, "POSITION": 1},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
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
              {
                  "attributes": {"NORMAL": 5, "POSITION": 4},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
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
              {
                  "attributes": {"NORMAL": 2, "POSITION": 11},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
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
              },
              {
                  "attributes": {"NORMAL": 2, "POSITION": 0},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
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
              {
                  "attributes": {"NORMAL": 2, "POSITION": 11},
                  "indices": 10,
                  "material": 5,
                  "mode": 1,
              },
          ],
      },
  ]

  if outputBase:
    for i in range(1, 15):
      gltf["nodes"][0]["children"].append(i)
      gltf["nodes"].append(
          {"name": f"mesh{i-1}", "mesh": i - 1, "translation": [0, i - 1, 0]}
      )
    if not base_file:
      base_file = "./base.gltf"
    with open(base_file, "w") as f:
      json.dump(gltf, f)

  return gltf


def tube_gen(
    SEP, loc, dir, color, stabilizer, corr, noColor, rm_dir: str = ":+J"
):
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
      rectangles.append({
          "name": f"edgeI{loc}:CorrIJ",
          "mesh": 10,
          "translation": [
              1 + SEP * loc[0],
              0.5 + SEP * loc[2],
              -SEP * loc[1],
          ],
      })
    if corr[1]:
      rectangles.append({
          "name": f"edgeI{loc}:CorrIK",
          "mesh": 10,
          "translation": [
              1 + SEP * loc[0],
              SEP * loc[2],
              -0.5 - SEP * loc[1],
          ],
          "rotation": [SQ2, 0, 0, SQ2],
      })
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
      rectangles.append({
          "name": f"edgeJ{loc}:CorrJK",
          "mesh": 10,
          "rotation": [0.5, 0.5, -0.5, 0.5],
          "translation": [
              0.5 + SEP * loc[0],
              SEP * loc[2],
              -1 - SEP * loc[1],
          ],
      })
    if corr[1]:
      rectangles.append({
          "name": f"edgeJ{loc}:CorrJI",
          "mesh": 10,
          "rotation": [0, SQ2, 0, SQ2],
          "translation": [
              1 + SEP * loc[0],
              0.5 + SEP * loc[2],
              -1 - SEP * loc[1],
          ],
      })

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
      rectangles.append({
          "name": f"edgeK{loc}:CorrKI",
          "mesh": 10,
          "rotation": [0.5, 0.5, 0.5, 0.5],
          "translation": [
              1 + SEP * loc[0],
              1 + SEP * loc[2],
              -0.5 - SEP * loc[1],
          ],
      })
    if corr[1]:
      rectangles.append({
          "name": f"edgeK{loc}:CorrKJ",
          "mesh": 10,
          "rotation": [0, 0, SQ2, SQ2],
          "translation": [
              0.5 + SEP * loc[0],
              1 + SEP * loc[2],
              -SEP * loc[1],
          ],
      })

  if stabilizer > -1:
    rectangles = [rect for rect in rectangles if rm_dir not in rect["name"]]
  else:
    rectangles = [rect for rect in rectangles if "Corr" not in rect["name"]]
  return rectangles


def cube_gen(
    SEP,
    loc,
    exists,
    colors,
    stabilizer,
    corr,
    noColor,
    rm_dir: str = ":+J",
):
  squares = []
  for face in ["-K", "+K"]:
    if exists[face] == 0:
      squares.append({
          "name": f"spider{loc}:{face}",
          "mesh": 2,
          "translation": [
              SEP * loc[0],
              (1 if face == "+K" else 0) + SEP * loc[2],
              -SEP * loc[1],
          ],
      })
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
      squares.append({
          "name": f"spider{loc}:{face}",
          "mesh": 2,
          "translation": [
              (1 if face == "+I" else 0) + SEP * loc[0],
              SEP * loc[2],
              -SEP * loc[1],
          ],
          "rotation": [0, 0, SQ2, SQ2],
      })
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
      squares.append({
          "name": f"spider{loc}:{face}",
          "mesh": 2,
          "translation": [
              1 + SEP * loc[0],
              SEP * loc[2],
              (-1 if face == "+J" else 0) - SEP * loc[1],
          ],
          "rotation": [0.5, 0.5, 0.5, 0.5],
      })
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
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                SEP * loc[0],
                0.5 + SEP * loc[2],
                -SEP * loc[1],
            ],
        })
      if corr["-I"][1]:
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                1 + SEP * loc[0],
                SEP * loc[2],
                -0.5 - SEP * loc[1],
            ],
            "rotation": [0.5, 0.5, 0.5, 0.5],
        })
    elif (
        exists["-I"] == 0
        and exists["+I"] == 0
        and exists["-J"]
        and exists["+J"]
        and exists["-K"] == 0
        and exists["+K"] == 0
    ):
      if corr["-J"][0]:
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                0.5 + SEP * loc[0],
                SEP * loc[2],
                -SEP * loc[1],
            ],
            "rotation": [0, 0, SQ2, SQ2],
        })
      if corr["-J"][1]:
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                SEP * loc[0],
                0.5 + SEP * loc[2],
                -SEP * loc[1],
            ],
        })
    elif (
        exists["-I"] == 0
        and exists["+I"] == 0
        and exists["-J"] == 0
        and exists["+J"] == 0
        and exists["-K"]
        and exists["+K"]
    ):
      if corr["-K"][0]:
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                1 + SEP * loc[0],
                SEP * loc[2],
                -0.5 - SEP * loc[1],
            ],
            "rotation": [0.5, 0.5, 0.5, 0.5],
        })
      if corr["-K"][1]:
        squares.append({
            "name": f"spider{loc}:Corr",
            "mesh": 9,
            "translation": [
                0.5 + SEP * loc[0],
                SEP * loc[2],
                -SEP * loc[1],
            ],
            "rotation": [0, 0, SQ2, SQ2],
        })
    else:
      if normal["I"]:
        if corr["-J"][0] or corr["+J"][0] or corr["-K"][1] or corr["+K"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  0.5 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })

        if corr["-J"][1] and corr["+J"][1] and corr["-K"][0] and corr["+K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  SEP * loc[2],
                  -1 - SEP * loc[1],
              ],
              "rotation": [0, -SQ2, 0, SQ2],
          })
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0, -SQ2, 0, SQ2],
          })
        elif corr["-J"][1] and corr["+J"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
          })
        elif corr["-K"][0] and corr["+K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0.5, 0.5, 0.5, 0.5],
          })
        elif corr["-J"][1] and corr["-K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, SQ2, 0, SQ2],
          })
        elif corr["+J"][1] and corr["+K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  1 + SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0, SQ2, 0, SQ2],
          })
        elif corr["+J"][1] and corr["-K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  SEP * loc[2],
                  -1 - SEP * loc[1],
              ],
              "rotation": [0, -SQ2, 0, SQ2],
          })
        elif corr["-J"][1] and corr["+K"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0, -SQ2, 0, SQ2],
          })
      elif normal["J"]:
        if corr["-K"][0] or corr["+K"][0] or corr["-I"][1] or corr["+I"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0.5, 0.5, 0.5, 0.5],
          })

        if corr["-K"][1] and corr["+K"][1] and corr["-I"][0] and corr["+I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
        elif corr["-K"][1] and corr["+K"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  0.5 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
        elif corr["-I"][0] and corr["+I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
          })
        elif corr["-K"][1] and corr["-I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
          })
        elif corr["+K"][1] and corr["+I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
          })
        elif corr["+K"][1] and corr["-I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
        elif corr["-K"][1] and corr["+I"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
      else:
        if corr["-I"][0] or corr["+I"][0] or corr["-J"][1] or corr["+J"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  SEP * loc[0],
                  0.5 + SEP * loc[2],
                  -SEP * loc[1],
              ],
          })

        if corr["-I"][1] and corr["+I"][1] and corr["-J"][0] and corr["+J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [SQ2, 0, 0, SQ2],
          })
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  SEP * loc[2],
                  -1 - SEP * loc[1],
              ],
              "rotation": [SQ2, 0, 0, SQ2],
          })
        elif corr["-I"][1] and corr["+I"][1]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  1 + SEP * loc[0],
                  SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [0.5, 0.5, 0.5, 0.5],
          })
        elif corr["-J"][0] and corr["+J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 9,
              "translation": [
                  0.5 + SEP * loc[0],
                  SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [0, 0, SQ2, SQ2],
          })
        elif corr["-I"][1] and corr["-J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  1 + SEP * loc[2],
                  -SEP * loc[1],
              ],
              "rotation": [-SQ2, 0, 0, SQ2],
          })
        elif corr["+I"][1] and corr["+J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  1 + SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [-SQ2, 0, 0, SQ2],
          })
        elif corr["+I"][1] and corr["-J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  0.5 + SEP * loc[0],
                  SEP * loc[2],
                  -0.5 - SEP * loc[1],
              ],
              "rotation": [SQ2, 0, 0, SQ2],
          })
        elif corr["-I"][1] and corr["+J"][0]:
          squares.append({
              "name": f"spider{loc}:Corr",
              "mesh": 11,
              "translation": [
                  SEP * loc[0],
                  SEP * loc[2],
                  -1 - SEP * loc[1],
              ],
              "rotation": [SQ2, 0, 0, SQ2],
          })

  if stabilizer > -1:
    squares = [sqar for sqar in squares if rm_dir not in sqar["name"]]
  else:
    squares = [sqar for sqar in squares if "Corr" not in sqar["name"]]
  return squares


def special_gen(
    SEP,
    loc,
    exists,
    type,
    stabilizer,
    rm_dir: str = ":+J",
):
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
    shapes.append({
        "name": f"spider{loc}:top:-K",
        "mesh": square_mesh,
        "translation": [
            SEP * loc[0],
            0.55 + SEP * loc[2],
            -SEP * loc[1],
        ],
    })
    shapes.append({
        "name": f"spider{loc}:top:-I",
        "mesh": half_dist_mesh,
        "rotation": [0, 0, SQ2, SQ2],
        "translation": [SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:top:+I",
        "mesh": half_dist_mesh,
        "rotation": [0, 0, SQ2, SQ2],
        "translation": [1 + SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:top:-J",
        "mesh": half_dist_mesh,
        "rotation": [0.5, 0.5, 0.5, 0.5],
        "translation": [1 + SEP * loc[0], 0.55 + SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:top:+J",
        "mesh": half_dist_mesh,
        "rotation": [0.5, 0.5, 0.5, 0.5],
        "translation": [
            1 + SEP * loc[0],
            0.55 + SEP * loc[2],
            -SEP * loc[1] - 1,
        ],
    })

  if exists["-K"]:
    # need connect to bottom
    shapes.append({
        "name": f"spider{loc}:bottom:+K",
        "mesh": square_mesh,
        "translation": [
            SEP * loc[0],
            0.45 + SEP * loc[2],
            -SEP * loc[1],
        ],
    })
    shapes.append({
        "name": f"spider{loc}:bottom:-I",
        "mesh": half_dist_mesh,
        "rotation": [0, 0, SQ2, SQ2],
        "translation": [SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:bottom:+I",
        "mesh": half_dist_mesh,
        "rotation": [0, 0, SQ2, SQ2],
        "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:bottom:-J",
        "mesh": half_dist_mesh,
        "rotation": [0.5, 0.5, 0.5, 0.5],
        "translation": [1 + SEP * loc[0], SEP * loc[2], -SEP * loc[1]],
    })
    shapes.append({
        "name": f"spider{loc}:bottom:+J",
        "mesh": half_dist_mesh,
        "rotation": [0.5, 0.5, 0.5, 0.5],
        "translation": [
            1 + SEP * loc[0],
            SEP * loc[2],
            -SEP * loc[1] - 1,
        ],
    })

  if stabilizer > -1:
    shapes = [shp for shp in shapes if rm_dir not in shp["name"]]
  return shapes


def gltf_generator(
    lasir: Mapping[str, Any],
    output_file: Optional[str] = "",
    stabilizer: Optional[int] = -1,
    tube_len: Optional[float] = 2.0,
    no_color_z: Optional[bool] = False,
) -> Mapping[str, Any]:
  gltf_file, s, tubelen, noColor = (
      output_file,
      stabilizer,
      tube_len,
      no_color_z,
  )
  rm_dir = ":+J"

  gltf = base_gen(BASE_GLTF, tubelen)

  gltf["nodes"] = [
      {"name": "smaple"},
  ]
  i_bound = lasir["n_i"]
  j_bound = lasir["n_j"]
  k_bound = lasir["n_k"]
  NodeY = lasir["NodeY"]
  ExistI = lasir["ExistI"]
  ColorI = lasir["ColorI"]
  ExistJ = lasir["ExistJ"]
  ColorJ = lasir["ColorJ"]
  ExistK = lasir["ExistK"]
  CorrIJ = lasir["CorrIJ"]
  CorrIK = lasir["CorrIK"]
  CorrJI = lasir["CorrJI"]
  CorrJK = lasir["CorrJK"]
  CorrKI = lasir["CorrKI"]
  CorrKJ = lasir["CorrKJ"]
  if "ColorKP" not in lasir:
    ColorKP = [
        [[-1 for _ in range(k_bound)] for _ in range(j_bound)]
        for _ in range(i_bound)
    ]
  else:
    ColorKP = lasir["ColorKP"]
  if "ColorKM" not in lasir:
    ColorKM = [
        [[-1 for _ in range(k_bound)] for _ in range(j_bound)]
        for _ in range(i_bound)
    ]
  else:
    ColorKM = lasir["ColorKM"]
  port_cubes = lasir["port_cubes"]
  t_injections = (
      lasir["optional"]["t_injections"]
      if "t_injections" in lasir["optional"]
      else []
  )

  if s >= len(CorrIJ) or s < -1:
    raise ValueError(f"No such stabilizer {s}.")
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
              (CorrIJ[s][i][j][k], CorrIK[s][i][j][k]),
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
              (CorrJK[s][i][j][k], CorrJI[s][i][j][k]),
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
              (CorrKI[s][i][j][k], CorrKJ[s][i][j][k]),
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
          corr["-I"] = (CorrIJ[s][i - 1][j][k], CorrIK[s][i - 1][j][k])
        if ExistI[i][j][k]:
          exists["+I"] = 1
          colors["+I"] = ColorI[i][j][k]
          corr["+I"] = (CorrIJ[s][i][j][k], CorrIK[s][i][j][k])
        if j > 0 and ExistJ[i][j - 1][k]:
          exists["-J"] = 1
          colors["-J"] = ColorJ[i][j - 1][k]
          corr["-J"] = (CorrJK[s][i][j - 1][k], CorrJI[s][i][j - 1][k])
        if ExistJ[i][j][k]:
          exists["+J"] = 1
          colors["+J"] = ColorJ[i][j][k]
          corr["+J"] = (CorrJK[s][i][j][k], CorrJI[s][i][j][k])
        if k > 0 and ExistK[i][j][k - 1]:
          exists["-K"] = 1
          colors["-K"] = ColorKP[i][j][k - 1]
          corr["-K"] = (CorrKI[s][i][j][k - 1], CorrKJ[s][i][j][k - 1])
        if ExistK[i][j][k]:
          exists["+K"] = 1
          colors["+K"] = ColorKM[i][j][k]
          corr["+K"] = (CorrKI[s][i][j][k], CorrKJ[s][i][j][k])
        if sum([v for (k, v) in exists.items()]) > 0:
          if [i, j, k] not in port_cubes:
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

  gltf["nodes"][0]["children"] = list(range(1, len(gltf["nodes"])))
  with open(gltf_file, "w") as f:
    json.dump(gltf, f)

  return gltf
