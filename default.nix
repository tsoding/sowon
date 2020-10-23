# This allows sowon to be built from within a nix-shell:
#   nix-shell
#   make
#
with import <nixpkgs> {};

let
    stdenv8 = overrideCC stdenv gcc8;
in
    stdenv8.mkDerivation rec {
        name = "sowon-build";
        env = buildEnv {
            name = name;
            paths = buildInputs;
        };
        buildInputs = [
            pkgconfig
            SDL2
        ];
    }
