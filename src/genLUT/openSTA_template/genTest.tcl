#! /usr/bin/tclsh
set net_inpin_dict  [dict create]
set net_outpin_dict [dict create]
set namemap_dict    [dict create]
set cell_coord_dict [dict create]
set net_wirelength_dict [dict create]
set net_dict [dict create]
set net_cap [dict create]

#source config.tcl

#set Q_ffpin "Q"
#set D_ffpin "D"
#set buff_inPin "A"
#set buff_outPin "Y"
#set clk_pin "CK"

#set incap_buf 1.8 
#set incap_D_ff 0.67
#set incap_CK_ff 0.696
#set split_num 4	

proc printDict {ref_dict} {

 	dict for {key val} $ref_dict {
 		puts "$key ==> $val"
 	}
	
}
proc gen_spef {res_per_unit cap_per_unit} {
	
	global net_inpin_dict
	global net_outpin_dict
	global namemap_dict
	global cell_coord_dict
	global net_wirelength_dict
	global net_dict
	global design
	global bufName 
	global FFName
	global D_ffpin
	global incap_buf
	global incap_D_ff
	global incap_CK_ff
	global split_num
    global buff_inPin

	set outFp_spef [open ${design}.spef w]

    set systemTime [clock seconds]
    set date_time [clock format $systemTime -format %D:%H:%M:%S]
	puts $outFp_spef "*SPEF \"IEEE 1481-1998\""
	puts $outFp_spef "*DESIGN \"$design\""
	puts $outFp_spef "*DATE \"$date_time\""
	puts $outFp_spef "*VENDOR \"NONE\""
	puts $outFp_spef "*PROGRAM \"TritonCTS\""
	puts $outFp_spef "*VERSION \"alpha\""
	puts $outFp_spef "*DESIGN_FLOW \"PIN_CAP NONE\" \"NAME_SCOPE LOCAL\""
	puts $outFp_spef "*DIVIDER /"
	puts $outFp_spef "*DELIMITER :"
	puts $outFp_spef "*BUS_DELIMITER \[\]"
	puts $outFp_spef "*T_UNIT 1 PS"
	puts $outFp_spef "*C_UNIT 1 FF"
	puts $outFp_spef "*R_UNIT 1 KOHM"
	puts $outFp_spef "*L_UNIT 1 HENRY"
	puts $outFp_spef ""

	##Dummy net all output pins aLL_ff/D
	#Dummy net all input pins all_ff/Q
	
	##y coordinate
	#lindex [dict get $cell_coord_dict ff] 1
	puts $outFp_spef "*NAME_MAP"
	puts $outFp_spef ""
		
	dict for {key val} $namemap_dict {
 		puts $outFp_spef "$val $key"
 	}
	puts $outFp_spef ""
	puts $outFp_spef "*PORTS"
	puts $outFp_spef ""
	puts $outFp_spef "*1 I *C 0 0"
	
	set idx 0

	dict for {net_key val} $net_dict {
		#if {[dict get  $net_wirelength_dict $net_key ] == 0} {
		#	puts "Net: $net_key Wirelength is 0. Skipping!"
		#	continue
		#}
		##val {pin cell I/O}
		set net_cap  [expr [dict get $net_wirelength_dict $net_key]*$cap_per_unit]
		set net_res [expr [dict get $net_wirelength_dict $net_key]*$res_per_unit]
		puts $outFp_spef ""
 		puts $outFp_spef "*D_NET [dict get $namemap_dict $net_key]  [ format "%.7f" $net_cap]"
		puts $outFp_spef ""
		puts $outFp_spef "*CONN"
		##iterating through all the pins of the net
		for {set i 0} {$i < [llength $val]} {set i [expr $i + 3]} {
		
			set pin_nm [lindex $val $i]
			set cell_name [ lindex $val [expr $i + 1] ]
			set in_out [ lindex $val [expr $i + 2] ]
			
			if { $in_out == "I" } {
	
				if {[regexp "^ff_" $cell_name ]} {
					#puts "FF! $cell_name"
					puts $outFp_spef "*I $cell_name:$pin_nm I *C [dict get $cell_coord_dict $cell_name] *L $incap_D_ff *D $FFName"
			
				} else {
					#puts "BUF: $cell_name"
					puts $outFp_spef "*I $cell_name:$pin_nm I *C [dict get $cell_coord_dict $cell_name] *L $incap_buf *D $bufName"
				}

			}	else {
				
				if {[regexp "^ff_" $cell_name ]} {
					#puts "FF! $cell_name"
					puts $outFp_spef "*I $cell_name:$pin_nm O *C [dict get $cell_coord_dict $cell_name] *L 0 *D $FFName"
			
				} else {
					#puts "BUF: $cell_name"
					puts $outFp_spef "*I $cell_name:$pin_nm O *C [dict get $cell_coord_dict $cell_name] *L 0 *D $bufName"
				}
					
			}	

		
		}
		
		###CAP SECTION ####################################################################
		set nodes_nt ""
		puts $outFp_spef ""
		#puts "########################################"
		puts $outFp_spef "*CAP"
			
		set num_pins [ expr [llength $val]/3 ]
		set total_split [expr $num_pins*$split_num]
		set per_split_cap [expr $net_cap/($total_split-1)]
		set cnt 0
		set ln 1
		for {set i 0} {$i < [llength $val]} {set i [expr $i + 3]} {
			set pin_nm [lindex $val $i]
			set cell_name [lindex $val [expr $i + 1] ]
			
			if { [regexp ".*ff_.*:$D_ffpin.*" "$cell_name:$pin_nm"] } {
				puts $outFp_spef "$ln $cell_name:$pin_nm  _CAP_"
            } elseif { [regexp ".*buf_.*:$buff_inPin.*" "$cell_name:$pin_nm"] } {
				puts $outFp_spef "$ln $cell_name:$pin_nm  0"
			} else {
				puts $outFp_spef "$ln $cell_name:$pin_nm  [ format "%.7f" $per_split_cap]"
			}
			lappend nodes_nt $cnt
			incr cnt
			incr ln
			for {set j 1} {$j < $split_num} {incr j} {
				puts $outFp_spef "$ln $net_key:$cnt  [format "%.7f" $per_split_cap]"
				incr cnt
				incr ln
			}
			
					
		}
		#puts "#########################################"

		###RES SECTION###########################################################################
		puts $outFp_spef ""
		puts $outFp_spef "*RES"
		#puts "Nodes Taken: $nodes_nt"
		set per_split_res [expr $net_res/$total_split] 
		set prev_node 1
		set next_node 1
		set ln 1
		set pin_nm [lindex $val 0]
		set cell_name [lindex $val 1]
		###
		puts $outFp_spef "$ln $net_key:$prev_node $cell_name:$pin_nm $per_split_res"
		set next_node [expr $prev_node + 1]
		incr ln
		for {set i 0} {$i< $split_num - 1} {incr i} {
			if {[lsearch $nodes_nt $next_node] == -1} {
				puts $outFp_spef "$ln $net_key:$prev_node $net_key:$next_node $per_split_res"
				incr ln 
				set prev_node $next_node
				set next_node [expr $next_node + 1]
			} else {
				set next_node [expr $next_node + 1]
			}
		}
		###
		#puts "prev_node: $prev_node next_node: $next_node"
		for {set i 3} {$i < [llength $val]} {set i [expr $i + 3]} {
			set pin_nm [lindex $val $i]
			set cell_name [lindex $val [expr $i + 1] ]

			for {set j 0} {$j < $split_num - 1} {incr j} {
				if {[lsearch $nodes_nt $next_node] == -1} {
					puts $outFp_spef "$ln $net_key:$prev_node $net_key:$next_node $per_split_res"
					incr ln
					set prev_node $next_node
					set next_node [expr $next_node + 1]
				} else {
					set next_node [expr $next_node + 1]
				}
				
			}
			
			puts $outFp_spef "$ln $net_key:$prev_node $cell_name:$pin_nm $per_split_res"
		}		

		####################################################################################
		puts $outFp_spef "*END"
		puts $outFp_spef ""
 	}
	

}

