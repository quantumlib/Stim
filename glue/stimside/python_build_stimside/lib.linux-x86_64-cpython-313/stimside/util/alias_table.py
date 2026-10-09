from typing import TypeVar

import numpy as np

T = TypeVar("T")


def build_alias_table(
    elements: list[tuple[T, float]],
) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """given a list of elements with associated disjoint probabilities, build an alias table.

    Args:
        list of elements, each of which is a tuple of an element and a probability (float 0<=p<=1)

    Returns:
        a tuple containing 3 numpy arrays:
            bases: contains the base elements for each entry
            aliases: contains the aliases related to each base element
            thresholds: contains the thresholds as a float 0<=p<=1 to return the base value
                otherwise, returns the alias value

    see https://en.wikipedia.org/wiki/Alias_method for details
    """
    # check for no duplicates
    if len(set([e[0] for e in elements])) != len(elements):
        raise ValueError(
            f"Received duplicate entries in {elements}. "
            f"Combine duplicates into a single entry."
        )
    # remove any elements with 0 probability
    elements = [e for e in elements if e[1] != 0.0]
    # check for disjointness
    if any(e[1] < 0 or e[1] > 1 for e in elements):
        raise ValueError(
            f"Can only build an alias table for disjoint probabilities. "
            f"Got {elements}, with probabilities outside 0<=p<=1."
        )
    if sum([e[1] for e in elements]) != 1:
        raise ValueError(
            f"Can only build an alias table for a complete set of disjoint probabilities. "
            f"Got {elements}, with probabilities that sum to {sum([e[1] for e in elements])} != 1."
        )

    mean = 1 / len(elements)

    unhandled_elements = elements.copy()
    unhandled_elements.sort(key=lambda e: e[1])
    bases: list[T] = []
    aliases: list[T] = []
    thresholds: list[float] = []
    while len(unhandled_elements) != 0:
        if len(unhandled_elements) == 1:
            # only one thing left, so it better have p == mean
            last = unhandled_elements[0]
            if not np.isclose(last[1], mean):
                raise ValueError(
                    "Somehow, alias table final entry did not have mean probability. "
                    "This one is on us not you, file a bug."
                )
            bases.append(last[0])
            aliases.append(last[0])
            thresholds.append(1.0)
            break

        smallest = unhandled_elements.pop(0)
        largest = unhandled_elements.pop(-1)
        diff = mean - smallest[1]

        bases.append(smallest[0])
        aliases.append(largest[0])
        thresholds.append(smallest[1] / mean)

        new_largest = (largest[0], largest[1] - diff)
        unhandled_elements.append(new_largest)
        unhandled_elements.sort(key=lambda e: e[1])

    # check whether all possible samples are the same type:
    all_potential_elements = [e[0] for e in elements]
    candidate_type = type(all_potential_elements[0])
    use_object_type = not all(
        isinstance(e, candidate_type) for e in all_potential_elements
    )

    if use_object_type:
        return (
            np.array(bases, dtype=object),
            np.array(aliases, dtype=object),
            np.array(thresholds),
        )
    else:
        return np.array(bases), np.array(aliases), np.array(thresholds)


def sample_from_alias_table(
    num_samples: int,
    bases: np.ndarray,
    aliases: np.ndarray,
    thresholds: np.ndarray,
    np_rng: np.random.Generator,
) -> np.ndarray:
    """given an alias table, produce an array of samples.

    Args:
        num_samples: the number of samples to take, determines the output array shape
        bases, aliases, thresholds: the elements of the alias table to sample from
            see build_alias_table for details
        np_rng: a numpy Generator instance from which to get entropy to make samples

    Returns:
        a numpy array of shape=(num_samples,) containing samples from the alias table
    """

    # for each sample, choose which column in the table to use,
    # and generate a random number to compare to the threshold to switch to the alias
    cols = np_rng.integers(len(bases), size=num_samples)
    rands = np_rng.random(size=num_samples)

    sampled_thresholds = thresholds[cols]
    sampled_bases = bases[cols]
    sampled_aliases = aliases[cols]

    # combine, taking the base when rand is below threshold, else taking alias
    condition = rands < sampled_thresholds
    if sampled_bases.ndim > 1:
        reshape_dims = (len(rands),) + (1,) * (sampled_bases.ndim - 1)
        condition = condition.reshape(reshape_dims)
    return np.where(condition, sampled_bases, sampled_aliases)
