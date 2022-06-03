 {
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, tinycmmc }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in rec {
        packages = flake-utils.lib.flattenTree {
          udraw = pkgs.stdenv.mkDerivation {
            pname = "udraw";
            version = "0.0.0";
            src = nixpkgs.lib.cleanSource ./.;
            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
            ];
            buildInputs = with pkgs; [
              libusb
              fmt
            ] ++ [
              tinycmmc.defaultPackage.${system}
            ];
           };
        };
        defaultPackage = packages.udraw;
      }
    );
}
