# Stim

Quantum stabilizer circuit simulator.

# Building

```bash
cmake .
make stim
# output file: ./stim
```

# Testing

```bash
cmake .
make stim_tests
./stim_tests
```

# Manual Build

```bash
ls | grep "\\.cc" | grep -v "\\.test\\.cc" | xargs g++ -std=c++17 -march=native -O3
# output file: ./a.out
```
