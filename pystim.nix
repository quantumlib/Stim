{ buildPythonPackage
, stdenv
, gitignoreSource
, numpy
, pybind11
, pytestCheckHook
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

  checkInputs = [ pytestCheckHook ];

  disabledTestPaths = [
    "glue/*"
  ];
}
