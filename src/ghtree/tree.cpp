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

#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <limits.h>
#include <math.h>

/*** Constructor *********************************************************/
pin::pin (string _name, float _x, float _y, float _cap) {
    name = _name;
    x    = _x;
    y    = _y;
    cap  = _cap;
}

region::region(unsigned _idx, unsigned _lvl, float _w, float _h, unsigned _num, float _rx, float _ry, float _ox, float _oy, vector<pin*> _pins) {
    idx       = _idx;
    lvl       = _lvl;
    w         = _w;
    h         = _h;
    num_sinks = _num;
    r_x       = _rx;
    r_y       = _ry;
    o_x       = _ox;
    o_y       = _oy;
    for (unsigned i = 0; i < _pins.size(); ++i) {
        pins.push_back(_pins[i]);
    }
}

tree::tree(unsigned _idx) {
    idx = _idx;
}

tree::tree(unsigned _idx, segment * s) {
    idx = _idx;
    for (unsigned i = 0; i < s->buf_sols.size(); ++i) {
        buf_sols.push_back(s->buf_sols[i]);
    }
    min_laten = s->min_laten;
    max_laten = s->max_laten;
    power     = 2*s->power;
    in_cap    = 2*s->in_cap;
}

LUT::LUT(unsigned _idx, float _power, unsigned _delay, unsigned _cap, unsigned _slew, string _wire, unsigned min_cap) {
    idx     = _idx;
    power   = _power;
    delay   = _delay;
    if (_cap < 5) 
        in_cap  = _cap+min_cap;
    else 
        in_cap  = (_cap+min_cap-4)*5;
    in_slew = _slew;
    if (_wire == "1") 
        pure_wire = true;
    else 
        pure_wire = false;
}

segment::segment (segment * _s, LUT * _lut) {
    power      = _s->power + _lut->power;
    max_laten  = _s->max_laten + _lut->delay;
    min_laten  = _s->min_laten + _lut->delay;
    in_slew    = _lut->in_slew;
    buf_sols.push_back(_lut->idx);
    for (unsigned i = 0; i < _s->buf_sols.size(); ++i) {
        buf_sols.push_back(_s->buf_sols[i]);
    }
    //if (_lut->pure_wire) 
    //    in_cap = _lut->in_cap + _s->in_cap;
    //else 
        in_cap = _lut->in_cap;
}

blockage::blockage (float _x1, float _y1, float _x2, float _y2) {
    x1 = _x1; 
    y1 = _y1;
    x2 = _x2; 
    y2 = _y2;
}

bool blockage::isOverlap (blockage &target) {
    if (x1 < target.x2 && x2 > target.x1 && y1 < target.y2 && y2 > target.y1) 
        return true;
    else
        return false;
}
