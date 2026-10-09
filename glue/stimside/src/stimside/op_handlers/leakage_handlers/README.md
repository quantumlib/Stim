# Leakage Tags

The leakage noise tag family is designed to support leakage errors:

Basically, each qubit has hidden classical state, and these tags allow you to manipulate that state,
and use the tracked state to add Pauli noise to the simulator

All leakage error tag starts with `LEAKAGE`, to make them easy to notice and to not waste time
parsing a tag you don't care about.

The leakage tags follow the general form:

    `LEAKAGE_NAME: (arg0) (arg1) ...`

Each argument contains comma separated fields depending on the leakage tag.

The leakage tags are as follows:

#### LEAKAGE_CONTROLLED_ERROR - if first qubit is leaked, apply an error to second qubit

Arguments look like:

    (p, 2-->X) = if first qubit is in 2, do X_ERROR(p) to the second qubit

The controlling leakage state must be an integer greater than 2.

#### LEAKAGE_TRANSITION_1
Apply transitions from single qubit leakage states to other single qubit leakage states

Arguments look like:

    (p, 2-->3) = if the qubit is in 2, transition to 3 with probability p

There is a quick syntactic sugar for symmetric processes:

    (p, 2<->3) = (p:2-->3) (p:3-->2)

You can specify an unleaked state using `U`

    (p, U-->2) (p, 2-->U)

With a `U` input, an unleaked qubit, regardless of computational state, will undergo the transition with probability `p`.
`0` and `1` are also accepted as inputs (the transition only fires in that Z state, collapsing superpositions)
and as outputs (resetting the qubit into that Z state without depolarizing it); see the notes at the end.
If you don't want the collapse, you can include `(p, U-->2)` where `p` is the average of the probability for the more
specific matching processes `(p0, 0-->2)` and `(p1, 1-->2)`.

#### LEAKAGE_TRANSITION_Z

As with `LEAKAGE_TRANSITION_1`, except supports transitions into known qubit Z states.
This tag is only valid in circuit locations where the Z state of the qubit is known under error-free execution.

`0` and `1` are valid state arguments.
Processes with these states as inputs will occur only when the qubit is in the given known state.
Processes with these states as outputs will prepare the qubit into that state, unleaking it but not depolarizing it.

    (p, 0-->2) (p, 2-->1)

As such, this gate is implementable only by simulators and in circuit locations where the single qubit Z state is known.

Unlike `LEAKAGE_TRANSITION_1`, we permit description of non-leakage state transitions as well,
permitting you to implement arbitrary state dependant behaviour.

    (p, 0-->1), (p, 1-->0)

The general unleaked state `U` is not a valid argument.

#### LEAKAGE_TRANSITION_2
Apply transitions from leakage states over a qubit pair to other leakage states

Arguments look like:

    (p, 2_3<->3_4) = if this pair is in (2, 3), transition to (3, 4) with probability p

Computational states are handled similarly to `LEAKAGE_TRANSITION_1`

    (p, U_2-->U_3) =    if the first qubit is unleaked and the second qubit is in 2,
                        with probability p, transition the second qubit to 3.

    (p, U_2-->3_U) =    if the first qubit is unleaked and the second qubit is in 2,
                        with probability p, leak the first qubit to 3 and unleak the second qubit.

A qubit that transitions from a leaked state to `U` (or `V`) is always fully depolarized.
A `0` or `1` output instead resets the qubit into that Z state without depolarizing it (whatever its input),
and a leaked qubit transitioning to `X`, `Y` or `Z` is reset to `0` before that Pauli is applied.

On the output side, you can also use `V` to indicate an unleaked state that is not the same
the input unleaked state. A qubit that selects to transition `U --> U` is left alone, but one
that transitions `U --> V` is fully depolarized.

    (p, U_2-->V_3) =    if the first qubit is unleaked and the second qubit is in 2,
                        with probability p, depolarize the first qubit and transition the second qubit to 3.

Any unleaked input state matches to `U`. As with `LEAKAGE_TRANSITION_1`, each leg also accepts `0`/`1`
(e.g. `(p0, 0_2 --> 2_2)`), which collapses that qubit's superpositions. To avoid the collapse,
you can include `(p, U_2 --> 2_2)` where `p` is the average of the probability for the more
specific matching processes `(p0, 0_2 --> 2_2)` and `(p1, 1_2 --> 2_2)`.

If you have a strong desire for new instructions supporting known or partially known pair states,
like `LEAKAGE_TRANSITIONS_UZ` or `LEAKAGE_TRANSITIONS_ZZ`, reach out.

#### LEAKAGE_PROJECTION_Z

This tag can be applied to `M`/`MZ`, `MR`/`MRZ`, `MX`, `MY`, `MRX` and `MRY` gates, and determines the classical outcome of the measurement.
In particular, it does not change the leakage state of involved qubits.

