#include "stim/io/stim_data_formats.h"

using namespace stim;

extern const std::map<std::string, stim::FileFormatData> stim::format_name_to_enum_map{
    {
        "01",
        FileFormatData{
            "01",
            SAMPLE_FORMAT_01,
            R"HELP(
A human readable format that stores shots as lines of '0' and '1' characters.

The data from each shot is terminated by a newline character '\n'. Each character in the line is a '0' (indicating
False) or a '1' (indicating True) corresponding to a measurement result (or a detector result) from a circuit.

This is the default format used when saving to files.

Example:

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
A binary format that stores shots as bit-packed bytes.

Each shot is stored into ceil(n / 8) bytes, where n is the number of bits in the shot. Effectively, each shot is padded
up to a multiple of 8 by appending False bits, so that shots always start on a byte boundary. Bits are packed into bytes
in significance order (the 1s bit is the first bit, the 2s bit is the second, the 4s bit is the third, and so forth
until the 128s bit which is the eighth bit).

This format requires the reader to know the number of bits in each shot.

Example:

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
A binary format that stores shots as partially transposed bit-packed data.

Each 64 bit word (8 bytes) of the data contains bits from the same measurement result across 64 separate shots. The next
8 bytes contain bits for the next measurement result from the 64 separate shots. This continues until the 8 bytes
containing the bits from the last measurement result, and then starts over again with data from the next 64 shots (if
there are more).

The shots are stored by byte order then significant order. The first shot's data goes into the least significant bit of
the first byte of each 8 byte group. When the number of shots is not a multiple of 64, the bits corresponding to the
missing shots are always zero.

This format requires the reader to know the number of bits in each shot.
This format requires the reader to know the number of shots that were taken.

This format is generally more tedious to work with, but useful for achieving good performance on data processing tasks
where it is possible to parallelize across shots using SIMD instructions.

Example:

    >>> import pathlib
    >>> import stim
    >>> import tempfile
    >>> with tempfile.TemporaryDirectory() as d:
    ...     path = str(pathlib.Path(d) / "tmp.dat")
    ...     stim.Circuit("""
    ...         X 1
    ...         M 0 0 0 0 1 1 1 1 0 0 1 1 0 1
    ...     """).compile_sampler().sample_write(shots=10, filepath=path, format="ptb64")
    ...     with open(path, 'rb') as f:
    ...         print(' '.join(hex(e)[2:] for e in f.read()))
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 ff 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ff 3 0 0 0 0 0 0
)HELP",
            R"PYTHON(
from typing import List

def save_ptb64(shots: List[List[bool]]):
    output = b""
    for shot_offset in range(0, len(shots), 64):
        bits_per_shot = len(shots[0])
        for measure_index in range(bits_per_shot):
            v = 0
            for k in reversed(range(min(64, len(shots) - shot_offset))):
                v <<= 1
                v += shots[shot_offset + k][measure_index]
            output += v.to_bytes(8, 'little')
    return output
)PYTHON",
            R"PYTHON(
from typing import List

def parse_ptb64(data: bytes, bits_per_shot: int, num_shots: int) -> List[List[bool]]:
    result = [[False] * bits_per_shot for _ in range(num_shots)]
    group_byte_stride = bits_per_shot * 8
    for shot_offset in range(num_shots):
        shot_group = shot_offset // 64
        shot_stripe = shot_offset % 64
        for measure_index in range(bits_per_shot):
            byte_offset = group_byte_stride*shot_group + measure_index * 8 + shot_stripe // 8
            bit = (data[byte_offset] >> (shot_stripe % 8)) % 2 == 1
            result[shot_offset][measure_index] = bit
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
A human readable format that stores shots as a comma-separated list of integers indicating which bits were true.

The data from each shot is terminated by a newline character '\n'. The line is a series of non-negative integers
separated by commas, with each integer indicating a bit from the shot that was true.

This format requires the reader to know the number of bits in each shot (if they want to get a list instead of a set).
This format requires the reader to know how many trailing newlines, that don't correspond to shots with no hit, are in
the text data.

This format is useful in contexts where the number of set bits is expected to be low, e.g. when sampling detection
events.

Example:

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
A binary format that stores shots as the length of runs between 1s.

Each byte in the data indicates how many False bits there are before the next True bit. The maximum byte value (255) is
special; it indicates to include 255 False bits but not follow them by a True bit. A shot always has a terminating True
bit appended to it before encoding. Decoding of the shot ends (and the next shot begin) when this True bit just past the
end of the shot data is reached.

This format requires the reader to know the number of bits in each shot.

This format is useful in contexts where the number of set bits is expected to be low, e.g. when sampling detection
events.

Example:

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
A human readable format that stores shots as lines starting with 'shot' and space-separated prefixed values like 'D5'.

The data from each shot is started with the text 'shot' and terminated by a newline character '\n'. The rest of the
line is a series of integers, separated by spaces and prefixed by a single letter. The prefix letter indicates the type
of value ('M' for measurement, 'D' for detector, and 'L' for logical observable). The integer indicates the index of the
value. For example, "D1 D3 L0" indicates detectors 1 and 3 fired, and logical observable 0 was flipped.

This format requires the reader to know the number of measurements/detectors/observables in each shot, if the reader
wants to produce vectors of bits instead of sets.

Example:

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
