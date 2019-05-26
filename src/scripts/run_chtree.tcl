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
set path [file normalize [dict get $parms "path"]]
puts [concat "path : " $path]
set verilog [file normalize [dict get $parms "verilog"]]
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
set lef [file normalize [dict get $parms "lef"]]
puts [concat "lef : " $lef]
set clkx [dict get $parms "clkx"]
puts [concat "clkx : " $clkx]
set clky [dict get $parms "clky"]
puts [concat "clky : " $clky]
set tech [dict get $parms "tech"]
puts [concat "tech : " $tech]
set ck_port [dict get $parms "ck_port"]
puts [concat "ck_port : " $ck_port]
set db_units [dict get $parms "db_units"]
puts [concat "db_units : " $db_units]
set root_buff [dict get $parms "root_buff"]
puts [concat "root_buff : " $root_buff]
set toler [dict get $parms "toler"]
puts [concat "tolerance : " $toler]

set db_ratio [expr $db_units/1000.0]

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
	set buf_regex "BFX"
	set ck_pin "CP"
	set buf_out_pin "Z"
} elseif {$tech==16} {
	set buf_regex "BUF"
	set ck_pin "CK"
	set buf_out_pin "Y"
} elseif {$tech==65} {
	set buf_regex "BUF"
	set ck_pin "CK CLKA CLKB" 
	set buf_out_pin "Y"
}

set inst_ck_pin [lindex [split $ck_pin] 0]
exec cp -rf ../src/scripts/remove_dummies.py remove_dummies.py
exec sed -i s/_CK_PIN_/$inst_ck_pin/g remove_dummies.py
exec sed -i s/_CK_PORT_/$ck_port/g remove_dummies.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g remove_dummies.py

exec cp -rf ../src/scripts/parse_sol.tcl parse_sol.tcl
exec sed -i s/_WIDTH_/$width/g parse_sol.tcl
exec sed -i s/_HEIGHT_/$height/g parse_sol.tcl
exec sed -i s/_CK_PORT_/$ck_port/g parse_sol.tcl
exec sed -i s/_ROOT_BUFF_/$root_buff/g parse_sol.tcl
exec sed -i s/_BUFF_REGEX_/$buf_regex/g parse_sol.tcl

foreach pin $ck_pin {  
	puts "../third_party/lefdef2cts -lef $lef -def $path -cpin $pin -cts sinks.txt -blk blks.txt" 
	catch {exec ../third_party/lefdef2cts -lef $lef -def $path -cpin $pin -cts sinks_part.txt -blk blks_tmp.txt}
	exec cat sinks_part.txt >> sinks.txt
}

set blkFileIn [open blks_tmp.txt r]              
set blkFileOut [open blks.txt w]              
while {[gets $blkFileIn line]>=0} {
	set xmin [lindex $line 0]
	set ymin [lindex $line 1]
	set w [lindex $line 2]
	set h [lindex $line 3]
	puts $blkFileOut "[expr $xmin/15.0] [expr $ymin/15.0] [expr ($xmin+$w)/15.0] [expr ($ymin+$h)/15.0]"
}
close $blkFileIn
close $blkFileOut
exec tail -n+1 sinks.txt > sink_cap_tmp.txt
exec awk {{print $1,$2,$3,1.0}} sink_cap_tmp.txt > sink_cap.txt

# run DP-based clock tree topology and buffering / ILP-based clustering
puts "\nRunning GH-tree (should take a while...)"
puts "genHtree -n $number -s $target_skew -tech $tech -compute_sink_region_mode -t $toler"
catch {exec ../bin/genHtree -n $number -s $target_skew -tech $tech -compute_sink_region_mode -t $toler | tee rpt}

exec cp sol_0.txt sol.txt

# Update the netlist
exec ./parse_sol.tcl

set solFileIn [open locations.txt r]              
set solFileOut [open locations_final.txt w]              
while {[gets $solFileIn line]>=0} {
	set name [lindex $line 0]
	set x 	 [lindex $line 1]
	set y	 [lindex $line 2]
	puts $solFileOut "$name [expr $x*$db_ratio] [expr $y*$db_ratio]"
}
close $solFileIn
close $solFileOut

exec mv locations_final.txt locations.txt
exec cp -rf ../src/scripts/update_def.py update_def.py
exec sed -i s/_CK_PORT_/$ck_port/g update_def.py
exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g update_def.py
exec python update_def.py > cts.def 

set width  [expr $width*$db_ratio]
set height [expr $height*$db_ratio]
set clkx   [expr $clkx*$db_ratio]
set clky   [expr $clky*$db_ratio]

# Remove previous replace run
set replace_dir "leg"
if {[file exists $replace_dir]} {
	exec rm -rf 
}

exec python ../src/scripts/extract_locations.py cts.def > cell_locs_pre_leg.txt
exec python ../src/scripts/verilog_preprocess.py
exec python remove_dummies.py > cts_no_dummies.def

set legPath [file normalize ../third_party/ntuplace4h]
puts "Running legalization..."
puts "../third_party/RePlAce -bmflag etc -lef $lef -def cts.def -output leg -t 1 -dpflag NTU4 -dploc $legPath -onlyLG -onlyDP -fragmentedRow -denDP 0.9 -plot -pcofmax 1.04"
catch {exec ../third_party/RePlAce -bmflag etc -lef $lef -def cts_no_dummies.def -output leg -t 1 -dpflag NTU4 -dploc $legPath -onlyLG -onlyDP -fragmentedRow -denDP 0.9 -plot -pcofmax 1.04 > leg_rpt} 
exec cp leg/etc/cts_no_dummies/experiment000/cts_no_dummies_final.def cts_final.def

