#! /usr/bin/tclsh
set_units -capacitance fF -resistance kOhm -time ps -voltage V -current uA -power mw
set sta_report_default_digits 6

source config.tcl
source sta_proc.tcl

set design $designName


set sta_crpr_enabled 1

set list_lib "$lib"
set link_library $list_lib
set target_library $list_lib

set power_default_signal_toggle_rate 2
#set_delay_calc arnoldi
#find_timing -full_update

foreach lib $list_lib {
    read_liberty  $lib
}

read_verilog $design\.v
current_design $design 
link_design $design 

read_sdc ./$design\.sdc
read_spef ./spef_files/spef_100\.spef

charTables $dist $unitDist
exit
