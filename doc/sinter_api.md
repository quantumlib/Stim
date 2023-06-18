# Sinter (Development Version) API Reference

*CAUTION*: this API reference is for the in-development version of sinter.
Methods and arguments mentioned here may not be accessible in stable versions, yet.
API references for stable versions are kept on the [stim github wiki](https://github.com/quantumlib/Stim/wiki)

## Index
- [`sinter.AnonTaskStats`](#sinter.AnonTaskStats)
    - [`sinter.AnonTaskStats.__add__`](#sinter.AnonTaskStats.__add__)
- [`sinter.CSV_HEADER`](#sinter.CSV_HEADER)
- [`sinter.CollectionOptions`](#sinter.CollectionOptions)
    - [`sinter.CollectionOptions.combine`](#sinter.CollectionOptions.combine)
- [`sinter.CompiledDecoder`](#sinter.CompiledDecoder)
    - [`sinter.CompiledDecoder.decode_shots_bit_packed`](#sinter.CompiledDecoder.decode_shots_bit_packed)
- [`sinter.Decoder`](#sinter.Decoder)
    - [`sinter.Decoder.compile_decoder_for_dem`](#sinter.Decoder.compile_decoder_for_dem)
    - [`sinter.Decoder.decode_via_files`](#sinter.Decoder.decode_via_files)
- [`sinter.Fit`](#sinter.Fit)
- [`sinter.Progress`](#sinter.Progress)
- [`sinter.Task`](#sinter.Task)
    - [`sinter.Task.__init__`](#sinter.Task.__init__)
    - [`sinter.Task.strong_id`](#sinter.Task.strong_id)
    - [`sinter.Task.strong_id_bytes`](#sinter.Task.strong_id_bytes)
    - [`sinter.Task.strong_id_text`](#sinter.Task.strong_id_text)
    - [`sinter.Task.strong_id_value`](#sinter.Task.strong_id_value)
- [`sinter.TaskStats`](#sinter.TaskStats)
    - [`sinter.TaskStats.to_anon_stats`](#sinter.TaskStats.to_anon_stats)
    - [`sinter.TaskStats.to_csv_line`](#sinter.TaskStats.to_csv_line)
- [`sinter.better_sorted_str_terms`](#sinter.better_sorted_str_terms)
- [`sinter.collect`](#sinter.collect)
- [`sinter.comma_separated_key_values`](#sinter.comma_separated_key_values)
- [`sinter.fit_binomial`](#sinter.fit_binomial)
- [`sinter.fit_line_slope`](#sinter.fit_line_slope)
- [`sinter.fit_line_y_at_x`](#sinter.fit_line_y_at_x)
- [`sinter.group_by`](#sinter.group_by)
- [`sinter.iter_collect`](#sinter.iter_collect)
- [`sinter.log_binomial`](#sinter.log_binomial)
- [`sinter.log_factorial`](#sinter.log_factorial)
- [`sinter.plot_discard_rate`](#sinter.plot_discard_rate)
- [`sinter.plot_error_rate`](#sinter.plot_error_rate)
- [`sinter.post_selection_mask_from_4th_coord`](#sinter.post_selection_mask_from_4th_coord)
- [`sinter.predict_discards_bit_packed`](#sinter.predict_discards_bit_packed)
- [`sinter.predict_observables`](#sinter.predict_observables)
- [`sinter.predict_observables_bit_packed`](#sinter.predict_observables_bit_packed)
- [`sinter.predict_on_disk`](#sinter.predict_on_disk)
- [`sinter.read_stats_from_csv_files`](#sinter.read_stats_from_csv_files)
- [`sinter.shot_error_rate_to_piece_error_rate`](#sinter.shot_error_rate_to_piece_error_rate)
- [`sinter.stats_from_csv_files`](#sinter.stats_from_csv_files)
```python
# Types used by the method definitions.
from typing import overload, TYPE_CHECKING, Any, Counter, Dict, Iterable, List, Optional, Tuple, Union
import abc
import dataclasses
import io
import numpy as np
import pathlib
import stim
```

<a name="sinter.AnonTaskStats"></a>
```python
# sinter.AnonTaskStats

# (at top-level in the sinter module)
@dataclasses.dataclass(frozen=True)
class AnonTaskStats:
    """Statistics sampled from an unspecified task.

    Attributes:
        shots: Number of times the task was sampled.
        errors: Number of times a sample resulted in an error.
        discards: Number of times a sample resulted in a discard. Note that
            discarded a task is not an error.
        seconds: The amount of CPU core time spent sampling the tasks, in
            seconds.
        custom_counts: A counter mapping string keys to integer values. Used for
            tracking arbitrary values, such as per-observable error counts or
            the number of times detectors fired. The meaning of the information
            in the counts is not specified; the only requirement is that it
            should be correct to add each key's counts when merging statistics.

            Although this field is an editable object, it's invalid to edit the
            counter after the stats object is initialized.
    """
    shots: int = 0
    errors: int = 0
    discards: int = 0
    seconds: float = 0
    custom_counts: Counter[str]
```

<a name="sinter.AnonTaskStats.__add__"></a>
```python
# sinter.AnonTaskStats.__add__

# (in class sinter.AnonTaskStats)
def __add__(
    self,
    other: sinter.AnonTaskStats,
) -> sinter.AnonTaskStats:
    """Returns the sum of the statistics from both anonymous stats.

    Adds the shots, the errors, the discards, and the seconds.

    Examples:
        >>> import sinter
        >>> a = sinter.AnonTaskStats(
        ...    shots=100,
        ...    errors=20,
        ... )
        >>> b = sinter.AnonTaskStats(
        ...    shots=1000,
        ...    errors=200,
        ... )
        >>> a + b
        sinter.AnonTaskStats(shots=1100, errors=220)
    """
```

<a name="sinter.CSV_HEADER"></a>
```python
# sinter.CSV_HEADER

# (at top-level in the sinter module)
CSV_HEADER: str = '     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts'
```

<a name="sinter.CollectionOptions"></a>
```python
# sinter.CollectionOptions

# (at top-level in the sinter module)
@dataclasses.dataclass(frozen=True)
class CollectionOptions:
    """Describes options for how data is collected for a decoding problem.

    Attributes:
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.
    """
    max_shots: Optional[int] = None
    max_errors: Optional[int] = None
    start_batch_size: Optional[int] = None
    max_batch_size: Optional[int] = None
    max_batch_seconds: Optional[float] = None
```

<a name="sinter.CollectionOptions.combine"></a>
```python
# sinter.CollectionOptions.combine

# (in class sinter.CollectionOptions)
def combine(
    self,
    other: sinter.CollectionOptions,
) -> sinter.CollectionOptions:
    """Returns a combination of multiple collection options.

    All fields are combined by taking the minimum from both collection
    options objects, with None treated as being infinitely large.

    Args:
        other: The collections options to combine with.

    Returns:
        The combined collection options.

    Examples:
        >>> import sinter
        >>> a = sinter.CollectionOptions(
        ...    max_shots=1_000_000,
        ...    start_batch_size=100,
        ... )
        >>> b = sinter.CollectionOptions(
        ...    max_shots=100_000,
        ...    max_errors=100,
        ... )
        >>> a.combine(b)
        sinter.CollectionOptions(max_shots=100000, max_errors=100, start_batch_size=100)
    """
```

<a name="sinter.CompiledDecoder"></a>
```python
# sinter.CompiledDecoder

# (at top-level in the sinter module)
class CompiledDecoder(metaclass=abc.ABCMeta):
    """Abstract class for decoders preconfigured to a specific decoding task.

    This is the type returned by `sinter.Decoder.compile_decoder_for_dem`. The
    idea is that, when many shots of the same decoding task are going to be
    performed, it is valuable to pay the cost of configuring the decoder only
    once instead of once per batch of shots. Custom decoders can optionally
    implement that method, and return this type, to increase sampling
    efficiency.
    """
```

<a name="sinter.CompiledDecoder.decode_shots_bit_packed"></a>
```python
# sinter.CompiledDecoder.decode_shots_bit_packed

# (in class sinter.CompiledDecoder)
@abc.abstractmethod
def decode_shots_bit_packed(
    self,
    *,
    bit_packed_detection_event_data: np.ndarray,
) -> np.ndarray:
    """Predicts observable flips from the given detection events.

    All data taken and returned must be bit packed with bitorder='little'.

    Args:
        bit_packed_detection_event_data: Detection event data stored as a
            bit packed numpy array. The numpy array will have the following
            dtype/shape:

                dtype: uint8
                shape: (num_shots, ceil(dem.num_detectors / 8))

            where `num_shots` is the number of shots to decoder and `dem` is
            the detector error model this instance was compiled to decode.

    Returns:
        Bit packed observable flip data stored as a bit packed numpy array.
        The numpy array must have the following dtype/shape:

            dtype: uint8
            shape: (num_shots, ceil(dem.num_observables / 8))

        where `num_shots` is bit_packed_detection_event_data.shape[0] and
        `dem` is the detector error model this instance was compiled to
        decode.
    """
```

<a name="sinter.Decoder"></a>
```python
# sinter.Decoder

# (at top-level in the sinter module)
class Decoder(metaclass=abc.ABCMeta):
    """Abstract base class for custom decoders.

    Custom decoders can be explained to sinter by inheriting from this class and
    implementing its methods.

    Decoder classes MUST be serializable (e.g. via pickling), so that they can
    be given to worker processes when using python multiprocessing.
    """
```

<a name="sinter.Decoder.compile_decoder_for_dem"></a>
```python
# sinter.Decoder.compile_decoder_for_dem

# (in class sinter.Decoder)
def compile_decoder_for_dem(
    self,
    *,
    dem: stim.DetectorErrorModel,
) -> sinter.CompiledDecoder:
    """Creates a decoder preconfigured for the given detector error model.

    This method is optional to implement. By default, it will raise a
    NotImplementedError. When sampling, sinter will attempt to use this
    method first and otherwise fallback to using `decode_via_files`.

    The idea is that the preconfigured decoder amortizes the cost of
    configuration over more calls. This makes smaller batch sizes efficient,
    reducing the amount of memory used for storing each batch, improving
    overall efficiency.

    Args:
        dem: A detector error model for the samples that will need to be
            decoded. What to configure the decoder to decode.

    Returns:
        An instance of `sinter.CompiledDecoder` that can be used to invoke
        the preconfigured decoder.

    Raises:
        NotImplementedError: This `sinter.Decoder` doesn't support compiling
            for a dem.
    """
```

<a name="sinter.Decoder.decode_via_files"></a>
```python
# sinter.Decoder.decode_via_files

# (in class sinter.Decoder)
@abc.abstractmethod
def decode_via_files(
    self,
    *,
    num_shots: int,
    num_dets: int,
    num_obs: int,
    dem_path: pathlib.Path,
    dets_b8_in_path: pathlib.Path,
    obs_predictions_b8_out_path: pathlib.Path,
    tmp_dir: pathlib.Path,
) -> None:
    """Performs decoding by reading/writing problems and answers from disk.

    Args:
        num_shots: The number of times the circuit was sampled. The number
            of problems to be solved.
        num_dets: The number of detectors in the circuit. The number of
            detection event bits in each shot.
        num_obs: The number of observables in the circuit. The number of
            predicted bits in each shot.
        dem_path: The file path where the detector error model should be
            read from, e.g. using `stim.DetectorErrorModel.from_file`. The
            error mechanisms specified by the detector error model should be
            used to configure the decoder.
        dets_b8_in_path: The file path that detection event data should be
            read from. Note that the file may be a named pipe instead of a
            fixed size object. The detection events will be in b8 format
            (see
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
            ). The number of detection events per shot is available via the
            `num_dets` argument or via the detector error model at
            `dem_path`.
        obs_predictions_b8_out_path: The file path that decoder predictions
            must be written to. The predictions must be written in b8 format
            (see
            https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
            ). The number of observables per shot is available via the
            `num_obs` argument or via the detector error model at
            `dem_path`.
        tmp_dir: Any temporary files generated by the decoder during its
            operation MUST be put into this directory. The reason for this
            requirement is because sinter is allowed to kill the decoding
            process without warning, without giving it time to clean up any
            temporary objects. All cleanup should be done via sinter
            deleting this directory after killing the decoder.
    """
```

<a name="sinter.Fit"></a>
```python
# sinter.Fit

# (at top-level in the sinter module)
@dataclasses.dataclass(frozen=True)
class Fit:
    """The result of a fitting process.

    Attributes:
        low: The hypothesis with the smallest parameter whose cost or score was
            still "close to" the cost of the best hypothesis. For example, this
            could be a hypothesis whose squared error was within some tolerance
            of the best fit's square error, or whose likelihood was within some
            maximum Bayes factor of the max likelihood hypothesis.
        best: The max likelihood hypothesis. The hypothesis that had the lowest
            squared error, or the best fitting score.
        high: The hypothesis with the larger parameter whose cost or score was
            still "close to" the cost of the best hypothesis. For example, this
            could be a hypothesis whose squared error was within some tolerance
            of the best fit's square error, or whose likelihood was within some
            maximum Bayes factor of the max likelihood hypothesis.
    """
    low: float
    best: float
    high: float
```

<a name="sinter.Progress"></a>
```python
# sinter.Progress

# (at top-level in the sinter module)
@dataclasses.dataclass(frozen=True)
class Progress:
    """Describes statistics and status messages from ongoing sampling.

    This is the type yielded by `sinter.iter_collect`, and given to the
    `progress_callback` argument of `sinter.collect`.

    Attributes:
        new_stats: New sampled statistics collected since the last progress
            update.
        status_message: A free form human readable string describing the current
            collection status, such as the number of tasks left and the
            estimated time to completion for each task.
    """
    new_stats: Tuple[sinter._task_stats.TaskStats, ...]
    status_message: str
```

<a name="sinter.Task"></a>
```python
# sinter.Task

# (at top-level in the sinter module)
class Task:
    """A decoding problem that sinter can sample from.

    Attributes:
        circuit: The annotated noisy circuit to sample detection event data
            and logical observable data form.
        decoder: The decoder to use to predict the logical observable data
            from the detection event data. This can be set to None if it
            will be specified later (e.g. by the call to `collect`).
        detector_error_model: Specifies the error model to give to the decoder.
            Defaults to None, indicating that it should be automatically derived
            using `stim.Circuit.detector_error_model`.
        postselection_mask: Defaults to None (unused). A bit packed bitmask
            identifying detectors that must not fire. Shots where the
            indicated detectors fire are discarded.
        postselected_observables_mask: Defaults to None (unused). A bit
            packed bitmask identifying observable indices to postselect on.
            Anytime the decoder's predicted flip for one of these
            observables doesn't agree with the actual measured flip value of
            the observable, the shot is discarded instead of counting as an
            error.
        json_metadata: Defaults to None. Custom additional data describing
            the problem. Must be JSON serializable. For example, this could
            be a dictionary with "physical_error_rate" and "code_distance"
            keys.
        collection_options: Specifies custom options for collecting this
            single task. These options are merged with the global options
            to determine what happens.

            For example, if a task has `collection_options` set to
            `sinter.CollectionOptions(max_shots=1000, max_errors=100)` and
            `sinter.collect` was called with `max_shots=500` and
            `max_errors=200`, then either 500 shots or 100 errors will be
            collected for the task (whichever comes first).

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit.generated(
        ...         'repetition_code:memory',
        ...         rounds=10,
        ...         distance=10,
        ...         before_round_data_depolarization=1e-3,
        ...     ),
        ... )
    """
```

<a name="sinter.Task.__init__"></a>
```python
# sinter.Task.__init__

# (in class sinter.Task)
def __init__(
    self,
    *,
    circuit: stim.Circuit,
    decoder: Optional[str] = None,
    detector_error_model: Optional[ForwardRef(stim.DetectorErrorModel)] = None,
    postselection_mask: Optional[np.ndarray] = None,
    postselected_observables_mask: Optional[np.ndarray] = None,
    json_metadata: Any = None,
    collection_options: sinter.CollectionOptions = sinter.CollectionOptions(),
    skip_validation: bool = False,
    _unvalidated_strong_id: Optional[str] = None,
) -> None:
    """
    Args:
        circuit: The annotated noisy circuit to sample detection event data
            and logical observable data form.
        decoder: The decoder to use to predict the logical observable data
            from the detection event data. This can be set to None if it
            will be specified later (e.g. by the call to `collect`).
        detector_error_model: Specifies the error model to give to the decoder.
            Defaults to None, indicating that it should be automatically derived
            using `stim.Circuit.detector_error_model`.
        postselection_mask: Defaults to None (unused). A bit packed bitmask
            identifying detectors that must not fire. Shots where the
            indicated detectors fire are discarded.
        postselected_observables_mask: Defaults to None (unused). A bit
            packed bitmask identifying observable indices to postselect on.
            Anytime the decoder's predicted flip for one of these
            observables doesn't agree with the actual measured flip value of
            the observable, the shot is discarded instead of counting as an
            error.
        json_metadata: Defaults to None. Custom additional data describing
            the problem. Must be JSON serializable. For example, this could
            be a dictionary with "physical_error_rate" and "code_distance"
            keys.
        collection_options: Specifies custom options for collecting this
            single task. These options are merged with the global options
            to determine what happens.

            For example, if a task has `collection_options` set to
            `sinter.CollectionOptions(max_shots=1000, max_errors=100)` and
            `sinter.collect` was called with `max_shots=500` and
            `max_errors=200`, then either 500 shots or 100 errors will be
            collected for the task (whichever comes first).
        skip_validation: Defaults to False. Normally the arguments given to
            this method are checked for consistency (e.g. the detector error
            model should have the same number of detectors as the circuit).
            Setting this argument to True will skip doing the consistency
            checks. Note that this can result in confusing errors later, if
            the arguments are not actually consistent.
        _unvalidated_strong_id: Must be set to None unless `skip_validation`
            is set to True. Otherwise, if this is specified then it should
            be equal to the value returned by self.strong_id().
    """
```

<a name="sinter.Task.strong_id"></a>
```python
# sinter.Task.strong_id

# (in class sinter.Task)
def strong_id(
    self,
) -> str:
    """Computes a cryptographically unique identifier for this task.

    This value is affected by:
        - The exact circuit.
        - The exact detector error model.
        - The decoder.
        - The json metadata.
        - The postselection mask.

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit(),
        ...     detector_error_model=stim.DetectorErrorModel(),
        ...     decoder='pymatching',
        ... )
        >>> task.strong_id()
        '7424ea021693d4abc1c31c12e655a48779f61a7c2969e457ae4fe400c852bee5'
    """
```

<a name="sinter.Task.strong_id_bytes"></a>
```python
# sinter.Task.strong_id_bytes

# (in class sinter.Task)
def strong_id_bytes(
    self,
) -> bytes:
    """The bytes that are hashed to get the strong id.

    This value is converted into the actual strong id by:
        - Hashing these bytes using SHA256.

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit('H 0'),
        ...     detector_error_model=stim.DetectorErrorModel(),
        ...     decoder='pymatching',
        ... )
        >>> task.strong_id_bytes()
        b'{"circuit": "H 0", "decoder": "pymatching", "decoder_error_model": "", "postselection_mask": null, "json_metadata": null}'
    """
```

<a name="sinter.Task.strong_id_text"></a>
```python
# sinter.Task.strong_id_text

# (in class sinter.Task)
def strong_id_text(
    self,
) -> str:
    """The text that is serialized and hashed to get the strong id.

    This value is converted into the actual strong id by:
        - Serializing into bytes using UTF8.
        - Hashing the UTF8 bytes using SHA256.

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit('H 0'),
        ...     detector_error_model=stim.DetectorErrorModel(),
        ...     decoder='pymatching',
        ... )
        >>> task.strong_id_text()
        '{"circuit": "H 0", "decoder": "pymatching", "decoder_error_model": "", "postselection_mask": null, "json_metadata": null}'
    """
```

<a name="sinter.Task.strong_id_value"></a>
```python
# sinter.Task.strong_id_value

# (in class sinter.Task)
def strong_id_value(
    self,
) -> Dict[str, Any]:
    """Contains all raw values that affect the strong id.

    This value is converted into the actual strong id by:
        - Serializing it into text using JSON.
        - Serializing the JSON text into bytes using UTF8.
        - Hashing the UTF8 bytes using SHA256.

    Examples:
        >>> import sinter
        >>> import stim
        >>> task = sinter.Task(
        ...     circuit=stim.Circuit('H 0'),
        ...     detector_error_model=stim.DetectorErrorModel(),
        ...     decoder='pymatching',
        ... )
        >>> task.strong_id_value()
        {'circuit': 'H 0', 'decoder': 'pymatching', 'decoder_error_model': '', 'postselection_mask': None, 'json_metadata': None}
    """
```

<a name="sinter.TaskStats"></a>
```python
# sinter.TaskStats

# (at top-level in the sinter module)
@dataclasses.dataclass(frozen=True)
class TaskStats:
    """Statistics sampled from a task.

    The rows in the CSV files produced by sinter correspond to instances of
    `sinter.TaskStats`. For example, a row can be produced by printing a
    `sinter.TaskStats`.

    Attributes:
        strong_id: The cryptographically unique identifier of the task, from
            `sinter.Task.strong_id()`.
        decoder: The name of the decoder that was used to decode the task.
            Errors are counted when this decoder made a wrong prediction.
        json_metadata: A JSON-encodable value (such as a dictionary from strings
            to integers) that were included with the task in order to describe
            what the task was. This value can be a huge variety of things, but
            typically it will be a dictionary with fields such as 'd' for the
            code distance.
        shots: Number of times the task was sampled.
        errors: Number of times a sample resulted in an error.
        discards: Number of times a sample resulted in a discard. Note that
            discarded a task is not an error.
        seconds: The amount of CPU core time spent sampling the tasks, in
            seconds.
        custom_counts: A counter mapping string keys to integer values. Used for
            tracking arbitrary values, such as per-observable error counts or
            the number of times detectors fired. The meaning of the information
            in the counts is not specified; the only requirement is that it
            should be correct to add each key's counts when merging statistics.

            Although this field is an editable object, it's invalid to edit the
            counter after the stats object is initialized.
    """
    strong_id: str
    decoder: str
    json_metadata: Any
    shots: int = 0
    errors: int = 0
    discards: int = 0
    seconds: float = 0
    custom_counts: Counter[str]
```

<a name="sinter.TaskStats.to_anon_stats"></a>
```python
# sinter.TaskStats.to_anon_stats

# (in class sinter.TaskStats)
def to_anon_stats(
    self,
) -> sinter._anon_task_stats.AnonTaskStats:
    """Returns a `sinter.AnonTaskStats` with the same statistics.

    Examples:
        >>> import sinter
        >>> stat = sinter.TaskStats(
        ...     strong_id='test',
        ...     json_metadata={'a': [1, 2, 3]},
        ...     decoder='pymatching',
        ...     shots=22,
        ...     errors=3,
        ...     discards=4,
        ...     seconds=5,
        ... )
        >>> stat.to_anon_stats()
        sinter.AnonTaskStats(shots=22, errors=3, discards=4, seconds=5)
    """
```

<a name="sinter.TaskStats.to_csv_line"></a>
```python
# sinter.TaskStats.to_csv_line

# (in class sinter.TaskStats)
def to_csv_line(
    self,
) -> str:
    """Converts into a line that can be printed into a CSV file.

    Examples:
        >>> import sinter
        >>> stat = sinter.TaskStats(
        ...     strong_id='test',
        ...     json_metadata={'a': [1, 2, 3]},
        ...     decoder='pymatching',
        ...     shots=22,
        ...     errors=3,
        ...     seconds=5,
        ... )
        >>> print(sinter.CSV_HEADER)
             shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
        >>> print(stat.to_csv_line())
                22,         3,         0,       5,pymatching,test,"{""a"":[1,2,3]}",
    """
```

<a name="sinter.better_sorted_str_terms"></a>
```python
# sinter.better_sorted_str_terms

# (at top-level in the sinter module)
def better_sorted_str_terms(
    val: Any,
) -> Any:
    """A function that orders "a10000" after "a9", instead of before.

    Normally, sorting strings sorts them lexicographically, treating numbers so
    that "1999999" ends up being less than "2". This method splits the string
    into a tuple of text pairs and parsed number parts, so that sorting by this
    key puts "2" before "1999999".

    Because this method is intended for use in plotting, where it's more
    important to see a bad result than to see nothing, it returns a type that
    tries to be comparable to everything.

    Args:
        val: The value to convert into a value with a better sorting order.

    Returns:
        A custom type of object with a better sorting order.

    Examples:
        >>> import sinter
        >>> items = [
        ...    "distance=199999, rounds=3",
        ...    "distance=2, rounds=3",
        ...    "distance=199999, rounds=199999",
        ...    "distance=2, rounds=199999",
        ... ]
        >>> for e in sorted(items, key=sinter.better_sorted_str_terms):
        ...    print(e)
        distance=2, rounds=3
        distance=2, rounds=199999
        distance=199999, rounds=3
        distance=199999, rounds=199999
    """
```

<a name="sinter.collect"></a>
```python
# sinter.collect

# (at top-level in the sinter module)
def collect(
    *,
    num_workers: int,
    tasks: Union[Iterator[sinter.Task], Iterable[sinter.Task]],
    existing_data_filepaths: Iterable[Union[str, pathlib.Path]] = (),
    save_resume_filepath: Union[NoneType, str, pathlib.Path] = None,
    progress_callback: Optional[Callable[[sinter.Progress], NoneType]] = None,
    max_shots: Optional[int] = None,
    max_errors: Optional[int] = None,
    count_observable_error_combos: bool = False,
    count_detection_events: bool = False,
    decoders: Optional[Iterable[str]] = None,
    max_batch_seconds: Optional[int] = None,
    max_batch_size: Optional[int] = None,
    start_batch_size: Optional[int] = None,
    print_progress: bool = False,
    hint_num_tasks: Optional[int] = None,
    custom_decoders: Optional[Dict[str, sinter.Decoder]] = None,
) -> List[sinter.TaskStats]:
    """Collects statistics from the given tasks, using multiprocessing.

    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        save_resume_filepath: Defaults to None (unused). If set to a filepath,
            results will be saved to that file while they are collected. If the
            python interpreter is stopped or killed, calling this method again
            with the same save_resume_filepath will load the previous results
            from the file so it can resume where it left off.

            The stats in this file will be counted in addition to each task's
            previous_stats field (as opposed to overriding the field).
        existing_data_filepaths: CSV data saved to these files will be loaded,
            included in the returned results, and count towards things like
            max_shots and max_errors.
        progress_callback: Defaults to None (unused). If specified, then each
            time new sample statistics are acquired from a worker this method
            will be invoked with the new `sinter.TaskStats`.
        hint_num_tasks: If `tasks` is an iterator or a generator, its length
            can be given here so that progress printouts can say how many cases
            are left.
        decoders: Defaults to None (specified by each Task). The names of the
            decoders to use on each Task. It must either be the case that each
            Task specifies a decoder and this is set to None, or this is an
            iterable and each Task has its decoder set to None.
        count_observable_error_combos: Defaults to False. When set to to True,
            the returned stats will have a custom counts field with keys
            like `obs_mistake_mask=E_E__` counting how many times specific
            combinations of observables were mispredicted by the decoder.
        count_detection_events: Defaults to False. When set to True, the
            returned stats will have a custom counts field withs the
            key `detection_events` counting the number of times a detector fired
            and also `detectors_checked` counting the number of detectors that
            were executed. The detection fraction is the ratio of these two
            numbers.
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        print_progress: When True, progress is printed to stderr while
            collection runs.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.
        custom_decoders: Named child classes of `sinter.decoder`, that can be
            used if requested by name by a task or by the decoders list.
            If not specified, only decoders with support built into sinter, such
            as 'pymatching' and 'fusion_blossom', can be used.

    Returns:
        A list of sample statistics, one from each problem. The list is not in
        any specific order. This is the same data that would have been written
        to a CSV file, but aggregated so that each problem has exactly one
        sample statistic instead of potentially multiple.

    Examples:
        >>> import sinter
        >>> import stim
        >>> tasks = [
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=5,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 5},
        ...     ),
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=7,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 7},
        ...     ),
        ... ]
        >>> stats = sinter.collect(
        ...     tasks=tasks,
        ...     decoders=['vacuous'],
        ...     num_workers=2,
        ...     max_shots=100,
        ... )
        >>> for stat in sorted(stats, key=lambda e: e.json_metadata['d']):
        ...     print(stat.json_metadata, stat.shots)
        {'d': 5} 100
        {'d': 7} 100
    """
```

<a name="sinter.comma_separated_key_values"></a>
```python
# sinter.comma_separated_key_values

# (at top-level in the sinter module)
def comma_separated_key_values(
    path: str,
) -> Dict[str, Any]:
    """Converts paths like 'folder/d=5,r=3.stim' into dicts like {'d':5,'r':3}.

    On the command line, specifying `--metadata_func auto` results in this
    method being used to extra metadata from the circuit file paths. Integers
    and floats will be parsed into their values, instead of being stored as
    strings.

    Args:
        path: A file path where the name of the file has a series of terms like
            'a=b' separated by commas and ending in '.stim'.

    Returns:
        A dictionary from named keys to parsed values.

    Examples:
        >>> import sinter
        >>> sinter.comma_separated_key_values("folder/d=5,r=3.5,x=abc.stim")
        {'d': 5, 'r': 3.5, 'x': 'abc'}
    """
```

<a name="sinter.fit_binomial"></a>
```python
# sinter.fit_binomial

# (at top-level in the sinter module)
def fit_binomial(
    *,
    num_shots: int,
    num_hits: int,
    max_likelihood_factor: float,
) -> sinter.Fit:
    """Determine hypothesis probabilities compatible with the given hit ratio.

    The result includes the best fit (the max likelihood hypothis) as well as
    the smallest and largest probabilities whose likelihood is within the given
    factor of the maximum likelihood hypothesis.

    Args:
        num_shots: The number of samples that were taken.
        num_hits: The number of hits that were seen in the samples.
        max_likelihood_factor: The maximum Bayes factor between the low/high
            hypotheses and the best hypothesis (the max likelihood hypothesis).
            This value should be larger than 1 (as opposed to between 0 and 1).

    Returns:
        A `sinter.Fit` with the low, best, and high hypothesis probabilities.

    Examples:
        >>> import sinter
        >>> sinter.fit_binomial(
        ...     num_shots=100_000_000,
        ...     num_hits=2,
        ...     max_likelihood_factor=1000,
        ... )
        sinter.Fit(low=2e-10, best=2e-08, high=1.259e-07)
        >>> sinter.fit_binomial(
        ...     num_shots=10,
        ...     num_hits=5,
        ...     max_likelihood_factor=9,
        ... )
        sinter.Fit(low=0.202, best=0.5, high=0.798)
    """
```

<a name="sinter.fit_line_slope"></a>
```python
# sinter.fit_line_slope

# (at top-level in the sinter module)
def fit_line_slope(
    *,
    xs: Sequence[float],
    ys: Sequence[float],
    max_extra_squared_error: float,
) -> sinter.Fit:
    """Performs a line fit of the given points, focusing on the line's slope.

    Finds the slope of the best fit, but also the minimum and maximum slopes
    for line fits whose squared error cost is within the given
    `max_extra_squared_error` cost of the best fit.

    Note that the extra squared error is computed while including a specific
    offset of some specific line. So the low/high estimates are for specific
    lines, not for the general class of lines with a given slope, adding
    together the contributions of all lines in that class.

    Args:
        xs: The x coordinates of points to fit.
        ys: The y coordinates of points to fit.
        max_extra_squared_error: When computing the low and high fits, this is
            the maximum additional squared error that can be introduced by
            varying the slope away from the best fit.

    Returns:
        A sinter.Fit containing the best fit, as well as low and high fits that
        are as far as possible from the best fit while respective the given
        max_extra_squared_error.

    Examples:
        >>> import sinter
        >>> sinter.fit_line_slope(
        ...     xs=[1, 2, 3],
        ...     ys=[10, 12, 14],
        ...     max_extra_squared_error=1,
        ... )
        sinter.Fit(low=1.2928924560546875, best=2.0, high=2.7071075439453125)
    """
```

<a name="sinter.fit_line_y_at_x"></a>
```python
# sinter.fit_line_y_at_x

# (at top-level in the sinter module)
def fit_line_y_at_x(
    *,
    xs: Sequence[float],
    ys: Sequence[float],
    target_x: float,
    max_extra_squared_error: float,
) -> sinter.Fit:
    """Performs a line fit, focusing on the line's y coord at a given x coord.

    Finds the y value at the given x of the best fit, but also the minimum and
    maximum values for y at the given x amongst all possible line fits whose
    squared error cost is within the given `max_extra_squared_error` cost of the
    best fit.

    Args:
        xs: The x coordinates of points to fit.
        ys: The y coordinates of points to fit.
        target_x: The fit values are the value of y at this x coordinate.
        max_extra_squared_error: When computing the low and high fits, this is
            the maximum additional squared error that can be introduced by
            varying the slope away from the best fit.

    Returns:
        A sinter.Fit containing the best fit for y at the given x, as well as
        low and high fits that are as far as possible from the best fit while
        respecting the given max_extra_squared_error.

    Examples:
        >>> import sinter
        >>> sinter.fit_line_y_at_x(
        ...     xs=[1, 2, 3],
        ...     ys=[10, 12, 14],
        ...     target_x=4,
        ...     max_extra_squared_error=1,
        ... )
        sinter.Fit(low=14.47247314453125, best=16.0, high=17.52752685546875)
    """
```

<a name="sinter.group_by"></a>
```python
# sinter.group_by

# (at top-level in the sinter module)
def group_by(
    items: Iterable[~TVal],
    *,
    key: Callable[[~TVal], ~TKey],
) -> Dict[~TKey, List[~TVal]]:
    """Groups items based on whether they produce the same key from a function.

    Args:
        items: The items to group.
        key: Items that produce the same value from this function get grouped together.

    Returns:
        A dictionary mapping outputs that were produced by the grouping function to
        the list of items that produced that output.

    Examples:
        >>> import sinter
        >>> sinter.group_by([1, 2, 3], key=lambda i: i == 2)
        {False: [1, 3], True: [2]}

        >>> sinter.group_by(range(10), key=lambda i: i % 3)
        {0: [0, 3, 6, 9], 1: [1, 4, 7], 2: [2, 5, 8]}
    """
```

<a name="sinter.iter_collect"></a>
```python
# sinter.iter_collect

# (at top-level in the sinter module)
def iter_collect(
    *,
    num_workers: int,
    tasks: Union[Iterator[sinter.Task], Iterable[sinter.Task]],
    hint_num_tasks: Optional[int] = None,
    additional_existing_data: Optional[sinter._existing_data.ExistingData] = None,
    max_shots: Optional[int] = None,
    max_errors: Optional[int] = None,
    decoders: Optional[Iterable[str]] = None,
    max_batch_seconds: Optional[int] = None,
    max_batch_size: Optional[int] = None,
    start_batch_size: Optional[int] = None,
    count_observable_error_combos: bool = False,
    count_detection_events: bool = False,
    custom_decoders: Optional[Dict[str, sinter.Decoder]] = None,
) -> Iterator[sinter.Progress]:
    """Iterates error correction statistics collected from worker processes.

    It is important to iterate until the sequence ends, or worker processes will
    be left alive. The values yielded during iteration are progress updates from
    the workers.

    Note: if max_batch_size and max_batch_seconds are both not used (or
    explicitly set to None), a default batch-size-limiting mechanism will be
    chosen.

    Args:
        num_workers: The number of worker processes to use.
        tasks: Decoding problems to sample.
        hint_num_tasks: If `tasks` is an iterator or a generator, its length
            can be given here so that progress printouts can say how many cases
            are left.
        additional_existing_data: Defaults to None (no additional data).
            Statistical data that has already been collected, in addition to
            anything included in each task's `previous_stats` field.
        decoders: Defaults to None (specified by each Task). The names of the
            decoders to use on each Task. It must either be the case that each
            Task specifies a decoder and this is set to None, or this is an
            iterable and each Task has its decoder set to None.
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        count_observable_error_combos: Defaults to False. When set to to True,
            the returned stats will have a custom counts field with keys
            like `obs_mistake_mask=E_E__` counting how many times specific
            combinations of observables were mispredicted by the decoder.
        count_detection_events: Defaults to False. When set to True, the
            returned stats will have a custom counts field withs the
            key `detection_events` counting the number of times a detector fired
            and also `detectors_checked` counting the number of detectors that
            were executed. The detection fraction is the ratio of these two
            numbers.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.
        custom_decoders: Custom decoders that can be used if requested by name.
            If not specified, only decoders built into sinter, such as
            'pymatching' and 'fusion_blossom', can be used.

    Yields:
        sinter.Progress instances recording incremental statistical data as it
        is collected by workers.

    Examples:
        >>> import sinter
        >>> import stim
        >>> tasks = [
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=5,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 5},
        ...     ),
        ...     sinter.Task(
        ...         circuit=stim.Circuit.generated(
        ...             'repetition_code:memory',
        ...             distance=7,
        ...             rounds=5,
        ...             before_round_data_depolarization=1e-3,
        ...         ),
        ...         json_metadata={'d': 7},
        ...     ),
        ... ]
        >>> iterator = sinter.iter_collect(
        ...     tasks=tasks,
        ...     decoders=['vacuous'],
        ...     num_workers=2,
        ...     max_shots=100,
        ... )
        >>> total_shots = 0
        >>> for progress in iterator:
        ...     for stat in progress.new_stats:
        ...         total_shots += stat.shots
        >>> print(total_shots)
        200
    """
```

<a name="sinter.log_binomial"></a>
```python
# sinter.log_binomial

# (at top-level in the sinter module)
def log_binomial(
    *,
    p: Union[float, np.ndarray],
    n: int,
    hits: int,
) -> np.ndarray:
    """Approximates the natural log of a binomial distribution's probability.

    When working with large binomials, it's often necessary to work in log space
    to represent the result. For example, suppose that out of two million
    samples 200_000 are hits. The maximum likelihood estimate is p=0.2. Even if
    this is the true probability, the chance of seeing *exactly* 20% hits out of
    a million shots is roughly 10^-217322. Whereas the smallest representable
    double is roughly 10^-324. But ln(10^-217322) ~= -500402.4 is representable.

    This method evaluates $\ln(P(hits = B(n, p)))$, with all computations done
    in log space to ensure intermediate values can be represented as floating
    point numbers without underflowing to 0 or overflowing to infinity. This
    method can be broadcast over multiple hypothesis probabilities by giving a
    numpy array for `p` instead of a single float.

    Args:
        p: The hypotehsis probability. The independent probability of a hit
            occurring for each sample. This can also be an array of
            probabilities, in which case the function is broadcast over the
            array.
        n: The number of samples that were taken.
        hits: The number of hits that were observed amongst the samples that
            were taken.

    Returns:
        $\ln(P(hits = B(n, p)))$

    Examples:
        >>> import sinter
        >>> sinter.log_binomial(p=0.5, n=100, hits=50)
        array(-2.5283785, dtype=float32)
        >>> sinter.log_binomial(p=0.2, n=1_000_000, hits=1_000)
        array(-216626.97, dtype=float32)
        >>> sinter.log_binomial(p=0.1, n=1_000_000, hits=1_000)
        array(-99654.86, dtype=float32)
        >>> sinter.log_binomial(p=0.01, n=1_000_000, hits=1_000)
        array(-6742.573, dtype=float32)
        >>> sinter.log_binomial(p=[0.01, 0.1, 0.2], n=1_000_000, hits=1_000)
        array([  -6742.573,  -99654.86 , -216626.97 ], dtype=float32)
    """
```

<a name="sinter.log_factorial"></a>
```python
# sinter.log_factorial

# (at top-level in the sinter module)
def log_factorial(
    n: int,
) -> float:
    """Approximates $\ln(n!)$; the natural logarithm of a factorial.

    Uses Stirling's approximation for large n.

    Args:
        n: The input to the factorial.

    Returns:
        Evaluates $ln(n!)$ using Stirling's approximation.

    Examples:
        >>> import sinter
        >>> sinter.log_factorial(0)
        0.0
        >>> sinter.log_factorial(1)
        0.0
        >>> sinter.log_factorial(2)
        0.6931471805599453
        >>> sinter.log_factorial(100)
        363.7385422250079
    """
```

<a name="sinter.plot_discard_rate"></a>
```python
# sinter.plot_discard_rate

# (at top-level in the sinter module)
def plot_discard_rate(
    *,
    ax: 'plt.Axes',
    stats: 'Iterable[sinter.TaskStats]',
    x_func: Callable[[sinter.TaskStats], Any],
    failure_units_per_shot_func: Callable[[sinter.TaskStats], Any] = lambda _: 1,
    group_func: Callable[[sinter.TaskStats], ~TCurveId] = lambda _: None,
    filter_func: Callable[[sinter.TaskStats], Any] = lambda _: True,
    plot_args_func: Callable[[int, ~TCurveId, List[sinter.TaskStats]], Dict[str, Any]] = lambda index, group_key, group_stats: dict(),
    highlight_max_likelihood_factor: Optional[float] = 1000.0,
) -> None:
    """Plots discard rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        failure_units_per_shot_func: How many discard chances there are per shot. This rescales what the
            discard rate means. By default, it is the discard rate per shot, but this allows
            you to instead make it the discard rate per round. For example, if the metadata
            associated with a shot has a field 'r' which is the number of rounds, then this can be
            achieved with `failure_units_per_shot_func=lambda stats: stats.metadata['r']`.
        group_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `group_func=lambda stat: stat.decoder`.
        filter_func: Optional. When specified, some curves will not be plotted.
            The statistics are filtered and only plotted if filter_func(stat) returns True.
            For example, `filter_func=lambda s: s.json_metadata['basis'] == 'x'` would plot only stats
            where the saved metadata indicates the basis was 'x'.
        plot_args_func: Optional. Specifies additional arguments to give the the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if group_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_max_likelihood_factor: Controls how wide the uncertainty highlight region around curves is.
            Must be 1 or larger. Hypothesis probabilities at most that many times as unlikely as the max likelihood
            hypothesis will be highlighted.
    """
```

<a name="sinter.plot_error_rate"></a>
```python
# sinter.plot_error_rate

# (at top-level in the sinter module)
def plot_error_rate(
    *,
    ax: 'plt.Axes',
    stats: 'Iterable[sinter.TaskStats]',
    x_func: Callable[[sinter.TaskStats], Any],
    failure_units_per_shot_func: Callable[[sinter.TaskStats], Any] = lambda _: 1,
    failure_values_func: Callable[[sinter.TaskStats], Any] = lambda _: 1,
    group_func: Callable[[sinter.TaskStats], ~TCurveId] = lambda _: None,
    filter_func: Callable[[sinter.TaskStats], Any] = lambda _: True,
    plot_args_func: Callable[[int, ~TCurveId, List[sinter.TaskStats]], Dict[str, Any]] = lambda index, group_key, group_stats: dict(),
    highlight_max_likelihood_factor: Optional[float] = 1000.0,
) -> None:
    """Plots error rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        failure_units_per_shot_func: How many error chances there are per shot. This rescales what the
            logical error rate means. By default, it is the logical error rate per shot, but this allows
            you to instead make it the logical error rate per round. For example, if the metadata
            associated with a shot has a field 'r' which is the number of rounds, then this can be
            achieved with `failure_units_per_shot_func=lambda stats: stats.metadata['r']`.
        failure_values_func: How many independent ways there are for a shot to fail, such as
            the number of independent observables in a memory experiment. This affects how the failure
            units rescaling plays out (e.g. with 1 independent failure the "center" of the conversion
            is at 50% whereas for 2 independent failures the "center" is at 75%).
        group_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `group_func=lambda stat: stat.decoder`.
        filter_func: Optional. When specified, some curves will not be plotted.
            The statistics are filtered and only plotted if filter_func(stat) returns True.
            For example, `filter_func=lambda s: s.json_metadata['basis'] == 'x'` would plot only stats
            where the saved metadata indicates the basis was 'x'.
        plot_args_func: Optional. Specifies additional arguments to give the the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if group_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_max_likelihood_factor: Controls how wide the uncertainty highlight region around curves is.
            Must be 1 or larger. Hypothesis probabilities at most that many times as unlikely as the max likelihood
            hypothesis will be highlighted.
    """
```

<a name="sinter.post_selection_mask_from_4th_coord"></a>
```python
# sinter.post_selection_mask_from_4th_coord

# (at top-level in the sinter module)
def post_selection_mask_from_4th_coord(
    dem: Union[stim.Circuit, stim.DetectorErrorModel],
) -> np.ndarray:
    """Returns a mask that postselects detector's with non-zero 4th coordinate.

    This method is a leftover from before the existence of the command line
    argument `--postselected_detectors_predicate`, when
    `--postselect_detectors_with_non_zero_4th_coord` was the only way to do
    post selection of detectors.

    Args:
        dem: The detector error model to pull coordinate data from.

    Returns:
        A bit packed numpy array where detectors with non-zero 4th coordinate
        data have a True bit at their corresponding index.

    Examples:
        >>> import sinter
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     detector(1, 2, 3) D0
        ...     detector(1, 1, 1, 1) D1
        ...     detector(1, 1, 1, 0) D2
        ...     detector(1, 1, 1, 999) D80
        ... ''')
        >>> sinter.post_selection_mask_from_4th_coord(dem)
        array([2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], dtype=uint8)
    """
```

<a name="sinter.predict_discards_bit_packed"></a>
```python
# sinter.predict_discards_bit_packed

# (at top-level in the sinter module)
def predict_discards_bit_packed(
    *,
    dem: stim.DetectorErrorModel,
    dets_bit_packed: np.ndarray,
    postselect_detectors_with_non_zero_4th_coord: bool,
) -> np.ndarray:
    """Determines which shots to discard due to postselected detectors firing.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to
            determine which detectors to postselect as not having fired.
        dets_bit_packed: A uint8 numpy array with shape
            (num_shots, math.ceil(num_dets / 8)). Contains bit packed detection
            event data.
        postselect_detectors_with_non_zero_4th_coord: Determines how
            postselection is done. Currently, this is the only option so it has
            to be set to True. Any detector from the detector error model that
            specifies coordinate data with at least four coordinates where the
            fourth coordinate (coord index 3) is non-zero will be postselected.

    Returns:
        A numpy bool_ array with shape (num_shots,) where False means not discarded and
        True means yes discarded.
    """
```

<a name="sinter.predict_observables"></a>
```python
# sinter.predict_observables

# (at top-level in the sinter module)
def predict_observables(
    *,
    dem: stim.DetectorErrorModel,
    dets: np.ndarray,
    decoder: str,
    bit_pack_result: bool = False,
    custom_decoders: Optional[Dict[str, sinter.Decoder]] = None,
) -> np.ndarray:
    """Predicts which observables were flipped based on detection event data.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to
            determine which detectors to postselect as not having fired.
        dets: The detection event data. Can be bit packed or not bit packed.
            If dtype=np.bool_ then shape=(num_shots, num_detectors)
            If dtype=np.uint8 then shape=(num_shots, math.ceil(num_detectors/8))
        decoder: The decoder to use for decoding, e.g. "pymatching".
        bit_pack_result: Defaults to False. Determines if the result is bit packed
            or not.
        custom_decoders: Custom decoders that can be used if requested by name.
            If not specified, only decoders built into sinter, such as
            'pymatching' and 'fusion_blossom', can be used.

    Returns:
        If bit_packed_result=False (default):
            dtype=np.bool_
            shape=(num_shots, num_observables)
        If bit_packed_result=True:
            dtype=np.uint8
            shape=(num_shots, math.ceil(num_observables / 8))

    Examples:
        >>> import numpy as np
        >>> import sinter
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     error(0.1) D0 L0
        ...     error(0.1) D0 D1
        ...     error(0.1) D1
        ... ''')
        >>> sinter.predict_observables(
        ...     dem=dem,
        ...     dets=np.array([
        ...         [False, False],
        ...         [True, False],
        ...         [False, True],
        ...         [True, True],
        ...     ], dtype=np.bool_),
        ...     decoder='vacuous',  # try replacing with 'pymatching'
        ...     bit_pack_result=False,
        ... )
        array([[False],
               [False],
               [False],
               [False]])
    """
```

<a name="sinter.predict_observables_bit_packed"></a>
```python
# sinter.predict_observables_bit_packed

# (at top-level in the sinter module)
def predict_observables_bit_packed(
    *,
    dem: stim.DetectorErrorModel,
    dets_bit_packed: np.ndarray,
    decoder: str,
    custom_decoders: Optional[Dict[str, sinter.Decoder]] = None,
) -> np.ndarray:
    """Predicts which observables were flipped based on detection event data.

    This method predates `sinter.predict_observables` gaining optional bit
    packing arguments.

    Args:
        dem: The detector error model the detector data applies to.
            This is also where coordinate data is read from, in order to
            determine which detectors to postselect as not having fired.
        dets_bit_packed: A uint8 numpy array with shape
            (num_shots, math.ceil(num_dets / 8)). Contains bit packed detection
            event data.
        decoder: The decoder to use for decoding, e.g. "pymatching".
        custom_decoders: Custom decoders that can be used if requested by name.
            If not specified, only decoders built into sinter, such as
            'pymatching' and 'fusion_blossom', can be used.

    Returns:
        A numpy uint8 array with shape (num_shots, math.ceil(num_obs / 8)).
        Contains bit packed observable prediction data.

    Examples:
        >>> import numpy as np
        >>> import sinter
        >>> import stim
        >>> dem = stim.DetectorErrorModel('''
        ...     error(0.1) D0 L0
        ...     error(0.1) D0 D1
        ...     error(0.1) D1
        ... ''')
        >>> sinter.predict_observables_bit_packed(
        ...     dem=dem,
        ...     dets_bit_packed=np.array([
        ...         [0b00],
        ...         [0b01],
        ...         [0b10],
        ...         [0b11],
        ...     ], dtype=np.uint8),
        ...     decoder='vacuous',  # try replacing with 'pymatching'
        ... )
        array([[0],
               [0],
               [0],
               [0]], dtype=uint8)
    """
```

<a name="sinter.predict_on_disk"></a>
```python
# sinter.predict_on_disk

# (at top-level in the sinter module)
def predict_on_disk(
    *,
    decoder: str,
    dem_path: Union[str, pathlib.Path],
    dets_path: Union[str, pathlib.Path],
    dets_format: str,
    obs_out_path: Union[str, pathlib.Path],
    obs_out_format: str,
    postselect_detectors_with_non_zero_4th_coord: bool = False,
    discards_out_path: Union[str, pathlib.Path, NoneType] = None,
    discards_out_format: Optional[str] = None,
    custom_decoders: Dict[str, sinter.Decoder] = None,
) -> None:
    """Performs decoding and postselection on disk.

    Args:
        decoder: The decoder to use for decoding.
        dem_path: The detector error model to use to configure the decoder.
        dets_path: Where the detection event data is stored on disk.
        dets_format: The format the detection event data is stored in (e.g. '01' or 'b8').
        obs_out_path: Where to write predicted observable flip data on disk.
            Note that the predicted observable flip data will not included data from shots discarded by postselection.
            Use the data in discards_out_path to determine which shots were discarded.
        obs_out_format: The format to write the observable flip data in (e.g. '01' or 'b8').
        postselect_detectors_with_non_zero_4th_coord: Activates postselection. Detectors that have a non-zero 4th
            coordinate will be postselected. Any shot where a postselected detector fires will be discarded.
            Requires specifying discards_out_path, for indicating which shots were discarded.
        discards_out_path: Only used if postselection is being used. Where to write discard data on disk.
        discards_out_format: The format to write discard data in (e.g. '01' or 'b8').
        custom_decoders: Custom decoders that can be used if requested by name.
    """
```

<a name="sinter.read_stats_from_csv_files"></a>
```python
# sinter.read_stats_from_csv_files

# (at top-level in the sinter module)
def read_stats_from_csv_files(
    *paths_or_files: Any,
) -> List[sinter.TaskStats]:
    """Reads and aggregates shot statistics from CSV files.

    Assumes the CSV file was written by printing `sinter.CSV_HEADER` and then
    a list of `sinter.TaskStats`. When statistics from the same task appear
    in multiple files (identified by the strong id being the same), the
    statistics for that task are folded together (so only the total shots,
    total errors, etc for each task are included in the results).

    Args:
        *paths_or_files: Each argument should be either a path (in the form of
            a string or a pathlib.Path) or a TextIO object (e.g. as returned by
            `open`). File data is read from each argument.

    Returns:
        A list of task stats, where each task appears only once in the list and
        the stats associated with it are the totals aggregated from all files.

    Examples:
        >>> import sinter
        >>> import io
        >>> in_memory_file = io.StringIO()
        >>> _ = in_memory_file.write('''
        ...     shots,errors,discards,seconds,decoder,strong_id,json_metadata
        ...     1000,42,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     3000,24,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     1000,250,0,0.125,pymatching,deadbeef08,"{""d"":7}"
        ... '''.strip())
        >>> _ = in_memory_file.seek(0)
        >>> stats = sinter.read_stats_from_csv_files(in_memory_file)
        >>> for stat in stats:
        ...     print(repr(stat))
        sinter.TaskStats(strong_id='9c31908e2b', decoder='pymatching', json_metadata={'d': 9}, shots=4000, errors=66, seconds=0.25)
        sinter.TaskStats(strong_id='deadbeef08', decoder='pymatching', json_metadata={'d': 7}, shots=1000, errors=250, seconds=0.125)
    """
```

<a name="sinter.shot_error_rate_to_piece_error_rate"></a>
```python
# sinter.shot_error_rate_to_piece_error_rate

# (at top-level in the sinter module)
def shot_error_rate_to_piece_error_rate(
    shot_error_rate: float,
    *,
    pieces: float,
    values: float = 1,
) -> float:
    """Convert from total error rate to per-piece error rate.

    Args:
        shot_error_rate: The rate at which shots fail.
        pieces: The number of xor-pieces we want to subdivide each shot into,
            as if each piece was an independent chance for the shot to fail and
            the total chance of a shot failing was the xor of each piece
            failing.
        values: The number of or-pieces each shot's failure is being formed out
            of.

    Returns:
        Let N = `pieces` (number of rounds)
        Let V = `values` (number of observables)
        Let S = `shot_error_rate`
        Let R = the returned result

        R satisfies the following property. Let X be the probability of each
        observable flipping, each round. R will be the probability that any of
        the observables is flipped after 1 round, given this X. X is chosen to
        satisfy the following condition. If a Bernoulli distribution with
        probability X is sampled V*N times, and the results grouped into V
        groups of N, and each group is reduced to a single value using XOR, and
        then the reduced group values are reduced to a single final value using
        OR, then this final value will be True with probability S.

        Or, in other words, if a shot consists of N rounds which V independent
        observables must survive, then R is like the per-round failure for
        any of the observables.

    Examples:
        >>> import sinter
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.1,
        ...     pieces=2,
        ... )
        0.05278640450004207
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.05278640450004207,
        ...     pieces=1 / 2,
        ... )
        0.10000000000000003
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=1e-9,
        ...     pieces=100,
        ... )
        1.000000082740371e-11
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.6,
        ...     pieces=10,
        ...     values=2,
        ... )
        0.12052311142021144
    """
```

<a name="sinter.stats_from_csv_files"></a>
```python
# sinter.stats_from_csv_files

# (at top-level in the sinter module)
def stats_from_csv_files(
    *paths_or_files: Any,
) -> List[sinter.TaskStats]:
    """Reads and aggregates shot statistics from CSV files.

    (An old alias of `read_stats_from_csv_files`, kept around for backwards
    compatibility.)

    Assumes the CSV file was written by printing `sinter.CSV_HEADER` and then
    a list of `sinter.TaskStats`. When statistics from the same task appear
    in multiple files (identified by the strong id being the same), the
    statistics for that task are folded together (so only the total shots,
    total errors, etc for each task are included in the results).

    Args:
        *paths_or_files: Each argument should be either a path (in the form of
            a string or a pathlib.Path) or a TextIO object (e.g. as returned by
            `open`). File data is read from each argument.

    Returns:
        A list of task stats, where each task appears only once in the list and
        the stats associated with it are the totals aggregated from all files.

    Examples:
        >>> import sinter
        >>> import io
        >>> in_memory_file = io.StringIO()
        >>> _ = in_memory_file.write('''
        ...     shots,errors,discards,seconds,decoder,strong_id,json_metadata
        ...     1000,42,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     3000,24,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     1000,250,0,0.125,pymatching,deadbeef08,"{""d"":7}"
        ... '''.strip())
        >>> _ = in_memory_file.seek(0)
        >>> stats = sinter.stats_from_csv_files(in_memory_file)
        >>> for stat in stats:
        ...     print(repr(stat))
        sinter.TaskStats(strong_id='9c31908e2b', decoder='pymatching', json_metadata={'d': 9}, shots=4000, errors=66, seconds=0.25)
        sinter.TaskStats(strong_id='deadbeef08', decoder='pymatching', json_metadata={'d': 7}, shots=1000, errors=250, seconds=0.125)
    """
```
