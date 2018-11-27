#! /usr/bin/tclsh
#////////////////////////////////////////////////////////////////////////////////////
#// Authors: Kwangsoo Han and Jiajia Li
#//          (Ph.D. advisor: Andrew B. Kahng),
#//          Many subsequent changes for open-sourcing were made by Mateus Fogaça
#//          (Ph.D. advisor: Ricardo Reis)
#//
#// BSD 3-Clause License
#//
#// Copyright (c) 2018, The Regents of the University of California
#// All rights reserved.
#//
#// Redistribution and use in source and binary forms, with or without
#// modification, are permitted provided that the following conditions are met:
#//
#// * Redistributions of source code must retain the above copyright notice, this
#//   list of conditions and the following disclaimer.
#//
#// * Redistributions in binary form must reproduce the above copyright notice,
#//   this list of conditions and the following disclaimer in the documentation
#//   and/or other materials provided with the distribution.
#//
#// * Neither the name of the copyright holder nor the names of its
#//   contributors may be used to endorse or promote products derived from
#//   this software without specific prior written permission.
#//
#// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#////////////////////////////////////////////////////////////////////////////////////

# Define basic paths
set cfg_file ".config"

# Verify wether the config file exists
if {![file exists $cfg_file]} {
	puts "Configuration file not found! Exiting..."
	exit
}

# Read the config file
set fp [open $cfg_file r]
set file_data [read $fp]
close $fp

# Populate the dict with input parameters
set data [split $file_data "\n"]
foreach line $data {
	set tokens [split $line]
	set key [lindex $tokens 0]
	set val [lindex $tokens 1]
	dict set parms $key $val
}

# Print the parameters
puts "+------------------------+"
puts "|      Configuration     |"
puts "+------------------------+"
set path [dict get $parms "path"]
puts [concat "path : " $path]
set verilog [dict get $parms "verilog"]
puts [concat "verilog : " $verilog]
set design [dict get $parms "design"]
puts [concat "design : " $design]
set target_skew [dict get $parms "target_skew"]
puts [concat "target_skew : " $target_skew]
set width [dict get $parms "width"]
puts [concat "width : " $width]
set height [dict get $parms "height"]
puts [concat "height : " $height]
set number [dict get $parms "num_sinks"]
puts [concat "num_sinks : " $number]
set lef [dict get $parms "lef"]
puts [concat "lef : " $lef]
set clkx [dict get $parms "clkx"]
puts [concat "clkx : " $clkx]
set clky [dict get $parms "clky"]
puts [concat "clky : " $clky]
set gcellw [dict get $parms "gcellw"]
puts [concat "gcellw : " $gcellw]
set gcellh [dict get $parms "gcellh"]
puts [concat "gcellh : " $gcellh]
set tech [dict get $parms "tech"]
puts [concat "tech : " $tech]
set enable_pd [dict get $parms "enable_pd"]
puts [concat "enable_pd : " $enable_pd]

set dir "$design\_$target_skew\_$tech"

if {[file exists $dir]} {
	exec rm -rf $dir
}

exec mkdir $dir
exec cp -rf $path $dir/place.def
exec cp -rf $verilog $dir/place.v
cd $dir

exec python ../src/scripts/extract_locations.py place.def > cell_locs.txt
exec cp ../src/tech/lut-$tech.txt lut.txt
exec cp ../src/tech/sol_list-$tech.txt sol_list.txt

if {$tech==28} {
    catch {exec ../third_party/lefdef2cts -lef $lef -def $path -cpin CP > sinks.txt}
} elseif {$tech==16} {
    catch {exec ../third_party/lefdef2cts -lef $lef -def $path -cpin CK > sinks.txt}
}

exec tail -n+3 sinks.txt > sink_cap_tmp.txt
exec awk {NF{print $1 " " $2 " " $3 " 1.0"}} sink_cap_tmp.txt > sink_cap.txt
#exec awk {NF{print $0 " 1.0"}} sink_cap_tmp.txt > sink_cap.txt

# run DP-based clock tree topology and buffering / ILP-based clustering
puts "\nRunning GH-tree (should take a while...)"
puts "genHtree -w [expr $width/15] -h [expr $height/15] -n $number -s $target_skew -tech $tech"
catch {exec ../bin/genHtree -w [expr $width/15] -h [expr $height/15] -n $number -s $target_skew -tech $tech | tee rpt}

exec cp sol_0.txt sol.txt

# Update the netlist
exec cp -rf ../src/scripts/parse_sol.tcl parse_sol.tcl
exec sed -i s/_WIDTH_/$width/g parse_sol.tcl
exec sed -i s/_HEIGHT_/$height/g parse_sol.tcl

if {$tech==28} {
	set root_buff "C12T32_LR_BFX67"
	set buf_regex "BFX"
	set ck_pin "CP"
	set buf_out_pin "Z"
} elseif {$tech==16} {
	set root_buff "BUF_X32N_A9PP96CTS_C16"
	set buf_regex "BUF"
	set ck_pin "CK"
	set buf_out_pin "Y"
}
exec sed -i s/_ROOT_BUFF_/$root_buff/g parse_sol.tcl
exec sed -i s/_BUFF_REGEX_/$buf_regex/g parse_sol.tcl
exec ./parse_sol.tcl

exec cp -rf ../src/scripts/update_def.py update_def.py
exec sed -i s/_CK_PIN_/$ck_pin/g update_def.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g update_def.py
exec python update_def.py > cts.def 

# Remove previous replace run
set replace_dir "leg"
if {[file exists $replace_dir]} {
	exec rm -rf 
}

puts "Running legalization..."
catch {exec ../third_party/RePlAce -bmflag etc -lef $lef -def cts.def -output leg -t 1 -dpflag NTU3 -dploc ../third_party/ntuplace3 -onlyLG -onlyDP -denDP 0.6 > leg_rpt} 
exec cp leg/etc/cts/experiment0/cts_final.def post_leg.def
#exec cp cts.def post_leg.def

# Update cell locations
exec python ../src/scripts/extract_locations.py post_leg.def > cell_locs_final.txt

exec echo "clk" > clockNet.txt
exec grep ck_net* post_leg.def | awk {{print $2}} >> clockNet.txt

# Dump GCELLs for clock pins
exec cp ../src/scripts/router.param .
exec sed -i s#_LEF_#$lef#g router.param
exec sed -i s/_DEF_/post_leg.def/g router.param
exec sed -i s/_GCELLH_/$gcellh/g router.param
exec sed -i s/_GCELLW_/$gcellw/g router.param
exec ../third_party/FlexRoute router.param

# Build guides 
exec cp -rf ../src/scripts/build_guides.py build_guides.py
exec sed -i s/_CK_PIN_/$ck_pin/g build_guides.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g build_guides.py
exec python build_guides.py $clkx $clky $gcellw $gcellh $width $height $enable_pd > guides.log

# Merge guides
exec cp -rf ../src/scripts/merge_guides.py merge_guides.py
exec sed -i s/_CK_PIN_/$ck_pin/g merge_guides.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g merge_guides.py
exec python merge_guides.py > cts.guides

# Generate the final def and verilog
exec python ../src/scripts/verilog_preprocess.py
exec cp -rf ../src/scripts/remove_dummies.py remove_dummies.py
exec sed -i s/_CK_PIN_/$ck_pin/g remove_dummies.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g remove_dummies.py
exec python remove_dummies.py > cts_final.def

