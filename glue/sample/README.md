# simmer: stim sampling helper

Simmer is a bit of glue code that allows using Stim and a decoder in tandem
in order to benchmark quantum error correction circuits using Monte Carlo sampling.
Simmer supports using pymatching to decode the samples, and can use python
multiprocessing to fully utilize a computer's resources to get good performance.

**simmer is still in development. Its API and output formats are not stable.** 

# How to Install

Simmer is currently in development, so it can only be installed from source.
For example:

```
git clone git@github.com:quantumlib/stim.git
pip install -e stim/glue/sample
```

# How to Use: Python API

This example assumes you are in a python environment with stim and simmer
installed.

```bash
import stim
import simmer


# Generates surface code circuit tasks using Stim's circuit generation.
def generate_example_tasks():
    for p in [0.001, 0.005, 0.01]:
        for d in [3, 5]:
            yield simmer.Task(
                circuit=stim.Circuit.generated(
                    rounds=d,
                    distance=d,
                    after_clifford_depolarization=p,
                    code_task=f'surface_code:rotated_memory_x',
                ),
                json_metadata={
                    'p': p,
                    'd': d,
                },
            )


def main():
    # Collect the samples (takes a few minutes).
    samples = simmer.collect(
        num_workers=4,
        max_shots=1_000_000,
        max_errors=1000,
        tasks=generate_example_tasks(),
        decoders=['pymatching'],
    )

    # Print as CSV data.
    print(simmer.CSV_HEADER)
    for sample in samples:
        print(sample.to_csv_line())

    # Render a matplotlib plot of the data into a png image.
    fig, axs = simmer.plot(
        samples=samples,
        x_func=lambda e: e.json_metadata['p'],
        xaxis='[log]Physical Error Rate',
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
    )
    fig.savefig('plot.png')


# NOTE: This is actually necessary! If the code inside 'main()' was at the
# module level, the multiprocessing children spawned by simmer.collect would
# also attempt to run that code.
if __name__ == '__main__':
    main()
```

Example output to stdout:

```
     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
   1000000,       837,         0,    36.6,pymatching,9f7e20c54fec45b6aef7491b774dd5c0a3b9a005aa82faf5b9c051d6e40d60a9,"{""d"":3,""p"":0.001}"
     53498,      1099,         0,    6.52,pymatching,3f40432443a99b933fb548b831fb54e7e245d9d73a35c03ea5a2fb2ce270f8c8,"{""d"":3,""p"":0.005}"
     16269,      1023,         0,    3.23,pymatching,17b2e0c99560d20307204494ac50e31b33e50721b4ebae99d9e3577ae7248874,"{""d"":3,""p"":0.01}"
   1000000,       151,         0,    77.3,pymatching,e179a18739201250371ffaae0197d8fa19d26b58dfc2942f9f1c85568645387a,"{""d"":5,""p"":0.001}"
     11363,      1068,         0,    12.5,pymatching,a4dec28934a033215ff1389651a26114ecc22016a6e122008830cf7dd04ba5ad,"{""d"":5,""p"":0.01}"
     61569,      1001,         0,    24.5,pymatching,2fefcc356752482fb4c6d912c228f6d18762f5752796c668b6abeb7775f5de92,"{""d"":5,""p"":0.005}"
```

and the corresponding image saved to `plot.png`:

![Example plot](readme_example_plot.png)

# How to Use: Linux Command Line

This example assumes you are using a linux command line in a python virtualenv with `simmer` installed.

## pick circuits

For this example, we will use Stim's circuit generation functionality to produce
circuits to benchmark.
We will make rotated surface code circuits with various physical error rates,
with filenames like `rotated_d5_p0.001_surface_code.stim`.

