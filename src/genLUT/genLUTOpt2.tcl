#! /usr/bin/tclsh

set selPoints "0.1 0.5 0.9"

#set lutList "50.lut 100.lut 150.lut 200.lut 250.lut"
set lutList "20.lut 40.lut 60.lut 80.lut"

array set opt1Power {}
array set opt1inCap {}
array set opt1inSlew {}
array set totSol {}

set opt1List ""
foreach lut $lutList {
    set inFp [open $lut]
    while {[gets $inFp line] >= 0} {
        set power [lindex $line 0]
        set load [lindex $line 1]
        set delay [lindex $line 2]
        set dist [lindex $line 3]
        set outSlew [lindex $line 4]
        set inSlew [lindex $line 5]
        set inCap [lindex $line 6]
        set sol [lindex $line 7]

        #set opt1 "${dist}-${outSlew}-${load}"
        set opt1 "${dist}-${outSlew}-${load}-${inCap}"
        if {![info exists opt1Delay($opt1)]} {
            set opt1Delay($opt1) $delay 
            set opt1List "$opt1List $opt1"
        } else {
            set opt1Delay($opt1) "$opt1Delay($opt1) $delay"
        }

        if {![info exists opt1Power($opt1-${delay})]} {
            set opt1Power($opt1-${delay}) $power
        } else {
            set opt1Power($opt1-${delay}) "$opt1Power($opt1-${delay}) $power"
        }

        #if {![info exists opt1inCap($opt1-${delay}-${power})]} {
        #    set opt1inCap($opt1-${delay}-${power}) $inCap
        #} else {
        #    set opt1inCap($opt1-${delay}-${power}) "$opt1inCap($opt1-${delay}-${power}) $inCap"
        #}

        if {![info exists opt1inSlew($opt1-${delay}-${power})]} {
            set opt1inSlew($opt1-${delay}-${power}) $inSlew
        } else {
            set opt1inSlew($opt1-${delay}-${power}) "$opt1inSlew($opt1-${delay}-${power}) $inSlew"
        }

        #set cur "${opt1}-${delay}-${power}-${inSlew}-${inCap}"
        set cur "${opt1}-${delay}-${power}-${inSlew}"

        if {![info exists totSol($cur)]} {
            set totSol($cur) $sol
        } else {
            puts "ERROR!!: overlapping solutions"
        }
    }
    close $inFp
}

set opt1List [lsort -uniq $opt1List]
foreach opt1 $opt1List {
    if {[llength $opt1Delay($opt1)] <= [llength $selPoints]} {
        foreach cDelay $opt1Delay($opt1) {
            set cPower [lindex [lsort -real -increasing $opt1Power($opt1-${cDelay})] 0]
            #set cInCap [lindex $opt1inCap($opt1-${cDelay}-${cPower}) 0]
            set cInSlew [lindex $opt1inSlew($opt1-${cDelay}-${cPower}) end]
            #set cur "$opt1-${cDelay}-${cPower}-${cInSlew}-${cInCap}"
            set cur "$opt1-${cDelay}-${cPower}-${cInSlew}"
            set cSol $totSol($cur)
            regsub -all {\-} $opt1 " " temp 
            set cInCap [lindex $temp 3]
            set cLoad [lindex $temp 2]
            set cOutSlew [lindex $temp 1]
            set cDist [lindex $temp 0]
            set output "${cPower} ${cLoad} ${cDelay} ${cDist} ${cOutSlew} ${cInSlew} ${cInCap} ${cSol}"
            puts "$output"
        }
    } else {
        foreach loc $selPoints {
            set idx [expr int(floor($loc*1.0*[llength $opt1Delay($opt1)]))]
            set cDelay [lindex $opt1Delay($opt1) $idx]
            set cPower [lindex [lsort -real -increasing $opt1Power($opt1-${cDelay})] 0]
            #set cInCap [lindex $opt1inCap($opt1-${cDelay}-${cPower}) 0]
            set cInSlew [lindex $opt1inSlew($opt1-${cDelay}-${cPower}) end]
            #set cur "$opt1-${cDelay}-${cPower}-${cInSlew}-${cInCap}"
            set cur "$opt1-${cDelay}-${cPower}-${cInSlew}"
            set cSol $totSol($cur)
            regsub -all {\-} $opt1 " " temp 
            set cInCap [lindex $temp 3]
            set cLoad [lindex $temp 2]
            set cOutSlew [lindex $temp 1]
            set cDist [lindex $temp 0]
            set output "${cPower} ${cLoad} ${cDelay} ${cDist} ${cOutSlew} ${cInSlew} ${cInCap} ${cSol}"
            puts "$output"
        }
    }
}



