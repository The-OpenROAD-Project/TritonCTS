////////////////////////////////////////////////////////////////////////////////////
// Authors: Kwangsoo Han and Jiajia Li
//          (Ph.D. advisor: Andrew B. Kahng),
//          Many subsequent changes for open-sourcing were made by Mateus Fogaça
//          (Ph.D. advisor: Ricardo Reis)
//
// BSD 3-Clause License
//
// Copyright (c) 2018, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////////

#include "argument.h"
#include "mystring.h"
#include "mymeasure.h"
#include "design.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <omp.h>
#include <vector>
#include <iostream>
#include <fstream>

using   namespace   std;

int main(int argc, char* argv[]){
    CArgument          argument;
    CMeasure           measure;
    design * my_design = new design();

    cout << "------------------------------------------------------"<<endl;
    cout << "  UCSD Gen H-Tree Constructor                         "<<endl; 
    cout << "  coded by Kwangsoo Han and Jiajia Li                 "<<endl;
    cout << "------------------------------------------------------"<<endl;

    bool para = true;
    para = argument.argument(argc, argv);
    if (!para) {
        argument.help();
        return 0;
    }
	
	if (argument.tech_node == 28) {
		my_design->out_slew_idx = 7;
		my_design->min_slew_idx = 3;
	} else if (argument.tech_node == 65){
		my_design->out_slew_idx = 5;
		my_design->min_slew_idx = 3;
	} else {
		my_design->out_slew_idx = 2;
		my_design->min_slew_idx = 0;
	}

    class number {
        public:
            int val;
    };

    measure.start_clock();
    float startTime = omp_get_wtime();

    my_design->parseDesignInfo(argument.W, argument.H, argument.dist_i, argument.cap_i, argument.max_skew, argument.time_i, argument.num_sinks, argument.verbose, argument.toler, argument.max_delay, argument.max_solnum, argument.cluster_only, argument.sol_file, argument.compute_sink_region_mode, argument.percentile);
    //my_design->parseBlks();
    if (!my_design->cluster_only) {
        my_design->optTree();
       my_design->reconstructTree();
        my_design->placeTree();
    } else {
        my_design->selectTreeSol();
    }

    measure.stop_clock("genHtree");

    float endTime = omp_get_wtime();
    cout<<"Wall-time taken: "<<(endTime - startTime)<<endl;

    measure.print_clock();
    measure.printMemoryUsage();


    return 1;
}