```bash
mkdir -p circuits
python -c "

import stim

for p in [0.001, 0.005, 0.01]:
    for d in [3, 5]:
        with open(f'circuits/rotated_d{d}_p{p}_surface_code.stim', 'w') as f:
            c = stim.Circuit.generated(
                rounds=d,
                distance=d,
                after_clifford_depolarization=p,
                after_reset_flip_probability=p,
                before_measure_flip_probability=p,
                before_round_data_depolarization=p,
                code_task=f'surface_code:rotated_memory_x')
            print(c, file=f)

"
```

Normally, making the circuit files is the hardest step, because they are what
specifies the problem you are sampling from.
Almost all of the work you do will generally involve creating the exact perfect
circuit file for your needs.
But this is just an example, so we'll use normal surface code circuits.

# collect

You can use simmer to collect statistics on each circuit by using the `simmer collect` command.
This command takes options specifying how much data to collect, how to do decoding, etc.

By default, simmer writes the collected statistics to stdout as CSV data.
One particularly important option that changes this behavior is `-save_resume_filepath`,
which allows the command to be interrupted and restarted without losing data.
Any data already at the file specified by `-save_resume_filepath` will count towards the
amount of statistics asked to be collected, and simmer will append new statistics to this file
instead of overwriting it.

```bash
simmer collect \
    -processes 4 \
    -circuits circuits/*.stim \
    -metadata_func "(v := path.split('/')[-1].split('_')) and {
        'd': int(v[1][1:]),
        'p': float(v[2][1:])
    }" \
    -decoders pymatching \
    -max_shots 1_000_000 \
    -max_errors 1000 \
    -save_resume_filepath stats.csv
```

Beware that if you SIGKILL or SIGTEM simmer, instead of just using SIGINT, it's possible
(though unlikely) that you are killing it just as it writes a row of CSV data. This truncates
the data, which requires manual intervention on your part to fix (e.g. by deleting the partial row
using a text editor).

# combine

Note that the CSV data written by simmer will contain multiple rows for each case, because
simmer starts by running small batches to see roughly what the error rate is before moving
to larger batch sizes. 

You can get a single-row-per-case CSV file by using `simmer combine`:

```bash
simmer combine stats.csv
```

```
     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
   1000000,       837,         0,    36.6,pymatching,9f7e20c54fec45b6aef7491b774dd5c0a3b9a005aa82faf5b9c051d6e40d60a9,"{""d"":3,""p"":0.001}"
     53498,      1099,         0,    6.52,pymatching,3f40432443a99b933fb548b831fb54e7e245d9d73a35c03ea5a2fb2ce270f8c8,"{""d"":3,""p"":0.005}"
     16269,      1023,         0,    3.23,pymatching,17b2e0c99560d20307204494ac50e31b33e50721b4ebae99d9e3577ae7248874,"{""d"":3,""p"":0.01}"
   1000000,       151,         0,    77.3,pymatching,e179a18739201250371ffaae0197d8fa19d26b58dfc2942f9f1c85568645387a,"{""d"":5,""p"":0.001}"
     11363,      1068,         0,    12.5,pymatching,a4dec28934a033215ff1389651a26114ecc22016a6e122008830cf7dd04ba5ad,"{""d"":5,""p"":0.01}"
     61569,      1001,         0,    24.5,pymatching,2fefcc356752482fb4c6d912c228f6d18762f5752796c668b6abeb7775f5de92,"{""d"":5,""p"":0.005}"
```

# plot

You can use `simmer plot` to view the results you've collected.
This command takes a CSV file, and also some command indicating how to group each case
into single curves and also what the desired X coordinate of a case is.
This is done in a flexible but very hacky way, by specifying a python expression using the case's filename: 

```bash
simmer plot \
    -in stats.csv \
    -group_func "'Rotated Surface Code d=' + str(metadata['d'])" \
    -x_func "metadata['p']" \
    -fig_size 1024 1024 \
    -xaxis "[log]Physical Error Rate" \
    -out surface_code_figure.png \
    -show
```

Which will save a png image of, and also open a window showing, a plot like this one:

![Example plot](readme_example_plot.png)
