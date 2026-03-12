{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  name = "sowon";

  # Packages containing the libraries you listed
  buildInputs = with pkgs; [
    libX11
    libXrandr
    libXcursor
    libXext
    libXi
    libXinerama
    libXrender
    libXfixes
    libxcb
    libXau
    libXdmcp
    libGL
    # Standard C++ and C libraries
    stdenv.cc.cc.lib
    glibc
  ];

  # This environment variable tells the linker where to find the libs
  shellHook = ''
    export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:${
      with pkgs;
      lib.makeLibraryPath [
        libX11
        libXrandr
        libXcursor
        libXext
        libXi
        libXinerama
        libXrender
        libXfixes
        libxcb
        libXau
        libXdmcp
        libGL
        stdenv.cc.cc.lib
      ]
    }"
  '';
}
