from __future__ import annotations

import stim


class Layer:
    def copy(self) -> Layer:
        """Returns an independent copy of the layer."""
        raise NotImplementedError()

    def touched(self) -> set[int]:
        """Returns the set of qubit indices touched by the layer."""
        raise NotImplementedError()

    def to_z_basis(self) -> list[Layer]:
        """Decomposes into a series of layers do the same thing with Z basis interactions.

        For example, it should use:
            - CZ instead of CX
            - CZSWAP instead of CXSWAP
            - MZ instead of MX
            - MZZ instead of MXX
            - etc

        This will typically be achieved by adding rotation layers before/afterward.
        """
        return [self]

    def append_into_stim_circuit(self, out: stim.Circuit) -> None:
        """Appends the layer's contents into the given stim circuit."""
        raise NotImplementedError()

    def locally_optimized(self, next_layer: Layer | None) -> list[Layer | None]:
        """Returns an equivalent series of layers that has been optimized.

        For example, if this is a RotationLayer and next_layer is also a RotationLayer,
        then the result will be a single merged RotationLayer.
        """
        return [self, next_layer]

    def is_vacuous(self) -> bool:
        """Returns True if the layer doesn't do anything.

        For example, a RotationLayer with no rotations is vacuous.
        """
        return False

    def requires_tick_before(self) -> bool:
        """Returns True if the layer should be separated from the preceding layer."""
        return True

    def implies_eventual_tick_after(self) -> bool:
        """Returns True if the layer should take time to perform."""
        return True
