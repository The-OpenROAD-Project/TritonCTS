# TritonCTS

How to run our tool:
1) Download the repo.
```
$ git clone https://github.com/abk-openroad/TritonCTS.git
```


2) Technology characterization:

- Change directory to genLoad folder.

```
$ cd src/scripts/genLoad
```

- Edit "temp_lump_header.txt" to match the header of your liberty file. 

- Edit "temp_lump_cell.txt" to match the description of a flip flop cell in your liberty file.

- In previous steps, please note that your files must keep the \_CORNER\_, \_VOLT\_, \_TEMP\_, \_LTEMP\_ variables. 

- Run the gen_load_lib.tcl script. This will generate a liberty file with the fake loads.

- Use Synopsys Library Compiler to a .db from the liberty file.

- Change directory to genLUT folder.

```
$ cd ../genLUT
```
 - Edit template/pt_config.tcl to add paths to .db files a the CTS buffer names.
 
 - Edit template/pt_config.tcl to add paths to library files
 
 - Edit run_all.tcl to specify wire sizes and NDR.
 
 - Run the characterization script (Innovus and PrimeTime required)

```
$ cd ./run_all.tcl
```
 
