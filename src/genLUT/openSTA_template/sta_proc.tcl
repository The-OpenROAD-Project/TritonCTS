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

proc pathDelay {path} {

    set start_arrival [get_property $path startpoint_arrival]
    set end_arrival [get_property $path endpoint_arrival]

    set delay  [expr $end_arrival - $start_arrival]

    #puts "(Delay) $delay"
    return $delay
}

proc combinations2 {myList size} {
    ;# End recursion when size is 0 or equals our list size
    if {$size == 0} {return}
    if {$size == 1} {return $myList}

    set newList {}
    foreach item $myList {
        foreach addList [combinations2 $myList [expr {$size-1}]] {
            lappend newList [concat $item $addList]
        }
    }

    return $newList
}

proc getBufList {bufTypes bufNum} {

    if {$bufNum == 0} {
        return [list]
    } else {
        set bufIdx [list]
        for {set i 0} {$i < [llength $bufTypes]} {incr i} {
            lappend bufIdx $i
        }
        return [combinations2 $bufIdx $bufNum]
    }
}

##pwr_list "Internal Switching Leakage Total"
proc get_power {inst_nm} {
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

    return $pwr_list
    #    if {$type == "switching"} {
    #        return [lindex $pwr_list 1]
    #    } elseif {$type == "internal"} {
    #        return [lindex $pwr_list 0]
    #    } elseif {$type == "leakage"} {
    #        return [lindex $pwr_list 2]
    #    } elseif {$type == "total"} {
    #        return [lindex $pwr_list 3]
    #    }
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

            #puts "LINE370: PIN: [llength $line ]"
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

    #    set totList ""
    set loadList [list]
    for {set i 1} {$i <= $outLoadNum} {incr i} {
        if {$i <= 5} {
            set load_val [expr ($i/1000.0)*$cap_unit]
        } else {
            set load_val [expr $baseLoad + $loadInter*($i - 5)]
        }
        lappend loadList $load_val
    }
    puts "Loads: $loadList"

    #Use 1.0 instead of 2.0 until OpenSTA is fixed.
    set power_default_signal_toggle_rate 2

    for {set i 0} {$i < $totNum} {incr i} {
        set bin [listOptions $i $numbuf_loc 2]

        set outPin "ff_${bin}_0/$Q_ffpin"
        set drivenCell "ff_${bin}_1"
        set inPin "ff_${bin}_1/$D_ffpin"
        set bufNames [list]
        set cnt 1
        set preloc 0
        set segList [list]

        set wirePower  [lindex [get_power [get_property [get_fanin -to "net_${bin}_0" -levels 1 -only_cells] full_name]] 1]

        for {set j 0} {$j < [string length $bin]} {incr j} {
            if {[string index $bin $j]} {
                lappend bufNames "buf_${bin}_${cnt}"
                incr cnt
                lappend segList [expr ($j + 1)*$unit_dist - $preloc]
                set preloc [expr ($j + 1)*$unit_dist]
            }
        }

        lappend segList [expr $dist - $preloc]
        set optList [getBufList $bufTypes [expr $cnt - 1]]
        puts "Test $bin : $optList"

        if { [llength $optList] } {
            foreach curList $optList {
                puts "  $curList : [array size power]"

                set cur_sol ""

                for {set k 0} {$k < [llength $curList]} {incr k} {
                    set curBuf [lindex $bufTypes [lindex $curList $k]]
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

                set inPinCap [get_pincapmax "[lindex $bufNames 0]/$buff_inPin"]
                set inNetCap [expr ([get_netCap "net_${bin}_0"] / 1000)*$cap_unit]

                set inCap [expr $inPinCap + $inNetCap]
                if {$inCap <= $baseLoad} {
                    set inCap [expr int(($inCap + ($initial_cap_interval/2)*$cap_unit)/($initial_cap_interval*$cap_unit))*($initial_cap_interval*$cap_unit)]
                } else {
                    set inCap [expr int(($inCap + ($final_cap_interval/2)*$cap_unit)/($final_cap_interval*$cap_unit))*($final_cap_interval*$cap_unit)]
                }

                set outPinName [get_property [get_pins $outPin] full_name]

                for {set slew $inputSlew} {$slew <= $maxSlew} {set slew [expr $slew + $slewInter]} {
                    set_assigned_transition -rise $slew $outPinName
                    set_assigned_transition -fall $slew $outPinName
                    for {set idx 0} {$idx < $outLoadNum} {incr idx} {
                        read_spef ./spef_files/spef_${idx}.spef
                        
                        set path [find_timing_paths -through $outPin]
                        set end_pin [get_property $path endpoint]

                        set tr [get_property $end_pin actual_rise_transition_max]
                        set tf [get_property $end_pin actual_fall_transition_max]
                        set trans [expr ($tr + $tf)/2]
                        set outSlew [expr int(($trans + $slewInter/2)/$slewInter)*$slewInter]

                        # Skip rest of computation if not needed
                        if {$outSlew > [expr 2*$maxSlew]} { continue }

                        set load_val [lindex $loadList $idx]
                        set delay [format "%.3f" [pathDelay $path]]

                        set totPower $wirePower
                        foreach bufName $bufNames {
                            set totPower [expr $totPower + [lindex [get_power $bufName] 3]]
                        }

                        set psLoad [format "%.3f" $load_val]
                        set inCap [format "%.3f" $inCap]
                        set psSlew [format "%.3f" $slew]
                        set psIdx "$psLoad-$delay-$dist-$outSlew-$psSlew-$inCap"
                        if {[info exists power($psIdx)]} {
                            if {[lindex $power($psIdx) 0] > $totPower} {
                                set power($psIdx) {$totPower $cur_sol}
                            }
                        } else {
                            set power($psIdx) {$totPower $cur_sol}
                        }
                    }
                }
            }
        } else {
            #puts "pureWire Solution"
            set cur_sol "$dist"
            set totPower $wirePower
            set outPinName [get_property  [get_pins $outPin] full_name]
            for {set slew $inputSlew} {$slew <= $maxSlew} {set slew [expr $slew + $slewInter]} {
                set_assigned_transition -rise $slew $outPinName
                set_assigned_transition -fall $slew $outPinName
                for {set idx 0} {$idx < $outLoadNum} {incr idx} {
                    read_spef ./spef_files/spef_${idx}.spef
                    
                    set path [find_timing_paths -through $outPin]
                    set end_pin [get_property $path endpoint]

                    set tr [get_property $end_pin actual_rise_transition_max]
                    set tf [get_property $end_pin actual_fall_transition_max]
                    set trans [expr ($tr + $tf)/2]
                    set outSlew [expr int(( $trans + $slewInter/2.0)/$slewInter)*$slewInter]

                    if {$outSlew > [expr $maxSlew*2]} { continue }

                    set delay [format "%.3f" [pathDelay $path]]

                    set load_val [lindex $loadList $idx]

                    set inPinCap [lindex $loadList $idx]
                    set inNetCap [expr ([get_netCap "net_${bin}_0"] / 1000)*$cap_unit]

                    set inCap [expr $inPinCap + $inNetCap]

                    if {$inCap <= $baseLoad} {
                        set inCap [expr int(($inCap + ($initial_cap_interval/2)*$cap_unit)/($initial_cap_interval*$cap_unit))*($initial_cap_interval*$cap_unit)]
                    } else {
                        set inCap [expr int(($inCap + ($final_cap_interval/2)*$cap_unit)/($final_cap_interval*$cap_unit))*($final_cap_interval*$cap_unit)]
                    }
                    
                    set psLoad [format "%.3f" $load_val]
                    set inCap [format "%.3f" $inCap]
                    set psSlew [format "%.3f" $slew]
                    set psIdx "$psLoad-$delay-$dist-$outSlew-$psSlew-$inCap"
                    if {[info exists power($psIdx)]} {
                        if {[lindex $power($psIdx) 0] > $totPower} {
                            set power($psIdx) {$totPower $cur_sol}
                        }
                    } else {
                        set power($psIdx) {$totPower $cur_sol}
                    }
               }
            }
        }
    }

    set outFp [open ${dist}.lut w]
    foreach cur [array names power] {
        regsub -all {\-} $cur " " fSol
        set fSol "[lindex $power($cur) 0] $fSol [lindex $power($cur) 1]"
        puts $outFp $fSol
    }
    close $outFp
}

