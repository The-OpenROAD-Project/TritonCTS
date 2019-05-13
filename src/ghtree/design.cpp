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

#include "design.h"
#include "mymeasure.h"
#include "clustering.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <list>
#include <iterator>
#include <string>
#include <stack>
#include <map>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <cstdlib>
#include <omp.h>
#include <sys/timeb.h>
//#include <ilcplex/ilocplex.h>
#include <numeric>

using   namespace   std;

#define RON        1 // NEED TO UPDATE
#define CWIRE      0.00016 // M3
#define RWIRE      14 // M3
#define CPIN       0.001891 // BFX50_LR
#define TOT_THREAD_NUM 20 // TOTAL NUMBER OF THREADS

ostream& operator<< (ostream& os, blockage b) {
    cout << "(" << b.x1 << ", " << b.y1 << ", " << b.x2 << ", " << b.y2 << ")";
    return os ;
}
ostream& operator<< (ostream& os, pair<float,float> b) {
    cout << "(" << b.first << ", " << b.second << ")";
    return os ;
}


bool sortByX(pin *p1, pin *p2) {
	return p1->x < p2->x;
}

bool sortByY(pin* p1, pin* p2) {
	return p1->y < p2->y;
}


/*** Parse the input file **************************************************/
bool design::parseDesignInfo(float _W, float _H, float dist_i, float cap_i, float _skew, float time_i, unsigned _n, unsigned _v, int _toler, unsigned _max_delay, unsigned _max_solnum, bool _cluster_only, string _sol_file, bool computeSinkRegionMode) {
    
    max_skew        = (unsigned)floor(_skew / time_i);
    num_sinks       = _n;
    verbose         = _v;
    toler           = _toler;
    max_delay       = _max_delay;
    cur_sol         = 0;
    max_solnum      = _max_solnum;
    cluster_only    = _cluster_only;
    sol_file        = _sol_file;
	xoffset			= 0.0f;
	yoffset			= 0.0f;

    parseSinkCap();

	// MF @ 180206: hacking sink region
	if (computeSinkRegionMode) {
		computeSinkRegion(dist_i);		
	} else {
		W = (unsigned)ceil(_W / (dist_i*2))*2;
    	H = (unsigned)ceil(_H / (dist_i*2))*2;
	}
	// end 

    parseLUT();
	
	cout << "min_slew_idx: " << min_slew_idx << " out_slew_idx: " << out_slew_idx << "\n";

    avg_cap = total_cap / num_sinks;

    return true;
}

// MF @ 180206: hacking sink region
void design::computeSinkRegion(const float dist_i) {
	std::cout << " TritonCTS is running on 'computing sink region mode'\n";
	std::cout << " Computing sink region now...\n";
	
	int percentile = (int) std::round(0.00 * pins.size());
	std::sort(pins.begin(), pins.end(), sortByX);
	float minx = pins[percentile]->x;
	float maxx = pins[pins.size() - 1 - percentile]->x;

	std::sort(pins.begin(), pins.end(), sortByY);
	float miny = pins[percentile]->y;
	float maxy = pins[pins.size()- 1 -percentile]->y;
	
	std::cout << "\tminx =\t" << minx << "\n";	
	std::cout << "\tminy =\t" << miny << "\n";	
	std::cout << "\tmaxx =\t" << maxx << "\n";	
	std::cout << "\tmaxy =\t" << maxy << "\n";

	W = (unsigned)ceil((maxx-minx) / (dist_i*2))*2;
    H = (unsigned)ceil((maxy-miny) / (dist_i*2))*2;
	
	xoffset = minx;
	yoffset = miny;

	std::cout << "\txoffset =\t" << xoffset << "\n";
	std::cout << "\tyoffset =\t" << yoffset << "\n";
	std::cout << "\twidth =\t" << W << "\n";
	std::cout << "\theight =\t" << H << "\n";

	for (unsigned i = 0; i < pins.size(); ++i) {
		pins[i]->x -= xoffset;
		pins[i]->y -= yoffset;
	}
}

void design::parseSinkCap() {
    ifstream inFile;
    string str;
    inFile.open("sink_cap.txt");
    if (!inFile.is_open()) {
        cout << "***Error :: unable to open the sink cap file !" << endl;
    }
    while (getline(inFile, str)) {
        stringstream ss(str);
        string name, x, y, cap;
        ss >> name >> x >> y >> cap ;
        pin * p = new pin(name, atof(x.c_str()), atof(y.c_str()), atof(cap.c_str()));
        pins.push_back(p);
    }
    inFile.close();
    total_cap = 0;
    // consider wire cap
    for (unsigned i = 0; i < pins.size(); ++i) {
        pin * p = pins[i];
        total_cap += p->cap;
    }
    total_cap = total_cap;
    cout << "TOTAL CAP = " << total_cap << " fF" << endl << endl;
}

void design::parseLUT() {

    ifstream inFile;
    string str;
    inFile.open("lut.txt");
    if (!inFile.is_open()) {
        cout << "***Error :: unable to open the LUT file !" << endl;
    }
    getline(inFile, str);
    stringstream ss(str);
    string min_d, max_d, min_l, max_l, min_s, max_s;
    ss >> min_d >> max_d >> min_l >> max_l >> min_s >> max_s;

    min_dist = atoi(min_d.c_str()); // always start from 1, to be removed 
    min_cap  = atoi(min_l.c_str());
    min_slew = atoi(min_s.c_str());

    unsigned size_d = atoi(max_d.c_str()) - min_dist + 1; 
    max_dist = size_d + min_dist - 1;
    size_cap  = atoi(max_l.c_str()) - min_cap  + 1; 
    size_slew = atoi(max_s.c_str()) - min_slew + 1; 
    vector<vector<LUT*>> dummy1;
    dummy1.resize(size_slew);
    vector<vector<vector<LUT*>>> dummy2;
    for (unsigned i = 0; i < size_cap; ++i) {
        dummy2.push_back(dummy1);
    }
    luts.clear();
    for (unsigned i = 0; i < size_d; ++i) {
        luts.push_back(dummy2);
    }

    while (getline(inFile, str)) {
        string sol_idx, dist, load, out_slew, power, delay, in_cap, in_slew, pure_wire;
        stringstream ss(str);
        ss >> sol_idx >> dist >> load >> out_slew >> power >> delay >> in_cap >> in_slew >> pure_wire;
        LUT * lut    = new LUT(atoi(sol_idx.c_str()), atof(power.c_str()), atoi(delay.c_str()), atoi(in_cap.c_str()), atoi(in_slew.c_str()), "0",min_cap);
        lut_map[lut->idx] = lut;
        string len; // segment length between buffers
        while (ss >> len) {
            lut->lens.push_back(atof(len.c_str())*atof(dist.c_str()));
        }
        lut->len = atof(dist.c_str());

        unsigned idx_d = atoi(dist.c_str()) - min_dist;
        unsigned idx_l = atoi(load.c_str()) - min_cap;
        unsigned idx_s = atoi(out_slew.c_str()) - min_slew;
        if (atoi(out_slew.c_str()) == lut->in_slew && idx_s > 0)
            --idx_s;
        float tmp_dist = atof(dist.c_str())*15;
        if (lut->pure_wire) {
            int tmp_delay = (int)nearbyint(0.04*tmp_dist);
            if (tmp_delay > 0) 
                lut->delay += tmp_delay;
        }
        luts[idx_d][idx_l][idx_s].push_back(lut);
    }
    inFile.close();
    //size_cap = 20;
}


void design::parseBlks() {

    cout << "Start parseBlks()" << endl;
    ifstream inFile;
    string str;
    inFile.open("blks.txt");
    if (!inFile.is_open()) {
        cout << "***Error :: unable to open the blk file !" << endl;
    }
    string x1, y1, x2, y2;
    while (getline(inFile, str)) {
        stringstream ss(str);
        ss >> x1 >> y1 >> x2 >> y2;
        blockage * blk = new blockage(
						atof(x1.c_str())-xoffset, 
						atof(y1.c_str())-yoffset, 
						atof(x2.c_str())-xoffset,
					   	atof(y2.c_str())-yoffset);
        blks.push_back(blk);
    }
    inFile.close();

    cout << "End parseBlks()" << endl;
}

