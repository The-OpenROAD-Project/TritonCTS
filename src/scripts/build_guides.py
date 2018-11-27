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
import subprocess
import sys


netlistFilePath	= 'netlist.txt'  
placementFilePath	= 'clockNetPinGCell.txt'  
pinLayerFile = 'sinks.txt'
numLayers = 13

mapping			= defaultdict(list)
nets				= defaultdict(list)
netIsLeaf 	= defaultdict(list)
placement 	= defaultdict(list)
pinLayers   = defaultdict(list)

netsPostProc	= defaultdict(list) # these are nets wo/ dummy buffers

#------------------------------------------------------------------------------
# Read netlist from file
def readNetlistFile():
	nets['clk'].append(['PIN', 'clk'])
	clkx = sys.argv[1]
	clky = sys.argv[2]
	#placement['clk/clk'] = [clkx, clky]

	with open(netlistFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split(' ')
			if terms[0] is "B":
				netIsLeaf[terms[1]] = True
				#print("Is leaf " + terms[1])
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
				netIsLeaf[terms[1]].append(False)

#------------------------------------------------------------------------------

def readPinLayers():
	with open(pinLayerFile) as fp:
		for line in fp:
			if "normal cellHeight" in line or "def scale" in line:
				continue
			entries = line.split()
			pinLayers[entries[0]] = entries[3]

#------------------------------------------------------------------------------

# Read placement from locations file (Original placement)
def readPlacementFile():
	with open(placementFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split()
			if len(terms) < 2:
				continue
			pin = terms[0]
			x = float(terms[1].replace("\n","").replace("(", "").replace(",",""))/1000
			y = float(terms[2].replace("\n","").replace(",",""))/1000
			placement[pin].append(x)
			placement[pin].append(y)
			pinLayers[pin] = int(int(terms[3].replace(")",""))/2)
			
#------------------------------------------------------------------------------

def appendPinGuides(guides, net):
	gcellw 	= int(sys.argv[3])
	gcellh 	= int(sys.argv[4])
	width 	= int(float(sys.argv[5]) * 1000)
	height	= int(float(sys.argv[6]) * 1000)

	for node, pin in nets[net]:
		if "clk" in pin:
			continue
		if node in mapping.keys() and "DUMMY" in mapping[node]:
			continue
		x = int(float(placement[node+"/"+pin][0]) * 1000)
		y = int(float(placement[node+"/"+pin][1]) * 1000)
		xgrid = floor(x/gcellw)
		ygrid = floor(y/gcellh)
		guide = str(max((xgrid)*gcellw,0)) + " " + str(max((ygrid)*gcellh,0)) + " " + str(min((xgrid+1)*gcellw, width)) + " " + str(min((ygrid+1)*gcellh,height))
		
		if pin == "_BUFF_OUT_PIN_" or pin == "A" or pin == "clk":
			guides[0] += guide + " M1\n"
			guides[1] += guide + " M2\n"
			guides[2] += guide + " M3\n"
		else:
			pinLayer = pinLayers[node+"/"+pin]
			minLayer = min(pinLayer, 3)
			maxLayer = max(pinLayer, 3)
			for i in range(minLayer, maxLayer+1):
				guides[i] += guide + " M" + str(i+1) + "\n"

	return guides	
#------------------------------------------------------------------------------

def writeGuidesLeafLevel():
	gcellw 	= int(sys.argv[3])
	gcellh 	= int(sys.argv[4])
	width 	= int(float(sys.argv[5]) * 1000)
	height	= int(float(sys.argv[6]) * 1000)

	pdFile = open("pd.in",  "w")
	f = open("g.guides", "a+")

	netNames = []	
	for net, components in nets.items():
		if not netIsLeaf[net]:
			continue
		netNames.append(net)
		pdFile.write("Net " + net + " " + str(len(components)) + "\n")

		i = 0
		for node, pin in components:
			pdFile.write(str(i) + " " + str(int(float(placement[node+"/"+pin][0])*1000)) + " " + str(int(float(placement[node+"/"+pin][1])*1000)) + "\n")
			i += 1
		pdFile.write("\n")
	pdFile.close()

	#subprocess.call("rm dump.txt", shell=True)	
	subprocess.call("../third_party/pd_rev -v 1 -bu 1 -hv 1 -f pd.in", shell=True)

	pdOutFile = open("dump.txt", "r")

	line = pdOutFile.readline()
	netCount = 0
	while line:
		#print(line)
		entry = line.split()
		terminals = []
		
		for i in range(int(entry[2])):
			entry2 = pdOutFile.readline().split()
			x = int(entry2[2].replace("(", ""))
			y = int(entry2[4].replace(")", ""))
			parent = int(entry2[6])
			terminals.append([x, y, parent])

		f.write(netNames[netCount] + "\n")
		f.write("(\n")
		guides = []
		for layer in range(numLayers):
				guides.append("")
		#guidesm3 = ""
		#guidesm4 = ""
		for i in range(1, len(terminals)):
				x = terminals[i][0]
				y = terminals[i][1]
				parent = terminals[i][2]
				xPar = terminals[parent][0]
				yPar = terminals[parent][1]
 
				xmin = min(x, xPar)
				ymin = min(y, yPar)
				xmax = max(x, xPar)
				ymax = max(y, yPar)
				
				xgrid		  = floor(x/gcellw)
				ygridmin	= floor(ymin/gcellh)	
				ygridmax	= floor(ymax/gcellh)
				guide = str(max((xgrid)*gcellw,0)) + " " + str(max((ygridmin)*gcellh,0)) + " " + str(min((xgrid+1)*gcellw, width)) + " " + str(min((ygridmax+1)*gcellh, height));
				guides[3] += guide + " M4\n" 

				ygrid 		= floor(yPar/gcellh)
				xgridmin	= floor(xmin/gcellw)	
				xgridmax	= floor(xmax/gcellw)
				guide = str(max((xgridmin)*gcellw,0)) + " " + str(max((ygrid)*gcellh,0)) + " " + str(min((xgridmax+1)*gcellw, width)) + " " + str(min((ygrid+1)*gcellh,height))
				guides[2] += guide + " M3\n"
		
		guides = appendPinGuides(guides, netNames[netCount])

		for layer in range(numLayers):
			f.write(guides[layer]);	
		#f.write(guidesm3);
		#f.write(guidesm4);
		f.write(")\n")
		netCount += 1
		#print(netCount)
		line = pdOutFile.readline()
		
#------------------------------------------------------------------------------

def writeGuides():
	f = open("g.guides", "w")
    
	gcellw 	= int(sys.argv[3])
	gcellh 	= int(sys.argv[4])
	width 	= int(float(sys.argv[5]) * 1000)
	height	= int(float(sys.argv[6]) * 1000)
	enablePD = int(sys.argv[7])
	for net, components in nets.items():
		if enablePD == 1 and netIsLeaf[net]:
			continue

		xlocs 	= []
		ylocs 	= []
		isReal 	= []
		for node, pin in components:
			#print("node " + node + "\n")
			print(node + "/" + pin)
			xlocs.append(float(placement[node+"/"+pin][0])*1000)	
			ylocs.append(float(placement[node+"/"+pin][1])*1000)
			if node in mapping.keys() and "DUMMY" in mapping[node]:
				isReal.append(False)
			else:
				isReal.append(True)

		xmax = max(xlocs)
		xmin = min(xlocs)
		ymax = max(ylocs)
		ymin = min(ylocs)
			
		f.write(net + "\n")
		f.write("(\n")

		guides = []
		for layer in range(numLayers):
				guides.append("")
		#guidesm3 = ""
		#guidesm4 = ""
		if xmax-xmin > ymax-ymin: #trunk horizontal
			ycoord = median(ylocs)
			
			# build trunk...
			ygrid 		= floor(ycoord/gcellh)
			xgridmin	= floor(xmin/gcellw)	
			xgridmax	= floor(xmax/gcellw)
			
			guide = str(max((xgridmin)*gcellw,0)) + " " + str(max((ygrid)*gcellh,0)) + " " + str(min((xgridmax+1)*gcellw, width)) + " " + str(min((ygrid+1)*gcellh,height))
			guides[2] = guides[2] + guide + " M3\n"

			# build stems...
			for i in range(0, len(xlocs)):
				xgrid = floor(xlocs[i]/gcellw)
				ygridmin = min(ygrid, floor(ylocs[i]/gcellh))
				ygridmax = max(ygrid, floor(ylocs[i]/gcellh))	
				
				guide = str(max((xgrid)*gcellw,0)) + " " + str(max((ygridmin)*gcellh,0)) + " " + str(min((xgrid+1)*gcellw, width)) + " " + str(min((ygridmax+1)*gcellh, height))
				guides[3] = guides[3] + guide + " M4\n"

		else: # trunk vertical
			xcoord = median(xlocs)
			
			# build trunk...
			xgrid 		= floor(xcoord/gcellw)
			ygridmin	= floor(ymin/gcellh)	
			ygridmax	= floor(ymax/gcellh)
			
			guide = str(max((xgrid)*gcellw,0)) + " " + str(max((ygridmin)*gcellh,0)) + " " + str(min((xgrid+1)*gcellw, width)) + " " + str(min((ygridmax+1)*gcellh, height));
			guides[3] = guides[3] + guide + " M4\n"

			# build stems...
			for i in range(0, len(xlocs)):
				ygrid = floor(ylocs[i]/gcellh)
				xgridmin = min(xgrid, floor(xlocs[i]/gcellw))
				xgridmax = max(xgrid, floor(xlocs[i]/gcellw))	
				guide = str(max((xgridmin)*gcellw,0)) + " " + str(max((ygrid)*gcellh,0)) + " " + str(min((xgridmax+1)*gcellw, width)) + " " + str(min((ygrid+1)*gcellh, height))
				guides[2] = guides[2] + guide + " M3\n"

				if isReal[i]:
					nodegridx = floor(xlocs[i]/gcellw)
					nodegridy = floor(ylocs[i]/gcellh)
					guide = str(max((nodegridx)*gcellw,0)) + " " + str(max((nodegridy)*gcellh,0)) + " " + str(min((nodegridx+1)*gcellw, width)) + " " + str(min((nodegridy+1)*gcellh, height))
					guides[3] = guides[3] + guide + " M4\n"

		guides = appendPinGuides(guides, net)
		for layer in range(numLayers):
			f.write(guides[layer]);	
		f.write(")\n")
    
	f.close()

	if enablePD == 1:	
		writeGuidesLeafLevel()
#------------------------------------------------------------------------------

### main
readNetlistFile()
readPlacementFile()
#readPinLayers()

writeGuides()
