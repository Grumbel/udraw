 {
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.tinycmmc.follows = "tinycmmc";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";
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
      in {
        packages = rec {
          default = udraw;

          udraw = pkgs.stdenv.mkDerivation {
            pname = "udraw";
            version = "0.1.0";

            src = nixpkgs.lib.cleanSource ./.;

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
            ];

            buildInputs = with pkgs; [
              libusb
              fmt_8
            ] ++ [
              tinycmmc.packages.${system}.default
              logmich.packages.${system}.default
              uinpp.packages.${system}.default
              strutcpp.packages.${system}.default
            ];

            meta = {
              mainProgram = "udraw-driver";
            };
           };
        };
      }
    );
}