/*** Dynamic programming-based optimization to minimize wirelength *********/
void design::optTree () {

    omp_set_num_threads(TOT_THREAD_NUM);

    cout << "***** Optimization *****" << endl;

    CMeasure           measure;

    // calculation of max. depth range 
    unsigned     n = 1;
    unsigned max_d = 0;    
    while (n < num_sinks) {
        n *= 2;
        ++max_d;
    }
    
    // JL: additional layer to ensure valid solution
    ++max_d;

    // initialization of solution space
    unsigned num_max, num_min;
    if (W > H) {
        num_max = W/2;
        num_min = H/2; 
    } else {
        num_max = H/2;
        num_min = W/2; 
    }

    vector<vector<tree*>> dummy1;
    dummy1.resize(size_slew); 
    vector<vector<vector<tree*>>> dummy2;
    for (unsigned i = 0; i < size_cap; ++i) {
        dummy2.push_back(dummy1);
    }

    vector<vector<vector<vector<vector<tree*>>>>>  dummy3_l;
    vector<vector<vector<vector<vector<tree*>>>>>  dummy3_s;
    dummy3_l.resize(num_max);
    dummy3_s.resize(num_min);
    vector<vector<vector<vector<vector<vector<tree*>>>>>> prep_sol;
    for (unsigned i = 0; i < num_max; ++i) {
        if (i < num_min) {
            prep_sol.push_back(dummy3_l);
        } else {
            prep_sol.push_back(dummy3_s);
        }
    }

    for (unsigned i = 0; i < num_max; ++i) {
        unsigned w = (i+1)*2;
        for (unsigned j = 0; j < num_max; ++j) {
            unsigned h = (j+1)*2; 
            if (i >= num_min && j >= num_min) 
                continue;
            if ((float)w/h >= 30 || (float)w/h <= 0.03) 
            //if ((float)w/h >= 10 || (float)w/h <= 0.1) 
                continue; 
           // if ((float)w/h <= 0.025) 

            // number of sinks is an even number
            //unsigned m = (int)ceil(num_sinks*1.5/((W*H)/(w*h))/2) - (int)floor(num_sinks*0.6/((W*H)/(w*h))/2) + 1; 
            unsigned m = (int)ceil(num_sinks*10.0/((W*H)/(w*h))/2); 
            prep_sol[i][j].resize(m, dummy2);
        }
    }

    sols.clear();
    // only keep two-level solutions in memory
    sols.resize(2,prep_sol);
    
    //prep_sol.clear();

    // basic solutions (d == 1)
    unsigned tree_idx = 1; // idx = 0 : no subtree
    cout << "Depth = " << 1 << " .." << endl;
    #pragma omp parallel for collapse(2)
    for (unsigned i = 0; i < num_max; ++i) {
        for (unsigned j = 0; j < num_max; ++j) {
            unsigned w = (i+1)*2;
            if (i >= num_min && j >= num_min) 
                continue;
            unsigned h  = (j+1)*2;

            if ((float)w/h >= 20 || (float)w/h <= 0.05) 
                continue; 
            //if ((float)w/h <= 0.025) 

            //unsigned baseM = (unsigned)floor(num_sinks*0.6/((W*H)/(w*h))/2);
            //for (unsigned m = 0; m < sols[0][i][j].size(); ++m)
            for (unsigned m = 0; m < sols[0][i][j].size(); ++m) {
                // JL: skip the following to ensure valid solution
                //if (m < (int)floor(num_sinks*0.6/((W*H)/(w*h))/2)) {
                //    continue;
                //}
                //unsigned n = (baseM+m+1)*2;
                unsigned n = (m+1)*2;
                // JL: restrict branching factor to be less than 10
                if (n > 10)
                  continue;

                vector<unsigned> dists;
                vector<float> loads;
                vector<vector<unsigned>> subtrees;
                vector<unsigned> dummy_trees;
                float load_cap      = total_cap*w*h/(W*H*n); // raw data (normalized to 1fF)
                //load_cap           += 2.931*w*h/n + 0.2781*num_sinks*w*h/(W*H*n);
                float cur_cap       = 0; // current accumulated cap
                float seg           = float(w)/n;
                float cur_dist_c    = 0; // continues distance: track braching points
                float pre_dist_c    = 0;
                unsigned cur_dist_d = 0, pre_dist_d = 0; // discrete distance: track buffering grids

                //cout << "D1 A: " << w << "x" << h << " N: " << n << " " << load_cap << endl; 
                for (unsigned r = 0; r <= m; ++r) {
                    if (r == 0) 
                        cur_dist_c += seg/2;
                    else 
                        cur_dist_c += seg;
                    cur_dist_d  = (unsigned)nearbyint(cur_dist_c+0.001);   // do not insert buffer on additional wires
                    //cout << "r " << r << " " << cur_dist_c << " " << cur_dist_d << " " << pre_dist_d << endl;
                    if (cur_dist_d != pre_dist_d) {
                        unsigned dist_d = (unsigned)nearbyint(cur_dist_c-pre_dist_c+0.001);
                        //cout << "input " <<  cur_dist_c-pre_dist_c << " output " << dist_d << endl;
                        loads.push_back(cur_cap); // store cap in unit of 1fF
                        cur_cap = 0;
                        subtrees.push_back(dummy_trees);
                        dummy_trees.clear();
                        if (dist_d > max_dist) {
                            dists.push_back(max_dist);
                            dist_d -= max_dist;
                        } else {
                            dists.push_back(dist_d);
                            dist_d  = 0;
                        }
                        while (dist_d > 0) {
                            if (dist_d > max_dist) {
                                dists.push_back(max_dist);
                                dist_d -= max_dist;
                            } else {
                                dists.push_back(dist_d);
                                dist_d = 0;
                            }
                            loads.push_back(0); // store cap in unit of 1fF
                            subtrees.push_back(dummy_trees);
                        }
                        pre_dist_d = cur_dist_d;
                        pre_dist_c = cur_dist_c;
                    }
                    cur_cap += load_cap;
                    dummy_trees.push_back(r);
                    if (r == m) {
                        loads.push_back(cur_cap);
                        subtrees.push_back(dummy_trees);
                    }
                }
                
                if (verbose > 2) {
                   for (unsigned i = 0; i <= dists.size(); ++i) {
                       cout << i << " cap: " << loads[i] << endl;
                   }
                   cout << endl;
                   for (unsigned i = 0; i < dists.size(); ++i) {
                       cout << i << " dist: " << dists[i] << endl;
                   }
                   cout << endl;
                }
                if (dists.size() == 0) {
                    // no buffer insertion

                    #pragma omp critical (update)
                    {
                        tree * t = new tree(tree_idx);
                        t->tree_sols.resize(m+1, 0);
                        // length less than grid size == approximate as zero delay
                        t->min_laten = 0;
                        t->max_laten = 0;
                        t->power     = 0;
                        t->in_cap    = 2*loads[0];
                        int idx_c;
                        if (loads[0] < 5) {
                            idx_c    = (int)ceil(2*loads[0])-min_cap;
                        } else {
                            idx_c    = (int)ceil(2*loads[0]/5)+4-min_cap;
                        }
                        if (idx_c  < 0) 
                            idx_c  = 0;
                        if (idx_c  >= size_cap) {     // violates max-cap constraints
                            //cout << "VIO CAP CON (sink cap)" << endl;
                            delete t;
                        } else {
                            unsigned idx_s = out_slew_idx;
                            bool find = evalTree(t, 0, i, j , m, idx_c, idx_s);
                            if (!find)  
                                ++tree_idx;
                        }
                    }
                } else {
                    // with buffering on segment
                    vector<vector<vector<segment*>>> seg_dummy;
                    seg_dummy.resize(size_cap);
                    for (unsigned l = 0; l < seg_dummy.size(); ++l) 
                        seg_dummy[l].resize(size_slew);
                    vector<vector<vector<vector<segment*>>>> segments;
                    segments.resize(dists.size()+1, seg_dummy);
                    bool flag = true;
                    for (int r = dists.size(); r >= 0; --r) {
                        if (r == dists.size()) {
                            segment * s    = new segment();
                            // assume zero latency at the bottom level
                            s->max_laten   = 0;
                            s->min_laten   = 0;
                            s->power       = 0;
                            s->in_cap      = loads[r];
                            int idx_c;
                            if (s->in_cap < 5) {
                                idx_c   = (int)ceil(s->in_cap) - min_cap;
                            } else {
                                idx_c   = (int)ceil(s->in_cap/5)+4-min_cap;
                            }
                            if (idx_c < 0) 
                                idx_c = 0;
                            if (idx_c >= size_cap) {  // violates max-cap constraint
                                //cout << "VIO CAP CON (segment)" << endl;
                                delete s;
                                continue;
                            }
                            s->in_slew = out_slew_idx + min_slew;
                            evalSegment(s, segments[r][idx_c][out_slew_idx]);
                        } else {
                            for (unsigned _idx_c = 0; _idx_c < size_cap; ++_idx_c) {
                                for (unsigned _idx_s = 0; _idx_s < size_slew; ++_idx_s) {

                                    if (segments[r+1][_idx_c][_idx_s].empty()) 
                                        continue;
                                    for (unsigned p = 0; p < segments[r+1][_idx_c][_idx_s].size(); ++p) {
                                        int _idx_d = dists[r] - min_dist;
                                        if (_idx_d < 0) 
                                            _idx_d = 0;
                                        for (unsigned q = 0; q < luts[_idx_d][_idx_c][_idx_s].size(); ++q) {
                                            segment * s = new segment(segments[r+1][_idx_c][_idx_s][p], luts[_idx_d][_idx_c][_idx_s][q]);
                                            //if (w == 4 && h == 8 && n == 12) {
                                            //    cout << "!!!! INCAP: " << s->in_cap << " " << subtrees[r].size() << " " << loads[r] << " " << luts[_idx_d][_idx_c][_idx_s][q]->pure_wire << endl;

                                            //}
                                            if (subtrees[r].size() > 0) {
                                                s->min_laten      = 0;
                                                s->in_cap        += loads[r];
                                                //unsigned tmp_slew = out_slew_idx + min_slew;
                                                //if (tmp_slew > s->in_slew) 
                                                //    s->in_slew    = tmp_slew;
                                            }
                                            if (s->max_laten - s->min_laten > max_skew) { // violates max-skew constraints
                                                //cout << "VIO SKEW CON (segment) " << s->max_laten - s->min_laten << endl;
                                                delete s;
                                                continue;
                                            }
                                            int idx_c;
                                            if (s->in_cap < 5) {
                                                idx_c   = (int)ceil(s->in_cap) - min_cap;
                                            } else {
                                                idx_c   = (int)ceil(s->in_cap/5)+4-min_cap;
                                            }
                                            if (idx_c < 0) 
                                                idx_c = 0;
                                            if (idx_c >= size_cap) {  // violates max-cap constraint
                                                //cout << "VIO CAP CON (segment)" << endl;
                                                delete s;
                                                continue;
                                            }
                                            int idx_s = s->in_slew - min_slew;
                                            if (idx_s < min_slew_idx) {
                                                //cout << "VIO TRANS CON (segment)" << endl;
                                                delete s;
                                                continue;
                                            }
                                            if (idx_s >= size_slew) {  // violates max-slew constraint
                                                idx_s  = size_slew - 1;
                                            }
                                            
                                            evalSegment(s, segments[r][idx_c][idx_s]);
                                        }
                                        delete segments[r+1][_idx_c][_idx_s][p];
                                    }
                                }
                            }
                        }
                    }

                    for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                        for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                            if (segments[0][idx_c][idx_s].empty()) 
                                continue;
                            for (unsigned p = 0; p < segments[0][idx_c][idx_s].size(); ++p) {
                                segment * s = segments[0][idx_c][idx_s][p];
                                #pragma omp critical (update)
                                {
                                    tree * t    = new tree(tree_idx, s);
                                    t->tree_sols.resize(m+1, 0);
                                    int tmp_cap = s->in_cap * 2;
                                    int _idx_c;
                                    if (tmp_cap < 5) {
                                        _idx_c  = tmp_cap-min_cap;
                                    } else {
                                        _idx_c  = (int)ceil(float(tmp_cap)/5)+4-min_cap;
                                    }
                                    if (_idx_c  < 0) 
                                        _idx_c  = 0;
                                    if (_idx_c  >= size_cap) {     // violates max-cap constraints
                                        //cout << "VIO CAP CON (root of subtree)" << endl;
                                        delete t;
                                    } else {
                                        bool find = evalTree(t, 0, i, j, m, _idx_c, idx_s);
                                        if (!find) 
                                            ++tree_idx;
                                    }
                                }
                                delete s;
                            }
                        }
                    }
                }
            }
        }
    }

    if (verbose > 2) {
        for (unsigned i = 0; i < num_max; ++i) {
            unsigned w = (i+1)*2;
            for (unsigned j = 0; j < num_max; ++j) {
                if (i >= num_min && j >= num_min) 
                    continue;
                unsigned h  = (j+1)*2;
                for (unsigned m = 0; m < sols[0][i][j].size(); ++m) {
                    unsigned n = (m+1)*2;
                    for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                        for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                            int cap;
                            if (idx_c < 5) {
                                cap = idx_c + min_cap;
                            } else {
                                cap = (idx_c+min_cap-4)*5;
                            }
                            if (sols[0][i][j][m][idx_c][idx_s].empty()) 
                                continue;
                            for (unsigned l = 0; l < sols[0][i][j][m][idx_c][idx_s].size(); ++l) {
                                tree * t = sols[0][i][j][m][idx_c][idx_s][l];
                                cout << "Area " << w << " x " << h << " N " << n << " slew " << (idx_s + min_slew)*5 << " cap " << cap << " laten " << t->min_laten << " " << t->max_laten << " power " << t->power << " input cap " << t->in_cap << endl;
                            }
                        }
                    }
                }
            }
        }
    }

    // DP-based optimization
    opt_t_idx  = -1;
    float min_power     = std::numeric_limits<float>::max();
    unsigned opt_skew   = -1;
	int n_dbg = 0;
    for (unsigned d = 2; d <= max_d; ++d) {    // DEPTH
        cout << "Depth = " << d << " .." << endl;
        unsigned k   = 1;  // new solution
        unsigned d_b = d -1, k_b = 0;           // Increase by one level (0: previous solution)
        #pragma omp parallel for collapse(2)
        for (unsigned i = 0; i < num_max; ++i) {    // WIDTH
            for (unsigned j = 0; j < num_max; ++j) {    // HEIGHT
                unsigned w = (i+1)*2;
                if (i >= num_min && j >= num_min) 
                    continue;
                unsigned h = (j+1)*2;
                if ((float)w/h >= 20 || (float)w/h <= 0.05) 
                    continue; 
                //if ((float)w/h <= 0.025) 


                //unsigned baseM = (unsigned)floor(num_sinks*0.6/((W*H)/(w*h))/2);
                for (unsigned m = 0; m < sols[k][i][j].size(); ++m) {  // SINK NUMBER
                    // JL: skip the following to ensure valid solution
                    //if (m < (int)floor(num_sinks*0.6/((W*H)/(w*h))/2)) {
                    //    continue;
                    //}
                    //unsigned n = (baseM+m+1)*2;
                    unsigned n = (m+1)*2;
					
                    for (unsigned m_t = 0; m_t < m; ++m_t) {
                        unsigned n_t = (m_t+1)*2;
                        // JL: restrict branching factor to be less than 10
                        if (n_t > 10)
                         continue;
                        float  tmp_n = float(n)/n_t;
                        if (fmod(tmp_n,2) != 0)          // multiplication does not lead to desired sink number
                            continue;
                        unsigned n_b = unsigned(tmp_n);
                        unsigned m_b = n_b/2-1;

                        int w_b = h, h_b = (unsigned)ceil(float(w)/n_t/2)*2;  // BOTTOM TREE AREA
                        int i_b = w_b/2-1, j_b = h_b/2-1;
                        if (j_b < 0)
                            continue;
                        if (i_b >= num_min && j_b >= num_min) {
                            j_b = num_min - 1;
                        }
                        if (m_b >= sols[k_b][i_b][j_b].size()) 
                            continue;
                        
                        //cout << "D1 A Top (Bottom): " << w << "x" << h << " (" << w_b << "x" << h_b << ") N Top (Bottom): " << n  << " (" << n_b << ")" << endl; 

                        if (verbose > 1) {
                            cout << "Area = " << w << " x " << h << endl;
                            cout << "NTop/NBottom " << n_t << " " << n_b << endl;
                            cout << "BArea " << w_b << " x " << h_b << endl;
                        }

                        // collect all combinations of subtree solutions
                        vector<vector<tree*>> combs;
                        vector<tree*> subtrees;
                        map<unsigned, float> tree_in_caps;
                        map<unsigned, unsigned> tree_in_slews;
                        for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                            for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                                if (sols[k_b][i_b][j_b][m_b][idx_c][idx_s].empty()) 
                                    continue;
                                for (unsigned l = 0; l < sols[k_b][i_b][j_b][m_b][idx_c][idx_s].size(); ++l) {
                                    tree * t = sols[k_b][i_b][j_b][m_b][idx_c][idx_s][l];
                                    subtrees.push_back(t);
                                    tree_in_caps[t->idx]  = t->in_cap;
                                    tree_in_slews[t->idx] = idx_s + min_slew;
                                }
                            }
                        }
                        if (subtrees.empty()) {   // no available subtree solution
                            //cout << "NO SUBTREE SOL" << endl;
                            continue;
                        }
                        vector<bool> dummy;
                        dummy.resize(subtrees.size(), false);
                        vector<vector<bool>> skip_mtx;
                        skip_mtx.resize(subtrees.size(), dummy);
                        map<unsigned, unsigned> idx_map;
                        for (unsigned p = 0; p < subtrees.size(); ++p) {
                            tree * t1        = subtrees[p];
                            idx_map[t1->idx] = p;
                            for (unsigned q = 0; q < subtrees.size(); ++q) {
                                if (p == q)
                                    continue;
                                tree * t2 = subtrees[q];
                                //if (d <= 2) {
                                //    if (t2->max_laten >= t1->max_laten) {
                                //        skip_mtx[p][q] = true;
                                //    }
                                //} else {
                                    skip_mtx[p][q] = true;
                                //}
                            }
                        }
                        // enumerate combinations of subtrees (from center to boundary)
                        for (unsigned r = 0; r <= m_t; ++r) {
                            if (r == 0) {
                                for (unsigned p = 0; p < subtrees.size(); ++p) {
                                    vector<tree*> dummy;
                                    dummy.push_back(subtrees[p]);
                                    combs.push_back(dummy);
                                }
                            } else {
                                unsigned p_max = combs.size(); 
                                for (unsigned p = 0; p < p_max; ++p) {
                                    for (unsigned q = 0; q < subtrees.size(); ++q) {
                                        vector<tree*> dummy;
                                        bool skip = false;
                                        for (unsigned l = 0; l < combs[p].size(); ++l) {
                                            if (skip_mtx[idx_map[combs[p][l]->idx]][idx_map[subtrees[q]->idx]]) {
                                                skip = true;
                                                break;
                                            }
                                            dummy.push_back(combs[p][l]);
                                        }
                                        if (!skip) {
                                            dummy.push_back(subtrees[q]);
                                            combs.push_back(dummy);
                                        }
                                    }
                                }
                                combs.erase(combs.begin(), combs.begin()+p_max);
                            }
                        }
                        if (verbose > 1) 
                            cout << "COMB SIZE " << combs.size() << endl;
                        
                        // buffering solution
                        for (unsigned o = 0; o < combs.size(); ++o) {
                
                            // properties for each segments (subtrees on segments)
                            vector<unsigned> dists;
                            vector<float>    loads;
                            vector<unsigned> slews;
                            vector<unsigned> max_latens;
                            vector<unsigned> min_latens;
                            vector<float>    powers;
                            vector<unsigned> subtrees;

                            float cur_cap       = 0; // current accumulated cap
                            unsigned cur_slew      = INT_MAX; // current worst slew (from subtrees)
                            unsigned cur_max_laten = 0, cur_min_laten = INT_MAX; // current max/min latencies
                            float cur_power        = 0; // current power of subtrees
                            float seg              = float(w)/n_t;
                            float cur_dist_c       = 0, pre_dist_c = 0; // continues distance: track braching points
                            unsigned cur_dist_d    = 0, pre_dist_d = 0; // discrete distance: track buffering grids
                
                            for (unsigned r = 0; r <= m_t; ++r) {

                                tree * t    = combs[o][r];
                                if (r == 0) 
                                    cur_dist_c += seg/2;
                                else 
                                    cur_dist_c += seg;
                                cur_dist_d  = (unsigned)nearbyint(cur_dist_c+0.001);   // do not insert buffer on additional wires
                                if (cur_dist_d != pre_dist_d) {
                                    unsigned dist_d = (unsigned)nearbyint(cur_dist_c-pre_dist_c+0.001);
                                    loads.push_back(cur_cap);
                                    slews.push_back(cur_slew);
                                    max_latens.push_back(cur_max_laten);
                                    min_latens.push_back(cur_min_laten);
                                    powers.push_back(cur_power);
                                    cur_cap       = tree_in_caps[t->idx];
                                    cur_slew      = tree_in_slews[t->idx];
                                    cur_max_laten = t->max_laten;
                                    cur_min_laten = t->min_laten;
                                    cur_power     = t->power;
                                    if (dist_d > max_dist) {
                                        dists.push_back(max_dist);
                                        dist_d -= max_dist;
                                    } else {
                                        dists.push_back(dist_d);
                                        dist_d  = 0;
                                    }
            
                                    while (dist_d > 0) {
                                        if (dist_d > max_dist) {
                                            dists.push_back(max_dist);
                                            dist_d -= max_dist;
                                        } else {
                                            dists.push_back(dist_d);
                                            dist_d = 0;
                                        }
                                        loads.push_back(0); // store cap in unit of 1fF
                                        slews.push_back(INT_MAX);
                                        max_latens.push_back(0);
                                        min_latens.push_back(INT_MAX);
                                        powers.push_back(0);
                                    }
                                    pre_dist_d = cur_dist_d;
                                    pre_dist_c = cur_dist_c;

                                } else {
                                    cur_cap    += tree_in_caps[t->idx];
                                    cur_power  += t->power;
                                    unsigned tmp_slew  = tree_in_slews[t->idx];
                                    if (tmp_slew < cur_slew) 
                                        cur_slew = tmp_slew;
                                    unsigned tmp_laten = t->max_laten;
                                    if (tmp_laten > cur_max_laten) 
                                        cur_max_laten = tmp_laten;
                                    tmp_laten = t->min_laten;
                                    if (tmp_laten < cur_min_laten) 
                                        cur_min_laten = tmp_laten;
                                }

                                subtrees.push_back(t->idx);

                                if (r == m_t) {
                                    loads.push_back(cur_cap);
                                    slews.push_back(cur_slew);
                                    max_latens.push_back(cur_max_laten);
                                    min_latens.push_back(cur_min_laten);
                                    powers.push_back(cur_power);
                                }

                            }

                            if (dists.size() == 0) {
                                // no buffer insertion
                                #pragma omp critical (update)
                                {
                                    tree *t = new tree(tree_idx);
                                    for (unsigned l = 0; l < subtrees.size(); ++l) 
                                        t->tree_sols.push_back(subtrees[l]);
                                    t->power     = 2*powers[0];
                                    t->max_laten = max_latens[0];
                                    t->min_laten = min_latens[0];
                                    t->in_cap    = 2*loads[0];
                                    if (t->max_laten - t->min_laten > max_skew) {
                                        //cout << "VIO SKEW CON (no buffer on branch)" << endl;
                                        delete t;
                                    } else {
                                        int idx_c;
                                        if (loads[0] < 5) {
                                            idx_c  = (int)ceil(2*loads[0])-min_cap;
                                        } else {
                                            idx_c  = (int)ceil(2*loads[0]/5)+4-min_cap;
                                        }
                                        if (idx_c  < 0) 
                                            idx_c  = 0;
                                        if (idx_c  >= size_cap) {    // violates max-cap constraint
                                            //cout << "VIO CAP CON (subtree cap)" << endl;
                                            delete t;
                                        } else {
                                            unsigned idx_s = slews[0] - min_slew;
                                            if (idx_s < min_slew_idx) {
                                                //cout << "VIO TRANS CON (subtree cap) [should not happen]" << endl;
                                                delete t;
                                            } else {
                                                bool find = evalTree(t, k, i, j, m, idx_c, idx_s);
                                                if (!find) 
                                                    ++tree_idx;
                                            }
                                        }
                                    }
                                }
                            } else {
                                // with buffering on segment
                                vector<vector<vector<segment*>>> seg_dummy;
                                seg_dummy.resize(size_cap);
                                for (unsigned l = 0; l < seg_dummy.size(); ++l) 
                                    seg_dummy[l].resize(size_slew);
                                vector<vector<vector<vector<segment*>>>> segments;
                                segments.resize(dists.size()+1, seg_dummy);
                                bool flag = true;
                                for (int r = dists.size(); r >= 0; --r) {
                                    if (r == dists.size()) {
                                        segment * s    = new segment();
                                        // assume zero latency at the bottom level
                                        s->max_laten   = max_latens[r];
                                        s->min_laten   = min_latens[r];
                                        s->power       = powers[r];
                                        s->in_cap      = loads[r];
                                        s->in_slew     = slews[r];
                                        int idx_c;
                                        if (s->in_cap < 5) {
                                            idx_c   = (int)ceil(s->in_cap) - min_cap;
                                        } else {
                                            idx_c   = (int)ceil(s->in_cap/5)+4-min_cap;
                                        }
                                        if (idx_c < 0) 
                                            idx_c = 0;
                                        if (idx_c >= size_cap) {  // violates max-cap constraint
                                            //cout << "VIO CAP CON (segment end)" << endl;
                                            delete s;
                                            continue;
                                        }
                                        int idx_s = s->in_slew - min_slew;
                                        if (idx_s < min_slew_idx) {
                                            //cout << "VIO TRANS CON (segment end)" << endl;
                                            delete s;
                                            continue;
                                        }
                                        if (idx_s >= size_slew) {  // violates max-slew constraint
                                            idx_s  = size_slew -1; 
                                        }
                                        evalSegment(s, segments[r][idx_c][idx_s]);
                                    } else {
                                        for (unsigned _idx_c = 0; _idx_c < size_cap; ++_idx_c) {
                                            for (unsigned _idx_s = 0; _idx_s < size_slew; ++_idx_s) {
                                                if (segments[r+1][_idx_c][_idx_s].empty()) 
                                                    continue;
                                                for (unsigned p = 0; p < segments[r+1][_idx_c][_idx_s].size(); ++p) {
                                                    int _idx_d = dists[r] - min_dist;
                                                    if (_idx_d < 0) 
                                                        _idx_d = 0;
                                                    for (unsigned q = 0; q < luts[_idx_d][_idx_c][_idx_s].size(); ++q) {
                                                        segment * s = new segment(segments[r+1][_idx_c][_idx_s][p], luts[_idx_d][_idx_c][_idx_s][q]);
                                                        if (min_latens[r] < s->min_laten) 
                                                            s->min_laten = min_latens[r];
                                                        if (max_latens[r] > s->max_laten) 
                                                            s->max_laten = max_latens[r];
                                                        if (slews[r] < s->in_slew) 
                                                            s->in_slew = slews[r];
                                                        s->in_cap += loads[r];
                                                        s->power  += powers[r];
                                                        int idx_c;
                                                        if (s->in_cap < 5) {
                                                            idx_c   = (int)ceil(s->in_cap) - min_cap;
                                                        } else {
                                                            idx_c   = (int)ceil(s->in_cap/5)+4-min_cap;
                                                        }
                                                        if (idx_c < 0) 
                                                            idx_c = 0;
                                                        if (idx_c >= size_cap) {  // violates max-cap constraint
                                                            //cout << "VIO CAP CON (segment)" << endl;
                                                            delete s;
                                                            continue;
                                                        }
                                                        int idx_s = s->in_slew - min_slew;
                                                        if (idx_s < min_slew_idx) {
                                                            //cout << "VIO TRANS CON (segment)" << endl;
                                                            delete s;
                                                            continue;
                                                        }
                                                        if (idx_s >= size_slew) {  // violates max-slew constraint
                                                            idx_s  = size_slew - 1;
                                                        }
                                                        if (s->max_laten - s->min_laten > max_skew) { // violates max-skew constraints
                                                            //cout << "VIO SKEW CON (segment) " << s->max_laten - s->min_laten << endl;
                                                            delete s;
                                                            continue;
                                                        }

                                                        evalSegment(s, segments[r][idx_c][idx_s]);
                                                    }
                                                    delete segments[r+1][_idx_c][_idx_s][p];
                                                }
                                            }
                                        }
                                    }
                                }

                                for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                                    for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                                        if (segments[0][idx_c][idx_s].empty()) 
                                            continue;
                                        for (unsigned p = 0; p < segments[0][idx_c][idx_s].size(); ++p) {
                                            segment * s = segments[0][idx_c][idx_s][p];
                                            #pragma omp critical (update)
                                            {
                                                tree *t    = new tree(tree_idx, s);
                                                for (unsigned l = 0; l < subtrees.size(); ++l) 
                                                    t->tree_sols.push_back(subtrees[l]);
                                                float tmp_cap = s->in_cap * 2;
                                                int _idx_c;
                                                if (tmp_cap < 5) {
                                                    _idx_c  = (int)ceil(tmp_cap)-min_cap;
                                                } else {
                                                    _idx_c  = (int)ceil(tmp_cap/5)+4-min_cap;
                                                }
                                                if (_idx_c  < 0) 
                                                    _idx_c  = 0;
                                                if (_idx_c  >= size_cap) {     // violates max-cap constraints
                                                    //cout << "VIO SKEW CON (root of subtree)" << endl;
                                                    delete t;
                                                } else {
                                                    bool find = evalTree(t, k, i, j, m, _idx_c, idx_s);
                                                    if (!find) {
                                                      ++tree_idx;
                                                    }
                                                }
                                            }
                                            delete s;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        unsigned I = W/2 - 1;
        unsigned J = H/2 - 1;
        for (unsigned M = 0; M < sols[0][I][J].size(); ++M) {
            unsigned N = (M+1)*2;
			n_dbg = std::max((int)N, n_dbg);
            if (N < num_sinks) 
                continue;

            for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                    for (unsigned L = 0; L < sols[0][I][J][M][idx_c][idx_s].size(); ++L) {
                        tree * t = sols[0][I][J][M][idx_c][idx_s][L];
                        //cout << "SOL " << t->power << " " << t->max_laten - t->min_laten << " " << t->max_laten << " " << t->idx << endl;
                        //
                        //if (t->power < min_power && t->max_laten < 250) {
                        if (t->power < min_power) {
                            min_power = t->power;
                            opt_t_idx = t->idx;
                            opt_skew  = t->max_laten - t->min_laten;
                            avg_cap   = total_cap / num_sinks;
							//cout << "Intermediate n_dbg = " << n_dbg << "\n";
                        }
                    }
                }
            }
        }
        printSol("out.txt", d_b);
        sols.erase(sols.begin(), sols.begin()+1);
        sols.push_back(prep_sol);
    }

    unsigned I = W/2 - 1;
    unsigned J = H/2 - 1;
    for (unsigned M = 0; M < sols[0][I][J].size(); ++M) {
        unsigned N = (M+1)*2;
		n_dbg = max(n_dbg, (int) N);
        if (N < num_sinks) 
            continue;
        for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
            for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                for (unsigned L = 0; L < sols[0][I][J][M][idx_c][idx_s].size(); ++L) {
                    tree * t = sols[0][I][J][M][idx_c][idx_s][L];
                    //cout << "SOL " << t->power << " " << t->max_laten - t->min_laten << " " << t->max_laten << " " << t->idx << endl;
                    if (t->power < min_power) {
                        min_power = t->power;
                        opt_t_idx = t->idx;
                        opt_skew  = t->max_laten - t->min_laten;
                        avg_cap   = total_cap / num_sinks;
						//cout << "Intermediate n_dbg = " << n_dbg << "\n";
                    }
                }
            }
        }
    }
    printSol("out.txt", max_d);
    sols.erase(sols.begin(), sols.begin()+1);
   // sols.push_back(prep_sol);

	cout << "Final n_dbg = " << n_dbg << "\n";
    // select the optimal solution
    if (opt_t_idx == -1) {
        cout << "No feasible solution" << endl;
    } else {
        cout << "Optimal solution: " << opt_t_idx << endl;
        cout << "  power = ";
        cout << min_power << endl;
        cout << "  skew  = ";
        cout << opt_skew << endl;
    }

}

