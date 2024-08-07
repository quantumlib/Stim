import os
from typing import Optional, TYPE_CHECKING

from sinter._decoding import Sampler
from sinter._collection._collection_worker_state import CollectionWorkerState

if TYPE_CHECKING:
    import multiprocessing


def collection_worker_loop(
    flush_period: float,
    worker_id: int,
    sampler: Sampler,
    inp: 'multiprocessing.Queue',
    out: 'multiprocessing.Queue',
    core_affinity: Optional[int],
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
        sampler=sampler,
        inp=inp,
        out=out,
        custom_error_count_key=custom_error_count_key,
    )
    worker.run_message_loop()
