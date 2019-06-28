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

# Init program options
proc initProgramOptions {argv} {
	global configFilePath
	global executablePath
	global lefDefParserPath
	global scriptsPath
	global legalizerPath
	global outputPath
	global techFilesPath

	# Default values
	set configFilePath [file normalize "cts.cfg"]
	set executablePath [file normalize "./bin/genHtree"]
	set lefDefParserPath [file normalize "./third_party/lefdef2cts"]
	set scriptsPath [file normalize "./src/scripts"]
	set legalizerPath [file normalize "./third_party/opendp"]
	set techFilesPath [file normalize "./src/tech"]
	set outputPath "" 

	foreach arg $argv {
		set parm [lindex [split $arg "="] 0]
		set val [lindex [split $arg "="] 1]
		puts "$parm = $val"
		switch -exact $parm {
			"-configFilePath" { set configFilePath [file normalize $val] }
			"-executablePath" { set executablePath [file normalize $val] }
			"-lefDefParserPath" { set lefDefParserPath [file normalize $val] }
			"-scriptsPath" { set scriptsPath [file normalize $val] }
			"-legalizerPath" { set legalizerPath [file normalize $val] }
			"-techFilesPath" { set techFilesPath [file normalize $val] }
			"-outputPath" { set outputPath [file normalize $val] }
			default {
				puts "Invalid argment $parm"
			    exit
			}
		}	
	}

	puts "+------------------------+"
	puts "|    Program options     |"
	puts "+------------------------+"
	puts "Config file: $configFilePath"
	puts "Executable: $executablePath"
    puts "Lef/Def parser: $lefDefParserPath"
	puts "Scripts path: $scriptsPath"
	puts "Legalizer binary: $legalizerPath"
	puts "Output path: $outputPath" 
}

#------------------------------------------------------------------------------