/***  print partitioning solution *****************************************************/
void design::printSol (string file_name, unsigned d) {
    ofstream outFile;
    if (d == 1) {
        outFile.open(file_name.c_str());
    } else {
        outFile.open(file_name.c_str(), std::ofstream::out | std::ofstream::app);
    }
    unsigned num_max, num_min;
    if (W > H) {
        num_max = W/2;
        num_min = H/2; 
    } else {
        num_max = H/2;
        num_min = W/2; 
    }
    unsigned cnt = 0;
    for (unsigned i = 0; i < num_max; ++i) {    // WIDTH
        unsigned w = (i+1)*2;
        for (unsigned j = 0; j < num_max; ++j) {    // HEIGHT
            if (i >= num_min && j >= num_min) 
                continue;
                unsigned h = (j+1)*2;
            for (unsigned m = 0; m < sols[0][i][j].size(); ++m) {
                unsigned n = (m+1)*2;
                for (unsigned idx_c = 0; idx_c < size_cap; ++idx_c) {
                    for (unsigned idx_s = 0; idx_s < size_slew; ++idx_s) {
                        for (unsigned l = 0; l < sols[0][i][j][m][idx_c][idx_s].size(); ++l) {
                            tree * t = sols[0][i][j][m][idx_c][idx_s][l];

                            outFile << t->power << " ";
                            outFile << t->max_laten - t->min_laten << " ";
                            outFile << "d " << d ;
                            outFile << " w " << w << " h " << h;
                            outFile << " n " << n;
                            outFile << " t " << t->idx; 
                            outFile << " bufs ";
                            for (unsigned p = 0; p < t->buf_sols.size(); ++p) {
                                outFile << t->buf_sols[p] << " ";
                            }
                            outFile << " sinks ";
                            for (unsigned q = 0; q < t->tree_sols.size(); ++q) {
                                outFile << t->tree_sols[q] << " ";
                            }
                            outFile << " chk ";
                            outFile << t->max_laten - t->min_laten << " ";
                            outFile << t->max_laten << " ";
                            if (idx_c < 5) {
                                outFile << idx_c + min_cap << " " ;
                            } else {
                                outFile << (idx_c+min_cap-4)*5 << " ";
                            }
                            outFile << (idx_s + min_slew)*5 << " ";
                            outFile << t->in_cap;
                            outFile << endl;
                            ++cnt;
                            delete sols[0][i][j][m][idx_c][idx_s][l];
                        }
                    }
                }
            }
        }
    }
    outFile.close();
    cout << cnt <<  " solutions" << endl;
}

/*** Evaluate whether the current solution dominates a previous solution ******/
inline bool design::evalTree (tree* t, unsigned k, unsigned i, unsigned j, unsigned m, unsigned idx_c, unsigned idx_s) {

    bool find = false;
    for (unsigned l = 0; l < sols[k][i][j][m][idx_c][idx_s].size(); ++l) {
        tree * _t = sols[k][i][j][m][idx_c][idx_s][l];
        if (t->min_laten <= _t->min_laten + toler && t->max_laten >= _t->max_laten - toler && t->power >= _t->power) {
            //cout << "DOMINATED SOL" << endl;
            find = true;
            delete t;
            break;
        } else if (t->min_laten >= _t->min_laten - toler && t->max_laten <= _t->max_laten + toler && t->power <= _t->power) {
            //cout << "UPDATE SOL" << endl;
            t->idx = sols[k][i][j][m][idx_c][idx_s][l]->idx;
            delete sols[k][i][j][m][idx_c][idx_s][l];
            sols[k][i][j][m][idx_c][idx_s][l] = t;
            find = true;
            break;
        }
    }
    if (!find) { 
        //cout << "ADD NEW SOL" << endl;
        sols[k][i][j][m][idx_c][idx_s].push_back(t);
    }
    return find;
}

/*** Evaluate whether the current solution dominates a previous solution ******/
inline void design::evalSegment(segment * s, vector<segment*>& segs) {

    bool find = false;
    for (unsigned l = 0; l < segs.size(); ++l) {
        segment * _s = segs[l];
        if (s->min_laten <= _s->min_laten + toler && s->max_laten >= _s->max_laten - toler  && s->power >= _s->power) {
            delete s;
            find = true;
            break;
        } else if (s->min_laten >= _s->min_laten - toler && s->max_laten <= _s->max_laten + toler && s->power <= _s->power) {
            delete segs[l];
            segs[l] = s;
            find = true;
            break;
        }
    }
    if (!find) 
        segs.push_back(s);

}

