import abc
import pathlib

import numpy as np
import stim


class CompiledDecoder(metaclass=abc.ABCMeta):
    """Abstract class for decoders preconfigured to a specific decoding task.

    This is the type returned by `sinter.Decoder.compile_decoder_for_dem`. The
    idea is that, when many shots of the same decoding task are going to be
    performed, it is valuable to pay the cost of configuring the decoder only
    once instead of once per batch of shots. Custom decoders can optionally
    implement that method, and return this type, to increase sampling
    efficiency.
    """

    @abc.abstractmethod
    def decode_shots_bit_packed(
            self,
            *,
            bit_packed_detection_event_data: np.ndarray,
    ) -> np.ndarray:
        """Predicts observable flips from the given detection events.

        All data taken and returned must be bit packed with bitorder='little'.

        Args:
            bit_packed_detection_event_data: Detection event data stored as a
                bit packed numpy array. The numpy array will have the following
                dtype/shape:

                    dtype: uint8
                    shape: (num_shots, ceil(dem.num_detectors / 8))

                where `num_shots` is the number of shots to decoder and `dem` is
                the detector error model this instance was compiled to decode.

                It's guaranteed that the data will be laid out in memory so that
                detection events within a shot are contiguous in memory (i.e.
                that bit_packed_detection_event_data.strides[1] == 1).

        Returns:
            Bit packed observable flip data stored as a bit packed numpy array.
            The numpy array must have the following dtype/shape:

                dtype: uint8
                shape: (num_shots, ceil(dem.num_observables / 8))

            where `num_shots` is bit_packed_detection_event_data.shape[0] and
            `dem` is the detector error model this instance was compiled to
            decode.
        """
        pass


class Decoder(metaclass=abc.ABCMeta):
    """Abstract base class for custom decoders.

    Custom decoders can be explained to sinter by inheriting from this class and
    implementing its methods.

    Decoder classes MUST be serializable (e.g. via pickling), so that they can
    be given to worker processes when using python multiprocessing.
    """

    def compile_decoder_for_dem(
        self,
        *,
        dem: stim.DetectorErrorModel,
    ) -> CompiledDecoder:
        """Creates a decoder preconfigured for the given detector error model.

        This method is optional to implement. By default, it will raise a
        NotImplementedError. When sampling, sinter will attempt to use this
        method first and otherwise fallback to using `decode_via_files`.

        The idea is that the preconfigured decoder amortizes the cost of
        configuration over more calls. This makes smaller batch sizes efficient,
        reducing the amount of memory used for storing each batch, improving
        overall efficiency.

        Args:
            dem: A detector error model for the samples that will need to be
                decoded. What to configure the decoder to decode.

        Returns:
            An instance of `sinter.CompiledDecoder` that can be used to invoke
            the preconfigured decoder.

        Raises:
            NotImplementedError: This `sinter.Decoder` doesn't support compiling
                for a dem.
        """
        raise NotImplementedError('compile_decoder_for_dem')

    @abc.abstractmethod
    def decode_via_files(self,
                         *,
                         num_shots: int,
                         num_dets: int,
                         num_obs: int,
                         dem_path: pathlib.Path,
                         dets_b8_in_path: pathlib.Path,
                         obs_predictions_b8_out_path: pathlib.Path,
                         tmp_dir: pathlib.Path,
                       ) -> None:
        """Performs decoding by reading/writing problems and answers from disk.

        Args:
            num_shots: The number of times the circuit was sampled. The number
                of problems to be solved.
            num_dets: The number of detectors in the circuit. The number of
                detection event bits in each shot.
            num_obs: The number of observables in the circuit. The number of
                predicted bits in each shot.
            dem_path: The file path where the detector error model should be
                read from, e.g. using `stim.DetectorErrorModel.from_file`. The
                error mechanisms specified by the detector error model should be
                used to configure the decoder.
            dets_b8_in_path: The file path that detection event data should be
                read from. Note that the file may be a named pipe instead of a
                fixed size object. The detection events will be in b8 format
                (see
                https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                ). The number of detection events per shot is available via the
                `num_dets` argument or via the detector error model at
                `dem_path`.
            obs_predictions_b8_out_path: The file path that decoder predictions
                must be written to. The predictions must be written in b8 format
                (see
                https://github.com/quantumlib/Stim/blob/main/doc/result_formats.md
                ). The number of observables per shot is available via the
                `num_obs` argument or via the detector error model at
                `dem_path`.
            tmp_dir: Any temporary files generated by the decoder during its
                operation MUST be put into this directory. The reason for this
                requirement is because sinter is allowed to kill the decoding
                process without warning, without giving it time to clean up any
                temporary objects. All cleanup should be done via sinter
                deleting this directory after killing the decoder.
        """
        pass
