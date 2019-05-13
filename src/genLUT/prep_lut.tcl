#!/usr/bin/tclsh

source openSTA_template/config.tcl

set in_file [lindex $argv 0]

set inFile    [open $in_file]
set outFile_1 [open sol_list.txt w]
set outFile_2 [open tmp.txt w] 
set idx 0
set min_dist 99999
set max_dist -1
set min_cap 99999
set max_cap -1
set min_slew 99999
set max_slew -1
set skew_limit [expr 0.200 * $time_unit]

set max_in_slew_ori 0
set max_out_slew_ori 0
while {[gets $inFile line]>=0} {
	set out_slew_ori [lindex $line 4]
	set in_slew_ori [lindex $line 5]

	if {$out_slew_ori > $skew_limit} {
		continue
	}

    if {$max_out_slew_ori < $out_slew_ori} {
	    set max_out_slew_ori $out_slew_ori
	}	
    if {$max_in_slew_ori < $in_slew_ori} {
	    set max_in_slew_ori $in_slew_ori
	}	


    # power unit = mW
    set power    [expr [lindex $line 0] * $cap_unit]
    # load (unit = 5fF)
    set load     [lindex $line 1]
    if {$load < [expr 0.005 * $cap_unit]} {
        set load [format %.0f [expr $load/(0.001*$cap_unit)]]
    } else {
        set load [expr [format %.0f [expr $load/(0.005*$cap_unit)]]+4]
    }
    if {$load < $min_cap} {
        set min_cap $load
    }
    if {$load > $max_cap} {
        set max_cap $load
    }
    # delay (unit = 1ps)
    set delay    [format %.0f [expr [lindex $line 2]/(0.001*$time_unit)]]
    # dist (unit = 10um)
    set dist     [format %.0f [expr [lindex $line 3]/20]]
    if {$dist < $min_dist} {
        set min_dist $dist
    }
    if {$dist > $max_dist} {
        set max_dist $dist
    }

    # output slew (unit = 5ps)
    set out_slew [format %.0f [expr [lindex $line 4]/0.005*$time_unit]]
    if {$out_slew < $min_slew} {
        set min_slew $out_slew
    }
    if {$out_slew > $max_slew} {
        set max_slew $out_slew
    }
    # input slew  (unit = 5ps)
    set in_slew  [format %.0f [expr [lindex $line 5]/0.005*$time_unit]]
    if {$in_slew < $min_slew} {
        set min_slew $in_slew
    }
    if {$in_slew > $max_slew} {
        set max_slew $in_slew
    }
    # input cap  (unit = 5fF)
    set cap [lindex $line 6]
    if {$cap < [expr 0.005 * $cap_unit]} {
        set cap [format %.0f [expr $cap/(0.001*$cap_unit)]]
    } else {
        set cap [expr [format %.0f [expr $cap/(0.005*$cap_unit)]]+4]
    }

    if {$cap < $min_cap} {
        set min_cap $cap
    }
    if {$cap > $max_cap} {
        set max_cap $cap
    }
    set sol      [lindex $line 7]
    set pure_wire 1
    if {[regexp BUF $sol]} {
        set pure_wire 0
    }
    set bufDList ""
    set totD     ""
    if {$pure_wire == 0} {
        set tmpList [split $sol ","]
        set tmpD 0.0
        foreach tmp $tmpList {
            if {![regexp {BUF} $tmp]} {
                set tmpD [expr $tmpD + $tmp]
                set totD [expr $totD + $tmp]
            } else {
                lappend bufDList $tmpD
                set tmpD 0.0
            }
        }
        set tmpList $bufDList
        set bufDList ""
        foreach tmp $tmpList {
            lappend bufDList [expr $tmp / $totD]
        }
    }
    puts $outFile_1 "$idx $sol"
    # last item = indicator of whether the solution is a pure-wire solution
    puts $outFile_2 "$idx $dist $load $out_slew $power $delay $cap $in_slew $pure_wire $bufDList"
    incr idx
}
close $inFile
close $outFile_1
close $outFile_2

puts "Max in_slew before norm: $max_in_slew_ori"
puts "Max out_slew before norm: $max_out_slew_ori"

set inFile  [open tmp.txt]
set outFile [open lut.txt w]
puts $outFile "$min_dist $max_dist $min_cap $max_cap $min_slew $max_slew"
while {[gets $inFile line]>=0} {
    puts $outFile $line
}
close $inFile
close $outFile

exec rm tmp.txt