/*** Select tree solution ****************************************/
void design::selectTreeSol() {
    map<unsigned, pair<unsigned, unsigned>> area_map;
    map<unsigned, vector<unsigned>> sink_map;
    map<unsigned, vector<unsigned>> buf_map;
    map<float, vector<unsigned>> pow2idx_map;
    map<unsigned, unsigned> incap_map;
    list<float> pow_list; 
    list<float>::iterator powIter; 

    cout << "Reading solutions ..." << endl;
    ifstream inFile;
    string str;
    inFile.open(sol_file);
    if (!inFile.is_open()) {
        cout << "***Error :: unable to open the solution file !" << endl;
    }
    while (getline(inFile, str)) {
        stringstream ss(str);
        string token, _idx, _w, _h, _p, _s, _n, _d, _c;
        unsigned idx, w, h, n, s, d;
        float p;
        bool s_flag = false, b_flag = false;
        vector<unsigned> tmp_sinks;
        vector<unsigned> tmp_bufs;
        unsigned tmp_cap;

        ss >> _p;
        p = atof(_p.c_str());
        ss >> _s;
        s = atoi(_s.c_str());
        while (ss >> token) {
            if (token == "chk") {
                s_flag = false;
                ss >> token;
                ss >> _d;
                d = atoi(_d.c_str());
                ss >> _c;
                tmp_cap = atoi(_c.c_str());
            }
            if (s_flag) 
                tmp_sinks.push_back(atoi(token.c_str()));
            if (token == "sinks") {
                b_flag = false;
                s_flag = true;
            }
            if (b_flag) 
                tmp_bufs.push_back(atoi(token.c_str()));
            if (token == "bufs") 
                b_flag = true;
            if (token == "t") {
                ss >> _idx;
                idx = atoi(_idx.c_str());
            } else if (token == "n") {
                ss >> _n;
                n = atoi(_n.c_str());
            } else if (token == "w") {
                ss >> _w;
                w = atoi(_w.c_str());
            } else if (token == "h") {
                ss >> _h;
                h = atoi(_h.c_str());
            }
        }
        //cout << "IDX: " << idx << " " << w << " x " << h << 
        //    " SINKS " << tmp_sinks[0] <<
        //    " NUM_SINKS " << tmp_sinks.size() << " NUM_BUFS " << tmp_bufs.size() << endl;
        if (w == W && h == H && n >= num_sinks && s <= max_skew && d <= max_delay) {
            pow2idx_map[p].push_back(idx);
            pow_list.push_back(p);
        }
        area_map[idx] = make_pair(w, h);
        sink_map[idx] = tmp_sinks;
        buf_map[idx]  = tmp_bufs;
        incap_map[idx] = tmp_cap;
    }
    inFile.close();

    cout << "Finished reading solutions!" << endl;

    //sort solutions in ascending order of power
    pow_list.sort();
    pow_list.unique();

    unsigned k = 0;

    vector <unsigned> used_bp;

    for(powIter = pow_list.begin(); powIter != pow_list.end(); ++powIter) {
        cout << "Current solution power: " << *powIter << endl;
        for(unsigned i = 0; i < pow2idx_map[*powIter].size(); ++i) {
            unsigned cur_opt = pow2idx_map[*powIter][i];
            unsigned cur = cur_opt;
            stringstream bran_p_ss;
            bran_p_ss << sink_map[cur].size()*2;
            while (cur != 0) {
                cur = sink_map[cur][0]; 
                bran_p_ss << sink_map[cur].size()*2;
            }
            string bran_p = bran_p_ss.str();
            unsigned bp = atoi(bran_p.c_str());

            bool flag = true;
            //for(unsigned j = 0; j < used_bp.size(); ++j) {
            //    if (bp == used_bp[j]) {
            //        flag = false;
            //        break;
            //    }  
            //}

            if (flag) {
                used_bp.push_back(bp);

                branchs.clear();
                sink_areas.clear();
                buf_sols.clear();
               
                bool flag_cap = true;

                cur = cur_opt;
                cout << "SINK IDX: " << cur << endl;
                while (cur != 0) {
                    //if (incap_map[cur] > 100) {
                    //    cout << "Input cap. is too large: " << incap_map[cur] << endl;
                    //    flag_cap = false;
                    //    break;
                    //}
                    branchs.push_back(sink_map[cur].size()*2);
                    sink_areas.push_back(area_map[cur]);
                    buf_sols.push_back(buf_map[cur]);
                    cur = sink_map[cur][0]; 
                    cout << "SINK IDX: " << cur << endl;
                } 

                if (!flag_cap) continue;

                for (unsigned a = 0; a < sink_areas.size(); ++a) {
                    if (fmod(a,2) == 0) 
                        cout << "SINK AREA " << sink_areas[a].first << " " << sink_areas[a].second << endl;
                    else 
                        cout << "SINK AREA " << sink_areas[a].second << " " << sink_areas[a].first << endl;
                    cout << "BRANCHES  " << branchs[a] << endl;
                }
                ofstream outFile;
                stringstream outSeg_ss;
                outSeg_ss << "seg_" << k << ".txt";
                outFile.open(outSeg_ss.str());
                for (unsigned i = 0; i < sink_areas.size(); ++i) {
                    float seg = (float)sink_areas[i].first / branchs[i];
                    float    pre_dist_c = 0, cur_dist_c = 0;
                    unsigned pre_dist_d = 0, cur_dist_d = 0;
                    outFile << "LEVEL " << i+1 << " width " << sink_areas[i].first << " num_bufs ";
                    for (unsigned r = 0; r < branchs[i]/2; ++r) {
                        unsigned num = 0;
                        if (r == 0) 
                            cur_dist_c += seg/2;
                        else 
                            cur_dist_c += seg;
                        cur_dist_d  = (unsigned)nearbyint(cur_dist_c+0.001);   // do not insert buffer on additional wires
                        if (cur_dist_d != pre_dist_d) {
                            unsigned dist_d = (unsigned)nearbyint(cur_dist_c-pre_dist_c+0.001);
                            if (dist_d > max_dist) {
                                ++num;
                                dist_d -= max_dist;
                            } else {
                                ++num;
                                dist_d  = 0;
                            }
                            while (dist_d > 0) {
                                if (dist_d > max_dist) {
                                    ++num;
                                    dist_d -= max_dist;
                                } else {
                                    ++num;
                                    dist_d = 0;
                                }
                            }
                            pre_dist_d = cur_dist_d;
                            pre_dist_c = cur_dist_c;
                        }
                        outFile << num << " " ;
                    }
                    outFile << endl;
                }
                outFile.close();

                // call placeTree()
                cur_sol = k;
                cout << "Start placeTree() ..." << endl;
                placeTree(); 
                cout << "End placeTree() ..." << endl;
                ++k;
            }
            if (k >= max_solnum) break;
        }
        if (k >= max_solnum) break;
    }
}



/*** Reconstruct optimal tree solution ****************************************/
void design::reconstructTree() {
    map<unsigned, pair<unsigned, unsigned>> area_map;
    map<unsigned, vector<unsigned>> sink_map;
    map<unsigned, vector<unsigned>> buf_map;
    ifstream inFile;
    string str;
    inFile.open("out.txt");
    if (!inFile.is_open()) {
        cout << "***Error :: unable to open the solution file !" << endl;
    }
    while (getline(inFile, str)) {
        stringstream ss(str);
        string token, _idx, _w, _h;
        unsigned idx, w, h;
        bool s_flag = false, b_flag = false;
        vector<unsigned> tmp_sinks;
        vector<unsigned> tmp_bufs;
        while (ss >> token) {
            if (token == "chk") {
                s_flag = false;
            }
            if (s_flag) 
                tmp_sinks.push_back(atoi(token.c_str()));
            if (token == "sinks") {
                b_flag = false;
                s_flag = true;
            }
            if (b_flag) 
                tmp_bufs.push_back(atoi(token.c_str()));
            if (token == "bufs") 
                b_flag = true;
            if (token == "t") {
                ss >> _idx;
                idx = atoi(_idx.c_str());
            } else if (token == "w") {
                ss >> _w;
                w = atoi(_w.c_str());
            } else if (token == "h") {
                ss >> _h;
                h = atoi(_h.c_str());
            }
        }
        //cout << "IDX: " << idx << " " << w << " x " << h << 
        //    " SINKS " << tmp_sinks[0] <<
        //    " NUM_SINKS " << tmp_sinks.size() << " NUM_BUFS " << tmp_bufs.size() << endl;
        //cout << "BUF SOLS: ";
        //for (unsigned i = 0; i < tmp_bufs.size(); ++i) {
        //    cout << tmp_bufs[i] << " ";
        //}
        //cout << endl;
        area_map[idx] = make_pair(w, h);
        sink_map[idx] = tmp_sinks;
        buf_map[idx]  = tmp_bufs;
    }
    inFile.close();

    unsigned cur = opt_t_idx;
    while (cur != 0) {
        //cout << "OPT IDX: " << cur << " " << buf_map[cur].size() << endl;
        //cout << "BUF SOLS: ";
        //for (unsigned i = 0; i < buf_map[cur].size(); ++i) {
        //    cout << buf_map[cur][i] << " ";
        //}
        //cout << endl;
        branchs.push_back(sink_map[cur].size()*2);
        sink_areas.push_back(area_map[cur]);
        buf_sols.push_back(buf_map[cur]);
        cur = sink_map[cur][0]; 
    }

    /*
    for (unsigned i = 0; i < sink_areas.size(); ++i) {
        if (fmod(i,2) == 0) 
            cout << "SINK AREA " << sink_areas[i].first << " " << sink_areas[i].second << endl;
        else 
            cout << "SINK AREA " << sink_areas[i].second << " " << sink_areas[i].first << endl;
        cout << "BRANCHES  " << branchs[i] << endl;
    }
    */

    // print seg.txt

    ofstream outFile;
    stringstream outSeg_ss;
    outSeg_ss << "seg.txt";
    outFile.open(outSeg_ss.str());
    for (unsigned i = 0; i < sink_areas.size(); ++i) {
        float seg = (float)sink_areas[i].first / branchs[i];
        float    pre_dist_c = 0, cur_dist_c = 0;
        unsigned pre_dist_d = 0, cur_dist_d = 0;
        outFile << "LEVEL " << i+1 << " width " << sink_areas[i].first << " num_bufs ";
        for (unsigned r = 0; r < branchs[i]/2; ++r) {
            unsigned num = 0;
            if (r == 0) 
                cur_dist_c += seg/2;
            else 
                cur_dist_c += seg;
            cur_dist_d  = (unsigned)nearbyint(cur_dist_c+0.001);   // do not insert buffer on additional wires
            if (cur_dist_d != pre_dist_d) {
                unsigned dist_d = (unsigned)nearbyint(cur_dist_c-pre_dist_c+0.001);
                if (dist_d > max_dist) {
                    ++num;
                    dist_d -= max_dist;
                } else {
                    ++num;
                    dist_d  = 0;
                }
                while (dist_d > 0) {
                    if (dist_d > max_dist) {
                        ++num;
                        dist_d -= max_dist;
                    } else {
                        ++num;
                        dist_d = 0;
                    }
                }
                pre_dist_d = cur_dist_d;
                pre_dist_c = cur_dist_c;
            }
            outFile << num << " " ;
        }
        outFile << endl;
    }
    outFile.close();
}

//#define CPLEX_CLUSTERING
#ifdef CPLEX_CLUSTERING

/*** Random walk-based clustering to balance load ************************************/
void design::cluster(vector<pin*> in_pins, vector<vector<pin*>>& out_pins, vector<pair<float, float>> in_locs, vector<pair<float, float>>& out_locs, float num_regions) {

    if (verbose > 1) {
    cout << "IN_PINS " <<  in_pins.size() << endl;
    cout << "IN_LOCS " ;
    for (unsigned i = 0; i < in_locs.size(); ++i) {
        cout << "(" << in_locs[i].first << ", " << in_locs[i].second << ") ";
    }
    cout << endl; 
    }

   // estimate wire cap 
   float x1 = INT_MAX, y1 = INT_MAX, x2 = 0, y2 = 0;
   for (unsigned i = 0; i < in_pins.size(); ++i) {
       pin * p = in_pins[i];
       if (x1 > p->x) 
           x1 = p->x;
       if (x2 < p->x) 
           x2 = p->x;
       if (y1 > p->y) 
           y1 = p->y;
       if (y2 < p->y) 
           y2 = p->y;
   }
   float avg_hpwl     = ((x2-x1)+(y2-y1)) / in_locs.size() / num_regions; 
   float avg_num      = float(in_pins.size()) / in_locs.size() / num_regions;
   float avg_wire_cap = 2.931 * avg_hpwl + 0.2781 * avg_num;

    //cout << "pinBox: (" << x1 << ", " << y1 << ", " << x2 << ", " << y2 << ")" << endl;

   out_pins.resize(in_locs.size()); // clustering solution

   vector<float> caps;
   caps.resize(in_locs.size(), 0);  // record total cap of each cluster

    //Pre-calculate Manhattan dist between branching point to sinks
    vector < vector<float> > dist;
    for (unsigned i = 0; i < in_pins.size(); ++i) {
        pin * p = in_pins[i];
        vector<float> temp_dist;
        for (unsigned j = 0; j < in_locs.size(); ++j) {
            float t_dist = fabs(in_locs[j].first - p->x) + fabs(in_locs[j].second - p->y);
            temp_dist.push_back(t_dist); 
        }
        dist.push_back(temp_dist);
    }

    IloEnv env;
    try {

    bool flag = 0;
    unsigned gap = 1;
    unsigned thd = 40;
    while(!flag) {
        IloExpr objective(env);
        
        IloRangeArray Consts(env);

        vector < vector <IloNumVar> > var;
        vector < IloNumVar > d;
        vector < IloNumVar > c;
        vector < IloNumVar > x_min;
        vector < IloNumVar > y_min;
        vector < IloNumVar > x_max;
        vector < IloNumVar > y_max;
        IloNumVar d_max;
       
        // generate binary variables 
        char buffer [50];
        for (unsigned i = 0; i < in_locs.size(); ++i) {
            sprintf(buffer, "cap_%d", i);
            IloNumVar temp = IloNumVar(env, 0, 10000000, ILOFLOAT, buffer);
            c.push_back(temp);
        }

        if (in_pins.size() < thd) { 
            // bottom level
            for (unsigned i = 0; i < in_locs.size(); ++i) {
                float weight = 8/in_locs.size();
                sprintf(buffer, "x_min_%d", i);
                IloNumVar temp = IloNumVar(env, 0, 10000000, ILOFLOAT, buffer);
                x_min.push_back(temp);
                sprintf(buffer, "x_max_%d", i);
                temp = IloNumVar(env, 0, x2, ILOFLOAT, buffer);
                x_max.push_back(temp);
                sprintf(buffer, "y_min_%d", i);
                temp = IloNumVar(env, 0, 10000000, ILOFLOAT, buffer);
                y_min.push_back(temp);
                sprintf(buffer, "y_max_%d", i);
                temp = IloNumVar(env, 0, y2, ILOFLOAT, buffer);
                y_max.push_back(temp);
                objective += weight*(y_max[i] - y_min[i] + x_max[i] - x_min[i]);
            }
        } else {
            // top level
            sprintf(buffer, "d_max");
            d_max = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);

            objective = 10*d_max;
        }

        for (unsigned i = 0; i < in_pins.size(); ++i) {
            vector <IloNumVar> v_temp;
            for (unsigned j = 0; j < in_locs.size(); ++j) {
                sprintf(buffer, "cell_%d_%d", i, j);
                IloNumVar temp = IloNumVar(env, 0, 1, ILOINT, buffer);
                v_temp.push_back(temp); 
            }

            sprintf(buffer, "dist_%d", i);
            IloNumVar tmp_d = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);

            d.push_back(tmp_d);
            var.push_back(v_temp);
        }

        // ensure that one cell is assigned to exactly one cluster
        for (unsigned i = 0; i < in_pins.size(); ++i) {
            IloExpr ex(env);
            for (unsigned j = 0; j < in_locs.size(); ++j) {
                ex += var[i][j];
            }
            Consts.add(ex == 1);
        }
       
        if (in_pins.size() < thd) { 
            // bounding_box
            for (unsigned j = 0; j < in_locs.size(); ++j) {
                for (unsigned i = 0; i < in_pins.size(); ++i) {
                    IloExpr ex(env);
                    ex = x_min[j] - in_pins[i]->x - 10000*(1 - var[i][j]);
                    Consts.add(ex <= 0);
                    ex = y_min[j] - in_pins[i]->y - 10000*(1 - var[i][j]);
                    Consts.add(ex <= 0);
                    ex = x_max[j] - in_pins[i]->x*var[i][j];
                    Consts.add(ex >= 0);
                    ex = y_max[j] - in_pins[i]->y*var[i][j];
                    Consts.add(ex >= 0);
                }
            }
        }

        // calculate distance between each sink and the assigned cluster
        for (unsigned i = 0; i < in_pins.size(); ++i) {
            IloExpr ex(env);
            ex = d[i];
            for (unsigned j = 0; j < in_locs.size(); ++j) {
                ex -= dist[i][j]*var[i][j];
            }
            Consts.add(ex == 0);
        
            objective += d[i];
            if (in_pins.size() >= thd) {
                ex = d_max - d[i];
                Consts.add(ex >= 0);
            }
        }

        //if (in_locs.size() > 2) {
        //}
        for (unsigned j = 0; j < in_locs.size(); ++j) {
            IloExpr ex(env);
            
            if (in_pins.size() < thd) {
                //HPWL
                ex = c[j] - 2.931*(x_max[j] - x_min[j] + y_max[j] - y_min[j]);
                for (unsigned i = 0; i < in_pins.size(); ++i) {
                    float c_cap = in_pins[i]->cap + 0.2781;
                    ex -= c_cap*var[i][j];
                }
                Consts.add(ex == 0);
                ex = c[j];
                Consts.add(ex >= floor((avg_cap + avg_wire_cap - gap)*num_regions));
                Consts.add(ex <= ceil((avg_cap + avg_wire_cap + gap)*num_regions));
            } else {
                ex = c[j];
                for (unsigned i = 0; i < in_pins.size(); ++i) {
                    float c_cap = in_pins[i]->cap;
                    ex -= c_cap*var[i][j];
                }
                Consts.add(ex == 0);
                ex = c[j];
                Consts.add(ex >= floor((avg_cap - gap)*num_regions));
                Consts.add(ex <= ceil((avg_cap + gap)*num_regions));
            }
        }

        IloModel model(env);
        model.add(IloMinimize(env, objective));
        model.add(Consts);

        IloCplex cplex(model);

        //cplex.setParam(IloCplex::Threads, 5);
        cplex.setParam(IloCplex::Threads, 1);
        cplex.setParam(IloCplex::EpGap, 0.05);
        cplex.setParam(IloCplex::TiLim, 60);
        //cplex.setOut(env.getNullStream());

        string lpFile = "dist.lp";
        string solFile = "dist.sol";
        //cplex.exportModel(lpFile.c_str());

        if (cplex.solve()) {
            if (verbose > 2) {
                env.out() << "Clustering Feasible " << cplex.getStatus() << endl;
                env.out() << "Solution value = " << cplex.getObjValue() << endl;
                if (in_pins.size() >= thd) {
                env.out() << "min: " << floor((avg_cap - gap)*num_regions) 
                         << " max: " << ceil((avg_cap + gap)*num_regions)  << endl;
                } else {
                env.out() << "min: " << floor((avg_cap + avg_wire_cap - gap)*num_regions) 
                         << " max: " << ceil((avg_cap + avg_wire_cap + gap)*num_regions)  << endl;
                }
            }
            //cplex.writeSolution(solFile.c_str());
            for (unsigned i = 0; i < in_pins.size(); ++i) {
                for (unsigned j = 0; j < in_locs.size(); ++j) {
                    IloNum bin = cplex.getValue(var[i][j]);
                    if (bin > 0.5) {
                        pin * p = in_pins[i];
                        out_pins[j].push_back(p);
                        caps[j] += p->cap;
                    }
                }
            }

            flag = 1;
        } else {
            if (verbose > 2) {
                env.out() << "Infeasible!!!" << endl;
                if (in_pins.size() >= thd) {
                env.out() << "min: " << floor((avg_cap - gap)*num_regions) 
                         << " max: " << ceil((avg_cap + gap)*num_regions)  << endl;
                } else {
                env.out() << "min: " << floor((avg_cap + avg_wire_cap - gap)*num_regions) 
                         << " max: " << ceil((avg_cap + avg_wire_cap + gap)*num_regions)  << endl;
                }
            }
            flag = 0;
            ++gap;
        }
        cplex.end();
    } 
    } catch (IloException& ex){
	      cerr << "Error: " << ex << endl;
        cerr << ex.getMessage() << endl;
    }

    env.end();

    if (verbose > 2) {
    for (unsigned j = 0; j < in_locs.size(); ++j) {
        float max_dist = 0;
        
        for (unsigned i = 0; i < out_pins[j].size(); ++i) {
            pin * p = out_pins[j][i];
            float tmp_dist = fabs(in_locs[j].first - p->x) + fabs(in_locs[j].second - p->y);
            if (tmp_dist > max_dist) max_dist = tmp_dist;
        }
        cout << "CHK " << j << "th cluster max_dist: " <<  max_dist << endl;
    }
    }

    // calculate weighted center
    out_locs.clear();
    for (unsigned i = 0; i < out_pins.size(); ++i) {
        float tmp_x = 0, tmp_y = 0; 
        for (unsigned j = 0; j < out_pins[i].size(); ++j) {
            tmp_x += out_pins[i][j]->x;
            tmp_y += out_pins[i][j]->y;
        }
        tmp_x = tmp_x / out_pins[i].size();
        tmp_y = tmp_y / out_pins[i].size();
        out_locs.push_back(make_pair(tmp_x, tmp_y));
    }

    //if (verbose > 1) {
    cout << "OUT_PINS ";
    for (unsigned i = 0; i < out_pins.size(); ++i) {
        cout <<  out_pins[i].size() << " ";
    }
    cout << endl;
    cout << "OUT_LOCS " ;
    for (unsigned i = 0; i < out_locs.size(); ++i) {
        cout << "(" << out_locs[i].first << ", " << out_locs[i].second << ") ";
    }
    cout  << endl;
    //}

}

