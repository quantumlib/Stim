# simmer: stim sampling helper

Simmer is a bit of glue code that allows using Stim and a decoder in tandem
in order to benchmark quantum error correction circuits using Monte Carlo sampling.
Simmer supports using pymatching to decode the samples, and can use python
multiprocessing to fully utilize a computer's resources to get good performance.

**simmer is still in development. Its API and output formats are not stable.** 

# Usage: Command Line Example

This example assumes you are using a linux command line in a python virtualenv with `simmer` installed.

## circuits

For this example, we will use Stim's circuit generation functionality to produce circuits to benchmark.
We will make rotated and unrotated surface code circuits with various physical error rates, with filenames like
`rotated_d5_p0.001.stim`.

```bash
mkdir -p surface_code_circuits
python -c "

import stim

for p in [0.001, 0.005, 0.01]:
    for d in [3, 5]:
        for rot in ['rotated', 'unrotated']:
            with open(f'surface_code_circuits/{rot}_d{d}_p{p}.stim', 'w') as f:
                c = stim.Circuit.generated(
                    rounds=d,
                    distance=d,
                    after_clifford_depolarization=p,
                    code_task=f'surface_code:{rot}_memory_x')
                print(c, file=f)

"
```

In practice you'd have some other source of the circuits you wanted to benchmark,
such as custom python code you write to produce circuits.

# collection

You can use simmer to collect statistics on each circuit by using the `simmer collect` command.
This command takes options specifying how much data to collect, how to do decoding, etc.

By default, simmer writes the collected statistics to stdout as CSV data.
One particularly important option that changes this behavior is `-merge_data_location`,
which allows the command to be interrupted and restarted without losing data.
Any data already at the file specified by `-merge_data_location` will count towards the
amount of statistics asked to be collected, and simmer will append new statistics to this file
instead of overwriting it.

```bash
simmer collect \
    -processes 4 \
    -circuits surface_code_circuits/*.stim \
    -decoders pymatching \
    -max_shots 1_000_000 \
    -max_errors 1000 \
    -merge_data_location surface_code_stats.csv
```

Beware that if you SIGKILL or SIGTEM simmer, instead of just using SIGINT, it's possible
(though unlikely) that you are killing it just as it writes a row of CSV data. This truncates
the data, which requires manual intervention on your part to fix (e.g. by deleting the partial row
using a text editor).

# data

Note that the CSV data written by simmer will contain multiple rows for each case, because
simmer starts by running small batches to see roughly what the error rate is before moving
to larger batch sizes. 

You can get a single-row-per-case CSV file by using `simmer combine`:

```bash
simmer combine surface_code_stats.csv
```

```
     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
   1000000,       262,         0,    16.3,pymatching,f01233f0d6edafc76d03e8236995bb78ed09b48e2e5ebbf43ffbf02793d8e9d7,"{""d"":3,""p"":0.001,""rot"":""rotated""}"
   1000000,       239,         0,    17.9,pymatching,22952528d7ff664b2fe27c5cc3d338511e27eb95f224228bc860b67aef27dd58,"{""d"":3,""p"":0.001,""rot"":""unrotated""}"
   1000000,        51,         0,    34.0,pymatching,3355b9bf8560b45b8f7eac03398cb6e008e16de19fba2bd5b8b1a6f9c4332067,"{""d"":5,""p"":0.001,""rot"":""rotated""}"
   1000000,        13,         0,    65.8,pymatching,5fc9fa28cc477c212ab0cc654d1625a0573884e9691bb52179cb3d8f1446ce21,"{""d"":5,""p"":0.001,""rot"":""unrotated""}"
    166658,      1037,         0,    3.31,pymatching,a2fadab97195f83df0a505477885f53cfb64620b06501a849fb47617d1c42ee1,"{""d"":3,""p"":0.005,""rot"":""rotated""}"
    194010,      1100,         0,    5.62,pymatching,31f9a15a9cfa7cde5dc014ba3a352a2487412cfa1a9cc0a1d1ad5eaf784fcba1,"{""d"":3,""p"":0.005,""rot"":""unrotated""}"
    207350,      1001,         0,    23.2,pymatching,d7392e6975e4a79ba13e4dd0eeaf058351c3a57704c813bf6f8f40774bcb3d93,"{""d"":5,""p"":0.005,""rot"":""rotated""}"
    388484,      1032,         0,   113.7,pymatching,220ef6450c218c2c86e699253af5a7ec65a048ec4f2e2a3e63ecbd2fe84bfdeb,"{""d"":5,""p"":0.005,""rot"":""unrotated""}"
     43816,      1014,         0,    1.34,pymatching,142926adda22610adf785e42c7a2fe7b29df607687c0b3d231f3a856bc406369,"{""d"":3,""p"":0.01,""rot"":""rotated""}"
     47255,      1041,         0,    1.86,pymatching,2007687e756a06f41e62c82aed46276c98be16371948a07e8852f59102244273,"{""d"":3,""p"":0.01,""rot"":""unrotated""}"
     32150,      1012,         0,    7.14,pymatching,22ee39f253d37897894334649453037e1107cf2301e600b0f91be07555b2d81b,"{""d"":5,""p"":0.01,""rot"":""rotated""}"
     52068,      1182,         0,    31.5,pymatching,bfebe2cdf47cf94aee1422283efac3d47071729ccf254a092389933b594d24b7,"{""d"":5,""p"":0.01,""rot"":""unrotated""}"
```

