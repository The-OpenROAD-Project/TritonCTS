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

#ifndef TREE_H
#define TREE_H

#include <vector>
#include <list>
#include <string>
#include <iostream>
#include <cstdint>

using   namespace   std;

class pin {
    public:
        pin (string, float, float, float);
        string name;
        float cap;
        float x, y;
};

class region {
    public:
        region(unsigned, unsigned, float, float, unsigned, float, float, float, float, vector<pin*>);
        unsigned idx;
        unsigned lvl;
        float w, h;
        unsigned num_sinks;
        float r_x, r_y; // coordinate of root
        float o_x, o_y; // coordinate of origin
        vector<pin*> pins;
};

class LUT {
    public:
        LUT(unsigned, float, unsigned, unsigned, unsigned, string, unsigned);

        // variables
        unsigned idx;
        float power;
        unsigned delay;
        unsigned in_cap;
        unsigned in_slew;
        bool  pure_wire;
        vector<float> lens;
        float len;
};

class segment { 
    public:
        segment(segment*, LUT*);
        segment() {};
        vector<unsigned> buf_sols;
        unsigned max_laten, min_laten;
        float power;
        float in_cap; 
        unsigned in_slew;
};

class tree {
    public:
        tree(unsigned);
        tree(unsigned, segment*);
        // variables
        unsigned idx;
        vector<unsigned> buf_sols;               // buffering solution
        vector<unsigned> tree_sols;              // subtree solution
        unsigned max_laten, min_laten;           // max and min latency
        float power;                             // total power
        float in_cap;                            // input cap
};

class solution {
    public:
        int idx, lvl;
        vector<pair<float, float>> locs;
        vector<pair<float, float>> buf_locs;
        vector<int> buf_sols;
        vector<int> subtrees;
};

class blockage {
    public:
        blockage(float, float, float, float);
        bool isOverlap(blockage&);
        float x1, y1, x2, y2;
};


#endif
