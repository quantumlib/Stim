{ buildPythonPackage
, stdenv
, gitignoreSource
, numpy
, pybind11
, pytestCheckHook
, networkx
}:
let
  simdFlags =
    if stdenv.targetPlatform.avx2Support
    then "'-mavx2', '-msse2'"
    else if stdenv.targetPlatform.isx86_64
    then "'-mno-avx2', '-msse2'"
    else "'-mno-avx2', '-mno-sse2'";
in
buildPythonPackage {
  pname = "Stim";
  version = "1.7.dev1";

  src = gitignoreSource ./.;

  postPatch = ''
    sed -i setup.py -e "s/'-march=native'/${simdFlags}/"
  '';

  nativeBuildInputs = [ pybind11 ];

  propagatedBuildInputs = [ numpy ];

  pythonImportsCheck = [ "stim" ];

  checkInputs = [ pytestCheckHook networkx ];

  disabledTestPaths = [
    # requires cirq, which isn't in nixpkgs
    "glue/cirq/stimcirq/_cirq_to_stim_test.py"
    "glue/cirq/stimcirq/_stim_sampler_test.py"
    "glue/cirq/stimcirq/_stim_to_cirq_test.py"
  ];
}
