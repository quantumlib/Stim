import os
import pathlib
from typing import Optional, TYPE_CHECKING
from typing import Union

from sinter._decoding import Decoder, Sampler
from sinter._collection._collection_worker_state import CollectionWorkerState

if TYPE_CHECKING:
    import multiprocessing


def collection_worker_loop(
    flush_period: float,
    worker_id: int,
    custom_decoders: dict[str, Union[Decoder, Sampler]],
    inp: 'multiprocessing.Queue',
    out: 'multiprocessing.Queue',
    core_affinity: Optional[int],
    count_detection_events: bool,
    count_observable_error_combos: bool,
    tmp_dir: pathlib.Path,
    custom_error_count_key: Optional[str],
) -> None:
    try:
        if core_affinity is not None and hasattr(os, 'sched_setaffinity'):
            os.sched_setaffinity(0, {core_affinity})
    except:
        # If setting the core affinity fails, we keep going regardless.
        pass

    worker = CollectionWorkerState(
        flush_period=flush_period,
        worker_id=worker_id,
        custom_decoders=custom_decoders,
        inp=inp,
        out=out,
        count_detection_events=count_detection_events,
        count_observable_error_combos=count_observable_error_combos,
        tmp_dir=tmp_dir,
        custom_error_count_key=custom_error_count_key,
    )
    worker.run_message_loop()