proc dec2bin {i {width {}}} {
    #returns the binary representation of $i
    # width determines the length of the returned string (left truncated or added left 0)
    # use of width allows concatenation of bits sub-fields

    set res {}
    if {$i<0} {
        set sign -
        set i [expr {abs($i)}]
    } else {
        set sign {}
    }
    while {$i>0} {
        set res [expr {$i%2}]$res
        set i [expr {$i/2}]
    }
    if {$res == {}} {set res 0}

    if {$width != {}} {
        append d [string repeat 0 $width] $res
        set res [string range $d [string length $res] end]
    }
    return $sign$res
}


set design [lindex $argv 0]
set dist [lindex $argv 1]
#set unit_dist [lindex $argv 2]
set unit_buf_dist [lindex $argv 2]

source config.tcl
# !!
#set bufName "BUFH_X4N_A9PP96CTL_C16"

# !!
#set FFName "C12T32_LL_SDFPQX8"
#set FFName "DFFQ_X4N_A9PP96CTL_C16"

#set cellHeight "0.576"

# Not modified (random num)
set origX "10.0"
set origY "2.4"

set numbuf_loc [expr int($dist/$unit_buf_dist)]
set totNum [expr pow(2, $numbuf_loc)]
set outFp_v [open ${design}.v w]
#set outFp_place [open ${design}_place.tcl w]

