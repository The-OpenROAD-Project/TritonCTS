# Technology characterization

*** These steps are required per foundry/IP enablement, since the enablement is visible only under NDA, and often readable only by foundry-qualified tools. Please refer to [OpenROAD Flow and Notes](https://theopenroadproject.org/wp-content/uploads/2018/12/OpenROAD_Flow_and_Notes_Nov2018-v1p0-1.pdf) for a better understanding of supported technologies and current limitations / assumptions.

- Change directory to _genLUT_ folder.

```
$ cd ../genLUT
```

 - Update the following variables in _template/config.tcl_ accordingly:
 
   * list_lib - Path to you library files.
     * E.g.: _set list_lib "path/to/my/library.lib"_
   * cap_unit - Must set to 1 if your library has _pF_ as capacitance unit or 1000 if your library has _fF_ as capacitance unit.
     * E.g.: _set cap_unit 1_
   * time_unit - Must set to 1 if your library has _ns_ as timing unit or 1000 if your library has _ps_ as timing unit.
     * E.g.: _set time_unit 1_
   * bufTypes - List of standard cells to be used as clock buffers.
     * E.g.: _set bufTypes "BUF_X1 BUF_X2 BUF_X4 BUF_X16 BUF_X32"_
   * Q_ffpin - Name of the output data pin of the library flip-flops.
     * E.g.: _set Q_ffpin "Q"_
   * D_ffpin - Name of the input data pin of the library flip-flips.
     * E.g.: _set D_ffpin "D"_
   * buff_inPin - Name of the input pin of the library buffers.
     * E.g: _set buff_inPin "A"_
   * buff_outPin - Name of the output pin of the library buffers.
     * E.g.: _set buff_outPin "Y"_
   * clk_pin - Clock signal pin name of the library flip-flips.
     * E.g.: _set clk_pin "CK_
   * bufName - Library buffer to be used in the beggining of the wire during characterization.
     * E.g.: _set bufName "BUF_X16"_
   * FFName - Library flip-flop to be used in the end of the wire during characterization. 
     * E.g.: _set FFName "FF_X2"_
   * cellHeight - Height of your library cells in _um_.
     * E.g.: _set cellHeight "0.5"_
   * cap_per_unit_len - Capacitance per unit length of your technology. Use your technology units and update only the value after the keyword _expr_
     * E.g.: _set cap_per_unit_len [expr 0.8 / (1000 * $cap_unit) ]_
   * res_per_unit_len - Resistance per unit length of your technology. Use your technology units and update only the value after the keyword _expr_
     * set res_per_unit_len [expr 0.9 / (1000 * $cap_unit) ] # Assumes cap and res multipliers are the same

* We encourage you to **DO NOT** change the following variables unless you **REALLY** know what you are doing.
   * maxSlew, inputSlew, slewInter - max, min and step for slew in characterization scripts.
     * E.g.: set maxSlew [expr 0.060 * $time_unit]
     * E.g.: set inputSlew [expr 0.005 * $time_unit]
     * E.g.: set slewInter [expr 0.005 * $time_unit]

   * outloadNum, baseLoad, loadInter - Number of loads, min load value and step for the characterization scripts. For the last two, only change the value after _expr_ using your library units.     
     * E.g.: set outLoadNum 34
     * E.g.: set baseLoad [expr 0.005 * $cap_unit]
     * E.g.: set loadInter [ expr 0.005 * $cap_unit]

 - Run the characterization script, _run_all.tcl_ (a valid OpenSTA binary under _genLUT_ is required).

- After _run_all.tcl_ script has finished, make sure that a file named _XX.lut_ exists under each generated folder. The characterization folders have the naming convention: _test_XX_YY_NDR_, where _XX_ is the dist, _YY_ is the _unit_dist_ and _NDR_ is the non-default rule. When characterizing for larger values of _dist_ (e.g. _dist_=80 um), on a single core the expected runtime is around 1 hour or less depending on your local setup.

- Edit variable _lutList_ in _genLUTOPt2.tcl_ with the paths of the _XX.lut_ files

- Run the _genLUTOPt2.tcl_ script:

```
$ ./genLUTOPt2.tcl > concat.lut
```

- Run _prep_lut.tcl_
```
$ ./prep_lut.tcl concat.lut
```

- Copy the result files, _sol_list.txt_ and _lut.txt_, to TritonCTS/src/tech folder, renaming them according to your technology node.

For example, if you are using ST28, you would rename as:
```
$ cp sol_list.txt ../../src/tech/sol_list-28.txt
```
```
$ cp lut.txt ../../src/tech/lut-28.txt
```

For example, if you are using TSMC16, you would rename as:
```
$ cp sol_list.txt ../../src/tech/sol_list-16.txt
```
```
$ cp lut.txt ../../src/tech/lut-16.txt
```

- To check your setup, please verify the following:
    *   Folders _test_XX_YY_NDR_
    *   Non-empty _XX.lut_ file under each _test_XX_YY_NDR_ folder.
    *   Non-empty _sol_list-XX.txt_ file under ../../src/tech
    *   Non-empty _lut-XX.txt_ file under ../../src/tech

- You may now [run TritonCTS]().


### Alpha Release

- For the alpha release, we replace the commercial tools with an open-sourced tool and scripts.
    * The previous version uses a commercial signoff STA tool (commSTA) to obtain delay and power, and a commercial P&R to obtain spef files. The liberty template is required to generate fake filp-flops with various load values.
    * The current version (alpha) uses OpenSTA to obtain delay and power, and the scripts we made generate spef files from given R and C per unit distance.
    * No need to have the liberty template.

| <img src="error_dist_1.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 16nm* |

| <img src="28nm_error_dist.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 28nm* |

| <img src="65nm_error_dist.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 65nm* |

