#!/usr/bin/tclsh
#////////////////////////////////////////////////////////////////////////////////////
#// Authors: Kwangsoo Han and Jiajia Li
#//          (Ph.D. advisor: Andrew B. Kahng),
#//          Many subsequent changes for open-sourcing were made by Mateus Fogaça
#//          (Ph.D. advisor: Ricardo Reis)
#//
#// BSD 3-Clause License
#//
#// Copyright (c) 2018, The Regents of the University of California
#// All rights reserved.
#//
#// Redistribution and use in source and binary forms, with or without
#// modification, are permitted provided that the following conditions are met:
#//
#// * Redistributions of source code must retain the above copyright notice, this
#//   list of conditions and the following disclaimer.
#//
#// * Redistributions in binary form must reproduce the above copyright notice,
#//   this list of conditions and the following disclaimer in the documentation
#//   and/or other materials provided with the distribution.
#//
#// * Neither the name of the copyright holder nor the names of its
#//   contributors may be used to endorse or promote products derived from
#//   this software without specific prior written permission.
#//
#// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#////////////////////////////////////////////////////////////////////////////////////

set tcl_precision 5

set loc(0)  "[expr _WIDTH_/2] [expr _HEIGHT_/2]"
set udist   15.0

# function to round values
proc roundH {val} {
    set num [expr round($val/0.136)]
    return  [expr $num*0.136]
}
proc roundV {val} {
    set num [expr round($val/1.2)]
    return  [expr $num*1.2]
}
proc calcDist {loc1 loc2} {
    return [expr abs([lindex $loc1 0] - [lindex $loc2 0]) + abs([lindex $loc1 1] - [lindex $loc2 1])]
}
###########################

# parse buffering solutions
set inFile [open sol_list.txt]
while {[gets $inFile line]>=0} {
    regsub -all "," [lindex $line 1] " " sol([lindex $line 0]) 
}
close $inFile
###########################

# parse buffering number on each branch
set inFile [open seg.txt]
while {[gets $inFile line]>=0} {
    set level [lindex $line 1]
    set buf_nums($level) [lreplace $line 0 4]
}
close $inFile
###########################

