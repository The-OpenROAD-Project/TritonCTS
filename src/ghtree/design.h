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

#include <string>
#include <vector>
#include <map>
#include <list>
#include "tree.h"

using   namespace   std;

class design {
    public:
		unsigned out_slew_idx;
		unsigned min_slew_idx;

        // variables
        unsigned W, H; 
        unsigned num_sinks;
        float    total_cap, avg_cap;
        unsigned max_skew;
        unsigned verbose;
        unsigned opt_t_idx;
        int toler;
		float xoffset;
		float yoffset;

        unsigned max_delay;
        unsigned max_solnum;
        unsigned cur_sol;
        bool cluster_only;

        string sol_file;
        
        vector<pin*> pins;

        vector<unsigned> branchs;
        vector<pair<unsigned, unsigned>> sink_areas;
        vector<float> branch_locs;
        vector<vector<unsigned>> buf_sols;
        vector<blockage*> blks;

        unsigned min_dist, min_cap, min_slew, size_cap, size_slew;
        float max_dist;

       // map <unsigned, LUT*> lut_idx;

        vector<vector<vector<vector<vector<vector<vector<tree*>>>>>>> sols;
        vector<vector<vector<vector<LUT*>>>> luts;
        map<int, LUT*> lut_map;

        // functions
        bool parseDesignInfo(float, float, float, float, float, float, unsigned, unsigned, int, unsigned, unsigned, bool, string, bool);
        void parseLUT();
        void parseSinkCap();
        void parseBlks();
        void optTree();
        void printSol(string, unsigned);
        inline bool evalTree(tree*, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned);
        inline void evalSegment(segment*, vector<segment*>&);
        //float findLocs(vector<pair<float, float>>, vector<pair<float, float>>& , float);
        float findLocs(const vector<pair<float, float>>&in_locs, vector<pair<float, float>>&out_locs, vector<pair<float, float>>&opt_buf_locs, const vector<bool>& isBuffer, const vector <float>& dist, const vector < vector <blockage*> > &blks);

        void reconstructTree();
        void selectTreeSol();
        void cluster(vector<pin*>, vector<vector<pin*>>&, vector<pair<float,float>>, vector<pair<float,float>>&, float);
        void placeTree();
        int idenLoc(pair<float, float>, pair<float, float>);
        float calcDist(pair<float, float>, pair<float, float>);
        void printSol(vector<solution*> &all_sols);

		// MF @ 180206: hacking sink region
		void computeSinkRegion(const float);
};

