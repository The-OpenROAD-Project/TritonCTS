#! /usr/bin/tclsh

set distList "20 40 60 80"
#set distList "20"
set NDR "W2X"
set unit_dist 20

set start [clock seconds] 

foreach dist $distList {
    set dir "test_${dist}_${unit_dist}_${NDR}"
    set numbuf_loc [expr int(($dist)/$unit_dist)]
    set totNum [expr pow(2, $numbuf_loc)]

    set cellHeight 0.576
    set width [expr $dist + 30]
    set height [expr $cellHeight*4*$totNum*1.1]

    if {[file exist $dir]} {
        exec rm -rf $dir
    } 
    exec mkdir $dir
    exec cp -rf ./openSTA_template/genTest.tcl $dir/ 
    exec cp -rf ./openSTA_template/run_sta.tcl $dir/
    exec cp -rf ./openSTA_template/sta_proc.tcl $dir/
    exec cp -rf ./openSTA_template/config.tcl $dir/
    exec cp -rf ./openSTA_template/create_spef_files.tcl $dir/
    
    
    exec sed -i s/_DESIGN_/${dir}/g $dir/config.tcl
    exec sed -i s/_DIST_/${dist}/g $dir/config.tcl
    exec sed -i s/_UNITDIST_/${unit_dist}/g $dir/config.tcl
    	
    cd $dir
    exec chmod 750 create_spef_files.tcl
    exec chmod 750 genTest.tcl
    exec chmod 750 run_sta.tcl
    exec chmod 750 sta_proc.tcl
    exec chmod 750 config.tcl	
    #Generate netlist and .sdc and spef template for given distance and unit distance 
    exec ./genTest.tcl $dir $dist $unit_dist
    exec ./create_spef_files.tcl $dir
    puts "Running $dir"
    ##create spef files and characterize LUT
    catch {exec ../sta -f ./run_sta.tcl }
	
    # Do sizing and characterize LUT    
    cd ..
    puts "Completed $dir"
}

set end [clock seconds]
exec echo [expr {($end - $start)}] seconds


