
source config.tcl

proc listOptions {i {width {}} num} {
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
        set res [expr {$i%$num}]$res
        set i [expr {$i/$num}]
    }
    if {$res == {}} {set res 0}

    if {$width != {}} {
        append d [string repeat 0 $width] $res
        set res [string range $d [string length $res] end]
    }
    return $sign$res
}


proc pathDelay {outpin inpin} {
    
    set path [find_timing_paths -through $outpin ]; 
   
    set points [get_property $path points]
    set start  [lindex $points 0]
    set end  [lindex $points [expr [llength $points] - 1]]
    
    set start_arrival_time 0
    set end_arrival_time 0
    
    set delay  [expr {[get_property $end arrival] - [get_property $start arrival]} ]
    
    #puts "(Delay) $delay"
    return $delay
}

proc getBufList {bufTypes bufNum} {
   
    set length [llength $bufTypes]
    set totNum [expr pow($length, $bufNum)]

    set allList ""
    for {set i 0} {$i < $totNum} {incr i} {
        set cur [listOptions $i $bufNum $length]
        set allList "$allList $cur"
    }

    #puts $allList
    return $allList
}


##pwr_list "Internal Switching Leakgae" 
proc get_power {inst_nm type} {
	report_power -instance $inst_nm > pwr.rpt
	set pwr_rpt	[open "./pwr.rpt" r]
	
	set pwr_list ""

	while {[gets $pwr_rpt line] >= 0} { 
		
		if {[regexp -nocase [ subst -nocommands -nobackslashes {(.*)\s+(.*)\s+(.*)\s+(.*)\s+$inst_nm} ] $line]} {
			set wordlist [regexp -inline -all -- {\S+} $line]
			set pwr_list [split $wordlist " "]
		}
	
	}
	#puts "PWR LIST: $pwr_list"
	
	close $pwr_rpt
	file delete pwr.rpt
	
	if {$type == "switching"} {
		return [lindex $pwr_list 1]
	} elseif {$type == "internal"} {
		return [lindex $pwr_list 0]
	} elseif {$type == "leakage"} {
		return [lindex $pwr_list 2]
	} elseif {$type == "total"} {
		return [lindex $pwr_list 3]
	}	

 	
}

proc get_pincapmax {pin_nm} {
	
	report_pin $pin_nm > pin.rpt
	set pin_rpt	[open "./pin.rpt" r]
	
	while {[ gets $pin_rpt line ] >=0 } {
		
		set cell_nm [get_property [get_lib_cells -of_objects [get_cells -of_objects [lindex $line 1] ]	] full_name]
		
		if {[regex {^load.*} $cell_nm ]} {
		
			#puts "LINE 362: PIN: [llength $line ]"
			set cap [lindex $line 3]
			close $pin_rpt
			file delete pin.rpt
			return $cap
		
		} elseif {[regex {(.*)r(.*)f(.*)} $line ]} {		
			
			puts "LINE370: PIN: [llength $line ]"
			set r_cap [lindex $line 4]
			set f_cap [lindex $line 6]	
			
		} else {
			set cap [lindex $line 3]
			close $pin_rpt
			file delete pin.rpt
			return $cap
		}
	}
	
	close $pin_rpt
	file delete pin.rpt

    # select the maximum capacitance
    set r_cap [lindex [split $r_cap ":"] 1]
    set f_cap [lindex [split $f_cap ":"] 1]

	if {$r_cap > $f_cap} {
		return $r_cap

	} else {

		return $f_cap
	}

}

proc get_netCap {net_nm} {

	set net_rpt	[open "./netCap" r]	
	
	set net_list " "
	while {[gets $net_rpt line] >= 0} {
		 
		if { [regexp -nocase [ subst -nocommands -nobackslashes {^$net_nm.*} ] $line] } {
			set line [regexp -inline -all -- {\S+} $line]	
			set net_list [split $line " "]
		}

	}
	close $net_rpt
	return [lindex $net_list 1]

}


