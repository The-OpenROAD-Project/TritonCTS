set designName _DESIGN_

set list_lib "liberty_file"

# cap_unit is 1 if your library cap unit is pF, 1000 if cap unit is fF
set cap_unit 1
# time_unit is 1 if your library time unit is ns, 1000 if time unit is ps
set time_unit 1

set bufTypes "list_clock_buffers"

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
set bufName "init_buffer"
set FFName "flip_flop"

set cellHeight "row_height_in_um"

# Change 1.0 to your library values. Use your lib units.
set incap_buf [expr 1.0 / (1000 * $cap_unit) ]
set incap_D_ff [expr 1.0 / (1000 * $cap_unit) ]
set incap_CK_ff [expr 1.0 / (1000 * $cap_unit) ]
set split_num 4	
set cap_per_unit_len [expr 1.0 / (1000 * $cap_unit) ]
set res_per_unit_len [expr 1.0 / (1000 * $cap_unit) ] # Assumes cap and res multipliers are the same

set dist _DIST_
set unitDist _UNITDIST_