Arguments look like:

    (p, 2) = if the qubit is in 2,  the measurement is set to 1 with probability p, else set to 0

Similar to `LEAKAGE_TRANSISION_Z`, we accept argument that depend on the known Z state as well:

    (p, 1) = if the qubit is in 1,  the measurement is set to 1 with probability p, else set to 0
    (p, 0) = if the qubit is in 0,  the measurement is set to 1 with probability p, else set to 0

Notice that the `p` in `(p, 1)` is the probability that a qubit in 1 is read out correctly,
and the `p` in `(p, 0)` is the probability that a qubit in 0 is readout incorrectly.

Each `(p, s)` is the probability that a target in state `s` reads `1` (for `MX`, `MY`, `MRX` and `MRY`, `0` and `1`
are the outcomes in that basis), so the probabilities need not sum to 1, e.g. `(0.0, 0) (1.0, 1) (0.8, 2)`.
An unleaked target whose state isn't listed is read correctly, and a readout error changes only the recorded bit, not the qubit.
A leaked qubit reports `1` with the probability listed for its state (`0` if its state isn't listed), whatever its computational state.

The general unleaked state `U` is not a valid argument.

After the measurement, the leaked targets are fully depolarized.

#### LEAKAGE_SWAP

`SWAP[LEAKAGE_SWAP] 0 1` is a `SWAP` that also swaps the leakage states of each target pair, pair by pair from left
to right. Unlike the other leakage tags it takes no arguments (`LEAKAGE_SWAP`, not `LEAKAGE_SWAP: ...`), and it is
only allowed on `SWAP`. An untagged `SWAP` never moves leakage states.

`TablesideSimulator` and `CosetsideSimulator` apply the computational `SWAP` to every pair, leaked qubits included
(whatever `unconditional_condition_on_U` is), so a leaked qubit's frozen computational state moves with it.
`FlipsideSimulator` swaps the Pauli frames. The qubit that receives a leak counts as an unleaked-to-leaked transition,
so `get_unleaked_to_leaked_records()` over-counts moved leaks. `MarginalLeakageDemGenerator` and the decoders built on
it raise `NotImplementedError` for circuits with this tag. See "`SWAP[LEAKAGE_SWAP]`" in the top-level README.


#### Untagged gates on leaked qubits
`FlipsideSimulator` fully depolarizes a qubit when it leaks and again right after any `M`, `MX`, `MY`, `MR`, `MRX`,
`MRY`, `R`, `RX` or `RY` on it (not after `MPP`, `MXX`, `MYY` or `MZZ`), and applies untagged gates to it as usual
("scrambled, then applied"). `TablesideSimulator` and `CosetsideSimulator` freeze its computational state when it
leaks. By default (`unconditional_condition_on_U=True`) untagged unitary gates and non-heralded noise then skip it
("skipped"); with `unconditional_condition_on_U=False` they apply to the frozen state ("frozen, then applied").
There is no `LEAKAGE_DEPOLARIZE_1` tag. In `TablesideSimulator` and `CosetsideSimulator`,
`DEPOLARIZE1[CONDITIONED_ON_SELF: 2](0.75) q` fully depolarizes `q` in the shots where it is in state 2;
`FlipsideSimulator` doesn't support `CONDITIONED_ON_SELF`.
See "Untagged Instructions on Leaked Qubits" in the top-level README for the details.

#### Important Behavioral Notes

* **State Collapse When Conditioning on `0` or `1`**: Conditioning on computational basis states `0` or `1` (in `CONDITIONED_ON` / `CONDITIONED_ON_SELF` / `CONDITIONED_ON_OTHERS`, `LEAKAGE_TRANSITION_1`, `LEAKAGE_TRANSITION_2`, `LEAKAGE_PROJECTION_Z`, or `LEAKAGE_MEASUREMENT`) in `TablesideSimulator` and `CosetsideSimulator` projectively collapses any $Z$-basis superposition on the inspected unleaked qubit(s) into `|0>` or `|1>` before evaluating the condition or transition. Use `U` to condition on the unleaked subspace without collapsing superpositions.
* **`0`/`1` in `LEAKAGE_TRANSITION_1`/`LEAKAGE_TRANSITION_2` on `FlipsideSimulator`**: an input `0`/`1` is matched against the noiseless reference Z value XOR the shot's X frame. A target that is superposed in the noiseless reference is collapsed to `0` there, and each shot gets a 50% Z kick (the measurement back-action). An output `0`/`1` resets the qubit (X frame set so the value is right, plus a 50% Z kick, like `R`). This needs the target's reference Z value to be definite, or collapsed by a `0`/`1` input key of the same instruction; otherwise a `ValueError` is raised.
* **Repeated Qubit Targets**: Any instruction referencing the same qubit multiple times (such as `CX 0 1 1 0` or `I 0 0 [LEAKAGE_TRANSITION_1<...>]`) is processed sequentially in left-to-right target order, matching Stim.