# parse generalized H tree solution
# generate netlist structure file and placement location file
set inFile    [open sol.txt]
set outFile_1 [open netlist.txt   w]
set outFile_2 [open locations.txt w]
puts $outFile_1 "tree_0 _ROOT_BUFF_ _CK_PORT_ ck_net_1_0_0"
puts $outFile_2 "tree_0 [roundH [lindex $loc(0) 0]] [roundV [lindex $loc(0) 1]]"
set max_level 0
while {[gets $inFile line]>=0} {

    # bottom level 
    if {[lindex $line 0] == "B"} {
        set tree  [lindex $line 1]
        set sinks [lreplace $line 0 1]
        puts $outFile_1 "B ck_net_[expr ${max_level}+1]_${tree}_0 $sinks"
        continue
    }

    # top level
    set tree  [lindex $line 0]
    set level [lindex $line 2]
    if {$level > $max_level} {
        set max_level $level 
    }
    # buffering solutions
    set bufs  [lindex $line 3]
    set buf_segs  ""
    set buf_cells ""
    foreach buf $bufs  {
        lappend buf_segs $sol($buf)
        foreach item $sol($buf) {
            if {[regexp {_BUFF_REGEX_} $item]} {
                lappend buf_cells $item
            }
        }
    }

    # sub trees
    set subtrees ""
    set buf_locs ""
    for {set i 4} {$i < [llength $line]} {incr i} {
        if {[lindex $line $i] == "B"} {
            incr i
            while {$i < [llength $line]} {
                lappend buf_locs "[expr [lindex [lindex $line $i] 0]*$udist] [expr [lindex [lindex $line $i] 1]*$udist]"
                incr i
            }
            break
        }
        set subtree [lindex $line $i]
        lappend subtrees $subtree
        incr i
        set loc($subtree) "[expr [lindex [lindex $line $i] 0]*$udist] [expr [lindex [lindex $line $i] 1]*$udist]"
    }

    # left branch
    
    # subtrees
    set temp ""
    lappend temp "0 tree $tree \{ [roundH [lindex $loc($tree) 0]] [roundV [lindex $loc($tree) 1]] \}"
    set trees "$tree" 
    for {set i [expr [llength $subtrees]/2 - 1]} {$i >= 0 } {incr i -1} {
        lappend trees [lindex $subtrees $i]
    }
    set cur_dist 0
    for {set i 0} {$i < [expr [llength $trees]-1]} {incr i} {
        set cur_dist [expr $cur_dist + [calcDist $loc([lindex $trees $i]) $loc([lindex $trees [expr $i+1]])]]
        lappend temp "$cur_dist tree [lindex $trees [expr $i+1]] \{ [roundH [lindex $loc([lindex $trees [expr $i+1]]) 0]] [roundV [lindex $loc([lindex $trees [expr $i+1]]) 1]] \}"
        puts $outFile_2 "tree_[lindex $trees [expr $i+1]] [roundH [lindex $loc([lindex $trees [expr $i+1]]) 0]] [roundV [lindex $loc([lindex $trees [expr $i+1]]) 1]]"
    }

    # buffers 
    set buf_cnt 0
    for {set i [expr [llength $buf_locs]/2-1]} {$i >= 0} {incr i -1} {
        # to ensure buffer drives fanouts
        set cur_dist [expr [calcDist $loc($tree) [lindex $buf_locs $i]]  - 0.01]
        lappend temp "$cur_dist buf buf_${tree}_a_$buf_cnt-[lindex $buf_cells $buf_cnt] \{[roundH [lindex [lindex $buf_locs $i] 0]] [roundV [lindex [lindex $buf_locs $i] 1]]\}"
        puts $outFile_2 "buf_${tree}_a_$buf_cnt [roundH [lindex [lindex $buf_locs $i] 0]] [roundV [lindex [lindex $buf_locs $i] 1]]"
        incr buf_cnt
    }

    set temp [lsort -real -increasing -index 0 [lsort -dictionary -decreasing -index 1 $temp]]


    # dummy buffers for turning  (need for buffers)
    set cnt 0
    set cur_dist 0
    set sz [llength $temp]
    for {set i 0} {$i < [expr $sz-1]} {incr i} {
        set loc1 [lindex [lindex $temp $i] end]
        set loc2 [lindex [lindex $temp [expr $i+1]] end]
        set dist1 [lindex [lindex $temp $i] 0]
        set dist2 [lindex [lindex $temp [expr $i+1]] 0]
        set h_dist [expr abs([lindex $loc2 0] -  [lindex $loc1 0])]
        set v_dist [expr abs([lindex $loc2 1] -  [lindex $loc1 1])]
        if {$h_dist > $v_dist && $v_dist > 2} {
            lappend temp "[expr ($dist1 + $dist2)/2.0] dummy dummy_${tree}_a_$cnt \{ [roundH [lindex $loc2 0]] [roundV [lindex $loc1 1]] \}"
            puts $outFile_2 "dummy_${tree}_a_${cnt} [roundH [lindex $loc2 0]] [roundV [lindex $loc1 1]]"
            set loc(dummy_${tree}_a_${cnt}) "[lindex $loc2 0] [lindex $loc1 1]"
        } elseif {$v_dist > $h_dist && $h_dist > 2} {
            lappend temp "[expr ($dist1 + $dist2)/2.0] dummy dummy_${tree}_a_$cnt \{ [roundH [lindex $loc1 0]] [roundV [lindex $loc2 1]] \}"
            puts $outFile_2 "dummy_${tree}_a_${cnt} [roundH [lindex $loc1 0]] [roundV [lindex $loc2 1]]"
            set loc(dummy_${tree}_a_${cnt}) "[lindex $loc1 0] [lindex $loc2 1]"
        }
        incr cnt
        set cur_dist [expr $cur_dist + $h_dist + $v_dist]
    }

    set temp [lsort -real -increasing -index 0 [lsort -dictionary -decreasing -index 1 $temp]]
    set temp [lreplace $temp 0 0]

    set cnt 0
    foreach item $temp {
        set type [lindex $item 1]
        set node [lindex $item 2]
        if {$type == "buf"} {
            if {$cnt == 0} {
                puts $outFile_1 "[lindex [split $node {\-}] 0] [lindex [split $node {\-}] 1] ck_net_${level}_${tree}_$cnt ck_net_${level}_${tree}_a_[expr $cnt+1]" 
            } else {
                puts $outFile_1 "[lindex [split $node {\-}] 0] [lindex [split $node {\-}] 1] ck_net_${level}_${tree}_a_$cnt ck_net_${level}_${tree}_a_[expr $cnt+1]" 
            }
            incr cnt
        } elseif {$type == "dummy"} {
            if {$cnt == 0} {
                puts $outFile_1 "$node DUMMY ck_net_${level}_${tree}_$cnt ck_net_${level}_${tree}_a_[expr $cnt+1]" 
            } else {
                puts $outFile_1 "$node DUMMY ck_net_${level}_${tree}_a_$cnt ck_net_${level}_${tree}_a_[expr $cnt+1]" 
            }
            incr cnt
        } elseif {$type == "tree"} {
            if {$cnt == 0} {
                puts $outFile_1 "tree_$node DUMMY ck_net_${level}_${tree}_$cnt ck_net_[expr ${level}+1]_${node}_0" 
            } else {
                puts $outFile_1 "tree_$node DUMMY ck_net_${level}_${tree}_a_$cnt ck_net_[expr ${level}+1]_${node}_0" 
            }
        }
    }

    # right branch
    
    # subtrees
    set temp ""
    lappend temp "0 tree $tree \{ [roundH [lindex $loc($tree) 0]] [roundV [lindex $loc($tree) 1]] \}"
    set trees "$tree" 
    for {set i [expr [llength $subtrees]/2]} {$i < [llength $subtrees]} {incr i} {
        lappend trees [lindex $subtrees $i]
    }
    set cur_dist 0
    for {set i 0} {$i < [expr [llength $trees]-1]} {incr i} {
        set cur_dist [expr $cur_dist + [calcDist $loc([lindex $trees $i]) $loc([lindex $trees [expr $i+1]])]]
        lappend temp "$cur_dist tree [lindex $trees [expr $i+1]] \{[roundH [lindex $loc([lindex $trees [expr $i+1]]) 0]] [roundV [lindex $loc([lindex $trees [expr $i+1]]) 1]]\}"
        puts $outFile_2 "tree_[lindex $trees [expr $i+1]] [roundH [lindex $loc([lindex $trees [expr $i+1]]) 0]] [roundV [lindex $loc([lindex $trees [expr $i+1]]) 1]]"
    }

    # buffers 
    set buf_cnt 0
    for {set i [expr [llength $buf_locs]/2]} {$i < [llength $buf_locs]} {incr i} {
        # to ensure buffer drives fanouts
        set cur_dist [expr [calcDist $loc($tree) [lindex $buf_locs $i]]  - 0.01]
        lappend temp "$cur_dist buf buf_${tree}_b_$buf_cnt-[lindex $buf_cells $buf_cnt] \{[roundH [lindex [lindex $buf_locs $i] 0]] [roundV [lindex [lindex $buf_locs $i] 1]]\}"
        puts $outFile_2 "buf_${tree}_b_$buf_cnt [roundH [lindex [lindex $buf_locs $i] 0]] [roundV [lindex [lindex $buf_locs $i] 1]]"
        incr buf_cnt
    }

    set temp [lsort -real -increasing -index 0 [lsort -dictionary -decreasing -index 1 $temp]]

    # dummy buffers for turning  (need for buffers)
    set cnt 0
    set cur_dist 0
    set sz [llength $temp]
    for {set i 0} {$i < [expr $sz-1]} {incr i} {
        set loc1 [lindex [lindex $temp $i] end]
        set loc2 [lindex [lindex $temp [expr $i+1]] end]
        set dist1 [lindex [lindex $temp $i] 0]
        set dist2 [lindex [lindex $temp [expr $i+1]] 0]
        set h_dist [expr abs([lindex $loc2 0] -  [lindex $loc1 0])]
        set v_dist [expr abs([lindex $loc2 1] -  [lindex $loc1 1])]
        if {$h_dist > $v_dist && $v_dist > 2} {
            lappend temp "[expr ($dist1 + $dist2)/2.0] dummy dummy_${tree}_b_$cnt \{ [roundH [lindex $loc2 0]] [roundV [lindex $loc1 1]] \}"
            puts $outFile_2 "dummy_${tree}_b_${cnt} [roundH [lindex $loc2 0]] [roundV [lindex $loc1 1]]"
            set loc(dummy_${tree}_b_${cnt}) "[lindex $loc2 0] [lindex $loc1 1]"
        } elseif {$v_dist > $h_dist && $h_dist > 2} {
            lappend temp "[expr ($dist1 + $dist2)/2.0] dummy dummy_${tree}_b_$cnt \{ [roundH [lindex $loc1 0]] [roundV [lindex $loc2 1]] \}"
            puts $outFile_2 "dummy_${tree}_b_${cnt} [roundH [lindex $loc1 0]] [roundV [lindex $loc2 1]]"
            set loc(dummy_${tree}_b_${cnt}) "[lindex $loc1 0] [lindex $loc2 1]"
        }
        incr cnt
        set cur_dist [expr $cur_dist + $h_dist + $v_dist]
    }

    set temp [lsort -real -increasing -index 0 [lsort -dictionary -decreasing -index 1 $temp]]
    set temp [lreplace $temp 0 0]

    set cnt 0
    foreach item $temp {
        set type [lindex $item 1]
        set node [lindex $item 2]
        if {$type == "buf"} {
            if {$cnt == 0} {
                puts $outFile_1 "[lindex [split $node {\-}] 0] [lindex [split $node {\-}] 1] ck_net_${level}_${tree}_$cnt ck_net_${level}_${tree}_b_[expr $cnt+1]" 
            } else {
                puts $outFile_1 "[lindex [split $node {\-}] 0] [lindex [split $node {\-}] 1] ck_net_${level}_${tree}_b_$cnt ck_net_${level}_${tree}_b_[expr $cnt+1]" 
            }
            incr cnt
        } elseif {$type == "dummy"} {
            if {$cnt == 0} {
                puts $outFile_1 "$node DUMMY ck_net_${level}_${tree}_$cnt ck_net_${level}_${tree}_b_[expr $cnt+1]" 
            } else {
                puts $outFile_1 "$node DUMMY ck_net_${level}_${tree}_b_$cnt ck_net_${level}_${tree}_b_[expr $cnt+1]" 
            }
            incr cnt
        } elseif {$type == "tree"} {
            if {$cnt == 0} {
                puts $outFile_1 "tree_$node DUMMY ck_net_${level}_${tree}_$cnt ck_net_[expr ${level}+1]_${node}_0" 
            } else {
                puts $outFile_1 "tree_$node DUMMY ck_net_${level}_${tree}_b_$cnt ck_net_[expr ${level}+1]_${node}_0" 
            }
        } else {
            puts $item
        }
    }
    
}
close $inFile
close $outFile_1
close $outFile_2
###########################