#else

void design::cluster(vector<pin*> in_pins, vector<vector<pin*>>& out_pins, 
	vector<pair<float, float>> in_locs, vector<pair<float, float>>& out_locs, float num_regions,
	float xBranch, float yBranch) {
	
	static int i = 0;
	
	CKMeans::clustering c(in_pins, xBranch, yBranch);
	c.setPlotFileName("cluster-" + std::to_string(i++) + ".py");
	out_locs = in_locs;
	c.iterKmeans(1, in_locs.size(), in_pins.size()/in_locs.size(), 0, out_locs, 5);
	c.getClusters(out_pins);

	cout << "OUT_PINS (size " << out_pins.size() << ")\n";
    for (unsigned i = 0; i < out_pins.size(); ++i) {
        cout <<  out_pins[i].size() << " ";
    }
    cout << endl;
    cout << "OUT_LOCS " ;
    for (unsigned i = 0; i < out_locs.size(); ++i) {
        cout << "(" << out_locs[i].first << ", " << out_locs[i].second << ") ";
    }
    cout  << endl;
	
	//std::exit(1);
}
	
#endif

void design::printSol(vector<solution*> &all_sols) {

    ofstream fout;
    stringstream  sol_ss;
    sol_ss << "sol" << ".py";
    fout.open(sol_ss.str());
    fout << "#! /home/kshan/anaconda2/bin/python" << endl << endl;
    fout << "import numpy as np" << endl;
    fout << "import matplotlib.pyplot as plt" << endl;
    fout << "import matplotlib.path as mpath" << endl;
    fout << "import matplotlib.lines as mlines" << endl;
    fout << "import matplotlib.patches as mpatches" << endl;
    fout << "from matplotlib.collections import PatchCollection" << endl;

    fout << endl;
    fout << "fig, ax = plt.subplots()" << endl;
    fout << "patches = []" << endl;

    for (int j = 0; j < blks.size(); ++j) {
        blockage* cBlk = blks[j];
        fout << "rect = mpatches.Rectangle(["
             << cBlk->x1 << ", " << cBlk->y1 << "], "
             << cBlk->x2 - cBlk->x1 << ", "
             << cBlk->y2 - cBlk->y1 << ", ec=\"none\")" << endl;
        fout << "patches.append(rect)" << endl;
    }

    fout << "colors = [\"red\"]" << endl;
    fout << "collection = PatchCollection(patches, alpha=1)" << endl;
    fout << "collection.set_color(colors)" << endl;
    fout << "collection.set_edgecolor(\"black\")" << endl;
    fout << "collection.set_linewidth(1)" << endl;
    fout << "ax.add_collection(collection)" << endl << endl;

    map <int, pair<float, float> > idx2Loc;
    pair <float, float> center;
    center.first = W/2.0 + xoffset;
    center.second = H/2.0 + yoffset;
    idx2Loc[0] = center;
    for (int j = 0; j < all_sols.size(); ++j) {
      solution * sol = all_sols[j];
      //if (all_sols[all_sols.size()-1]->lvl == sol->lvl) {
      //    break;
      //}
      
	  //cout << "j: "<< j << " lvl of j:" << sol->lvl << " parent" << idx2Loc[sol->idx].first << " " << idx2Loc[sol->idx].second << "\n";
	  
      for (int i = 0; i < sol->locs.size(); ++i) {
        idx2Loc[sol->subtrees[i]] = sol->locs[i];
		//cout << " subtree" << sol->subtrees[i] << " loc " << sol->locs[i] << "\n";
      }  
    }

	std::array<char, 8> colors = {'b', 'r', 'g', 'm', 'c', 'y', 'b', 'r'};
    for (int j = 0; j < all_sols.size(); ++j) {
      solution * sol = all_sols[j];
	  //cout << "j = " << j << " idx = " << sol->idx << "\n";
	  
      //if (all_sols[all_sols.size()-1]->lvl == sol->lvl) {
      //    continue;
      //}
      
      for (int i = 0; i < sol->locs.size(); ++i) {
        int nIdx1;
        int nIdx2;
        if (i < sol->locs.size()/2 - 1) {
          nIdx1 = sol->subtrees[i];
          nIdx2 = sol->subtrees[i + 1];
        } else if (i < sol->locs.size()/2) {
          nIdx1 = sol->idx;
          nIdx2 = sol->subtrees[i];
        } else if (i >= sol->locs.size()/2 + 1) {
          nIdx1 = sol->subtrees[i];
          nIdx2 = sol->subtrees[i - 1];
        } else if (i >= sol->locs.size()/2 ) {
          nIdx1 = sol->idx;
          nIdx2 = sol->subtrees[i];
        }
     
        //cout << "Loc pair " << nIdx1 << " " << nIdx2 << endl;
        //cout << "Loc pair " << idx2Loc[nIdx1] << " " << idx2Loc[nIdx2] << endl;
		
		char colorIdx = std::min(sol->lvl, (int)colors.size()-1);
        fout << "plt.plot((" << idx2Loc[nIdx1].first << ", "
             << idx2Loc[nIdx2].first << "), ("
             << idx2Loc[nIdx1].second << ", "
             << idx2Loc[nIdx2].second << "), color='" << (char)colors[colorIdx] << "')"
             << endl;
      }
    }


	// MF @ 190214: ploting sinks
	fout << "\n\n";
	for (unsigned i = 0; i < pins.size(); i++) {
		fout << "plt.scatter(" << pins[i]->x+xoffset << ", " << pins[i]->y+yoffset << ", color='k', s=2)\n";  
	}

	fout << "plt.plot((" << xoffset << ", " << xoffset+W << ", " << xoffset+W << ", " << xoffset << ", " << xoffset <<
			"), (" << yoffset << ", " << yoffset << ", " << yoffset+H << ", " << yoffset+H << ", " << yoffset << "), color='g')\n";  
	
	fout << "plt.axis('equal')\n";
	// ---

    fout << "plt.show()" << endl;
    fout.close();
}