proc charTables {dist unit_dist} {
    global cap_unit
    global time_unit
    global initial_cap_interval  
    global final_cap_interval
    global bufTypes
    global maxSlew
    global inputSlew 
    global slewInter 
    global outLoadNum
    global baseLoad
    global loadInter 
    
    global Q_ffpin "Q"
    global D_ffpin "D"
    global buff_inPin "A" 

    set numbuf_loc [expr int($dist/$unit_dist)]
    set totNum [expr pow(2, $numbuf_loc)]
   
    set totList ""
    set loadList ""
    for {set i 1} {$i <= $outLoadNum} {incr i} {
        if {$i <= 5} {
            set load [expr ($i/1000.0)*$cap_unit]
        } else {
            set load [expr $baseLoad + $loadInter*($i - 5)]
        }
        set loadList "$loadList $load" 
    }

    #Use 1.0 instead of 2.0 until OpenSTA is fixed.
	set power_default_signal_toggle_rate 2
    
    for {set i 0} {$i < $totNum} {incr i} {
        set bin [listOptions $i $numbuf_loc 2]

        set outPin "ff_${bin}_0/$Q_ffpin"
        set drivenCell "ff_${bin}_1"
        set inPin "ff_${bin}_1/$D_ffpin"
        set bufNames ""
        set cnt 1
        set preloc 0
        set segList ""
        set netName "net_${bin}_0"
        
        set totPower 0
		set wirePower  [ get_power [ get_property [get_fanin -to $netName -levels 1 -only_cells] full_name ] switching ] 
		puts "$netName $wirePower"
        
		for {set j 0} {$j < [string length $bin]} {incr j} {
            if {[string index $bin $j]} {
                set bufNames "$bufNames buf_${bin}_${cnt}"
                set netName "net_${bin}_${cnt}"
                incr cnt 
                set segList "$segList [expr ($j + 1)*$unit_dist - $preloc]"
                set preloc [expr ($j + 1)*$unit_dist]
            }
        }

        set segList "$segList [expr $dist - $preloc]"
        set optList [getBufList $bufTypes [expr $cnt - 1]]
	
        if { [llength $optList] } {
            for {set j 0} {$j < [llength $optList]} {incr j} {
                
				set curList [lindex $optList $j]
                set totPower $wirePower
                set cur_sol ""

                for {set k 0} {$k < [string length $curList]} {incr k} {
                    
                    set curBuf [lindex $bufTypes [string index $curList $k]]
                    set bufName [lindex $bufNames $k]
                    if {[string compare [get_property [get_cells $bufName] ref_name] $curBuf]} { 
                        replace_cell $bufName $curBuf
                    }
                    set cur_sol "$cur_sol[lindex $segList $k],$curBuf,"
                }

				
                if {[lindex $segList end]} {
                    set cur_sol "$cur_sol[lindex $segList end]"
                } else {
                    set cur_sol [string replace $cur_sol end end ""]
                }

                set pin "[lindex $bufNames 0]/$buff_inPin" 
		 
				puts "pin: $pin"
                set inPinCap [get_pincapmax $pin]
                set net "net_${bin}_0"
				set inNetCap [expr ([get_netCap $net] / 1000)*$cap_unit]
				
                set inCap [expr $inPinCap + $inNetCap]
                puts "Cap Pin $net: $inPinCap Net: $inNetCap"
                if {$inCap <= $baseLoad} {
		     		set inCap [expr int(($inCap + ($initial_cap_interval/2)*$cap_unit)/($initial_cap_interval*$cap_unit))*($initial_cap_interval*$cap_unit)]
                } else {
		   			set inCap [expr int(($inCap + ($final_cap_interval/2)*$cap_unit)/($final_cap_interval*$cap_unit))*($final_cap_interval*$cap_unit)]
                }
                
                for {set slew $inputSlew} {$slew <= $maxSlew} {set slew [expr $slew + $slewInter]} {
			    	set_assigned_transition -rise $slew [get_property  [get_pins $outPin] full_name]
                 	set_assigned_transition -fall $slew [get_property  [get_pins $outPin] full_name]
                    for {set idx 0} {$idx < $outLoadNum} {incr idx} {
						read_spef ./spef_files/spef_${idx}.spef
                        puts "read_spef spef_${idx}.spef"
                        set load [lindex $loadList $idx]
                        set delay [format "%.3f" [pathDelay $outPin $inPin]]
    					
						set path [find_timing_paths -through $outPin ]; 

                        set points [get_property $path points]
                        set end    [lindex $points [expr [llength $points] - 1]]
						set tr	[get_property [get_property $end pin ] actual_rise_transition_max]
						set tf [get_property [get_property $end pin ] actual_fall_transition_max]
						set trans [expr [expr $tr + $tf]/2 ]
						set outSlew [expr int(( $trans + $slewInter/2)/$slewInter)*$slewInter]
                        
                        set totPower $wirePower
                        for {set k 0} {$k < [string length $curList]} {incr k} {
                            set curBuf [lindex $bufTypes [string index $curList $k]]
                            set bufName [lindex $bufNames $k]
                            set internalPower [get_power $bufName internal]
			    			set leakagePower [get_power $bufName leakage]
			    			set swPower [get_power $bufName switching]
                            set totPower [expr $totPower + $swPower + $internalPower + $leakagePower]
                            puts "(PDEBUG $bufName $totPower $swPower $internalPower $leakagePower"
                        }

                        if {$outSlew > [expr 2*$maxSlew]} { continue }

                        puts "(Debug-0) $cur_sol"
                        puts "(Debug-1) load: $load inpinCap: $inPinCap inNetCap: $inNetCap inSlew: $slew delay: $delay outSlew: $outSlew totpower: $totPower"
                        report_checks -fields transition -group_count 2

                        if {[info exists power($load-$delay-$dist-$outSlew-$slew-$inCap)]} {
                            if {$power($load-$delay-$dist-$outSlew-$slew-$inCap) > $totPower} {
                                set power($load-$delay-$dist-$outSlew-$slew-$inCap) $totPower
                                set sol($load-$delay-$dist-$outSlew-$slew-$inCap) $cur_sol
                            }
                        } else {
                            set power($load-$delay-$dist-$outSlew-$slew-$inCap) $totPower
                            set totList "$totList $load-$delay-$dist-$outSlew-$slew-$inCap"
                            set sol($load-$delay-$dist-$outSlew-$slew-$inCap) $cur_sol
                        }

                    }
                }
            }
        } else {
			puts "pureWire Solution"
            set cur_sol "$dist"
            set totPower $wirePower   
            for {set slew $inputSlew} {$slew <= $maxSlew} {set slew [expr $slew + $slewInter]} {
                set_assigned_transition -rise $slew [get_property  [get_pins $outPin] full_name]
                set_assigned_transition -fall $slew [get_property  [get_pins $outPin] full_name]
                for {set idx 0} {$idx < $outLoadNum} {incr idx} {
					read_spef ./spef_files/spef_${idx}.spef
                    puts "read_spef spef_${idx}.spef"
                    set load [lindex $loadList $idx]
                    set delay [format "%.3f" [pathDelay $outPin $inPin]]
	    	        set path [find_timing_paths -through $outPin ]
                    set points [get_property $path points]
                    set end [lindex $points [expr [llength $points] - 1]]
		    		set tr [get_property [get_property $end pin ] actual_rise_transition_max]
		    		set tf [get_property [get_property $end pin ] actual_fall_transition_max]
		    		set trans [expr [expr $tr + $tf]/2 ]
	            	set outSlew [expr int(( $trans + $slewInter/2.0)/$slewInter)*$slewInter]

                    if {$outSlew > [expr $maxSlew*2]} { continue }
                    
                    set pin "${drivenCell}/$D_ffpin"

		    		set inPinCap [lindex $loadList $idx]
			
                    set net net_${bin}_0
		            set inNetCap [expr ([get_netCap $net] / 1000)*$cap_unit]
                    
		    		set inCap [expr $inPinCap + $inNetCap]
                    
                    puts "(Debug-0) $cur_sol"
                    puts "(Debug-1) load: $load inpinCap: $inPinCap inNetCap: $inNetCap inSlew: $slew delay: $delay outSlew: $outSlew totpower: $totPower"
                    report_checks -fields transition -group_count 2

                    if {$inCap <= $baseLoad} {
						set inCap [expr int(($inCap + ($initial_cap_interval/2)*$cap_unit)/($initial_cap_interval*$cap_unit))*($initial_cap_interval*$cap_unit)]
                    } else {
                        set inCap [expr int(($inCap + ($final_cap_interval/2)*$cap_unit)/($final_cap_interval*$cap_unit))*($final_cap_interval*$cap_unit)]
                    }

                    if {[info exists power($load-$delay-$dist-$outSlew-$slew-$inCap)]} {
                        if {$power($load-$delay-$dist-$outSlew-$slew-$inCap) > $totPower} {
                            set power($load-$delay-$dist-$outSlew-$slew-$inCap) $totPower
                            set sol($load-$delay-$dist-$outSlew-$slew-$inCap) $cur_sol
                        }
                    } else {
                        set power($load-$delay-$dist-$outSlew-$slew-$inCap) $totPower
                        set totList "$totList $load-$delay-$dist-$outSlew-$slew-$inCap"
                        set sol($load-$delay-$dist-$outSlew-$slew-$inCap) $cur_sol
                    }
                }
            }
        }
    }

    set outFp [open ${dist}.lut w]
    foreach cur $totList {
       puts $cur
       regsub -all {\-} $cur " " fSol
       set fSol "$power($cur) $fSol $sol($cur)"
       puts $fSol
       puts $outFp $fSol
    }
    close $outFp
}



