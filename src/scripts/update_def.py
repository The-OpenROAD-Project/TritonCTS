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
from collections import defaultdict
from statistics import median
from math import floor, ceil

netlistFilePath				= 'netlist.txt'  
placementFilePath 			= 'locations.txt'  
originalPlacementFilePath 	= 'cell_locs.txt'  
defFile 					= 'place.def'

mapping 	= defaultdict(list)
nets 		= defaultdict(list)
placement 	= defaultdict(list)

#------------------------------------------------------------------------------
# Read netlist from
def readNetlistFile():
	with open(netlistFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split(' ')
			if terms[0] is "B":
				for i in range(2, len(terms)):
					nets[terms[1]].append([terms[i].rstrip("/_CK_PIN_"), "_CK_PIN_"])
			else:
				node 	= "ck_" + terms[0]
				libCell	= terms[1]
				inNet 	= terms[2]
				outNet	= terms[3]
				nets[inNet].append([node, "A"])
				nets[outNet].append([node, "_BUFF_OUT_PIN_"])
				mapping[node] = libCell
		nets["_CK_PORT_"].append(["PIN", "_CK_PORT_"])		

#------------------------------------------------------------------------------
# Read placement from locations file (CT file)
def readPlacementFile():
	with open(placementFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split(' ')
			placement["ck_" + terms[0]].append(terms[1])
			placement["ck_" + terms[0]].append(terms[2])

#------------------------------------------------------------------------------

# Read placement from locations file (Original placement)
def readOriginalPlacementFile():
	with open(originalPlacementFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split(' ')
			placement[terms[0]].append(terms[1])
			placement[terms[0]].append(terms[2])

#------------------------------------------------------------------------------
# Append clock components  
def writeComponents():
	for node, libCell in mapping.items():
		x = int(float(placement[node][0]) * 1000)
		y = int(float(placement[node][1]) * 1000)
		print(" - " + node + " " + libCell + " + PLACED ( " + str(x) + " " + str(y) + " ) N\n;")

#------------------------------------------------------------------------------
def writeNets():
	for net, components in nets.items():
		print("- " + net)	
		for node, pin in components:
			print("( " + node + " " + pin + " )")
		print(";")
	
#------------------------------------------------------------------------------

def writeGuides():
	f = open("g.guides", "w")

	gcellw = 5*1360
	gcellh = 5*1000
	tolx = 1500
	toly = 1200
	for net, components in nets.items():
		xlocs = []
		ylocs = []
		for node, pin in components:
			xlocs.append(float(placement[node][0])*1000)	
			ylocs.append(float(placement[node][1])*1000)
		xmax = max(xlocs)
		xmin = min(xlocs)
		ymax = max(ylocs)
		ymin = min(ylocs)
			
		f.write(net + "\n")
		f.write("(\n")

		guidesm1 = ""
		guidesm2 = ""
		guidesm3 = ""
		guidesm4 = ""
		if xmax-xmin > ymax-ymin: #trunk horizontal
			ycoord = median(ylocs)
			
			# build trunk...
			ygrid 		= floor(ycoord/gcellh)
			xgridmin	= floor(xmin/gcellw)	
			xgridmax	= floor(xmax/gcellw)
			
			guide = str((xgridmin-1)*gcellw) + " " + str((ygrid-1)*gcellh) + " " + str((xgridmax+2)*gcellw) + " " + str((ygrid+2)*gcellh)
			guidesm1 = guidesm1 + guide + " M1\n"
			guidesm2 = guidesm2 + guide + " M2\n"
			guidesm3 = guidesm3 + guide + " M3\n"
			guidesm4 = guidesm4 + guide + " M4\n"

			# build stems...
			for i in range(0, len(xlocs)):
				xgrid = floor(xlocs[i]/gcellw)
				ygridmin = min(ygrid, floor(ylocs[i]/gcellh))
				ygridmax = max(ygrid, floor(ylocs[i]/gcellh))	
				
				guide = str((xgrid-1)*gcellw) + " " + str((ygridmin-1)*gcellh) + " " + str((xgrid+2)*gcellw) + " " + str((ygridmax+2)*gcellh)
				guidesm1 = guidesm1 + guide + " M1\n"
				guidesm2 = guidesm2 + guide + " M2\n"
				guidesm3 = guidesm3 + guide + " M3\n"
				guidesm4 = guidesm4 + guide + " M4\n"
		else: # trunk vertical
			xcoord = median(xlocs)
			
			# build trunk...
			xgrid 		= floor(xcoord/gcellw)
			ygridmin	= floor(ymin/gcellh)	
			ygridmax	= floor(ymax/gcellh)
			
			guide = str((xgrid-1)*gcellw) + " " + str((ygridmin-1)*gcellh) + " " + str((xgrid+2)*gcellw) + " " + str((ygridmax+2)*gcellh);
			guidesm1 = guidesm1 + guide + " M1\n"
			guidesm2 = guidesm2 + guide + " M2\n"
			guidesm3 = guidesm3 + guide + " M3\n"
			guidesm4 = guidesm4 + guide + " M4\n"

			# build stems...
			for i in range(0, len(xlocs)):
				ygrid = floor(ylocs[i]/gcellh)
				xgridmin = min(xgrid, floor(xlocs[i]/gcellw))
				xgridmax = max(xgrid, floor(xlocs[i]/gcellw))	
				guide = str((xgridmin-1)*gcellw) + " " + str((ygrid-1)*gcellh - toly) + " " + str((xgridmax+2)*gcellw + tolx) + " " + str((ygrid+2)*gcellh)
				guidesm1 = guidesm1 + guide + " M1\n"
				guidesm2 = guidesm2 + guide + " M2\n"
				guidesm3 = guidesm3 + guide + " M3\n"
				guidesm4 = guidesm4 + guide + " M4\n"

		f.write(guidesm1);
		f.write(guidesm2);
		f.write(guidesm3);
		f.write(guidesm4);
		f.write(")\n")
	f.close()
#------------------------------------------------------------------------------

### main
readNetlistFile()
readPlacementFile()

with open(defFile) as fp:
		for line in fp:
			line = line.rstrip("\n")
			terms = line.split(" ")
			if "COMPONENTS" in line and "END" not in line:
				terms = line.split(" ")
				numComponents = int(terms[1]) + len(mapping)
				print("COMPONENTS " + str(numComponents) + " ;")
				writeComponents()
			elif "NETS" in line and "END" not in line and "SPECIAL" not in line: 
				terms = line.split(" ")
				numNets = int(terms[1]) + len(nets) - 1
				print("NETS " + str(numNets) + " ;")
				writeNets()
			elif "- _CK_PORT_" in line and not "NET" in line: # Skip the original clock net
				while ";" not in line:
					line = fp.readline()
			else: 
				print(line)

#readOriginalPlacementFile()
#writeGuides()
