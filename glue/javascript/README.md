# Stim in Javascript

This directory contains glue code for exposing stim to javascript as a web assembly module via enscriptem.

Due to difficulties getting enscriptem to work, building the module is only supported on linux systems.

The `glue/javascript/build_wasm.sh` bash script creates the web assembly module.
In order for the script to work, you must have a working `emcc` command in your PATH.
