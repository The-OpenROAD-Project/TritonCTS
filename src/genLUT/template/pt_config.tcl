set designName "_DESIGN_"

set dbPath "/HOME/XXX/YYY/db" 

set dbFiles [list * mydb1.db mydb2.db]

set unit 1000
set bufTypes "BUF_X20 BUF_X10"
   
set maxSlew [expr 0.060 * $unit]
set inputSlew [expr 0.005 * $unit]
set slewInter [expr 0.005 * $unit]
set outLoadNum 34
set baseLoad [expr 0.005 * $unit]
set loadInter [ expr 0.005 * $unit]

set dist _DIST_
set unitDist _UNITDIST_