# fix overlaps
set inFile [open netlist.txt]
set netList ""
while {[gets $inFile line]>=0} {
    set node    [lindex $line 0]
    set in_net  [lindex $line 2]
    set out_net [lindex $line 3]

    if {![info exists fi($in_net)] && ![info exists fo($in_net)]} {
        lappend netList $in_net
    }
    if {![info exists fo($in_net)]} {
        set     fo($in_net) $node 
    } else {
        lappend fo($in_net) $node 
    }

    if {![info exists fi($out_net)] && ![info exists fo($out_net)]} {
        lappend netList $out_net
    }
    if {![info exists fi($out_net)]} {
        set     fi($out_net) $node 
    } else {
        lappend fi($out_net) $node 
    }
}
close $inFile

foreach net $netList {
    if {![info exists fo($net)]} {
        continue
    }
    foreach n1 $fo($net) {
        set fi($n1) ""
        if {![info exists fi($net)]} {
            continue
        }
        foreach n2 $fi($net) {
            lappend fi($n1) $n2
        }
    }
}

set inFile  [open locations.txt]
set nodeList ""
while {[gets $inFile line]>=0} {
    set node     [lindex $line 0]
    set x($node) [lindex $line 1]
    set y($node) [lindex $line 2]
    lappend nodeList $node
}
close $inFile
set outFile [open locations.txt w]
foreach node $nodeList {
    if {[info exists visited($x($node)-$y($node)]} {
        set fi_node [lindex $fi($node) 0]
        if {$x($fi_node) > $x($node) || [regexp {buf} $node]} {
            set x($node) [expr $x($node) + 0.272]
        } else {
            set x($node) [expr $x($node) - 0.272]
        }
    }
    set visited($x($node)-$y($node) 1
    puts $outFile "$node $x($node) $y($node)"
}
close $outFile