void design::placeTree() {
    
    //parse blockage
    parseBlks();
    unsigned max_d = branchs.size();
    //cout << "Total depth: " << max_d << endl;

    vector<vector<region*>> regions;
    vector<region*> dummy;
    unsigned idx   = 0;
    region* r = new region(idx++, 1, W, H, branchs[0], W/2.0, H/2.0, 0.0, 0.0, pins);
    regions.push_back(dummy);
    regions[0].push_back(r);

    vector<pair<unsigned, vector<pin*>>> bottom_sols;
    vector<solution*> all_sols;

    // top-down tree construction
    for (unsigned lvl = 0; lvl < regions.size(); ++lvl) {
            
        cout << "Depth: " << lvl + 1 << endl;
        //unsigned idx_base = 0;
        //for (unsigned lvl2 = 0; lvl2 < lvl; ++lvl2) {
        //    idx_base += regions[lvl2].size();
        //}

        float _act_w = 0;
        _act_w = sink_areas[lvl].first;


        //#pragma omp parallel for
        for (unsigned k = 0; k < regions[lvl].size(); ++k) {

            region* r = regions[lvl][k];

            if (verbose > 1) {
            cout << "LEVEL " << r->lvl << endl;
            cout << "Area " << r->w << " x " << r->h << endl;
            cout << "SINK_NUM " << r->num_sinks << endl;
            }

            vector<vector<pin*>> out_pins;
            vector<pair<float, float>> out_locs;
            vector<pair<float, float>> in_locs;
            vector<pair<float, float>> orig_locs;
       
            // for each sink region, have a vector of pointers to blockages 
            // that are overlapped to the sink region
            vector< vector<blockage*> > blks_in_sinks; 

            if (verbose > 1) {
            cout << "Blockage number: " << blks.size() << endl;
            cout << "Blockages: " << endl;
            for (unsigned i = 0; i < blks.size(); ++i) {
                cout << " (" << blks[i]->x1 << ", "
                     << blks[i]->y1 << ", "
                     << blks[i]->x2 << ", "
                     << blks[i]->y2 << ") " << endl;
            }
            }

            float seg, _seg;
            float _w, _h;

            bool isBlkExist = false;
               
            _seg    = _act_w / r->num_sinks;

            // horizontal 
            if (fmod(r->lvl, 2) == 1) {
                seg     = r->w / r->num_sinks;
                _w      = r->w / r->num_sinks;
                _h      = r->h;

                // branching points
                float y = r->h / 2 + r->o_y;
                float x = r->o_x;
                // check the overlap between sink region and blockage
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    if (i == 0) {
                        x += seg / 2;
                    } else {
                        x += seg;
                    }
                    in_locs.push_back(make_pair(x, y));
                    blockage sink_region(x - seg/2, r->o_y, x + seg/2, r->h + r->o_y); 
                    if (verbose > 1) {
                    cout << "SINK REGION: " << sink_region << endl;
                    }
                    vector <blockage*> tmp_blks;
                    for (unsigned b = 0; b < blks.size(); ++b) {
                        if (blks[b]->isOverlap(sink_region)) {
                            tmp_blks.push_back(blks[b]);
                            isBlkExist = true;
                        }
                    }
                    blks_in_sinks.push_back(tmp_blks);
                }
                
                // origins
                x = r->o_x, y = r->o_y;
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    orig_locs.push_back(make_pair(x, y));
                    x += seg;
                }
            } else {
            // vertical
                seg     = r->h / r->num_sinks;
                _w      = r->w;
                _h      = r->h / r->num_sinks;

                // branching points
                float x = r->w / 2 + r->o_x;
                float y = r->o_y;
                // check the overlap between sink region and blockage
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    if (i == 0) {
                        y += seg / 2;
                    } else {
                        y += seg;
                    }
                    in_locs.push_back(make_pair(x, y));
                    blockage sink_region(r->o_x, y - seg/2, r->o_x + r->w, y + seg/2); 
                    if (verbose > 1) {
                    cout << "SINK REGION: " << sink_region << endl;
                    }
                    vector <blockage*> tmp_blks;
                    for (unsigned b = 0; b < blks.size(); ++b) {
                        if (blks[b]->isOverlap(sink_region)) {
                            tmp_blks.push_back(blks[b]);
                            isBlkExist = true;
                        }
                    }
                    blks_in_sinks.push_back(tmp_blks);
                }

                // origins
                x = r->o_x, y = r->o_y;
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    orig_locs.push_back(make_pair(x, y));
                    y += seg;
                }
            }

            // scale buffer solution distances
            vector<float> actual_dists;
            vector<unsigned> round_dists;
            // actual_dist = segment length between center of sink regions
            // round_dist = int(actual_dist)
            // seg / tmp_dist   = real implementation
            // _seg / _tmp_dist = size used in DP
            for (unsigned i = 0; i < r->num_sinks/2; ++i) {
                float tmp_dist = seg, _tmp_dist = _seg;
                if (i == 0) {
                    tmp_dist   =  seg / 2.0;       
                    _tmp_dist  = _seg / 2.0;       
                }
                actual_dists.push_back(tmp_dist);
                int tmp_round_dist = (unsigned)nearbyint(_tmp_dist+0.001);
                if (tmp_round_dist == 0) 
                    tmp_round_dist = 1;
                round_dists.push_back(tmp_round_dist);
            }

            if (verbose > 1) {
            cout << "ISBLKEXIST: " << isBlkExist << endl;
            cout << "2 Blockages: " << blks_in_sinks.size() << endl;
            for (unsigned i = 0; i < blks_in_sinks.size(); ++i) {
                for (unsigned j = 0; j < blks_in_sinks[i].size(); ++j) {
                    cout << *blks_in_sinks[i][j] << " ";
                }
                cout << endl;
            }
            }

            if (verbose > 1) {
            cout << "III " << r->num_sinks << " " << buf_sols[lvl].size() << endl; 
            }
            vector<float> ratios; // ratios to scale buffer solutions
            //
            if (buf_sols[lvl].size() == 1 && buf_sols[lvl].size() < r->num_sinks/2.0) {
                if (verbose > 1) {
                cout << "ONE BUF 1" << endl;
                }
                float ratio = 0.9 * actual_dists[0] / round_dists[0]; // add 0.9 scaling to ensure buffer drives all fanouts
                if (verbose > 1) {
                cout << "actual/round " << actual_dists[0] << " " << round_dists[0] << endl;
                }
                for (unsigned i = 0; i < buf_sols[lvl].size(); ++i) 
                    ratios.push_back(ratio);
            } else {
                unsigned segIdx = 0, cnt = 0;
                float remain_dist = round_dists[segIdx];
                for (unsigned i = 0; i < buf_sols[lvl].size(); ++i) {
                    if (verbose > 1) {
                    cout << "REMAIN_DIST " << remain_dist << " " << i << endl;
                    }
                    LUT * lut = lut_map[buf_sols[lvl][i]];
                    if (verbose > 1) {
                    cout << "IDX " << lut->idx << " LEN " << lut->len << endl;
                    }
                    remain_dist -= lut->len;
                    ++cnt;
                    cout << actual_dists[segIdx] << " " << round_dists[segIdx] << endl;
                    if (remain_dist <= 0) {
                        // change here
                        if (verbose > 1) {
                        cout << "actual/round " << actual_dists[segIdx] << " " << round_dists[segIdx] << endl;
                        }
                        float ratio = actual_dists[segIdx] / round_dists[segIdx];
                        /////
                        for (unsigned j = 0; j < cnt; ++j) {
                            ratios.push_back(ratio);
                        }
                        remain_dist = round_dists[++segIdx];
                        cnt = 0;
                    }
                }
            }
            if (verbose > 1) {
            cout << "SEG LEN " << seg  << endl;
            }
             
            // half branch
            vector<float> bufLocs;
            float tot_len = 0;
            for (unsigned i = 0; i < buf_sols[lvl].size(); ++i) {
                LUT * lut = lut_map[buf_sols[lvl][i]];
                if (verbose > 1) {
                cout << "BUF NUMBERS " << lut->lens.size() << endl;
                }
                vector<float> tmpLocs; // for each LUT item 
                for (unsigned j = 0; j < lut->lens.size(); ++j) {
                    if (verbose > 1) {
                    cout << i << " " << j << " SUBSEG LEN " << lut->lens[j] << " " << ratios[i] << " " << tot_len <<endl;
                    }
                    tmpLocs.push_back(tot_len + lut->lens[j]*ratios[i]);
                }
                if (verbose > 1) {
                cout << "LUT " << lut->idx << " LEN " << lut->len << " RATIO " << ratios[i]  << endl;
                }
                tot_len += lut->len * ratios[i];
                if (verbose > 1) {
                cout << "STEP " << i << " " << tot_len << endl;
                }
                bufLocs.insert(bufLocs.end(), tmpLocs.begin(), tmpLocs.end());
            }
            cout << "TOT LEN " << tot_len << " " << buf_sols[lvl].size() << " " << r->num_sinks/2.0 << endl;

            if (buf_sols[lvl].size() == 1 && buf_sols[lvl].size() < r->num_sinks/2.0) {
                cout << "ONE BUFFER" << endl;
                tot_len = 0;
                for (unsigned i = 0; i < actual_dists.size();  ++i) 
                    tot_len += actual_dists[i];
            }

            // mirror
            vector<float> m_bufLocs;
            for (int i = bufLocs.size()-1; i >= 0; --i) 
                m_bufLocs.push_back(tot_len - bufLocs[i]);

            // shift to right
            for (unsigned i = 0; i < bufLocs.size(); ++i) 
                bufLocs[i] += tot_len;

            // merge 
            bufLocs.insert(bufLocs.begin(), m_bufLocs.begin(), m_bufLocs.end());
             
            tot_len *= 2;

            sort(bufLocs.begin(), bufLocs.end());
            cout << "TOT_LENGTH: " << tot_len << endl;
            cout << "BUF LOCs ";
            for (unsigned i = 0; i < bufLocs.size(); ++i) {
                cout << bufLocs[i] << " ";
            }
            cout << endl;
			
            float num = (num_sinks*_w*_h)/(W*H);
			
			cout << "r_loc (" << r->r_x << ", " << r->r_y << ")\n";
			for (unsigned i = 0; i < in_locs.size(); ++i) {
				std::cout << "\t(" << in_locs[i].first << ", " << in_locs[i].second << ")\n";
			}
			
            cluster(r->pins, out_pins, in_locs, out_locs, num, r->r_x, r->r_y); 
            if (verbose > 1) {
            for (unsigned i = 0; i < in_locs.size(); ++i) {
                cout << endl;
                cout << "IN  " << in_locs[i].first << ", "  << in_locs[i].second << endl;;
                cout << "OUT " << out_locs[i].first << ", " << out_locs[i].second  << endl;
                float max_dist_1 = 0, max_dist_2 = 0;
                float xl = INT_MAX, yl = INT_MAX, xu = 0, yu = 0;
                float cap = 0;
                for (unsigned j = 0; j < out_pins[i].size(); ++j) {
                    pin * p = out_pins[i][j];
                    cap += p->cap;
                    float tmp_dist = calcDist(make_pair(float(p->x), float(p->y)), in_locs[i]);
                    if (tmp_dist > max_dist_1) {
                        max_dist_1 = tmp_dist;
                    }
                    tmp_dist = calcDist(make_pair(float(p->x), float(p->y)), out_locs[i]);
                    if (tmp_dist > max_dist_2) {
                        max_dist_2 = tmp_dist;
                    }
                    if (p->x < xl) 
                        xl = p->x;
                    if (p->x > xu) 
                        xu = p->x;
                    if (p->y < yl) 
                        yl = p->y;
                    if (p->y > yu) 
                        yu = p->y;
                }
                cout << "DIST    " << max_dist_1 << " " << max_dist_2 << endl;
                cout << "BBOX    " << xl << ", " << yl << " " << xu << ", " << yu << endl;
                cout << "P/W CAP " << cap << " " << 2.931*((xu-xl)+(yu-yl)) + 0.2781*out_pins[i].size() << endl;
                cout << "-------------------------------" << endl;
            }
            cout << endl;
            }

            float tot_seg_length = seg * (r->num_sinks - 1);
            // check total length (sum of buffer solutions) ==
            //       length of current tree level in this region
            cout << "CHECKING" << " " << seg << endl;
            if (tot_seg_length != tot_len) {
                cout << tot_len << " " << tot_seg_length << endl;
            }
            cout << endl;

            vector <bool> isBuffer;
            vector <float> rel_dist;

            int bufIdx = 0;
            float pLoc = 0;
            float cLoc = 0;
            for (unsigned i = 0; i < in_locs.size() + 1; ++i) {
              isBuffer.push_back(0);
              float LB, UB;
              if (i == in_locs.size() / 2 - 1) {
                LB = seg*i;
                UB = seg*i + seg/2.0;
              } else if ( i == in_locs.size() / 2 ) {
                LB = seg*(i - 1) + seg/2.0;
                UB = seg*i;
              } else if ( i > in_locs.size() / 2) {
                LB = seg*(i-1);
                UB = seg*i;
              } else {
                LB = seg*i;
                UB = seg*(i+1);
              }
              if (i != 0) {
                cLoc = LB;
                rel_dist.push_back(cLoc - pLoc);
                pLoc = cLoc;
              }
              while (bufIdx < bufLocs.size() && 
                      ((bufIdx < bufLocs.size()/2 && LB <= bufLocs[bufIdx] && UB > bufLocs[bufIdx]) 
                    || (bufIdx >= bufLocs.size()/2 && LB < bufLocs[bufIdx] && UB >= bufLocs[bufIdx])) ) {
                 isBuffer.push_back(1);
                 cLoc = bufLocs[bufIdx];
                 rel_dist.push_back(cLoc - pLoc); // be carefull about cases rel_dist = 0
                 pLoc = cLoc;
                 ++bufIdx;
              }
            }

            vector<pair<float, float>> opt_locs;
            vector<pair<float, float>> opt_buf_locs;
            in_locs.clear();
            for (unsigned i = 0; i < out_locs.size(); ++i) {
                if (i == out_locs.size() / 2) {
                    in_locs.push_back(make_pair(r->r_x, r->r_y));
                }
                in_locs.push_back(out_locs[i]);
            //    opt_locs.push_back(out_locs[i]);
            }
          
            /*
            if (!isBlkExist) {
                cout << "NO buffer existing" << endl;
                isBuffer.clear();
                blks_in_sinks.clear();
                rel_dist.clear();

                for (unsigned i=0; i < in_locs.size() - 1; ++i) {
                    isBuffer.push_back(0);
                    rel_dist.push_back(abs(in_locs[i].first - in_locs[i].first) + abs(in_locs[i].second - in_locs[i].second));
                }
                isBuffer.push_back(0);
            }
            */

            for (unsigned i=0; i < in_locs.size(); ++i) {
                cout << "(" << in_locs[i].first << " " << in_locs[i].second << ") ";
            }
            cout << endl;
            for (unsigned i=0; i < isBuffer.size(); ++i) {
                cout << isBuffer[i] << " ";
            }
            cout << endl;
            cout << "REL_DIST: " << endl;
            for (unsigned i=0; i < rel_dist.size(); ++i) {
                cout << rel_dist[i] << " "; 
            }
            cout << endl;

			cout << "In_locs size = " << in_locs.size() << "\n";
			cout << "opt_locs size = " << opt_locs.size() << "\n";
			cout << "opt_buf_locs size = " << opt_buf_locs.size() << "\n";
			cout << "isBuffer size = " << isBuffer.size() << "\n";
			cout << "rel_dist size = " << rel_dist.size() << "\n";
			cout << "blks_in_sinks size = " << blks_in_sinks.size() << "\n";
//#ifdef CPLEX_CLUSTERING
			findLocsGreedy(in_locs, opt_locs, opt_buf_locs, isBuffer, rel_dist, blks_in_sinks);
            //findLocs(in_locs, opt_locs, opt_buf_locs, isBuffer, rel_dist, blks_in_sinks);
//#endif			
			cout << "In_locs size = " << in_locs.size() << "\n";
			cout << "opt_locs size = " << opt_locs.size() << "\n";
			cout << "opt_buf_locs size = " << opt_buf_locs.size() << "\n";
			cout << "isBuffer size = " << isBuffer.size() << "\n";
			cout << "rel_dist size = " << rel_dist.size() << "\n";
			cout << "blks_in_sinks size = " << blks_in_sinks.size() << "\n";
				
            if (verbose > 1) {
            cout << "OPT_LOCS " ;
            for (unsigned i = 0; i < opt_locs.size(); ++i) {
                cout << "(" << opt_locs[i].first << ", " << opt_locs[i].second << ") ";
            }
            cout << endl; 

            cout << "OPT_BUF_LOCS " ;
            for (unsigned i = 0; i < opt_buf_locs.size(); ++i) {
                cout << "(" << opt_buf_locs[i].first << ", " << opt_buf_locs[i].second << ") ";
            }
            cout << endl; 

            for (unsigned i = 0; i < opt_locs.size(); ++i) {
                float max_dist = 0;
                for (unsigned j = 0; j < out_pins[i].size(); ++j) {
                    pin * p = out_pins[i][j];
                    float tmp_dist = calcDist(make_pair(float(p->x), float(p->y)), opt_locs[i]);
                    if (tmp_dist > max_dist) {
                        max_dist = tmp_dist;
                    }
                }
                cout << "OPT_DIST " << max_dist << endl ;
            }
            cout << "-------------------------------" << endl;
            cout << endl;
            }
            unsigned cur_l = 1;

            #pragma omp critical (update)
            {
            cur_l = r->lvl+1;
            solution * sol = new solution();
            sol->idx = r->idx; 
            sol->lvl = r->lvl; 
            for (unsigned i = 0; i < buf_sols[r->lvl-1].size(); ++i) {
                sol->buf_sols.push_back(buf_sols[r->lvl-1][i]);
            }
            for (unsigned i = 0; i < opt_buf_locs.size(); ++i) {
                sol->buf_locs.push_back(make_pair(opt_buf_locs[i].first, opt_buf_locs[i].second));
            }
            if (cur_l <= max_d) {
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    sol->locs.push_back(make_pair(opt_locs[i].first, opt_locs[i].second));
                    sol->subtrees.push_back(idx);
                    region* _r = new region (idx++, cur_l, _w, _h, branchs[cur_l-1], opt_locs[i].first, opt_locs[i].second, orig_locs[i].first, orig_locs[i].second, out_pins[i]);
                    if (regions.size() <= r->lvl) {
                        regions.push_back(dummy);
                    }
                    regions[r->lvl].push_back(_r);
                }
            } else {
                for (unsigned i = 0; i < r->num_sinks; ++i) {
                    sol->locs.push_back(make_pair(opt_locs[i].first, opt_locs[i].second));
                    sol->subtrees.push_back(idx);
                    bottom_sols.push_back(make_pair(idx, out_pins[i]));
                    ++idx;
                }
            }
            all_sols.push_back(sol);
            delete r;
            }
        }
    }

	// MF @ 180207: Fixing blockages, branching points and buffer locations
	for (unsigned i = 0; i < blks.size(); i++) {
		blockage* blk = blks[i];
		blk->x1 += xoffset;
		blk->x2 += xoffset;
		blk->y1 += yoffset;
		blk->y2 += yoffset;
	}
	
	for (unsigned j = 0; j < all_sols.size(); ++j) {
		solution * sol = all_sols[j];
		for (unsigned i = 0; i < sol->locs.size(); ++i) {
			sol->locs[i].first += xoffset;
			sol->locs[i].second += yoffset;
        }
		
		for (unsigned i = 0; i < sol->buf_locs.size(); ++i) {
			sol->buf_locs[i].first += xoffset;
			sol->buf_locs[i].second += yoffset;
		}
	}
	// end 

    cout << "Start writing sol.txt" << endl;
    ofstream outFile;
    stringstream  sol_ss;
    sol_ss << "sol_" << cur_sol << ".txt";
    outFile.open(sol_ss.str());
    for (unsigned j = 0; j < all_sols.size(); ++j) {
        solution * sol = all_sols[j];
        outFile << sol->idx << " : ";
        outFile << sol->lvl << " {";
        for (unsigned i = 0; i < sol->buf_sols.size(); ++i) {
            outFile << sol->buf_sols[i] << " ";
        }
        outFile << "} ";
        for (unsigned i = 0; i < sol->locs.size(); ++i) {
            outFile << sol->subtrees[i] << " {" <<  sol->locs[i].first << " " << sol->locs[i].second << "} " ;
        }
        if (sol->buf_locs.size()) {
            outFile << " B";
            for (unsigned i = 0; i < sol->buf_locs.size(); ++i) {
                outFile << " {" <<  sol->buf_locs[i].first << " " << sol->buf_locs[i].second << "} " ;
            }
        }
        outFile << endl;

    }

    // print out bottom level connections
    for (unsigned i = 0; i < bottom_sols.size(); ++i) {
        outFile << "B " << bottom_sols[i].first << " ";
        for (unsigned j = 0; j < bottom_sols[i].second.size(); ++j) {
            outFile << bottom_sols[i].second[j]->name << " ";
        }
        outFile << endl;
    }
    outFile.close();

    printSol(all_sols);

    cout << "Finished writing sol.txt" << endl;
}

