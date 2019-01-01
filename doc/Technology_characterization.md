# Technology characterization

*** These steps are required per foundry/IP enablement, since the enablement is visible only under NDA, and often readable only by foundry-qualified tools. Please refer to [OpenROAD Flow and Notes](https://theopenroadproject.org/wp-content/uploads/2018/12/OpenROAD_Flow_and_Notes_Nov2018-v1p0-1.pdf) for a better understanding of supported technologies and current limitations / assumptions.

- Change directory to _genLoad_ folder.
```
$ cd src/scripts/genLoad
```

- Edit _temp_lump_header.txt_ to match the header of your Liberty file.

- Edit _temp_lump_cell.txt_ to match the description of a flip-flop cell in your Liberty file.

- In previous steps, please note that your files must keep the _\_CORNER\__, _\_VOLT\__, _\_TEMP\__, _\_LTEMP\__ variables seen in the .txt files. 

- Run the _gen_load_lib.tcl_ script. This will generate a Liberty file with artificial loads; the artificial loads will eventually form the basis of a lookup table used by TritonCTS.

- Use Synopsys Library Compiler to create a .db from the Liberty file.  The command name to use in version O-2018.06 is seen on page 86 (out of 1134 pages) in the Library Compiler user Guide.

- Change directory to _genLUT_ folder.

```
$ cd ../genLUT
```

 - Edit _template/pt_config.tcl_ to add paths to .db files and the CTS buffer names.  Examples of path names and CTS buffer names are shown in the _pt_config.tcl_ file provided in the unpacked release.
 
 - Edit _template/pt_config.tcl_ to add paths to library files.  Examples of paths to library files are shown in the _pt_config.tcl_ file provided in the unpacked release.
 
 - Edit _run_all.tcl_ to specify wire sizes and NDRs (non-default routing rules).
 
 - Edit _template/genTest.tcl_ to specify a buffer cell (variable _bufName_), a flip-flop cell (variable _FFName_) and their respective cell heights (variable _cellHeight_) and footprints (variable _path_).
 
 - Run the characterization script, _run_all.tcl_ (valid Cadence Innovus and Synopsys PrimeTime licenses are required).

- After _run_all.tcl_ script has finished, make sure that a file named _XX.lut_ exists under each generated folder. The characterization folders have the naming convention: _test_XX_YY_NDR_, where _XX_ is the dist, _YY_ is the _unit_dist_ and _NDR_*** is the non-default rule). When characterizing for larger values of _dist_ (e.g. _dist_=80), and smaller values of _unit_dist_ (e.g. _unit_dist_=10) on 8 cores the expected runtime is around than 72 hours or more depending on your local setup.
*** Please note that as of January 2019 TritonCTS will only produce single-width clock routes even if NDRs are specified in the library characterization.

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
