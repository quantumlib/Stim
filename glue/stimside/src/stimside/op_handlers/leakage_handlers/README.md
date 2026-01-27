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

An unleaked qubit, regardless of computational state, will undergo the transition with probability `p`.
As such, you may need to re-express more specific processes.
For instance, you can include `(p, U-->2)` where `p` is the average of the probability for the more
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

A qubit that transitions from a leaked to an unleaked state is always fully depolarized.

On the output side, you can also use `V` to indicate an unleaked state that is not the same
the input unleaked state. A qubit that selects to transition `U --> U` is left alone, but one
that transitions `U --> V` is fully depolarized.

    (p, U_2-->V_3) =    if the first qubit is unleaked and the second qubit is in 2,
                        with probability p, depolarize the first qubit and transition the second qubit to 3.

Any unleaked input state matches to `U`, so you should re-express more specific processes.
For instance, you can include `(p, U_2 --> 2_2)` where `p` is the average of the probability for the more
specific matching processes `(p0, 0_2 --> 2_2)` and `(p1, 1_2 --> 2_2)`.

If you have a strong desire for new instructions supporting known or partially known pair states,
like `LEAKAGE_TRANSITIONS_UZ` or `LEAKAGE_TRANSITIONS_ZZ`, reach out.

#### LEAKAGE_PROJECTION_Z

This tag can only be applied to `MZ` gates, and determines the classical outcome of the measurement.
In particular, it does not change the leakage state of involved qubits.

Arguments look like:

    (p, 2) = if the qubit is in 2,  the measurement is set to 1 with probability p, else set to 0

Similar to `LEAKAGE_TRANSISION_Z`, we accept argument that depend on the known Z state as well:

    (p, 1) = if the qubit is in 1,  the measurement is set to 1 with probability p, else set to 0
    (p, 0) = if the qubit is in 0,  the measurement is set to 1 with probability p, else set to 0

Notice that the `p` in `(p, 1)` is the probability that a qubit in 1 is read out correctly,
and the `p` in `(p, 0)` is the probability that a qubit in 0 is readout incorrectly.

Included probabilities must be disjoint (i.e. sum to less than or equal to 1).
If they sum to less than 1, the remaining probability leaves the qubit state alone.
If the qubit is leaked, it will ahve been depolarized and this likely means it will return a random measurement value.

The general unleaked state `U` is not a valid argument.

This tag also implements the `LEAKAGE_DEPOLARIZE_1` behaviour on all qubits immediately after measurement.


#### LEAKAGE_DEPOLARIZE_1
Fully depolarizes a qubit if it is leaked.

By default, the qubit state in the underlying simulator is fully depolarized when it leaks.
However, if a qubit is leaked and goes through an M or R instruction, it is
polarized (or un-de-polarized) and prepared into a known computational state,
even though it should be leaked. By default, the leakage parsing adds the
`LEAKAGE_DEPOLARIZE_1` behaviour right after M and R gates to avoid confusion.

You are free to include this in the circuit by hand.
If you have turned off the default scrambling behaviour, it is on you to
insert these appropriately to ensure your simulation is valid.

Arguments are empty.

