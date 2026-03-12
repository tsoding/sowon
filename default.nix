{
  pkgs ? import <nixpkgs> { },
}:

pkgs.stdenv.mkDerivation {
  pname = "sowon";
  version = "1.0.0";

  # Point this to the directory containing your binary
  src = ./.;

  nativeBuildInputs = [
    pkgs.autoPatchelfHook
  ];

  buildInputs = with pkgs; [
    xorg.libX11
    xorg.libXrandr
    xorg.libXcursor
    xorg.libXext
    xorg.libXi
    xorg.libXinerama
    xorg.libXrender
    xorg.libXfixes
    xorg.libxcb
    xorg.libXau
    xorg.libXdmcp
    libGL
    stdenv.cc.cc.lib # Provides libstdc++ if needed
    glibc
  ];

  installPhase = ''
    mkdir -p $out/bin
    cp your-executable-name $out/bin/my-app
    chmod +x $out/bin/my-app
  '';

  meta = {
    description = "A binary patched for NixOS";
    platforms = pkgs.lib.platforms.linux;
  };
}
