#include "stim/io/stim_data_formats.h"

using namespace stim;

extern const std::map<std::string, stim::FileFormatData> stim::format_name_to_enum_map{
    {
        "01",
        FileFormatData{
            "01",
            SAMPLE_FORMAT_01,
            R"HELP(
The 01 format is a dense human readable format that stores shots as lines of '0' and '1' characters.

The data from each shot is terminated by a newline character '\n'. Each character in the line is a '0' (indicating
False) or a '1' (indicating True) corresponding to a measurement result (or a detector result) from a circuit.

This is the default format used by Stim, because it's the easiest to understand.

*Example of producing 01 format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="01")
    ...     with open(path) as f:
    ...         print(f.read().strip())
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
    00001111001101
)HELP",
            R"PYTHON(
from typing import List

def save_01(shots: List[List[bool]]) -> str:
    output = ""
    for shot in shots:
        for sample in shot:
            output += '1' if sample else '0'
        output += "\n"
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_01(data: str) -> List[List[bool]]:
    shots = []
    for line in data.split('\n'):
        if not line:
            continue
        shot = []
        for c in line:
            assert c in '01'
            shot.append(c == '1')
        shots.append(shot)
    return shots
)PYTHON",
        },
    },

    {
        "b8",
        FileFormatData{
            "b8",
            SAMPLE_FORMAT_B8,
            R"HELP(
The b8 format is a dense binary format that stores shots as bit-packed bytes.

Each shot is stored into ceil(n / 8) bytes, where n is the number of bits in the shot. Effectively, each shot is padded
up to a multiple of 8 by appending False bits, so that shots always start on a byte boundary. Bits are packed into bytes
in significance order (the 1s bit is the first bit, the 2s bit is the second, the 4s bit is the third, and so forth
until the 128s bit which is the eighth bit).

This format requires the reader to know the number of bits in each shot.

*Example of producing b8 format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="b8")
    ...     with open(path, 'rb') as f:
    ...         print(' '.join(hex(e)[2:] for e in f.read()))
    f0 2c f0 2c f0 2c f0 2c f0 2c f0 2c f0 2c f0 2c f0 2c f0 2c
)HELP",
            R"PYTHON(
from typing import List

