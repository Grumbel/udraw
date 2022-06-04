 {
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-22.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.flake-utils.follows = "flake-utils";
    logmich.inputs.tinycmmc.follows = "tinycmmc";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";
    strutcpp.inputs.flake-utils.follows = "flake-utils";
    strutcpp.inputs.tinycmmc.follows = "tinycmmc";

    uinpp.url = "github:grumbel/uinpp";
    uinpp.inputs.nixpkgs.follows = "nixpkgs";
    uinpp.inputs.flake-utils.follows = "flake-utils";
    uinpp.inputs.tinycmmc.follows = "tinycmmc";
    uinpp.inputs.logmich.follows = "logmich";
    uinpp.inputs.strutcpp.follows = "strutcpp";
  };

  outputs = { self, nixpkgs, flake-utils, tinycmmc, logmich, strutcpp, uinpp }:
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
              logmich.defaultPackage.${system}
              uinpp.defaultPackage.${system}
              strutcpp.defaultPackage.${system}
            ];
            meta = {
              mainProgram = "udraw-driver";
            };
           };
        };
        defaultPackage = packages.udraw;
      }
    );
}
