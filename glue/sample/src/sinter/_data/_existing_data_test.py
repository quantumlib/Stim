import collections
import pathlib
import tempfile

import sinter


def test_read_stats_from_csv_files():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)

        with open(d / 'tmp.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
  300,     1,      20,    1.0,pymatching,abc123,"{""d"":3}"
 1000,     3,      40,    3.0,pymatching,abc123,"{""d"":3}"
 2000,     0,      10,    2.0,pymatching,def456,"{""d"":5}"
    """.strip(), file=f)

        assert sinter.read_stats_from_csv_files(d / 'tmp.csv') == [
            sinter.TaskStats(strong_id='abc123', decoder='pymatching', json_metadata={'d': 3}, shots=1300, errors=4, discards=60, seconds=4.0),
            sinter.TaskStats(strong_id='def456', decoder='pymatching', json_metadata={'d': 5}, shots=2000, errors=0, discards=10, seconds=2.0),
        ]

        with open(d / 'tmp2.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata,custom_counts
  300,     1,      20,    1.0,pymatching,abc123,"{""d"":3}","{""dets"":1234}"
 1000,     3,      40,    3.0,pymatching,abc123,"{""d"":3}",
 2000,     0,      10,    2.0,pymatching,def456,"{""d"":5}"
    """.strip(), file=f)

        assert sinter.read_stats_from_csv_files(d / 'tmp2.csv') == [
            sinter.TaskStats(strong_id='abc123', decoder='pymatching', json_metadata={'d': 3}, shots=1300, errors=4, discards=60, seconds=4.0, custom_counts=collections.Counter({'dets': 1234})),
            sinter.TaskStats(strong_id='def456', decoder='pymatching', json_metadata={'d': 5}, shots=2000, errors=0, discards=10, seconds=2.0),
        ]

        assert sinter.read_stats_from_csv_files(d / 'tmp.csv', d / 'tmp2.csv') == [
            sinter.TaskStats(strong_id='abc123', decoder='pymatching', json_metadata={'d': 3}, shots=2600, errors=8, discards=120, seconds=8.0, custom_counts=collections.Counter({'dets': 1234})),
            sinter.TaskStats(strong_id='def456', decoder='pymatching', json_metadata={'d': 5}, shots=4000, errors=0, discards=20, seconds=4.0),
        ]
