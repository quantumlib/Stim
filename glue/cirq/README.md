# stimcirq

Implements `stimcirq.StimSampler`, a `cirq.Sampler` that uses the stabilizer circuit simulator `stim` to generate samples.

# Example

```python
import cirq
a, b = cirq.LineQubit.range(2)
c = cirq.Circuit(
    cirq.H(a),
    cirq.CNOT(a, b),
    cirq.measure(a, key="a"),
    cirq.measure(b, key="b"),
)

import stimcirq
sampler = stimcirq.StimSampler()
result = sampler.run(c, repetitions=30)

print(result)
# prints something like:
# a=000010100101000011001100110011
# b=000010100101000011001100110011
```
