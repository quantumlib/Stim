{ stdenv
, cmake
, gitignoreSource
, avx2Support ? stdenv.targetPlatform.avx2Support
, sseSupport ? stdenv.targetPlatform.isx86_64
}:
stdenv.mkDerivation {
  pname = "Stim";
  version = "1.7.dev1";

  src = gitignoreSource ./.;

  nativeBuildInputs = [ cmake ];

  cmakeFlags =
    if avx2Support
    then [ "-DSIMD_WIDTH=256" ]
    else if sseSupport
    then [ "-DSIMD_WIDTH=128" ]
    else [ "-DSIMD_WIDTH=64" ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/{bin,lib}
    install -m 755 out/stim $out/bin/
    install -m 644 out/libstim.a $out/lib
    runHook postInstall
  '';
}
