{ pkgs ? import <nixpkgs> {} }:
let
  stdenv = pkgs.overrideCC pkgs.stdenv pkgs.clang_18;
in rec {
  enableDebugging = false; #true;

  routerd = stdenv.mkDerivation {
    name = "routerd";
    dontStrip = enableDebugging;
    IS_DEV = enableDebugging;
    srcs = [./src ./ac ./contrib];
    sourceRoot = "src";
    buildInputs = [
      pkgs.cmake
      pkgs.libressl_3_9
      pkgs.pcre-cpp
      pkgs.gperftools
    ];
    #cmakeFlags = [
      #"-DCMAKE_BUILD_TYPE=Debug"
    #];
    enableParallelBuilding = true;
  };
}