def save_b8(shots: List[List[bool]]) -> bytes:
    output = b""
    for shot in shots:
        bytes_per_shot = (len(shot) + 7) // 8
        v = 0
        for b in reversed(shot):
            v <<= 1
            v += b
        output += v.to_bytes(bytes_per_shot, 'little')
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_b8(data: bytes, bits_per_shot: int) -> List[List[bool]]:
    shots = []
    bytes_per_shot = (bits_per_shot + 7) // 8
    for offset in range(0, len(data), bytes_per_shot):
        shot = []
        for k in range(bits_per_shot):
            byte = data[offset + k // 8]
            bit = (byte >> (k % 8)) % 2 == 1
            shot.append(bit)
        shots.append(shot)
    return shots
)PYTHON",
        },
    },

    {
        "ptb64",
        FileFormatData{
            "ptb64",
            SAMPLE_FORMAT_PTB64,
            R"HELP(
The ptb64 format is a dense SIMD-focused binary format that stores shots as partially transposed bit-packed data.

Each 64 bit word (8 bytes) of the data contains bits from the same measurement result across 64 separate shots. The next
8 bytes contain bits for the next measurement result from the 64 separate shots. This continues until the 8 bytes
containing the bits from the last measurement result, and then starts over again with data from the next 64 shots (if
there are more).

The shots are stored by byte order then significance order. The first shot's data goes into the least significant bit of
the first byte of each 8 byte group.

This format requires the number of shots to be a multiple of 64.
This format requires the reader to know the number of shots that were taken.

This format is generally more tedious to work with, but useful for achieving good performance on data processing tasks
where it is possible to parallelize across shots using SIMD instructions.

*Example of producing ptb64 format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 1
    ...     """).compile_sampler().sample_write(shots=64, filepath=path, format="ptb64")
    ...     with open(path, 'rb') as f:
    ...         print(' '.join(hex(e)[2:] for e in f.read()))
    0 0 0 0 0 0 0 0 ff ff ff ff ff ff ff ff
)HELP",
            R"PYTHON(
from typing import List

def save_ptb64(shots: List[List[bool]]):
    if len(shots) % 64 != 0:
        raise ValueError("Number of shots must be a multiple of 64.")

    output = b""
    for shot_offset in range(0, len(shots), 64):
        bits_per_shot = len(shots[0])
        for measure_index in range(bits_per_shot):
            v = 0
            for k in range(64)[::-1]:
                v <<= 1
                v += shots[shot_offset + k][measure_index]
            output += v.to_bytes(8, 'little')
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_ptb64(data: bytes, bits_per_shot: int) -> List[List[bool]]:
    num_shot_groups = int(len(data) * 8 / bits_per_shot / 64)
    if len(data) * 8 != num_shot_groups * 64 * bits_per_shot:
        raise ValueError("Number of shots must be a multiple of 64.")

    result = [[False] * bits_per_shot for _ in range(num_shot_groups * 64)]
    for group_index in range(num_shot_groups):
        group_bit_offset = 64 * bits_per_shot * group_index
        for m in range(bits_per_shot):
            m_bit_offset = m * 64
            for shot in range(64):
                bit_offset = group_bit_offset + m_bit_offset + shot
                byte_offset = bit_offset // 8
                bit = data[bit_offset // 8] & (1 << (bit_offset % 8)) != 0
                s = group_index * 64 + shot
                result[s][m] = bit
    return result
)PYTHON",
        },
    },

    {
        "hits",
        FileFormatData{
            "hits",
            SAMPLE_FORMAT_HITS,
            R"HELP(
The hits format is a dense human readable format that stores shots as a comma-separated list of integers.
Each integer indicates the position of a bit that was True.
Positions that aren't mentioned had bits that were False.

The data from each shot is terminated by a newline character '\n'. The line is a series of non-negative integers
separated by commas, with each integer indicating a bit from the shot that was true.

This format requires the reader to know the number of bits in each shot (if they want to get a list instead of a set).
This format requires the reader to know how many trailing newlines, that don't correspond to shots with no hit, are in
the text data.

This format is useful in contexts where the number of set bits is expected to be low, e.g. when sampling detection
events.

*Example of producing hits format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="hits")
    ...     with open(path) as f:
    ...         print(f.read().strip())
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
    4,5,6,7,10,11,13
)HELP",
            R"PYTHON(
from typing import List

def save_hits(shots: List[List[bool]]) -> str:
    output = ""
    for shot in shots:
        output += ",".join(str(index) for index, bit in enumerate(shot) if bit) + "\n"
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_hits(data: str, bits_per_shot: int) -> List[List[bool]]:
    shots = []
    if data.endswith('\n'):
        data = data[:-1]
    for line in data.split('\n'):
        shot = [False] * bits_per_shot
        if line:
            for term in line.split(','):
                shot[int(term)] = True
        shots.append(shot)
    return shots
)PYTHON",
        },
    },

    {
        "r8",
        FileFormatData{
            "r8",
            SAMPLE_FORMAT_R8,
            R"HELP(
The r8 format is a sparse binary format that stores shots as a series of lengths of runs between 1s.

Each byte in the data indicates how many False bits there are before the next True bit. The maximum byte value (255) is
special; it indicates to include 255 False bits but not follow them by a True bit. A shot always has a terminating True
bit appended to it before encoding. Decoding of the shot ends (and the next shot begin) when this True bit just past the
end of the shot data is reached.

This format requires the reader to know the number of bits in each shot.

This format is useful in contexts where the number of set bits is expected to be low, e.g. when sampling detection
events.

*Example of producing r8 format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="r8")
    ...     with open(path, 'rb') as f:
    ...         print(' '.join(hex(e)[2:] for e in f.read()))
    4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0 4 0 0 0 2 0 1 0

    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="r8")
    ...     with open(path, 'rb') as f:
    ...         print(' '.join(hex(e)[2:] for e in f.read()))
    9 1f 9 1f 9 1f 9 1f 9 1f 9 1f 9 1f 9 1f 9 1f 9 1f
)HELP",
            R"PYTHON(
from typing import List

def save_r8(shots: List[List[bool]]) -> bytes:
    output = b""
    for shot in shots:
        gap = 0
        for b in shot + [True]:
            if b:
                while gap >= 255:
                    gap -= 255
                    output += (255).to_bytes(1, 'big')
                output += gap.to_bytes(1, 'big')
                gap = 0
            else:
                gap += 1
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_r8(data: bytes, bits_per_shot: int) -> List[List[bool]]:
    shots = []
    shot = []
    for byte in data:
        shot += [False] * byte
        if byte != 255:
            shot.append(True)
        if len(shot) > bits_per_shot:
            assert len(shot) == bits_per_shot + 1 and shot[-1]
            shot.pop()
            shots.append(shot)
            shot = []
    assert len(shot) == 0
    return shots
)PYTHON",
        },
    },

    {
        "dets",
        FileFormatData{
            "dets",
            SAMPLE_FORMAT_DETS,
            R"HELP(
The dets format is a sparse human readable format that stores shots as lines starting with the word 'shot'
and then containing space-separated prefixed values like 'D5' and 'L2'. Each value's prefix indicates whether
it is a measurement (M), a detector (D), or observable frame change (L) and its integer indicates that
the corresponding measurement/detection-event/frame-change was True instead of False.

The data from each shot is started with the text 'shot' and terminated by a newline character '\n'. The rest of the
line is a series of integers, separated by spaces and prefixed by a single letter. The prefix letter indicates the type
of value ('M' for measurement, 'D' for detector, and 'L' for logical observable). The integer indicates the index of the
value. For example, "D1 D3 L0" indicates detectors 1 and 3 fired, and logical observable 0 was flipped.

This format requires the reader to know the number of measurements/detectors/observables in each shot, if the reader
wants to produce vectors of bits instead of sets.

*Example of producing dets format data using stim's python API:*

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1 0 1
    ...     """).compile_sampler().sample_write(shots=3, filepath=path, format="dets")
    ...     with open(path) as f:
    ...         print(f.read().strip())
    shot M4 M5 M6 M7 M10 M11 M13 M15
    shot M4 M5 M6 M7 M10 M11 M13 M15
    shot M4 M5 M6 M7 M10 M11 M13 M15

    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X_ERROR(1) 1
    ...         M 0 1 2
    ...         DETECTOR rec[-1]
    ...         DETECTOR rec[-2]
    ...         DETECTOR rec[-3]
    ...         OBSERVABLE_INCLUDE(5) rec[-2]
    ...     """).compile_detector_sampler().sample_write(shots=2, filepath=path, format="dets", append_observables=True)
    ...     with open(path) as f:
    ...         print(f.read().strip())
    shot D1 L5
    shot D1 L5
)HELP",
            R"PYTHON(
from typing import List

def save_dets(shots: List[List[bool]], num_detectors: int, num_observables: int):
    output = ""
    for shot in shots:
        assert len(shot) == num_detectors + num_observables
        detectors = shot[:num_detectors]
        observables = shot[num_detectors:]
        output += "shot"
        for k in range(num_detectors):
            if shot[k]:
                output += " D" + str(k)
        for k in range(num_observables):
            if shot[num_detectors + k]:
                output += " L" + str(k)
        output += "\n"
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_dets(data: str, num_detectors: int, num_observables: int) -> List[List[bool]]:
    shots = []
    for line in data.split('\n'):
        if not line.strip():
            continue
        assert line.startswith('shot')
        line = line[4:].strip()

        shot = [False] * (num_detectors + num_observables)
        if line:
            for term in line.split(' '):
                c = term[0]
                v = int(term[1:])
                if c == 'D':
                    assert 0 <= v < num_detectors
                    shot[v] = True
                elif c == 'L':
                    assert 0 <= v < num_observables
                    shot[num_detectors + v] = True
                else:
                    raise NotImplementedError(c)
        shots.append(shot)
    return shots
)PYTHON",
        },
    },
};
