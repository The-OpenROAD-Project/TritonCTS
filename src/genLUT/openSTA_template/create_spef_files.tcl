#! /usr/bin/tclsh

source config.tcl

#set outLoadNum 44
#set cap_unit 1000
#set baseLoad [expr 0.005 * $cap_unit]
#set loadInter [ expr 0.005 * $cap_unit]

proc create_spef_files {design} {
	
	global outLoadNum
	global cap_unit
	global baseLoad 
	global loadInter
	global dir
   	global incap_D_ff
	
	if {![file exists spef_files]} {
		exec mkdir spef_files
	} else {
		exec rm -rf spef_files
		exec mkdir spef_files
	}

	for {set i 0} {$i <= $outLoadNum} {incr i} {
    	
		if {$i <= 5} {
            set load [expr ($i/1000.0)*$cap_unit - $incap_D_ff]
        } else {
            set load [expr $baseLoad + $loadInter*($i - 5) - $incap_D_ff]
        }
        #set loadList "$loadList $load"
        
        #100= 0 load
        if {$i == 0} {
            set idx 100
        } else {
            set idx [expr $i-1]
        }
        exec cp -rf ${design}.spef ./spef_files/spef_${idx}.spef
		exec sed -i s/_CAP_/$load/g ./spef_files/spef_${idx}.spef
		exec chmod 750 ./spef_files/spef_${idx}.spef
    }
}

set design [lindex $argv 0]
create_spef_files $design
