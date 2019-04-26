#!/usr/bin/tclsh


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
set unit 1000
while {[gets $inFile line]>=0} {
    # power unit = mW
    set power    [expr [lindex $line 0] * 1000]
    # load (unit = 5fF)
    set load     [lindex $line 1]
    if {$load < [expr 0.005 * $unit]} {
        set load [format %.0f [expr $load/1]]
    } else {
        set load [expr [format %.0f [expr $load/5]]+4]
    }
    if {$load < $min_cap} {
        set min_cap $load
    }
    if {$load > $max_cap} {
        set max_cap $load
    }
    # delay (unit = 1ps)
    set delay    [format %.0f [expr [lindex $line 2]/1]]
    # dist (unit = 10um)
    set dist     [format %.0f [expr [lindex $line 3]/20]]
    if {$dist < $min_dist} {
        set min_dist $dist
    }
    if {$dist > $max_dist} {
        set max_dist $dist
    }
    # output slew (unit = 5ps)
    set out_slew [format %.0f [expr [lindex $line 4]/5]]
    if {$out_slew < $min_slew} {
        set min_slew $out_slew
    }
    if {$out_slew > $max_slew} {
        set max_slew $out_slew
    }
    # input slew  (unit = 5ps)
    set in_slew  [format %.0f [expr [lindex $line 5]/5]]
    if {$in_slew < $min_slew} {
        set min_slew $in_slew
    }
    if {$in_slew > $max_slew} {
        set max_slew $in_slew
    }
    # input cap  (unit = 5fF)
    set cap [lindex $line 6]
    if {$cap < [expr 0.005 * $unit]} {
        set cap [format %.0f [expr $cap/1]]
    } else {
        set cap [expr [format %.0f [expr $cap/5]]+4]
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

set inFile  [open tmp.txt]
set outFile [open lut.txt w]
puts $outFile "$min_dist $max_dist $min_cap $max_cap $min_slew $max_slew"
while {[gets $inFile line]>=0} {
    puts $outFile $line
}
close $inFile
close $outFile

exec rm tmp.txt
