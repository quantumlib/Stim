# simmer: stim sampling helper

Simmer is a bit of glue code that allows using Stim and a decoder in tandem
in order to benchmark quantum error correction circuits using Monte Carlo sampling.
Simmer supports using pymatching to decode the samples, and can use python
multiprocessing to fully utilize a computer's resources to get good performance.

**simmer is still in development. Its API and output formats are not stable.** 

# Example

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
    -circuits surface_code_circuits/*.stim \
    -max_shots 1_000_000 \
    -max_errors 100 \
    -decoders pymatching \
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
     shots,    errors,  discards, seconds,decoder,strong_id,custom_json
    403500,       101,         0,    9.83,pymatching,ef3da13b767334393de4d126f8224aad405265130e8fc78f3845679be52ec42f,"{""path"":""surface_code_circuits/rotated_d3_p0.001.stim""}"
     20130,       101,         0,   0.652,pymatching,43a7b4d873ffa6eedaa906d64abc4be7d7565e626ee2e24d779a1765ec5fd69b,"{""path"":""surface_code_circuits/rotated_d3_p0.005.stim""}"
      4215,       102,         0,   0.165,pymatching,c0bc6090f3165ef2b8df7bae2102b5cad8afd8027cc499b98bc7b7dc6822c93d,"{""path"":""surface_code_circuits/rotated_d3_p0.01.stim""}"
   1000000,        43,         0,    71.5,pymatching,903acf3fb1adcfb680786fc140f45ffeb5cc09f0707bd58a2c1a6040586b43bf,"{""path"":""surface_code_circuits/rotated_d5_p0.001.stim""}"
    378500,       101,         0,    12.0,pymatching,dd8ff3eb31268796201788b2e7e84788f5164cedd1003069feca364d6f0cf17b,"{""path"":""surface_code_circuits/unrotated_d3_p0.001.stim""}"
     21739,       101,         0,    3.70,pymatching,ecd7f4a93985876bde1eb5d03d8553323422da51e30f41d31c44401e6dbd6e05,"{""path"":""surface_code_circuits/rotated_d5_p0.005.stim""}"
     22521,       113,         0,   0.936,pymatching,dc03dea140b3df28e485c8dc5b3e310d0b7a3764f0bb4e12dd359245c62961b6,"{""path"":""surface_code_circuits/unrotated_d3_p0.005.stim""}"
      3672,       111,         0,    1.11,pymatching,7bab069683a2f6a669d54b4dbe3c0f6d72e6efc720091e76f18dbf6e0d3a4b5f,"{""path"":""surface_code_circuits/rotated_d5_p0.01.stim""}"
      5794,       120,         0,   0.359,pymatching,9d21b0d66df659a75dc1ea467d3b6c5eff81dc4924b722ae5f64ad8f7faee9bb,"{""path"":""surface_code_circuits/unrotated_d3_p0.01.stim""}"
   1000000,        24,         0,   130.1,pymatching,6a23f2748c9623e365b3d220c327d6cb35738fc4bd54274732d735541cc5b59d,"{""path"":""surface_code_circuits/unrotated_d5_p0.001.stim""}"
     43854,       104,         0,    16.2,pymatching,99e125b30772a79c8165d709934b7c77bd14a3bd4d9a437b42ea6458e68b1282,"{""path"":""surface_code_circuits/unrotated_d5_p0.005.stim""}"
      4208,       103,         0,    3.03,pymatching,9c235b930d8373583f26d60238985af91b09db14396e4e342332c6e4b6a1e664,"{""path"":""surface_code_circuits/unrotated_d5_p0.01.stim""}"
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

