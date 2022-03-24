from typing import Optional
from typing import Union

import numpy as np
import stim

from simmer.case_executable import CaseExecutable
from simmer.case_stats import CaseStats
from simmer.case_summary import JSON_TYPE
from simmer.existing_data import ExistingData


class Task:
    """A decoding problem and a specification of how to take samples from it."""

    def __init__(self,
                 *,
                 # Information related to what the problem being sampled is.
                 circuit: stim.Circuit,
                 decoder: str,
                 error_model_for_decoder: Optional[stim.DetectorErrorModel] = None,
                 postselection_mask: Optional[np.ndarray] = None,
                 json_metadata: JSON_TYPE = None,

                 # Information related to how to take samples from the problem.
                 max_shots: int,
                 max_errors: int,
                 start_batch_size: int = 100,
                 max_batch_size: Optional[int] = None,
                 max_batch_seconds: Optional[float] = None,
                 previous_stats: Union[CaseStats, ExistingData] = CaseStats(),
                 ) -> None:
        """Describes a decoding problem and roughly how to sample it.

        Args:
            circuit: The annotated noisy circuit to sample detection event data
                and logical observable data form.
            decoder: The decoder to use to predict the logical observable data
                from the detection event data.
            error_model_for_decoder: Defaults to None (automatically derive from
                circuit). The error model to configure the decoder with.
            postselection_mask: Defaults to None (unused). A bit packed bitmask
                identifying detectors that must not fire. Shots where the
                indicated detectors fire are discarded.
            json_metadata: Defaults to None. Custom additional data describing
                the problem. Must be JSON serializable. For example, this could
                be a dictionary with "physical_error_rate" and "code_distance"
                keys.
            max_shots: Stops the sampling process after this many samples have
                been taken from the circuit.
            max_errors: Stops the sampling process after this many errors have
                been seen in samples taken from the circuit. The actual number
                sampled errors may be larger due to batching.
            start_batch_size: Defaults to 100. The very first shots taken from
                the circuit will use a batch of this size, and no other batches
                will be taken in parallel. Once this initial fact finding batch
                is done, batches can be taken in parallel and the normal batch
                size limiting processes take over.
            max_batch_size: Defaults to None (unused). Limits batches from
                taking more than this many shots at once. For example, this can
                be used to ensure memory usage stays below some limit.
            max_batch_seconds: Defaults to None (unused). When set, the recorded
                data from previous shots is used to estimate how much time is
                taken per shot. This information is then used to predict the
                biggest batch size that can finish in under the given number of
                seconds. Limits each batch to be no larger than that.
            previous_stats: If previous information has already been collected,
                it can be specified here. This is useful if you want the manager
                to immediately know roughly what the error rate is and how long
                a batch might take, instead of having to wait for new shots to
                reveal this information again.
        """
        if max_shots < 0:
            raise ValueError(f'max_shots={max_shots} < 0')
        if max_errors < 0:
            raise ValueError(f'max_errors={max_errors} < 0')
        if start_batch_size <= 0:
            raise ValueError(f'start_batch_size={start_batch_size} <= 0')
        if max_batch_size is not None and max_batch_size <= 0:
            raise ValueError(
                f'max_batch_size={max_batch_size} is not None and max_batch_size <= 0')
        if max_batch_seconds is not None and max_batch_seconds <= 0:
            raise ValueError(
                f'max_batch_seconds={max_batch_seconds} is not None and max_batch_seconds <= 0')

        self.circuit = circuit
        self.decoder = decoder
        self.error_model_for_decoder = error_model_for_decoder
        self.postselection_mask = postselection_mask
        self.json_metadata = json_metadata

        self.max_shots = max_shots
        self.max_errors = max_errors
        self.start_batch_size = start_batch_size
        self.max_batch_size = max_batch_size
        self.max_batch_seconds = max_batch_seconds

        if isinstance(previous_stats, ExistingData):
            self.previous_stats = previous_stats.stats_for(
                    self.to_case_executable())
        elif isinstance(previous_stats, CaseStats):
            self.previous_stats = previous_stats
        else:
            raise NotImplementedError(f'previous_stats={previous_stats!r}')

    def to_case_executable(self) -> CaseExecutable:
        return CaseExecutable(
            decoder=self.decoder,
            circuit=self.circuit,
            error_model_for_decoder=self.error_model_for_decoder,
            postselection_mask=self.postselection_mask,
            json_metadata=self.json_metadata,
        )
