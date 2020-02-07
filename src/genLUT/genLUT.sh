#! /usr/bin/bash

chmod +x *.tcl

./run_all.sh

./genLUTOpt2.tcl > "concat.lut"
./prep_lut.tcl "concat.lut"

rm concat.lut

