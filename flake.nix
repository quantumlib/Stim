{
  description = "A fast stabilizer circuit simulator";

  inputs = {
    flake-utils.url = "github:numtide/flake-utils";
    gitignore = {
      url = "github:hercules-ci/gitignore.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, flake-utils, gitignore, nixpkgs }:
    let
      inherit (nixpkgs.lib) attrValues last listToAttrs mapAttrs nameValuePair replaceStrings;
      pyVersions = [ "3.7" "3.8" "3.9" "3.10" ];
      pyNixVersions = map (replaceStrings [ "." ] [ "" ]) pyVersions;
      pyLatest = "python${last pyNixVersions}";
    in
    flake-utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = import nixpkgs { inherit system; overlays = [ gitignore.overlay ]; };
        in
        {
          defaultPackage = pkgs.linkFarmFromDrvs "stim" (attrValues self.packages.${system});

          packages =
            let
              pyVersionToInterpreter = ver:
                nameValuePair "pystim-python${ver}" pkgs."python${ver}";
              pyOuts = map pyVersionToInterpreter pyNixVersions;
              mkPyStim = python: python.pkgs.callPackage ./pystim.nix { };
              allPyStims = mapAttrs (_: mkPyStim) (listToAttrs pyOuts);
            in
            flake-utils.lib.flattenTree (allPyStims // {
              stim-avx2 = pkgs.callPackage ./stim.nix { avx2Support = true; sseSupport = false; };
              stim-sse = pkgs.callPackage ./stim.nix { avx2Support = false; sseSupport = true; };
              stim = pkgs.callPackage ./stim.nix { }
            });


          devShell = pkgs.mkShell {
            inputsFrom = [
              self.packages.${system}.stim
              self.packages.${system}."pystim-${pyLatest}"
            ];

            nativeBuildInputs = with pkgs; [
              nix-linter
              nixpkgs-fmt
              pyright
            ];
          };
        }) // {
      overlay =
        final: prev:
        let
          gitignoreSource = (gitignore.overlay final prev).gitignoreSource;
          mkPyOverlay = ver:
            let
              py = prev."python${ver}";
              packageOverrides = pyFinal: _: {
                pystim = pyFinal.callPackage ./pystim.nix { inherit gitignoreSource; };
              };
              pyOverride = py.override { inherit packageOverrides; };
            in
            nameValuePair "python${ver}" pyOverride;
          pyOverlays = listToAttrs (map mkPyOverlay pyNixVersions);
        in
        {
          stim = final.callPackage ./stim.nix { inherit gitignoreSource; };
        } // pyOverlays;
    };
}
