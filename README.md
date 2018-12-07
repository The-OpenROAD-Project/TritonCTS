# TritonCTS

How to run our tool:
1) Download the repo.
```
$ git clone https://github.com/abk-openroad/TritonCTS.git
```

2) Technology characterization (these steps are required per foundry/IP enablement, since the enablement is visible only under NDA, and often readable only by foundry-qualified tools):

- Change directory to genLoad folder.
```
$ cd src/scripts/genLoad
```

- Edit "temp_lump_header.txt" to match the header of your Liberty file.

- Edit "temp_lump_cell.txt" to match the description of a flip-flop cell in your Liberty file.

- In previous steps, please note that your files must keep the \_CORNER\_, \_VOLT\_, \_TEMP\_, \_LTEMP\_ variables seem in the .txt files. 

- Run the gen_load_lib.tcl script. This will generate a Liberty file with artificial loads; the artificial loads will eventually form the basis of a lookup table used by TritonCTS.

- Use Synopsys Library Compiler to create a .db from the Liberty file.  The command name in version O-2018.06 is seen on page 86 (out of 1134 pages) in the Library Compiler user guide.

- Change directory to genLUT folder.

```
$ cd ../genLUT
```
 - Edit template/pt_config.tcl to add paths to .db files and the CTS buffer names.  Examples of path names and CTS buffer names are shown in the pt_config.tcl file provided in the unpacked release.
 
 - Edit template/pt_config.tcl to add paths to library files.  Examples of paths to library files are shown in the pt_config.tcl file provided in the unpacked release.
 
 - Edit run_all.tcl to specify wire sizes and NDRs (non-default routing rules).
 
 - Edit template/genTest.tcl to specify a buffer cell (variable bufName), a flip-flop cell (variable FFName) and their respective cell heights (variable cellHeight) and footprints (variable path).
 
 - Run the characterization script, run_all.tcl (valid Cadence Innovus and Synopsys PrimeTime licenses are required)

```
$ ./run_all.tcl
```
