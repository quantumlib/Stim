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
     shots,    errors,  discards, seconds_elapsed,                                                        strong_id, name
    359153,       106,         0,          10.617, b4ecb9a7f869b4b25ec6e977775f9afb266eb70e2d0fc8b60ea80dc7bd3533a0, surface_code_circuits/rotated_d3_p0.001.stim:pymatching
     16148,       101,         0,           0.801, e85b758843b9ec8f36631105c3d97ae2ed686aa04585010ae03c41a69e4d54f7, surface_code_circuits/rotated_d3_p0.005.stim:pymatching
      5554,       120,         0,           0.369, 199b6dc47483d27af3bff1d1050ef32be7786952dbdc0b7829577f6e58993c0a, surface_code_circuits/rotated_d3_p0.01.stim:pymatching
   1000000,        46,         0,          66.595, 67df742a0a1375f833adc062df1e5504fdb192cb6b9aee983aa531ef67a22c18, surface_code_circuits/rotated_d5_p0.001.stim:pymatching
     24131,       110,         0,           5.277, c6ebdd4e3d9ad97b9d629d3875634e044fe386979a9da81c3a91d29849252f2f, surface_code_circuits/rotated_d5_p0.005.stim:pymatching
      4215,       116,         0,           1.699, 0f91864e2f1e3bfafb1b99b364a64cab557e56119fd8cf20f5dbfe1085ce098a, surface_code_circuits/rotated_d5_p0.01.stim:pymatching
    399401,       102,         0,          13.962, c62cdbd0f2de7305f5bab155f05c3424090f768d96d7564f671398f126a765bb, surface_code_circuits/unrotated_d3_p0.001.stim:pymatching
     14829,       111,         0,           0.939, 4e6fd4a7ad4ee332913076a641349d27daabc1b7610ff8e5f85cdcc7485356ae, surface_code_circuits/unrotated_d3_p0.005.stim:pymatching
      6606,       118,         0,           0.660, 020bba7defa8f53830c3855f6345a678337edefe985373fea55866b9fad5f487, surface_code_circuits/unrotated_d3_p0.01.stim:pymatching
   1000000,        12,         0,         139.590, 46ddaaca1ec4034ea0dfdb42b9da3d63d990eb4804182397083e4a70edabcf56, surface_code_circuits/unrotated_d5_p0.001.stim:pymatching
     39344,       116,         0,          24.361, 4a8a37036b20570b312fd1fef9f4d2098973d2dd14cb6878d4bf831c533485cb, surface_code_circuits/unrotated_d5_p0.005.stim:pymatching
      4325,       100,         0,           5.095, 376ba47037f68f7f6bf2c756600eb4358fd5f5e0e899c76099aacdbf129d1bec, surface_code_circuits/unrotated_d5_p0.01.stim:pymatching
```

# display

You can use `simmer plot` to view the results you've collected.
This command takes a CSV file, and also some command indicating how to group each case
into single curves and also what the desired X coordinate of a case is.
This is done in a flexible but very hacky way, by specifying a python expression using the case's filename: 

```bash
simmer plot \
    -in surface_code_stats.csv \
    -group_func "' '.join(name.split('/')[-1].split('_')[:2])" \
    -x_func "'0.' + name.split('/')[-1].split('_')[-1].split('.')[1]" \
    -show
```

Which will show a plot like this one:

![Example plot](readme_example_plot.png)

