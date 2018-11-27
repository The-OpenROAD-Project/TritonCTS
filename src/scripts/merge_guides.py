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
placementFilePath 	= 'cell_locs_final.txt'  
guideFile			= 'g.guides'

nets 			= defaultdict(list) # these are nets with dummy buffers
netsPostProc	= defaultdict(list) # these are nets wo/ dummy buffers
buffers			= defaultdict(list)
guides 			= defaultdict(list)
rootNets 		= defaultdict(str)
guidesPostProc  = defaultdict(list)

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

def readGuides():
	with open(guideFile) as fp:
		for line in fp:
			net = line.rstrip("\n")
			line = fp.readline()
			while True:
				line = fp.readline().rstrip("\n")
				if ")" in line:
					break
				guides[net].append(line)

#------------------------------------------------------------------------------

def postProcNets():
	toVisit = Queue()
	toVisit.put("ck_tree_0")

	rootNets['clk'] = 'clk'
	while not toVisit.empty():
		currNode = toVisit.get()
		#print ("Curr node: " + currNode)

		netsToVisit = Queue()
		rootNet = buffers[currNode][2]
		netsPostProc[rootNet].append([currNode, '_BUFF_OUT_PIN_']) 
		netsToVisit.put(rootNet)
		rootNets[rootNet] = rootNet
		#print(rootNet + " --> " + rootNet)

		while not netsToVisit.empty():
			currNet = netsToVisit.get()
			driver, dPin = nets[currNet][0]

			rootNets[currNet] = rootNet;
			#print(currNet + " --> " + rootNet)

			for i in range(1, len(nets[currNet])):
				node, pin = nets[currNet][i]
				
				#print("\tTesting sink: " + node)
				if not node in buffers:
					continue
				elif "DUMMY" in buffers[node][0]:
					#print("\t\tis Dummy!")
					netsToVisit.put(buffers[node][2])
				else:
					#print("\tSink: " + node)
					toVisit.put(node)	

#------------------------------------------------------------------------------

def mergeGuides():
	for net, guidesList in guides.items():
		rootNet = rootNets[net]
		guidesPostProc[rootNet].extend(guidesList)

	for net, guidesList in guidesPostProc.items():
		print(net)
		print("(")
		for guide in guidesList:
			print(guide)
		print(")")

#------------------------------------------------------------------------------

### main
readNetlistFile()
postProcNets()
readGuides()
mergeGuides()
