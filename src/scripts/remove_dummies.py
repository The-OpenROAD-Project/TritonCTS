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
from queue import *
from math import floor, ceil
import matplotlib.pyplot as plt
import random
from itertools import cycle
import subprocess 


netlistFilePath		= 'netlist.txt'  
placementFilePath 	= 'cell_locs_pre_leg.txt'
defFile 			= 'cts.def'
verilogFile			= 'place.v'
	
nets 			= defaultdict(list) # these are nets with dummy buffers
netsPostProc	= defaultdict(list) # these are nets wo/ dummy buffers
placement 		= defaultdict(list)
buffers			= defaultdict(list)
segments		= defaultdict(list)


#------------------------------------------------------------------------------
# Read netlist from locations.txt file
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
				nets[outNet].insert(0, [node, "_BUFF_OUT_PIN_"])
				buffers[node] = [libCell, inNet, outNet]

#------------------------------------------------------------------------------
# Read placement from CT-related components locations file 
def readPlacementFile():
	with open(placementFilePath) as fp:
		for line in fp:
			terms = line.rstrip("\n").split(' ')
			placement[terms[0]].append(terms[1])
			placement[terms[0]].append(terms[2])

#------------------------------------------------------------------------------

def writeNets():
	print("- _CK_PORT_")
	print("( PIN _CK_PORT_ )")
	print("( ck_tree_0 A )")
	print(";")

	for net, components in netsPostProc.items():
		print("- " + net)	
		for node, pin in components:
			print("( " + node + " " + pin + " )")
		print(";")
	
#------------------------------------------------------------------------------

def postProcNets():
	toVisit = Queue()
	toVisit.put("ck_tree_0")

	while not toVisit.empty():
		currNode = toVisit.get()
		#print ("Curr node: " + currNode)

		netsToVisit = Queue()
		rootNet = buffers[currNode][2]
		netsPostProc[rootNet].append([currNode, '_BUFF_OUT_PIN_']) 
		netsToVisit.put(rootNet)	

		while not netsToVisit.empty():
			currNet = netsToVisit.get()
			driver, dPin = nets[currNet][0]
			for i in range(1, len(nets[currNet])):
				node, pin = nets[currNet][i]
				
				#segments for routing guides
				if node in buffers:
					segments[rootNet].append([placement[driver], placement[node]])
				
				#print("\tTesting sink: " + node)
				if not node in buffers:
					#print("\tSink: " + node)
					netsPostProc[rootNet].append([node, '_CK_PIN_'])
				elif "DUMMY" in buffers[node][0]:
					#print("\t\tis Dummy!")
					netsToVisit.put(buffers[node][2])
				else:
					#print("\tSink: " + node)
					netsPostProc[rootNet].append([node, 'A'])
					toVisit.put(node)	
		#print("-----------------------------")

#------------------------------------------------------------------------------

def writeComponents():
	for buf, data in buffers.items():
		libCell = data[0]
		if "DUMMY" in libCell:
			continue
		x = int(float(placement[buf][0]) * 1000)
		y = int(float(placement[buf][1]) * 1000)
		print(" - " + buf + " " + libCell + " + PLACED ( " + str(x) + " " + str(y) + " ) N\n;")

#------------------------------------------------------------------------------

def dumpVerilog():
	inputs = defaultdict(str)
	outputs = defaultdict(str)
	for net, components in netsPostProc.items():
		outputs[components[0][0]] = net
		for i in range(1, len(components)):
			inputs[components[i][0]] = net 
	
	inputs['ck_tree_0'] = '_CK_PORT_'
	printedNets = False
	outputfile = open('final.v', 'w')
	with open('place-2.v') as fp:
		for line in fp:
			if "wire" in line and not printedNets:
				for net, components in netsPostProc.items():
					outputfile.write("  wire " + net + ";\n")	
				printedNets = True
			elif "endmodule" in line:
				for mod, out in outputs.items():
					libcell = buffers[mod][0]
					outputfile.write("  " + libcell + " " + mod + "( .A(" + inputs[mod] + "), ._BUFF_OUT_PIN_(" + outputs[mod]  + ") );\n")
			elif "._CK_PIN_" in line:		
				line = line.replace("_CK_PORT_", inputs[line.split()[1].replace("\\", "")])
			outputfile.write(line)
	outputfile.close()

#------------------------------------------------------------------------------

def computeNumBuffers():
	numBuff = 0
	for buf, data in buffers.items():
		libCell = data[0]
		if "DUMMY" in libCell:
			continue
		numBuff = numBuff + 1
	return numBuff

#------------------------------------------------------------------------------

### main
readNetlistFile()
readPlacementFile()
postProcNets()
dumpVerilog()

cmd0 = "grep \"COMPONENTS \" place.def | tail -1 | awk '{gsub(/[^0-9. ]/,\"\")}1'"
cmd1 = "grep \"NETS \" place.def | tail -1 | awk '{gsub(/[^0-9. ]/,\"\")}1'"

with open(defFile) as fp:
	for line in fp:
		line = line.rstrip("\n")
		terms = line.split(" ")

		if "- ck_" in line:
			while ";" not in line: 
				line = fp.readline();
			continue
		elif "COMPONENTS" in line and "END" not in line:
			terms = line.split(" ")
			numComponents = int(subprocess.check_output(cmd0, shell=True)) + computeNumBuffers()
			print("COMPONENTS " + str(numComponents) + " ;")
			writeComponents()
		elif "NETS" in line and "END" not in line and "SPECIAL" not in line: 
			terms = line.split(" ")
			numNets = int(subprocess.check_output(cmd1, shell=True)) + len(netsPostProc) - 1
			print("NETS " + str(numNets) + " ;")
			writeNets()
		elif "- _CK_PORT_" in line and not "NET" in line: # Skip the original clock net
			while ";" not in line:
				line = fp.readline()
		else: 
			print(line)