# display

You can use `simmer plot` to view the results you've collected.
This command takes a CSV file, and also some command indicating how to group each case
into single curves and also what the desired X coordinate of a case is.
This is done in a flexible but very hacky way, by specifying a python expression using the case's filename: 

```bash
simmer plot \
    -in surface_code_stats.csv \
    -group_func "' '.join(custom['path'].split('/')[-1].split('_')[:2])" \
    -x_func "'0.' + custom['path'].split('/')[-1].split('_')[-1].split('.')[1]" \
    -fig_size 1024 1024 \
    -xaxis "[log]Physical Error Rate" \
    -out surface_code_figure.png \
    -show
```

Which will save a png image of, and also open a window showing, a plot like this one:

![Example plot](readme_example_plot.png)


# Usage: Python API Example

This example assumes you are in a python environment with stim and simmer
installed.

```bash
import stim
import simmer


# Generates surface code circuit tasks using Stim's circuit generation.
def generate_tasks():
    for p in [0.001, 0.005, 0.01]:
        for d in [3, 5]:
            for rot in ['rotated', 'unrotated']:
                yield simmer.Task(
                    # What to sample from.
                    circuit=stim.Circuit.generated(
                        rounds=d,
                        distance=d,
                        after_clifford_depolarization=p,
                        code_task=f'surface_code:{rot}_memory_x',
                    ),
                    decoder='pymatching',

                    # Helpful attached data to include in output.
                    json_metadata={
                        'p': p,
                        'd': d,
                        'rot': rot,
                    },

                    # How much work to do.
                    max_shots=1_000_000,
                    max_errors=1000,
                )


def main():
    # Collect the samples (takes a few minutes).
    samples = simmer.collect(num_workers=4, tasks=generate_tasks())

    # Print as CSV data.
    print(simmer.CSV_HEADER)
    for sample in samples:
        print(sample)

    # Render a matplotlib plot of the data into a png image.
    fig, axs = simmer.plot(
        samples=samples,
        x_func=lambda e: e.json_metadata['p'],
        xaxis='[log]Physical Error Rate',
        group_func=lambda e: f"{e.json_metadata['rot']} d{e.json_metadata['d']}",
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
   1000000,       262,         0,    16.3,pymatching,f01233f0d6edafc76d03e8236995bb78ed09b48e2e5ebbf43ffbf02793d8e9d7,"{""d"":3,""p"":0.001,""rot"":""rotated""}"
   1000000,       239,         0,    17.9,pymatching,22952528d7ff664b2fe27c5cc3d338511e27eb95f224228bc860b67aef27dd58,"{""d"":3,""p"":0.001,""rot"":""unrotated""}"
   1000000,        51,         0,    34.0,pymatching,3355b9bf8560b45b8f7eac03398cb6e008e16de19fba2bd5b8b1a6f9c4332067,"{""d"":5,""p"":0.001,""rot"":""rotated""}"
   1000000,        13,         0,    65.8,pymatching,5fc9fa28cc477c212ab0cc654d1625a0573884e9691bb52179cb3d8f1446ce21,"{""d"":5,""p"":0.001,""rot"":""unrotated""}"
    166658,      1037,         0,    3.31,pymatching,a2fadab97195f83df0a505477885f53cfb64620b06501a849fb47617d1c42ee1,"{""d"":3,""p"":0.005,""rot"":""rotated""}"
    194010,      1100,         0,    5.62,pymatching,31f9a15a9cfa7cde5dc014ba3a352a2487412cfa1a9cc0a1d1ad5eaf784fcba1,"{""d"":3,""p"":0.005,""rot"":""unrotated""}"
    207350,      1001,         0,    23.2,pymatching,d7392e6975e4a79ba13e4dd0eeaf058351c3a57704c813bf6f8f40774bcb3d93,"{""d"":5,""p"":0.005,""rot"":""rotated""}"
    388484,      1032,         0,   113.7,pymatching,220ef6450c218c2c86e699253af5a7ec65a048ec4f2e2a3e63ecbd2fe84bfdeb,"{""d"":5,""p"":0.005,""rot"":""unrotated""}"
     43816,      1014,         0,    1.34,pymatching,142926adda22610adf785e42c7a2fe7b29df607687c0b3d231f3a856bc406369,"{""d"":3,""p"":0.01,""rot"":""rotated""}"
     47255,      1041,         0,    1.86,pymatching,2007687e756a06f41e62c82aed46276c98be16371948a07e8852f59102244273,"{""d"":3,""p"":0.01,""rot"":""unrotated""}"
     32150,      1012,         0,    7.14,pymatching,22ee39f253d37897894334649453037e1107cf2301e600b0f91be07555b2d81b,"{""d"":5,""p"":0.01,""rot"":""rotated""}"
     52068,      1182,         0,    31.5,pymatching,bfebe2cdf47cf94aee1422283efac3d47071729ccf254a092389933b594d24b7,"{""d"":5,""p"":0.01,""rot"":""unrotated""}"
```

and the corresponding image saved to `plot.png`:

![Example plot](readme_example_plot.png)
