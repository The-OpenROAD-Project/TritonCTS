# Technology characterization

*** These steps are required per foundry/IP enablement, since the enablement is visible only under NDA, and often readable only by foundry-qualified tools. Please refer to [OpenROAD Flow and Notes](https://theopenroadproject.org/wp-content/uploads/2018/12/OpenROAD_Flow_and_Notes_Nov2018-v1p0-1.pdf) for a better understanding of supported technologies and current limitations / assumptions.

- Change directory to _genLUT_ folder.

```
$ cd ../genLUT
```

 - Edit _template/config.tcl_ to add paths to .lib files and the CTS buffer names.  Examples of path names and CTS buffer names
   are shown in the _config.tcl_ file provided in the unpacked release.
 
 - Edit _template/config.tcl_ to add paths to library files.  Examples of paths to library files are shown in the _config.tcl_ file provided in the unpacked release.
 
 - Edit _run_all.tcl_ to specify wire sizes and NDRs (non-default routing rules).
 
 - Edit _template/genTest.tcl_ to specify a buffer cell (variable _bufName_), a flip-flop cell (variable _FFName_) and their respective cell heights (variable _cellHeight_) and footprints (variable _path_).
 
 - Run the characterization script, _run_all.tcl_ (valid OpenSTA binary are required).

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


### Alpha Release

- For the alpha release, we replace the commercial tools with an open-sourced tool and scripts.
    * The previous version uses a commercial signoff STA tool (commSTA) to obtain delay and power, and a commercial P&R to obtain spef files. The liberty template is required to generate fake filp-flops with various load values.
    * The current version (alpha) uses OpenSTA to obtain delay and power, and the scripts we made generate spef files from given R and C per unit distance.
    * No need to have the liberty template.

| <img src="doc/error_dist_1.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 16nm* |

| <img src="doc/28nm_error_dist.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 28nm* |

| <img src="doc/65nm_error_dist.png" width=550px> |
|:--:|
| *Error distributions between the previous version vs alpha using in test_20_20_W2X with 65nm* |

