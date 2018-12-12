#!/usr/bin/tclsh

set libList  "ss-0.90-125"

foreach lib $libList {
    set unit 1000
    set corner [lindex [split $lib {-}] 0]
    set volt [lindex [split $lib {-}] 1]
    set ltemp [lindex [split $lib {-}] 2]
    regsub {m} $ltemp "-" temp
    set outFile [open load_${volt}V_${ltemp}C.lib w]
    set inFile  [open temp_lump_header.txt]
    while {[gets $inFile line]>=0} {
        regsub _CORNER_ $line $corner line
        regsub _VOLT_  $line $volt line
        regsub _TEMP_  $line $temp line
        regsub _LTEMP_ $line $ltemp line
        puts $outFile $line
    }
    close $inFile
    set idx 0
    #set maxCap [expr 0.003724*20]
    set maxCap [expr 0.200 * $unit] 
    set offset [expr 0.005 * $unit]
    for {set cap [expr 0.001 * $unit]} {$cap <= [expr 0.005 * $unit]} {set cap [expr $cap+0.001*$unit]} {
        set inFile  [open temp_lump_cell.txt]
        while {[gets $inFile line]>=0} {
            regsub _IDX_ $line $idx line
            regsub _CAP_ $line $cap line
            puts $outFile $line
        }
        close $inFile
        incr idx
    }

    for {set cap [expr 0.010 * $unit]} {$cap <= $maxCap} {set cap [format "%.3f" [expr $cap+$offset]]} {
        set inFile  [open temp_lump_cell.txt]
        while {[gets $inFile line]>=0} {
            regsub _IDX_ $line $idx line
            regsub _CAP_ $line $cap line
            puts $outFile $line
        }
        close $inFile
        incr idx
    }

    set inFile  [open temp_lump_cell.txt]
    while {[gets $inFile line]>=0} {
        regsub _IDX_ $line 100 line
        regsub _CAP_ $line 0.00 line
        puts $outFile $line
    }
    close $inFile

    puts $outFile "\}"
    close $outFile
}

