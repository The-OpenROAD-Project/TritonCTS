set designName _DESIGN_

set list_lib "/home/zf4_techdata/arm_nda/libraries/arm/tsmc/cln16fcll001/sc9mcpp96c_base_lvt_c16/r2p0/lib/sc9mcpp96c_cln16fcll001_base_lvt_c16_tt_typical_max_0p80v_25c.lib"

set cap_unit 1000
set time_unit 1000

set bufTypes "BUFH_X4N_A9PP96CTL_C16 BUFH_X16N_A9PP96CTL_C16 BUFH_X32N_A9PP96CTL_C16"

set maxSlew [expr 0.060 * $time_unit]
set inputSlew [expr 0.005 * $time_unit]
set slewInter [expr 0.005 * $time_unit]
set outLoadNum 34
set baseLoad [expr 0.005 * $cap_unit]
set loadInter [ expr 0.005 * $cap_unit]
set initial_cap_interval 0.001
set final_cap_interval 0.005
set Q_ffpin "Q"
set D_ffpin "D"
set buff_inPin "A"
set buff_outPin "Y"
set clk_pin "CK"

#initial buf/FF name (use the smallest size of buf/ff)
set bufName "BUFH_X4N_A9PP96CTL_C16"
set FFName "DFFQ_X4N_A9PP96CTL_C16"

set cellHeight "0.576"

set incap_buf 1.8 
set incap_D_ff 0.67
set incap_CK_ff 0.696
set split_num 4	

set cap_per_unit_len 0.17779
set res_per_unit_len 0.0156

set dist _DIST_
set unitDist _UNITDIST_
