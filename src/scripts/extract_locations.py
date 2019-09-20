#!/usr/bin/env python3
#////////////////////////////////////////////////////////////////////////////////////
#// Authors: Kwangsoo Han and Jiajia Li
#//          (Ph.D. advisor: Andrew B. Kahng),
#//          Many subsequent changes for open-sourcing were made by Mateus Fogaca
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
import os
import re
import sys

defFile = sys.argv[1]

ready = False
with open(defFile) as fp:
	while( True ):
		line = fp.readline();
		line = line.rstrip("\n")
		terms = line.split()
		if "COMPONENTS" in line:
			if "END" in line:
				break
			else:
				ready = True
		
		if ready == True and terms[0] == '-':
			while( ";" not in line ):
				line += " " + fp.readline().rstrip("\n")
			placement = re.search("[-]?\d+ [-]?\d+", line)
			x = float(placement.group(0).split(' ')[0])/1000
			y = float(placement.group(0).split(' ')[1])/1000
			print(terms[1] + " " + str("%.3f"%x) + " " + str("%.3f"%y))

