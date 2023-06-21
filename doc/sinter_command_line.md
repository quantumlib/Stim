# Sinter command line reference

## Index

- [sinter collect](#collect)
- [sinter combine](#combine)
- [sinter plot](#plot)

## Commands

<a name="collect"></a>
### sinter collect

```
NAME
    sinter collect

SYNOPSIS
    sinter collect \
        --circuits FILEPATH [...] \
        --decoders pymatching|fusion_blossom|...  [...] \
        --processes int \
        [--max_shots int] \
        [--max_errors int] \
        [--save_resume_filepath FILEPATH] \
        [--metadata_func auto|PYTHON_EXPRESSION] \
        \
        [--also_print_results_to_stdout] \
        [--custom_decoders_module_function PYTHON_EXPRESSION] \
        [--existing_data_filepaths FILEPATH [...]] \
        [--max_batch_size int] \
        [--max_batch_seconds int] \
        [--postselected_detectors_predicate PYTHON_EXPRESSION] \
        [--postselected_observables_predicate PYTHON_EXPRESSION] \
        [--quiet] \
        [--count_detection_events] \
        [--count_observable_error_combos] \
        [--start_batch_size int] \
        [--custom_error_count_key NAME] \
        [--allowed_cpu_affinity_ids PYTHON_EXPRESSION [ANOTHER_PYTHON_EXPRESSION ...]]

DESCRIPTION
    Uses python multiprocessing to collect shots from the given circuit, decode
    them using the given decoders, and report CSV statistics on error rates.

OPTIONS
    --circuits FILEPATH [ANOTHER_FILEPATH ...]
        Circuit files to sample from and decode. This parameter can be given
        multiple arguments.
    --decoders NAME [ANOTHER_NAME ...]
        The decoder to use to predict observables from detection events.
    --custom_decoders_module_function MODULE_NAME:FUNCTION_NAME
        Use the syntax "module:function" to "import function from module" and
        use the result of "function()" as the custom_decoders dictionary. The
        dictionary must map strings to stim.Decoder instances.
    --max_shots INT
        Sampling of a circuit will stop if this many shots have been taken.
    --max_errors INT
        Sampling of a circuit will stop if this many errors have been seen.
    --processes INT
        Number of processes to use for simultaneous sampling and decoding.
    --save_resume_filepath FILEPATH
        Activates MERGE mode. If save_resume_filepath doesn't exist, initializes
        it with a CSV header. CSV data already at save_resume_filepath counts
        towards max_shots and max_errors.
        Collected data is appended to save_resume_filepath. Note that MERGE mode
        is tolerant to failures: if the process is killed, it can simply be
        restarted and it will pick up where it left off. Note that MERGE mode is
        idempotent: if sufficient data has been collected, no additional work is
        done when run again.
    --start_batch_size INT
        Initial number of samples to batch together into one job. Starting small
        prevents over-sampling of circuits above threshold. The allowed batch
        size increases exponentially from this starting point.
    --max_batch_size INT
        Maximum number of samples to batch together into one job. Bigger values
        increase the delay between jobs finishing. Smaller values decrease the
        amount of aggregation of results, increasing the amount of output
        information.
    --max_batch_seconds INT
        Limits number of shots in a batch so that the estimated runtime of the
        batch is below this amount.
    --postselect_detectors_with_non_zero_4th_coord
        Turns on detector postselection. If any detector with a non-zero 4th
        coordinate fires, the shot is discarded.
    --postselected_detectors_predicate PYTHON_EXPRESSION
        Specifies a predicate used to decide which detectors to postselect. When
        a postselected detector produces a detection event, the shot is
        discarded instead of being given to the decoder. The number of discarded
        shots is tracked as a statistic.
        
        Available values:
            index: The unique number identifying the detector, determined by the
                order of detectors in the circuit file.
            coords: The coordinate data associated with the detector. An empty
                tuple, if the circuit file did not specify detector coordinates.
            metadata: The metadata associated with the task being sampled.

        Expected expression type: Something that can be given to `bool` to get
            False (do not postselect) or True (yes postselect).

        Examples:
            --postselected_detectors_predicate "coords[2] == 0"
            --postselected_detectors_predicate "coords[3] < metadata['postselection_level']"
    --postselected_observables_predicate PYTHON_EXPRESSION
        Specifies a predicate used to decide which observables to postselect.
        When a decoder mispredicts a postselected observable, the shot is
        discarded instead of counting as an error.
        
        Available values:
            index: The index of the observable to postselect or not.
            metadata: The metadata associated with the task.

        Expected expression type:
            Something that can be given to `bool` to get False (do not
            postselect) or True (yes postselect).

        Examples:
            --postselected_observables_predicate "False"
            --postselected_observables_predicate "metadata['d'] == 5 and index >= 2"
    --count_observable_error_combos
        When set, the returned stats will include custom counts like
        `obs_mistake_mask=E_E__` counting how many times the decoder made each
        pattern of observable mistakes.
    --count_detection_events
        When set, the returned stats will include custom counts
        `detectors_checked` and `detection_events`. The detection fraction is
        the ratio of these two numbers.
    --quiet
        Disables writing progress to stderr.
    --also_print_results_to_stdout
        Even if writing to a file, also write results to stdout.
    --existing_data_filepaths [FILEPATH ...]
        CSV data from these files counts towards max_shots and max_errors. This
        parameter can be given multiple arguments.
    --metadata_func PYTHON_EXPRESSION
        A python expression that associates json metadata with a circuit's
        results. Set to "auto" to use "sinter.comma_separated_key_values(path)".
        
        Values available to the expression:
            path: Relative path to the circuit file, from the command line
                arguments.
            circuit: The circuit itself, parsed from the file, as a
                stim.Circuit.
        
        Expected type:
            A value that can be serialized into JSON, like a Dict[str, int].

        Note that the decoder field is already recorded separately, so storing
        it in the metadata as well would be redundant. But something like
        decoder version could be usefully added.
        
        Examples:
            --metadata_func "{'path': path}"
            --metadata_func "auto"
            --metadata_func "{'n': circuit.num_qubits, 'p': float(path.split('/')[-1].split('.')[0])}"
    --custom_error_count_key NAME
        Makes `--max_errors` apply to `stat.custom_counts[key]` instead of to `stat.errors`.
    --allowed_cpu_affinity_ids PYTHON_EXPRESSION [ANOTHER_PYTHON_EXPRESSION ...],
        Controls which CPUs workers can be pinned to. By default, any CPU can be pinned to.
        Specifying this argument makes it so that only the given CPU ids can be pinned. The
        given arguments will be evaluated as python expressions. The expressions 
        should be integers or iterables of integers. So values like "1" and "[1, 2, 4]" and
        "range(5, 30)" all work.

EXAMPLES
    Example #1
        >>> stim gen --out "d=5,r=5,p=0.01.stim" --code repetition_code --task memory --distance 5 --rounds 5 --before_round_data_depolarization 0.01
        >>> stim gen --out "d=7,r=5,p=0.01.stim" --code repetition_code --task memory --distance 7 --rounds 5 --before_round_data_depolarization 0.01
        >>> sinter collect \
                --processes 4 \
                --circuits *.stim \
                --metadata_func auto \
                --decoders pymatching \
                --max_shots 1_000_000 \
                --max_errors 1_000 \
                --save_resume_filepath stats.csv
        >>> sinter combine stats.csv
             shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
           1000000,        12,         0,   0.716,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
           1000000,         1,         0,   0.394,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
```

<a name="combine"></a>
### sinter combine

```
NAME
    sinter combine

SYNOPSIS
    sinter combine \
        FILEPATH [...] \
        \
        [--order preserve|metadata|error] \
        [--strip_custom_counts]

DESCRIPTION
    Loads sample statistics from one or more CSV statistics files (produced by
    `sinter collect`), and aggregates rows corresponding to the same circuit
    together.

OPTIONS
    filepaths
        The locations of statistics files with rows aggregate together.
    --order
        Decides how to sort the output data.
        The options are:
            metadata (default): Orders by the json_metadata column.
            preserve: Keeps the same order as the input.
            error: Orders by the error rate.
    --strip_custom_counts
        Removes custom_counts data from the output. 


EXAMPLES
    Example #1
        >>> sinter combine stats.csv
             shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
           1000000,        12,         0,   0.716,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
           1000000,         1,         0,   0.394,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
        >>> cat stats.csv
             shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
               100,         0,         0,   0.174,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
               100,         0,         0,   0.000,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
               400,         0,         0,   0.000,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
              1200,         0,         0,   0.000,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
               100,         0,         0,   0.178,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
              3600,         0,         0,   0.001,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
              3600,         0,         0,   0.001,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
             10800,         0,         0,   0.002,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
             18000,         0,         0,   0.003,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
             39600,         0,         0,   0.007,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
             75600,         0,         0,   0.012,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
            154800,         2,         0,   0.028,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
            306000,         5,         0,   0.051,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
               200,         0,         0,   0.000,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
               600,         0,         0,   0.001,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
              1800,         0,         0,   0.001,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
              5400,         0,         0,   0.002,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
             16200,         0,         0,   0.004,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
             48600,         1,         0,   0.010,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
            385800,         5,         0,   0.071,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
            145800,         0,         0,   0.030,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
            145800,         0,         0,   0.032,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
               200,         0,         0,   0.178,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
               200,         0,         0,   0.188,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
            198100,         0,         0,   0.044,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
            437400,         0,         0,   0.092,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
```

<a name="plot"></a>
### sinter plot

```
NAME
    sinter plot

SYNOPSIS
    sinter plot \
        --in FILEPATH [...] \
        [--x_func PYTHON_EXPRESSION] \
        [--group_func PYTHON_EXPRESSION] \
        [--out FILEPATH] \
        [--show] \
        \
        [--filter_func PYTHON_EXPRESSION] \
        [--y_func PYTHON_EXPRESSION] \
        [--failure_unit_name string] \
        [--failure_units_per_shot_func PYTHON_EXPRESSION] \
        [--failure_values_func PYTHON_EXPRESSION] \
        [--fig_size float float] \
        [--highlight_max_likelihood_factor float] \
        [--plot_args_func PYTHON_EXPRESSION] \
        [--custom_error_count_keys] \
        [--subtitle "{common}"|text] \
        [--title text] \
        [--type "error_rate"|"discard_rate"|"custom_y" [...] \
        [--xaxis "text"|"[log]text"|"[sqrt]text"] \
        [--yaxis "text"|"[log]text"|"[sqrt]text"] \
        [--ymin float]

DESCRIPTION
    Creates a plot of statistics collected by `sinter collect` by grouping data
    into curves according to `--group_func`, and laying out points along that
    curve according to `--x_func`. Plots error rates by default, and also
    discard rates if there are any discards, but this can be customized by using
    `--y_func`.

OPTIONS
    --filter_func PYTHON_EXPRESSION
        A python expression that determines whether a case is kept or not.
        
        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.
            
        Expected expression type: Something that can be given to `bool` to get
            True or False.
            
        Examples:
            --filter_func "decoder=='pymatching'"
            --filter_func "0.001 < metadata['p'] < 0.005"
    --x_func PYTHON_EXPRESSION
        A python expression that determines where points go on the x axis.
        
        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.

        Expected expression type: Something that can be given to `float` to get
            a float.

        Examples:
            --x_func "metadata['p']"
            --x_func m.p
            --x_func "metadata['path'].split('/')[-1].split('.')[0]"
    --y_func PYTHON_EXPRESSION
        A python expression that determines where points go on the y axis. This
        argument is not used by error rate or discard rate plots; only by the
        "custom_y" type plot.

        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.

        Expected expression type: Something that can be given to `float` to get
            a float.

        Examples:
            --y_func "metadata['p']"
            --y_func m.p
            --y_func "metadata['path'].split('/')[-1].split('.')[0]"
    --fig_size WIDTH HEIGHT
        Desired figure width and height in pixels.
    --group_func PYTHON_EXPRESSION
        A python expression that determines how points are grouped into curves.
        
        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.

        Expected expression type:
            Something that can be given to `str` to get a useful string.
            
        Examples:
            --group_func "(decoder, metadata['d'])"
            --group_func m.d
            --group_func "metadata['path'].split('/')[-2]"
    --failure_unit_name FAILURE_UNIT_NAME
        The unit of failure, typically either "shot" (the default) or "round".
        If this argument is specified, --failure_units_per_shot_func must also
        be specified.
    --failure_units_per_shot_func PYTHON_EXPRESSION
        A python expression that evaluates to the number of failure units there
        are per shot. For example, if the failure unit is rounds, this should be
        an expression that returns the number of rounds in a shot. Sinter has no
        way of knowing what you consider a round to be, otherwise. This value is
        used to rescale the logical error rate plots. For example, if there are 4
        failure units per shot then a shot error rate of 10% corresponds to a
        unit failure rate of 2.7129%. The conversion formula (assuming less than
        50% error rates) is:
        
            P_unit = 0.5 - 0.5 * (1 - 2 * P_shot)**(1/units_per_shot)

        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.
        
        Expected expression type: float.
        
        Examples:
            --failure_units_per_shot_func "metadata['rounds']"
            --failure_units_per_shot_func m.r
            --failure_units_per_shot_func "m.distance * 3"
            --failure_units_per_shot_func "10"
    --failure_values_func FAILURE_VALUES_FUNC
        A python expression that evaluates to the number of independent ways a
        shot can fail. For example, if a shot corresponds to a memory experiment
        preserving two observables, then the failure unions is 2. This value is
        necessary to correctly rescale the logical error rate plots when using
        --failure_values_func. By default it is assumed to be 1.
        
        Values available to the python expression:
            metadata: The parsed value from the json_metadata for the data
                point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: The decoder that decoded the data for the data point.
            strong_id: The cryptographic hash of the case that was sampled for
                the data point.
            stat: The sinter.TaskStats object for the data point.

        Expected expression type: float.
        
        Examples:
            --failure_values_func "metadata['num_obs']"
            --failure_values_func "2"
    --plot_args_func PLOT_ARGS_FUNC
        A python expression used to customize the look of curves.
        
        Values available to the python expression:
            index: A unique integer identifying the curve.
            key: The group key (returned from --group_func) identifying the curve.
            stats: The list of sinter.TaskStats object in the group.
            metadata: (From one arbitrary data point in the group.) The parsed value from the json_metadata for the data point.
            m: `m.key` is a shorthand for `metadata.get("key", None)`.
            decoder: (From one arbitrary data point in the group.) The decoder that decoded the data for the data point.
            strong_id: (From one arbitrary data point in the group.) The cryptographic hash of the case that was sampled for the data point.
            stat: (From one arbitrary data point in the group.) The sinter.TaskStats object for the data point.

        Expected expression type:
            A dictionary to give to matplotlib plotting functions as a **kwargs argument.

        Examples:
            --plot_args_func "{'label': 'curve #' + str(index), 'linewidth': 5}"
            --plot_args_func "{'marker': 'ov*sp^<>8PhH+xXDd|'[index % 18]}"
    --in IN [IN ...]      Input files to get data from.
    --type {error_rate,discard_rate,custom_y} [{error_rate,discard_rate,custom_y} ...]
        Picks the figures to include.
    --out OUT
        Output file to write the plot to.
        The file extension determines the type of image.
        Either this or --show must be specified.
    --xaxis XAXIS
        Customize the X axis label.
        Prefix [log] for logarithmic scale.
        Prefix [sqrt] for square root scale.
    --yaxis YAXIS
        Customize the Y axis label.
        Prefix [log] for logarithmic scale.
        Prefix [sqrt] for square root scale.
    --split_custom_counts
        When a stat has custom counts, this splits it into multiple copies of
        the stat with each one having exactly one of the custom counts.
    --show
        Displays the plot in a window. Either this or --out must be specified.
    --ymin YMIN
        Sets the minimum value of the y axis (max always 1).
    --title TITLE
        Sets the title of the plot.
    --subtitle SUBTITLE
        Sets the subtitle of the plot. Note: The pattern "{common}" will expand
        to text including all json metadata values that are the same across all
        stats.
    --highlight_max_likelihood_factor HIGHLIGHT_MAX_LIKELIHOOD_FACTOR
        The relative likelihood ratio that determines the color highlights
        around curves. Set this to 1 or larger. Set to 1 to disable
        highlighting.


EXAMPLES
    Example #1
        >>> cat stats.csv
             shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
           1000000,        12,         0,   0.716,pymatching,41fe89ff6c51e598d51846cb7a2b626fbfcaa76adcd1be9e0f1e2dff1fe87f1c,"{""d"":5,""p"":0.01,""r"":5}"
           1000000,         1,         0,   0.394,pymatching,639d47a421d2a7661bb5b19255295767fc7cf0be7592fe4bcbc2639068e21349,"{""d"":7,""p"":0.01,""r"":5}"
        >>> sinter plot \
            --in stats.csv \
            --show \
            --x_func m.d \
            --group_func "f'''rounds={m.r}'''" \
            --xaxis "[log]distance"
```
