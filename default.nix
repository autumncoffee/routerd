{ pkgs ? import <nixpkgs> {} }:
let
  stdenv = pkgs.overrideCC pkgs.stdenv pkgs.clang_6;
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
      pkgs.openssl
      pkgs.pcre-cpp
      pkgs.gperftools
    ];
    #cmakeFlags = [
      #"-DCMAKE_BUILD_TYPE=Debug"
    #];
    enableParallelBuilding = true;
  };
}