puts $outFp_v "module ${design}( clk );"
puts $outFp_v ""
puts $outFp_v " input clk;"
puts $outFp_v ""

set path ""

####spef#####
set nm 1
dict set namemap_dict "clk" "*$nm"
set nm [expr $nm + 1]
dict set namemap_dict "dummy" "*$nm"
set nm [expr $nm + 1]
#############

for {set i 0} {$i < $totNum} {incr i} {
    set bin [dec2bin $i $numbuf_loc]
    #puts $bin
    set cnt 0
    set wire "net_${bin}_${cnt}"
    puts $outFp_v " wire ${wire};"
    #set path "$path $FFName ff_${bin}_0 ( .D(dummy), .TI(1'b0), .TE(1'b0), .CP(clk), .Q(${wire}) ); \n"
    set path "$path $FFName ff_${bin}_0 ( .$D_ffpin\(dummy), .$clk_pin\(clk), .$Q_ffpin\(${wire}) ); \n"
    set locX $origX
    set locY [expr $origY + $cellHeight*4*$i]
    #puts $outFp_place "placeInstance ff_${bin}_0 $locX $locY R0 -placed"
    set pre_wire $wire
    incr cnt
	
	#######spef#######
	dict set namemap_dict $wire "*$nm"
	set nm [expr $nm + 1]
	dict set namemap_dict "ff_${bin}_0" "*$nm"
	set nm [expr $nm + 1]

	dict set cell_coord_dict "ff_${bin}_0"  "$locX $locY"
	dict set net_outpin_dict $wire "ff_${bin}_0 $Q_ffpin"
	
	dict set net_dict $wire "$Q_ffpin ff_${bin}_0 O"
	set wirelen 0
	##################

    for {set j 0} {$j < [string length $bin]} {incr j} {
        if {[string index $bin $j]} {
            #puts "$bin $i $j"
            set wire "net_${bin}_${cnt}"
            puts $outFp_v " wire ${wire};"
            #set path "$path $bufName buf_${bin}_${cnt} ( .A(${pre_wire}), .Z(${wire})); \n"
            set path "$path $bufName buf_${bin}_${cnt} ( .$buff_inPin\(${pre_wire}), .$buff_outPin\(${wire})); \n"
            set locX [expr $origX + $unit_buf_dist*($j + 1)]
            #puts $outFp_place "placeInstance buf_${bin}_${cnt} $locX $locY R0 -placed"
            

			#######spef########
			dict set namemap_dict $wire "*$nm"
			set nm [expr $nm + 1]
			dict set namemap_dict "buf_${bin}_${cnt}" "*$nm"
			set nm [expr $nm + 1]

			dict set cell_coord_dict "buf_${bin}_${cnt}" "$locX $locY"
			dict set net_outpin_dict $wire "buf_${bin}_${cnt} $buff_outPin"
			dict set net_inpin_dict $pre_wire "buf_${bin}_${cnt} $buff_inPin"
			
			##out pin
			if {[dict exists $net_dict $wire]} {
				
				set nlist [dict get $net_dict $wire]
				lappend nlist "$buff_outPin"
				lappend nlist "buf_${bin}_${cnt}"
				lappend nlist "O"	
				dict set net_dict $wire $nlist
							
			} else {
				
				dict set net_dict $wire "$buff_outPin buf_${bin}_${cnt} O"
			}
			
			##in pin
			if {[dict exists $net_dict $pre_wire]} {
				
				set nlist [dict get $net_dict $pre_wire]
				lappend nlist $buff_inPin
				lappend nlist "buf_${bin}_${cnt}"
				lappend nlist "I"	
				dict set net_dict $pre_wire $nlist
				
			} else {
					
				dict set net_dict $wire "$buff_inPin buf_${bin}_${cnt} I"
			}			


			set wirelen  [expr $wirelen + $unit_buf_dist]
			dict set net_wirelength_dict $pre_wire $wirelen
			set wirelen 0
			
			###################
			
			incr cnt
			set pre_wire $wire

        } else {

			set  wirelen [expr $wirelen + $unit_buf_dist]
		}
    }

    #set path "$path $bufName buf_${bin}_${cnt} ( .A(${pre_wire}), .Z(${wire})); \n"
    #set locX [expr $origX + $dist]
    #puts $outFp_place "placeInstance buf_${bin}_${cnt} $locX $locY R0 -placed"
    set locX [expr $origX + $dist + 3]
    #puts $outFp_place "placeInstance ff_${bin}_1 $locX $locY R0 -placed"
    set path "$path $FFName ff_${bin}_1 ( .$D_ffpin\(${wire}), .$clk_pin\(clk), .$Q_ffpin\(dummy) ); \n"

	######spef########
	dict set namemap_dict "ff_${bin}_1" "*$nm"
	set nm [expr $nm + 1]

	dict set cell_coord_dict "ff_${bin}_1" "$locX $locY"
	dict set net_inpin_dict $wire "ff_${bin}_1 $D_ffpin"
	dict set net_wirelength_dict $pre_wire $wirelen

	if {[dict exists $net_dict $wire]} {
				
		set nlist [dict get $net_dict $wire]
		lappend nlist "$D_ffpin"
		lappend nlist "ff_${bin}_1"
		lappend nlist "I"	
		dict set net_dict $wire $nlist
							
	} else {
				
		dict set net_dict $wire "$D_ffpin ff_${bin}_1 I"
	}
			

	##################
	

}
puts $outFp_v ""
puts $outFp_v $path
puts $outFp_v ""
puts $outFp_v "endmodule"
#close $outFp_place
close $outFp_v

set outFp [open ${design}.sdc w]
puts $outFp "set sdc_version 2.0"
puts $outFp ""
#puts $outFp "set_units -capacitance fF -resistance kOhm -time ps -voltage V -current uA -power mw"
puts $outFp "create_clock \[get_ports clk\]  -period 1  -waveform {0 0.5}"
close $outFp

source config.tcl
#set cap_per_unit_len 0.17779
#set res_per_unit_len 0.0156
gen_spef $res_per_unit_len $cap_per_unit_len

set outF [open "netCap" w]
dict for {key val} $net_wirelength_dict {
    puts $outF "$key [expr $val*$cap_per_unit_len]"
 }
close $outF	