proc parseConfigFile {} {
	global configFilePath
	global path
	global verilog
	global design
	global target_skew
	global width
	global height
	global number
	global lef
	global tech
	global ck_port
	global db_units
	global root_buff
	global toler
	global db_ratio
	global buf_regex
	global ck_pin
	global buf_out_pin
	global inst_ck_pin

	# Define basic paths
	set cfg_file $configFilePath
	
	# Verify wether the config file exists
	if {![file exists $cfg_file]} {
		puts "Configuration file not found: $cfg_file"
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
	set lef [file normalize [dict get $parms "lef"]]
	puts [concat "lef : " $lef]
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
	set number 64

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
}

#------------------------------------------------------------------------------

proc initWorkingDirectory {} {
	global outputPath
	global design
	global target_skew
	global tech
	global path
	global verilog
	global scriptsPath
	global tech
	global inst_ck_pin
	global ck_port
	global buf_out_pin
	global width
	global height
	global ck_port
	global root_buff
	global buf_regex
	global techFilesPath

	if {$outputPath == ""} {
		set dir "$design\_$target_skew\_$tech"
	} else {
		set dir $outputPath
	}
	
	if {[file exists $dir]} {
		exec rm -rf $dir
	}
	
	exec mkdir $dir
	exec cp -rf $path $dir/place.def
	exec cp -rf $verilog $dir/place.v
	cd $dir

	if {![file exists $scriptsPath/extract_locations.py]} {
		puts "Script not found: $scriptsPath/extract_locations.py"
		exit		
	}
	exec python $scriptsPath/extract_locations.py place.def > cell_locs.txt
	
	
	if {![file exists $techFilesPath/lut-$tech.txt]} {
		puts "File not found: $techFilesPath/lut-$tech.txt"
		exit		
	}
	exec cp $techFilesPath/lut-$tech.txt lut.txt

	if {![file exists $techFilesPath/sol_list-$tech.txt]} {
		puts "File not found: $techFilesPath/sol_list-$tech.txt"
		exit		
	}
	exec cp $techFilesPath/sol_list-$tech.txt sol_list.txt
	
	if {![file exists $scriptsPath/remove_dummies.py]} {
		puts "Script not found: $scriptsPath/remove_dummies.py]"
		exit		
	}
	exec cp -rf $scriptsPath/remove_dummies.py remove_dummies.py
	exec sed -i s/_CK_PIN_/$inst_ck_pin/g remove_dummies.py
	exec sed -i s/_CK_PORT_/$ck_port/g remove_dummies.py
	exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g remove_dummies.py
	
	if {![file exists $scriptsPath/parse_sol.tcl]} {
		puts "Script not found: $scriptsPath/parse_sol.tcl]"
		exit		
	}

	# Special treatment for clk port names with []
	set ck_port_fixed [string map {"\[" "\\\\\[" } $ck_port]
	set ck_port_fixed [string map {"\]" "\\\\\]" } $ck_port_fixed]

	exec cp -rf $scriptsPath/parse_sol.tcl parse_sol.tcl
	exec sed -i s/_WIDTH_/$width/g parse_sol.tcl
	exec sed -i s/_HEIGHT_/$height/g parse_sol.tcl
	exec sed -i s#_CK_PORT_#$ck_port_fixed#g parse_sol.tcl
	exec sed -i s/_ROOT_BUFF_/$root_buff/g parse_sol.tcl
	exec sed -i s/_BUFF_REGEX_/$buf_regex/g parse_sol.tcl
}

#------------------------------------------------------------------------------

proc parseClockSinksAndBlockages {} {
	global ck_pin
	global lef
	global path
	global lefDefParserPath
	
	if {![file exists $lefDefParserPath]} {
		puts "Binary not found: $lefDefParserPath"
		exit		
	}

	foreach pin $ck_pin {  
		puts "$lefDefParserPath -lef $lef -def $path -cpin $pin -cts sinks.txt -blk blks.txt" 
		catch {exec $lefDefParserPath -lef $lef -def $path -cpin $pin -cts sinks_part.txt -blk blks_tmp.txt}
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
}

#------------------------------------------------------------------------------

proc runGHtree {} {
	global number
	global target_skew
	global tech
	global toler
	global db_ratio
	global executablePath
	
	if {![file exists $executablePath]} {
		puts "Binary not found: $executablePath"
		exit		
	}

	# run DP-based clock tree topology and buffering / ILP-based clustering
	puts "\nRunning GH-tree (should take a while...)"
	puts "$executablePath -n $number -s $target_skew -tech $tech -compute_sink_region_mode -t $toler"
	catch {exec $executablePath -n $number -s $target_skew -tech $tech -compute_sink_region_mode -t $toler | tee rpt}
	
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
}

#------------------------------------------------------------------------------

proc updateDEFAndVerilog {} {
	global ck_port
	global buf_out_pin
	global scriptsPath 

	exec mv locations_final.txt locations.txt
	exec cp -rf $scriptsPath/update_def.py update_def.py
	exec sed -i s/_CK_PORT_/$ck_port/g update_def.py
	exec sed -i s/_BUFF_OUT_PIN_/$buf_out_pin/g update_def.py
	exec python update_def.py > cts.def 
	
	exec python $scriptsPath/extract_locations.py cts.def > cell_locs_pre_leg.txt
	exec python $scriptsPath/verilog_preprocess.py
	exec python remove_dummies.py > cts_no_dummies.def
}

#------------------------------------------------------------------------------

proc legalize {} {
	global legalizerPath
	global lef

	if {![file exists $legalizerPath]} {
		puts "Binary not found: $legalizerPath"
		exit		
	}

	puts "Running legalization..."
	puts "$legalizerPath -lef $lef -def cts_no_dummies.def -output_def cts_final.def"
	catch {exec $legalizerPath -lef $lef -def cts_no_dummies.def -output_def cts_final.def > leg_rpt} 
}

#------------------------------------------------------------------------------

# CTS flow
initProgramOptions $argv
parseConfigFile
initWorkingDirectory
parseClockSinksAndBlockages
runGHtree
updateDEFAndVerilog
legalize
