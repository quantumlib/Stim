import pathlib
from typing import Optional
from typing import Union

from sinter._data import Task
from sinter._decoding._decoding_all_built_in_decoders import BUILT_IN_SAMPLERS
from sinter._decoding._decoding_decoder_class import Decoder
from sinter._decoding._sampler import CompiledSampler
from sinter._decoding._sampler import Sampler
from sinter._decoding._stim_then_decode_sampler import StimThenDecodeSampler


class MuxSampler(Sampler):
    """Looks up the sampler to use for a task, by the task's decoder name."""

    def __init__(
        self,
        *,
        custom_decoders: Union[dict[str, Union[Decoder, Sampler]], None],
        count_observable_error_combos: bool,
        count_detection_events: bool,
        tmp_dir: Optional[pathlib.Path],
    ):
        self.custom_decoders = custom_decoders
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.tmp_dir = tmp_dir

    def compiled_sampler_for_task(self, task: Task) -> CompiledSampler:
        return self._resolve_sampler(task.decoder).compiled_sampler_for_task(task)

    def _resolve_sampler(self, name: str) -> Sampler:
        sub_sampler: Union[Decoder, Sampler]

        if name in self.custom_decoders:
            sub_sampler = self.custom_decoders[name]
        elif name in BUILT_IN_SAMPLERS:
            sub_sampler = BUILT_IN_SAMPLERS[name]
        else:
            raise NotImplementedError(f'Not a recognized decoder or sampler: {name=}. Did you forget to specify custom_decoders?')

        if isinstance(sub_sampler, Sampler):
            if self.count_detection_events:
                raise NotImplementedError("'count_detection_events' not supported when using a custom Sampler (instead of a custom Decoder).")
            if self.count_observable_error_combos:
                raise NotImplementedError("'count_observable_error_combos' not supported when using a custom Sampler (instead of a custom Decoder).")
            return sub_sampler
        elif isinstance(sub_sampler, Decoder) or hasattr(sub_sampler, 'compile_decoder_for_dem'):
            return StimThenDecodeSampler(
                decoder=sub_sampler,
                count_detection_events=self.count_detection_events,
                count_observable_error_combos=self.count_observable_error_combos,
                tmp_dir=self.tmp_dir,
            )
        else:
            raise NotImplementedError(f"Don't know how to turn this into a Sampler: {sub_sampler!r}")