float design::findLocsGreedy( const vector<pair<float, float>>&in_locs, 
					    vector<pair<float, float>>&out_locs, 
					    vector<pair<float, float>>&opt_buf_locs, 
					    const vector<bool>& isBuffer, 
						const vector <float>& dist, 
						const vector < vector <blockage*> > &blks) {
	
	out_locs.reserve(in_locs.size() - 1);
	for (unsigned i = 0; i < in_locs.size(); ++i) {
		if (i != (in_locs.size() / 2)) {
			out_locs.push_back(in_locs[i]);
		}
	}
	
	/*------------*/
//	cout << "in_locs:\n"; 
//	for (unsigned  i = 0; i < in_locs.size(); ++i) {
//		cout << "(" << in_locs[i].first << ", " << in_locs[i].second << ")\n";
//	}
	
	float targetTotalDistance = std::accumulate(dist.begin(), dist.end(), 0.0);
	float realTotalDistance = 0;
	
	for (unsigned  i = 0; i < in_locs.size() - 1; ++i) {
		realTotalDistance += calcDist(in_locs[i], in_locs[i + 1]);
	}
	//std::cout << "targetTotalDistance = " << targetTotalDistance << " vs ";
	//std::cout << "realTotalDistance = " << realTotalDistance << "\n";
	/*------------*/
	
	std::vector<float> accumulatedDistance;
	accumulatedDistance.reserve(isBuffer.size());
	float currDist = 0;
	for (unsigned i = 0; i < isBuffer.size(); ++i) {
		accumulatedDistance.push_back(currDist);
		if (i < dist.size()) {
			currDist += dist[i];
		}
	}
	
	//vector<BufferSolution> bufferSols(in_locs.size() - 1);
	unsigned nextBuffer = 0;
	float startPos = 0;
	for (unsigned i = 0; i < in_locs.size() - 1; ++i) {
		//cout << "[BufferSplit] Segment [" << i << "] --> ";
		BufferSolution bufferingSolutions[2];
		bufferingSolutions[0]._originLoc = in_locs[i];
		bufferingSolutions[0]._targetLoc = in_locs[i+1];
		
		float segmentLength = calcDist(in_locs[i], in_locs[i+1]);
		
		//cout << "buffers = ";
		while(accumulatedDistance[nextBuffer] - startPos <= segmentLength + 0.0001) {
			bufferingSolutions[0]._isRealBuffer.push_back(isBuffer[nextBuffer]);
			bufferingSolutions[0]._relDist.push_back(accumulatedDistance[nextBuffer] - startPos);
			//cout << "(" << (accumulatedDistance[nextBuffer] - startPos) << ") ";
			++nextBuffer;
			if (nextBuffer == isBuffer.size()) {
				break;
			}
		}
		//cout << "\n";
		
		bufferingSolutions[1] = bufferingSolutions[0];
		createLShapeConnection(bufferingSolutions[0], blks, true);
		createLShapeConnection(bufferingSolutions[1], blks, false);
		
		int bestSol = 0;
		if (bufferingSolutions[1]._cost < bufferingSolutions[0]._cost) {
			bestSol = 1;
		}
		
		//pt_buf_locs.insert(opt_buf_locs.end(),
	//		bufferingSolutions[bestSol]._bufferLocs.begin(),
	//		bufferingSolutions[bestSol]._bufferLocs.end());
			
		startPos += segmentLength;
	}
}

void design::createLShapeConnection(BufferSolution& bufferSolution, 
									const vector<vector<blockage*>>& blks, 
									bool horFirst) {
	pair<float, float>& originLoc = bufferSolution._originLoc;
	pair<float, float>& targetLoc = bufferSolution._targetLoc;
	vector<float>& relDist = bufferSolution._relDist;
	vector<bool>& isBuffer = bufferSolution._isRealBuffer;
	vector<pair<float, float>>& bufferLocs = bufferSolution._bufferLocs;
	int& cost = bufferSolution._cost;
	
	cost = 0;
	
	float segmentMediumPoint = calcDist(originLoc, targetLoc) / 2.0;
	
	for (unsigned i = 0; i < relDist.size(); ++i) {
		if (!isBuffer[i]) {
			continue;
		}
		
		pair<float, float> bufferPos;
		if (horFirst) {
			if(relDist[i] < segmentMediumPoint) {
				if (originLoc.first < targetLoc.first) {
					bufferPos.first = originLoc.first + relDist[i];
				} else {
					bufferPos.first = originLoc.first - relDist[i];
				}
				bufferPos.second = originLoc.second;
			} else {
				if (originLoc.first < targetLoc.first) {
					bufferPos.first + segmentMediumPoint;
				} else {
					bufferPos.first - segmentMediumPoint;
				}
				
				if (originLoc.second < targetLoc.second) {
					bufferPos.second = originLoc.second + (relDist[i] - segmentMediumPoint);
				} else {
					bufferPos.second = originLoc.second - (relDist[i] - segmentMediumPoint);
				}
			}
		} else {
			if(relDist[i] < segmentMediumPoint) {
				if (originLoc.second < targetLoc.second) {
					bufferPos.second = originLoc.second + relDist[i];
				} else {
					bufferPos.second = originLoc.second - relDist[i];
				}
				bufferPos.first = originLoc.first;
			} else {
				if (originLoc.second < targetLoc.second) {
					bufferPos.second + segmentMediumPoint;
				} else {
					bufferPos.second - segmentMediumPoint;
				}
				
				if (originLoc.first < targetLoc.first) {
					bufferPos.first = originLoc.first + (relDist[i] - segmentMediumPoint);
				} else {
					bufferPos.first = originLoc.first - (relDist[i] - segmentMediumPoint);
				}
			}
		}
		
		cost += computeCost(bufferPos, blks);
		bufferLocs.push_back(bufferPos);
	}
}

int design::computeCost(const pair<float, float>& pos, const vector<vector<blockage*> >& blks) {
	for (unsigned i = 0; i < blks.size(); ++i) {
		for (unsigned j = 0; j < blks[i].size(); ++j) {
			blockage* blk = blks[i][j];
			if (pos.first > blk->x1 && 
				pos.first < blk->x2 && 
				pos.second > blk->y1 &&
				pos.second < blk->y2) {
				return 1;
			}
		}
	}
	
	return 0;
}


/*** Find the optimal branching location **************************************/
//float design::findLocs(const vector<pair<float, float>>&in_locs, vector<pair<float, float>>&out_locs, vector<pair<float, float>>&opt_buf_locs, const vector<bool>& isBuffer, const vector <float>& dist, const vector < vector <blockage*> > &blks) {
//
//    float max_dist = -1;
//
//    IloEnv env;
//    try {
//        IloExpr objective(env);
//
//        IloRangeArray Consts(env);
//       
//        // buffer and branching points 
//        vector < pair <IloNumVar, IloNumVar> > var; 
//        vector < pair <IloNumVar, IloNumVar> > delta;
//        pair <IloNumVar, IloNumVar> max_delta;
//        
//        char buffer [50];
//        sprintf(buffer, "max_delta_x");
//        max_delta.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//        sprintf(buffer, "max_delta_y");
//        max_delta.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//
//        objective = max_delta.first + max_delta.second;
//
//        int cIdx = (in_locs.size() - 1)/2;
//        int cIdx_in_var;
//        int cnt = 0;
//        vector<int> branching_idxes;
//
//      
//        for (unsigned i = 0; i < isBuffer.size(); ++i) {
//          pair <IloNumVar, IloNumVar> temp;
//          
//          if (!isBuffer[i]) {
//            sprintf(buffer, "x_%d_branch_loc", i);
//            temp.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//            sprintf(buffer, "y_%d_branch_loc", i);
//            temp.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//
//            pair <IloNumVar, IloNumVar> d_temp;
//            
//            sprintf(buffer, "delta_x_%d", i);
//            d_temp.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//            sprintf(buffer, "delta_y_%d", i);
//            d_temp.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//            
//            delta.push_back(d_temp);
//
//            branching_idxes.push_back(i);
//            if (cnt == cIdx) cIdx_in_var = i;
//            cnt++;
//          } else {
//            sprintf(buffer, "x_%d_buffer_loc", i);
//            temp.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//            sprintf(buffer, "y_%d_buffer_loc", i);
//            temp.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
//          }
//          var.push_back(temp);
//        }
//        
//        cout << "aaa" << endl;
//        cnt = 0;
//        for (unsigned i = 0; i < isBuffer.size() - 1; ++i) {
//            int idx;
//            IloExpr ex(env);
//
//            idx = idenLoc(in_locs[cnt], in_locs[cnt+1]);
//
//            if (idx == 1) {
//                ex = var[i].first - var[i+1].first + var[i].second - var[i+1].second;
//                Consts.add(ex == dist[i]);
//                ex = var[i].first - var[i+1].first;
//                Consts.add(ex >= 0);
//                ex = var[i].second - var[i+1].second;
//                Consts.add(ex >= 0);
//            } else if (idx == 2) {
//                ex = var[i].first - var[i+1].first + var[i+1].second - var[i].second;
//                Consts.add(ex == dist[i]);
//                ex = var[i].first - var[i+1].first;
//                Consts.add(ex >= 0);
//                ex = var[i+1].second - var[i].second;
//                Consts.add(ex >= 0);
//            } else if (idx == 3) {
//                ex = var[i+1].first - var[i].first + var[i].second - var[i+1].second;
//                Consts.add(ex == dist[i]);
//                ex = var[i+1].first - var[i].first;
//                Consts.add(ex >= 0);
//                ex = var[i].second - var[i+1].second;
//                Consts.add(ex >= 0);
//            } else if (idx == 4) {
//                ex = var[i+1].first - var[i].first + var[i+1].second - var[i].second;
//                Consts.add(ex == dist[i]);
//                ex = var[i+1].first - var[i].first;
//                Consts.add(ex >= 0);
//                ex = var[i+1].second - var[i].second;
//                Consts.add(ex >= 0);
//            }
//
//            if (!isBuffer[i+1]) ++cnt; 
//
//            // fix center location of current region
//            if (i == cIdx_in_var) {
//              ex = var[i].first - in_locs[cIdx].first;
//              Consts.add(ex == 0);
//              ex = var[i].second - in_locs[cIdx].second;
//              Consts.add(ex == 0);
//            }
//        }
//        
//        cout << "bbb" << endl;
//        // Blockage constraint
//        if (blks.size() != 0) {
//          cnt = 0;
//          IloNum BigConst = 10000000;
//          for (unsigned i = 0; i < isBuffer.size(); ++i) {
//            cout << "i = " << i << " buffer_size: " << isBuffer.size() << endl;
//            if (!isBuffer[i]) {
//              if (i != cIdx_in_var) {
//                ++cnt; 
//              }
//              continue;
//            } else {
//              //buffer
//              for (unsigned k = 0; k <= 1; ++k) {
//                cout << "cnt: " << cnt << " current_index: " << cnt - k 
//                     << " blks_size: " << blks.size() << endl;
//                const vector <blockage*> cBlks = blks[cnt - k];
//                for (unsigned j = 0; j < cBlks.size(); ++j) {
//                  cout << "  j = " << j << endl;
//                  IloNumVar var_x1, var_x2, var_y1, var_y2;
//                  sprintf(buffer, "xl_buf_%d_blk_%d", i, j);
//                  var_x1 = IloNumVar(env, 0, 1, ILOBOOL, buffer);
//                  sprintf(buffer, "xr_buf_%d_blk_%d", i, j);
//                  var_x2 = IloNumVar(env, 0, 1, ILOBOOL, buffer);
//                  sprintf(buffer, "yl_buf_%d_blk_%d", i, j);
//                  var_y1 = IloNumVar(env, 0, 1, ILOBOOL, buffer);
//                  sprintf(buffer, "yr_buf_%d_blk_%d", i, j);
//                  var_y2 = IloNumVar(env, 0, 1, ILOBOOL, buffer);
//                  
//                  IloExpr ex(env);
//                  // var_x1 = 1 if buf_x <= blk_x1
//                  // blk_xl - buf_x <= BigConstant * var_x1 - 1
//                  ex = BigConst * var_x1 + var[i].first;
//                  Consts.add(ex >= 0.000001 + cBlks[j]->x1);
//                   
//                  // var_x1 = 0 if buf_x > blk_x1
//                  // buf_x - blk_x1 <= BigConstant * (1 - var_x1)
//                  Consts.add(ex <= BigConst + cBlks[j]->x1);
//
//                  // var_x2 = 1 if buf_x >= blk_x2
//                  // buf_x - blk_x2 <= BigConstant * var_x2 - 1
//                  ex = BigConst * var_x2 - var[i].first;
//                  Consts.add(ex >= 0.000001 - cBlks[j]->x2);
//
//                  // var_x2 = 0 if buf_x < blk_x2
//                  // blk_x2 - buf_x <= BigConstant * (1 - var_x2)
//                  Consts.add(ex <= BigConst - cBlks[j]->x2);
//
//                  // var_y1 = 1 if buf_y <= blk_y1
//                  // blk_yl - buf_y <= BigConstant * var_y1 - 1
//                  ex = BigConst * var_y1 + var[i].second;
//                  Consts.add(ex >= 0.000001 + cBlks[j]->y1);
//
//                  // var_y1 = 0 if buf_y > blk_y1
//                  // buf_y - blk_y1 <= BigConstant * (1 - var_y1)
//                  Consts.add(ex <= BigConst + cBlks[j]->y1);
//
//                  // var_y2 = 1 if buf_y >= blk_y2
//                  // buf_y - blk_y2 <= BigConstant * var_y2 - 1
//                  ex = BigConst * var_y2 - var[i].second;
//                  Consts.add(ex >= 0.000001 - cBlks[j]->y2);
//                  
//                  // var_y2 = 0 if buf_y < blk_y2
//                  // blk_y2 - buf_y <= BigConstant * (1 - var_y2)
//                  Consts.add(ex <= BigConst - cBlks[j]->y2);
//                  
//                  ex = var_x1 + var_x2 + var_y1 + var_y2;
//                  Consts.add(ex >= 1);
//                }
//              }
//            }
//          } 
//        } 
//        cout << "ccc" << endl;
//        for (unsigned i = 0; i < in_locs.size(); ++i) {
//            IloExpr ex1(env), ex2(env);
//           
//            ex1 = delta[i].first + var[branching_idxes[i]].first - in_locs[i].first;
//            ex2 = delta[i].first - var[branching_idxes[i]].first + in_locs[i].first;
//            Consts.add(ex1 >= 0);
//            Consts.add(ex2 >= 0);
//            ex1 = delta[i].second + var[branching_idxes[i]].second - in_locs[i].second;
//            ex2 = delta[i].second - var[branching_idxes[i]].second + in_locs[i].second;
//            Consts.add(ex1 >= 0);
//            Consts.add(ex2 >= 0);
//
//            ex1 = max_delta.first - delta[i].first;
//            ex2 = max_delta.second - delta[i].second;
//            Consts.add(ex1 >= 0);
//            Consts.add(ex2 >= 0);
//        }
//
//        cout << "ddd" << endl;
//        IloModel model(env);
//        model.add(IloMinimize(env, objective));
//        model.add(Consts);
//
//        IloCplex cplex(model);
//
//        cplex.setParam(IloCplex::Threads, 1);
//        //cplex.setOut(env.getNullStream());
//
//        string lpFile = "dist.lp";
//        string solFile = "dist.sol";
//        //cplex.exportModel(lpFile.c_str());
//
//        out_locs.clear();
//        if (cplex.solve()) {
//            if (verbose > 2) {
//                env.out() << "Feasible " << cplex.getStatus() << endl;
//                env.out() << "Solution value = " << cplex.getObjValue() << endl;
//            }
//            max_dist = cplex.getObjValue();
//
//            //cplex.writeSolution(solFile.c_str());
//            //for (unsigned i = 0; i < in_locs.size() - 1; ++i) {
//            int cnt = 0;
//            for (unsigned i = 0; i < isBuffer.size(); ++i) {
//                IloNum x = cplex.getValue(var[i].first);
//                IloNum y = cplex.getValue(var[i].second);
//
//                if (isBuffer[i]) {
//                    opt_buf_locs.push_back(make_pair(x, y));
//
//                    if (verbose > 2) {
//                        cout << i << "th buffer location: (" 
//                            << x << ", " << y << ")" << endl;
//                    }
//                } else if (cIdx_in_var != i) {
//                    IloNum del_x = cplex.getValue(delta[cnt].first);
//                    IloNum del_y = cplex.getValue(delta[cnt].second);
//                    out_locs.push_back(make_pair(x, y));
//                    cnt++;
//                    if (verbose > 2) {
//                        cout << i << "th branching location: (" 
//                            << x << ", " << y << ")   delta: ("
//                            << del_x << ", " << del_y << ")" << endl;
//                    }
//                }
//            }
//
//
//        } else {
//            env.out() << "Infeasible!!!" << endl;
//            cplex.exportModel(lpFile.c_str());
//        }
//
//        cplex.end();
//    } catch (IloException& ex){
//	      cerr << "Error: " << ex << endl;
//        cerr << ex.getMessage() << endl;
//    }
//    env.end();
//
//    return max_dist;
//}

