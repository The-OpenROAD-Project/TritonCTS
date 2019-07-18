# Compile and run TritonCTS

- Change directory to the repository top folder "TritonCTS".

- Make sure you have files _src/tech/lut-XX.txt_ and _/src/tech/sol_list-XX.txt_, where _XX_ is your target enablement. Refer to the [Library Characterization]() tutorial otherwise. TritonCTS library characterization has been validated on TSMC65, ST28 and TSMC16, which would correspond respectively to XX = 65, 28 and 16.

- Compile the source code:
``` bash
$ ./compileAll.sh <path/to/lemon> <num_cores>
```

- Create a config file, e.g. _cts.cfg_, to specify the input parameters. See an example and description below:

## Example of a config file for TritonCTS
``` tcl
lef /path/to/file.lef
path /path/to/file.def
verilog /path/to/verilog
design my_design
target_skew 150
clkx 69.09 
clky 140.544
tech 16
width 140.736
height 140.544
ck_port clk
db_units 2000
root_buffer BUF_X16
toler 0
```

- Parameters description:
    - _lef_, _path_ and _verilog_ are system paths to the input files;
    - _design_ is a string value specifying the name of the design top module in DEF and Verilog;
    - _target_skew_ is an integer value specifying the target skew in ps;
    - _width_ and _height_ are float values specifying the core dimensions in um;
    - _clkx_ and _clky_ are float values specifying the clock entry point in _um_;
    - _tech_ is an integer number specifying the technology node. The values 16 and 28 are currently available, corresponding to TSMC16 and ST28, respectively;
    - _ck_port_ a string representing the clock port name in your design;
    - _db_units_ is an integer value representing your DEF db units;
    - _root_buffer_ is a string value naming the library cell of the root buffer;
    - _toler_ is an integer value that represents a tradeoff of QoR and runtime. Higher values will produce smaller runtime but worse QoR.

- Run TritonCTS:
``` bash
$ ./runTritonCTS.tcl -configFilePath=/path/to/configFile.cfg
```

- A folder named as _DESIGN_TSKEW_NODE_ will be created, where _DESIGN_ is the name of your design; _TSKEW_ is the target skew and _NODE_ is the technology node -- E.g.,  _jpeg_encoder_150_28_. In this folder you may find the output files: final.v and cts_final.def.
