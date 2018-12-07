#! /usr/bin/tclsh

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

# !!
set bufName "BUF_XXXX"

# !!

set FFName "DFF_QXXXX"

set cellHeight "0.576"

# Not modified
set origX "10.0"
set origY "2.4"

set numbuf_loc [expr int($dist/$unit_buf_dist)]
set totNum [expr pow(2, $numbuf_loc)]
set outFp_v [open ${design}.v w]
set outFp_place [open ${design}_place.tcl w]

puts $outFp_v "module ${design}( clk );"
puts $outFp_v ""
puts $outFp_v " input clk;"
puts $outFp_v ""

set path ""
for {set i 0} {$i < $totNum} {incr i} {
    set bin [dec2bin $i $numbuf_loc]
    #puts $bin
    set cnt 0
    set wire "net_${bin}_${cnt}"
    puts $outFp_v " wire ${wire};"

    set path "$path $FFName ff_${bin}_0 ( .D(dummy), .CK(clk), .Q(${wire}) ); \n"
    set locX $origX
    set locY [expr $origY + $cellHeight*4*$i]
    puts $outFp_place "placeInstance ff_${bin}_0 $locX $locY R0 -placed"
    set pre_wire $wire
    incr cnt

    for {set j 0} {$j < [string length $bin]} {incr j} {
        if {[string index $bin $j]} {
            #puts "$bin $i $j"
            set wire "net_${bin}_${cnt}"
            puts $outFp_v " wire ${wire};"
            #set path "$path $bufName buf_${bin}_${cnt} ( .A(${pre_wire}), .Z(${wire})); \n"
            set path "$path $bufName buf_${bin}_${cnt} ( .A(${pre_wire}), .Y(${wire})); \n"
            set locX [expr $origX + $unit_buf_dist*($j + 1)]
            puts $outFp_place "placeInstance buf_${bin}_${cnt} $locX $locY R0 -placed"
            set pre_wire $wire
            incr cnt
        }
    }

    #set path "$path $bufName buf_${bin}_${cnt} ( .A(${pre_wire}), .Z(${wire})); \n"
    #set locX [expr $origX + $dist]
    #puts $outFp_place "placeInstance buf_${bin}_${cnt} $locX $locY R0 -placed"
    set locX [expr $origX + $dist + 3]
    puts $outFp_place "placeInstance ff_${bin}_1 $locX $locY R0 -placed"
    set path "$path $FFName ff_${bin}_1 ( .D(${wire}), .CK(clk), .Q(dummy) ); \n"
}
puts $outFp_v ""
puts $outFp_v $path
puts $outFp_v ""
puts $outFp_v "endmodule"
close $outFp_place
close $outFp_v

set outFp [open ${design}.sdc w]
puts $outFp "set sdc_version 2.0"
puts $outFp ""
puts $outFp "set_units -time ps -resistance kOhm -capacitance fF -voltage V -current mA"
puts $outFp "create_clock \[get_ports clk\]  -period 1  -waveform {0 0.5}"
close $outFp