/*** Find the optimal branching location **************************************/
/*
float design::findLocs(vector<pair<float, float>>in_locs, vector<pair<float, float>>&out_locs, float dist) {

    float max_dist = -1;

    IloEnv env;
    try {
        IloExpr objective(env);

        IloRangeArray Consts(env);

        vector < pair <IloNumVar, IloNumVar> > var;
        vector < pair <IloNumVar, IloNumVar> > delta;
        pair <IloNumVar, IloNumVar> max_delta;
        
        char buffer [50];
        sprintf(buffer, "max_delta_x");
        max_delta.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
        sprintf(buffer, "max_delta_y");
        max_delta.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);

        objective = max_delta.first + max_delta.second;

        for (unsigned i = 0; i < in_locs.size() - 1; ++i) {
            pair <IloNumVar, IloNumVar> temp;

            sprintf(buffer, "x_%d", i);
            temp.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
            sprintf(buffer, "y_%d", i);
            temp.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);

            var.push_back(temp);
            
            pair <IloNumVar, IloNumVar> d_temp;
            
            sprintf(buffer, "delta_x_%d", i);
            d_temp.first = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
            sprintf(buffer, "delta_y_%d", i);
            d_temp.second = IloNumVar(env, 0, 1000000, ILOFLOAT, buffer);
            
            delta.push_back(d_temp);
        }

        int cIdx = (in_locs.size() - 1)/2;
        int idx;
        IloExpr ex(env);

        for (unsigned i = 0; i < cIdx - 1; ++i) {
            idx = idenLoc(in_locs[i], in_locs[i+1]);

            if (idx == 1) {
                ex = var[i].first - var[i+1].first + var[i].second - var[i+1].second;
                Consts.add(ex == dist);
                ex = var[i].first - var[i+1].first;
                Consts.add(ex >= 0);
                ex = var[i].second - var[i+1].second;
                Consts.add(ex >= 0);
            } else if (idx == 2) {
                ex = var[i].first - var[i+1].first + var[i+1].second - var[i].second;
                Consts.add(ex == dist);
                ex = var[i].first - var[i+1].first;
                Consts.add(ex >= 0);
                ex = var[i+1].second - var[i].second;
                Consts.add(ex >= 0);
            } else if (idx == 3) {
                ex = var[i+1].first - var[i].first + var[i].second - var[i+1].second;
                Consts.add(ex == dist);
                ex = var[i+1].first - var[i].first;
                Consts.add(ex >= 0);
                ex = var[i].second - var[i+1].second;
                Consts.add(ex >= 0);
            } else if (idx == 4) {
                ex = var[i+1].first - var[i].first + var[i+1].second - var[i].second;
                Consts.add(ex == dist);
                ex = var[i+1].first - var[i].first;
                Consts.add(ex >= 0);
                ex = var[i+1].second - var[i].second;
                Consts.add(ex >= 0);
            }
        }

        idx = idenLoc(in_locs[cIdx - 1], in_locs[cIdx]);
        if (idx == 1) {
            ex = var[cIdx - 1].first - in_locs[cIdx].first + var[cIdx - 1].second - in_locs[cIdx].second;
            Consts.add(ex == dist/2);
            ex = var[cIdx - 1].first - in_locs[cIdx].first;
            Consts.add(ex >= 0);
            ex = var[cIdx - 1].second - in_locs[cIdx].second;
            Consts.add(ex >= 0);
        } else if (idx == 2) {
            ex = var[cIdx - 1].first - in_locs[cIdx].first + in_locs[cIdx].second - var[cIdx - 1].second;
            Consts.add(ex == dist/2);
            ex = var[cIdx - 1].first - in_locs[cIdx].first ;
            Consts.add(ex >= 0);
            ex = in_locs[cIdx].second - var[cIdx - 1].second;
            Consts.add(ex >= 0);
        } else if (idx == 3) {
            ex = in_locs[cIdx].first - var[cIdx - 1].first + var[cIdx - 1].second - in_locs[cIdx].second;
            Consts.add(ex == dist/2);
            ex = in_locs[cIdx].first - var[cIdx - 1].first ;
            Consts.add(ex >= 0);
            ex = var[cIdx - 1].second - in_locs[cIdx].second;
            Consts.add(ex >= 0);
        } else if (idx == 4) {
            ex = in_locs[cIdx].first - var[cIdx - 1].first + in_locs[cIdx].second - var[cIdx - 1].second;
            Consts.add(ex == dist/2);
            ex = in_locs[cIdx].first - var[cIdx - 1].first ;
            Consts.add(ex >= 0);
            ex = in_locs[cIdx].second - var[cIdx - 1].second;
            Consts.add(ex >= 0);
        }

        idx = idenLoc(in_locs[cIdx], in_locs[cIdx + 1]);
        if (idx == 1) {
            ex = in_locs[cIdx].first - var[cIdx].first + in_locs[cIdx].second - var[cIdx].second;
            Consts.add(ex == dist/2);
            ex = in_locs[cIdx].first - var[cIdx].first ;
            Consts.add(ex >= 0);
            ex = in_locs[cIdx].second - var[cIdx].second;
            Consts.add(ex >= 0);
        } else if (idx == 2) {
            ex = in_locs[cIdx].first - var[cIdx].first + var[cIdx].second - in_locs[cIdx].second;
            Consts.add(ex == dist/2);
            ex = in_locs[cIdx].first - var[cIdx].first ;
            Consts.add(ex >= 0);
            ex = var[cIdx].second - in_locs[cIdx].second;
            Consts.add(ex >= 0);
        } else if (idx == 3) {
            ex = var[cIdx].first - in_locs[cIdx].first + in_locs[cIdx].second - var[cIdx].second;
            Consts.add(ex == dist/2);
            ex = var[cIdx].first - in_locs[cIdx].first ;
            Consts.add(ex >= 0);
            ex = in_locs[cIdx].second - var[cIdx].second;
            Consts.add(ex >= 0);
        } else if (idx == 4) {
            ex = var[cIdx].first - in_locs[cIdx].first + var[cIdx].second - in_locs[cIdx].second;
            Consts.add(ex == dist/2);
            ex = var[cIdx].first - in_locs[cIdx].first ;
            Consts.add(ex >= 0);
            ex = var[cIdx].second - in_locs[cIdx].second;
            Consts.add(ex >= 0);
        }

        for (unsigned i = cIdx; i < in_locs.size() - 2; ++i) {
            
            idx = idenLoc(in_locs[i+1], in_locs[i+2]);
            if (idx == 1) {
                ex = var[i].first - var[i+1].first + var[i].second - var[i+1].second;
                Consts.add(ex == dist);
                ex = var[i].first - var[i+1].first ;
                Consts.add(ex >= 0);
                ex = var[i].second - var[i+1].second;
                Consts.add(ex >= 0);
            } else if (idx == 2) {
                ex = var[i].first - var[i+1].first + var[i+1].second - var[i].second;
                Consts.add(ex == dist);
                ex = var[i].first - var[i+1].first ;
                Consts.add(ex >= 0);
                ex = var[i+1].second - var[i].second;
                Consts.add(ex >= 0);
            } else if (idx == 3) {
                ex = var[i+1].first - var[i].first + var[i].second - var[i+1].second;
                Consts.add(ex == dist);
                ex = var[i+1].first - var[i].first ;
                Consts.add(ex >= 0);
                ex = var[i].second - var[i+1].second;
                Consts.add(ex >= 0);
            } else if (idx == 4) {
                ex = var[i+1].first - var[i].first + var[i+1].second - var[i].second;
                Consts.add(ex == dist);
                ex = var[i+1].first - var[i].first ;
                Consts.add(ex >= 0);
                ex = var[i+1].second - var[i].second;
                Consts.add(ex >= 0);
            }
        }

        for (unsigned i = 0; i < in_locs.size() - 1; ++i) {
            int inIdx;
            if (i >= cIdx) inIdx = i + 1;
            else if (i < cIdx) inIdx = i;
            IloExpr ex1(env), ex2(env);
           
            ex1 = delta[i].first + var[i].first - in_locs[inIdx].first;
            ex2 = delta[i].first - var[i].first + in_locs[inIdx].first;
            Consts.add(ex1 >= 0);
            Consts.add(ex2 >= 0);
            ex1 = delta[i].second + var[i].second - in_locs[inIdx].second;
            ex2 = delta[i].second - var[i].second + in_locs[inIdx].second;
            Consts.add(ex1 >= 0);
            Consts.add(ex2 >= 0);

            ex1 = max_delta.first - delta[i].first;
            ex2 = max_delta.second - delta[i].second;
            Consts.add(ex1 >= 0);
            Consts.add(ex2 >= 0);
        }

        IloModel model(env);
        model.add(IloMinimize(env, objective));
        model.add(Consts);

        IloCplex cplex(model);

        cplex.setParam(IloCplex::Threads, 1);
        cplex.setOut(env.getNullStream());

        string lpFile = "dist.lp";
        string solFile = "dist.sol";
        //cplex.exportModel(lpFile.c_str());


        out_locs.clear();
        if (cplex.solve()) {
            if (verbose > 2) {
                env.out() << "Feasible " << cplex.getStatus() << endl;
                env.out() << "Solution value = " << cplex.getObjValue() << endl;
            }
            max_dist = cplex.getObjValue();

            //cplex.writeSolution(solFile.c_str());
            for (unsigned i = 0; i < in_locs.size() - 1; ++i) {
                IloNum del_x = cplex.getValue(delta[i].first);
                IloNum del_y = cplex.getValue(delta[i].second);
                IloNum x = cplex.getValue(var[i].first);
                IloNum y = cplex.getValue(var[i].second);
                out_locs.push_back(make_pair(x, y));
                if (verbose > 2) {
                    cout << i << "th branching location: (" 
                        << x << ", " << y << ")   delta: ("
                        << del_x << ", " << del_y << ")" << endl;
                }
            }
        } else {
            env.out() << "Infeasible!!!" << endl;
        }

        cplex.end();
    } catch (IloException& ex){
	      cerr << "Error: " << ex << endl;
        cerr << ex.getMessage() << endl;
    }
    env.end();

    return max_dist;
}
*/

/*** Identify the second loc compared to the first loc ************************/
int design::idenLoc(pair<float, float> loc1, pair<float, float> loc2) {
    if (loc1.first >= loc2.first && loc1.second >= loc2.second) {
        return 1;
    } else if (loc1.first >= loc2.first && loc1.second < loc2.second) {
        return 2;
    } else if (loc1.first < loc2.first && loc1.second >= loc2.second) {
        return 3;
    } else if (loc1.first < loc2.first && loc1.second < loc2.second) {
        return 4;
    }
}

/*** Utilities ****************************************************************/
float round(float val, float step) {
    float n = nearbyint(val/step);
    if (n < 1) 
        n = 1;
    return (step*n);
}

float design::calcDist(pair<float,float> loc1, pair<float,float> loc2) {
    return fabs(loc1.first - loc2.first) + fabs(loc1.second - loc2.second);
}

void design::runKMeansClustering() {
	std::cout << "Running CK-Means....\n";

//	{ // 1x2
//		CKMeans::clustering c(pins);
//
//		vector<pair<float, float>> means; 
//		means.push_back(make_pair(W/4.0, H/2.0));
//		means.push_back(make_pair(3.0*W/4.0, H/2.0));
//
//		c.setPlotFileName("cluster-1x2");
//		c.iterKmeans(1, 2, (unsigned)ceil(pins.size()/2.0), 0, means, 1);
//	}
//	
//	{ // 2x1
//		CKMeans::clustering c(pins);
//
//		vector<pair<float, float>> means; 
//		means.push_back(make_pair(W/2.0, H/2.0));
//		means.push_back(make_pair(W/2.0, 3.0*H/2.0));
//
//		c.setPlotFileName("cluster-2x1");
//		c.iterKmeans(1, 2, (unsigned)ceil(pins.size()/2.0), 0, means, 1);
//	}
//	
//	{ // 3x1
//		CKMeans::clustering c(pins);
//
//		vector<pair<float, float>> means; 
//		means.push_back(make_pair(0.3*W, H/2.0));
//		means.push_back(make_pair(0.6*W, H/2.0));
//		means.push_back(make_pair(0.9*W, H/2.0));
//
//		c.setPlotFileName("cluster-3x1");
//		c.iterKmeans(1, 3, (unsigned)ceil(pins.size()/3.0), 0, means, 1);
//	}
//	
//	{ // 1x3
//		CKMeans::clustering c(pins);
//
//		vector<pair<float, float>> means; 
//		means.push_back(make_pair(W/2.0, 0.3*H));
//		means.push_back(make_pair(W/2.0, 0.6*H));
//		means.push_back(make_pair(W/2.0, 0.9*H));
//
//		c.setPlotFileName("cluster-1x3");
//		c.iterKmeans(1, 3, (unsigned)ceil(pins.size()/3.0), 0, means, 1);
//	}
//	
//	{ // 4x4
//		CKMeans::clustering c(pins);
//
//		vector<pair<float, float>> means; 
//		means.push_back(make_pair(W/4.0, H/4.0));
//		means.push_back(make_pair(3.0*W/4.0, H/4.0));
//		means.push_back(make_pair(W/4.0, 3.0*H/4.0));
//		means.push_back(make_pair(3.0*W/4.0, 3.0*H/4.0));
//
//		c.setPlotFileName("cluster-4x4");
//		c.iterKmeans(1, 4, (unsigned)ceil(pins.size()/4.0), 0, means, 1);
//	}
}

