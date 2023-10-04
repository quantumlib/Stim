from lasir_gen import lasir_gen
from gltf_gen import gltf_gen
import argparse


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--tubelen', type=float, default=2.0,
			help='Ratio of tube length vs. width: large to see topology, small to see layout.')
    args = parser.parse_args()
    lasir_gen('./sample.olsco', './sample.lasir')
    gltf_gen('./sample.lasir', './sample.gltf', args.tubelen, './base.gltf')