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
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <limits>

using   namespace   std;

/*** Default Parameters **************************************************/
CArgument::CArgument(){
    W                   = 12;
    H                   = 12;
    num_sinks           = 32;
    max_skew            = 0;
    verbose             = 0;
    dist_i              = 1;
    time_i              = 1;
    cap_i               = 1;
    toler               = 0;
    max_delay           = std::numeric_limits<int>::max();
    max_solnum          = 1;
    cluster_only        = false;
	compute_sink_region_mode = false;
	percentile			= 0.0;
}

/*** Command Line Analyzer ***********************************************/
bool    CArgument::argument(int argc, char* argv[]){

    for (int i=1; i<argc; i++){
        string arg  = argv[i];

        if ((arg == "--help")){
            help();
            return  false;
        }

        else if(arg == "-f") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    cap_i = atof(argv[i]);
                } else {
                    cout << "*** Error :: cap interval is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: cap interval is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-w") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    W = atof(argv[i]);
                } else {
                    cout << "*** Error :: area width is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: area width is needed.." << endl;
                return  false;
            }
        }


        else if(arg == "-h") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    H = atof(argv[i]);
                } else {
                    cout << "*** Error :: area height is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: area height is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-t") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    toler = atoi(argv[i]);
                } else {
                    cout << "*** Error :: timing tolerance is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: timing tolerance is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-l") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    dist_i = atof(argv[i]);
                } else {
                    cout << "*** Error :: grid height/width is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: grid height/width is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-i") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    time_i = atof(argv[i]);
                } else {
                    cout << "*** Error :: latency interval is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: latency interval is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-n") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    num_sinks = atoi(argv[i]);
                } else {
                    cout << "*** Error :: number of leaf regions is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: number of leaf regions is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-d") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    max_delay = atoi(argv[i]);
                } else {
                    cout << "*** Error :: upper bound on max. delay is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: upper bound on max. delay needed.." << endl;
                return  false;
            }
        }
        
        else if(arg == "-cluster_only") {
            cluster_only = true;
        }
        
        else if(arg == "-sol") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    sol_file = argv[i];
                } else {
                    cout << "*** Error :: solution file name is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: solution file name is needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-k") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    max_solnum = atoi(argv[i]);
                } else {
                    cout << "*** Error :: max. number of solutions is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: max. number of solutions is needed.." << endl;
                return  false;
            }
        }
        else if(arg == "-s") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    max_skew = atof(argv[i]);
                } else {
                    cout << "*** Error :: upper bound on max. skew is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: upper bound on max. skew needed.." << endl;
                return  false;
            }
        }

        else if(arg == "-v") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    verbose = atoi(argv[i]);
                } else {
                    cout << "*** Error :: verbose mode is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: verbose mode is needed.." << endl;
                return  false;
            }
        }
		
        else if(arg == "-tech") {
            if (i+1 < argc){
                i++;

                if (argv[i][0] != '-') {
                    tech_node = atoi(argv[i]);
                } else {
                    cout << "*** Error :: technology node is needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: technology node is needed.." << endl;
                return  false;
            }
        }
		
		else if(arg == "-compute_sink_region_mode") {
            compute_sink_region_mode = true;
        }
 
        else if(arg == "-percentile") {
            if (i+1 < argc) {
                i++;

                if (argv[i][0] != '-') {
                    percentile = atof(argv[i]);
                } else {
                    cout << "*** Error :: percentile value needed.." << endl;
                    return  false;
                }
            } else {
                cout << "*** Error :: percentile value needed.." << endl;
                return  false;
            }
        }


		else {
            cout << "*** Error :: Option(s) is(are) NOT applicable.." << endl;
            return  false;
        }
    }

    cout << "*** Input parameters ***" << endl;
	if (compute_sink_region_mode) {
    	cout << "Area side:\t" << W << "x" << H << endl;
	}
    cout << "Max. skew:\t" << max_skew << endl;
    cout << "Max. latency:\t" << max_delay << endl;
    cout << "#Leaf regions:\t" << num_sinks << endl;
    cout << "Distance interval:\t" << dist_i << endl;
    cout << "Latency interval:\t" << time_i << endl;
    cout << "Capacitance interval:\t" << cap_i << endl;
    cout << "Timing tolerance:\t" << toler << endl;
	cout << "Tech node: \t" << tech_node << endl;
    if (verbose > 0) {
        cout << "Verbose mode:\t" << verbose << endl;
    }
	cout << "Compute sink region mode\t" << compute_sink_region_mode << endl;
	cout << "Percentile\t" << percentile << endl;
    cout << endl;
    return  true;
}

void    CArgument::help(){
    cout << "-w \t region width" << endl;
    cout << "-h \t region height" << endl;
    cout << "-l \t distance interval (granularity)" << endl;
    cout << "-c \t total sink capacitance" << endl;
    cout << "-f \t capacitance interval (granularity)" << endl;
    cout << "-s \t max. skew" << endl;
    cout << "-i \t timing interval (granularity)" << endl;
    cout << "-n \t number of sinks" << endl;
    cout << "-v \t verbose mode" << endl;
    cout << "-t \t timing tolerance" << endl;
    cout << "-k \t number of solutions" << endl;
    cout << "-sol \t solution file name" << endl;
    cout << "-cluster_only \t clustering only" << endl;
    cout << endl;
}
