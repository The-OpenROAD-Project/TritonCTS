#! /usr/bin/tclsh

#set distList "40 50 60"
#set distList "20"
#set distList "40"
#set distList "10 30 50 60"
set distList "20 40 60 80"
#set distList "50"
set NDR "W2X"
set unit_dist 20

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
    exec cp -rf ./template/genTest.tcl $dir/ 
    exec cp -rf ./template/run_pt.tbc $dir/
    exec cp -rf ./template/pt_proc.tbc $dir/
    exec cp -rf ./template/run_soce.tbc $dir/
    exec cp -rf ./template/libtbcload1.7.so $dir/

    exec cp -rf ./template/pt_config.tcl $dir/
    exec sed -i s/_DESIGN_/${dir}/g $dir/pt_config.tcl
    exec sed -i s/_DIST_/${dist}/g $dir/pt_config.tcl
    exec sed -i s/_UNITDIST_/${unit_dist}/g $dir/pt_config.tcl
    
    exec cp -rf ./template/soce_config.tcl $dir/
    exec sed -i s/_DESIGN_/${dir}/g $dir/soce_config.tcl
    exec sed -i s/_NONDR_/${NDR}/g $dir/soce_config.tcl
    exec sed -i s/_WIDTH_/${width}/g $dir/soce_config.tcl
    exec sed -i s/_HEIGHT_/${height}/g $dir/soce_config.tcl

    cd $dir
    # Generate netlist and .sdc for given distance and unit distance 
    exec ./genTest.tcl $dir $dist $unit_dist

    # Run P&R and extract SPEF 
    catch {exec innovus -64 -nowin -init run_soce.tbc}

    # Do sizing and characterize LUT    
    catch {exec pt_shell -f run_pt.tbc | tee pt.log}
    cd ..
}




