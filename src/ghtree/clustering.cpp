#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <list>
#include <stack>
#include <limits.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <algorithm>
#include "clustering.h"

namespace CKMeans{
using namespace std;

int myrandom (int i) {
    srand(time(NULL));
    return rand()%i;
}

/*** Capacitated K means **************************************************/
void clustering::iterKmeans(unsigned ITER, unsigned N, unsigned CAP, unsigned IDX, vector<pair<float, float>>& _means, unsigned MAX) {

    vector<pair<float, float>> sol;
    sol.resize(flops.size());

    float max_silh = -1;
    for (unsigned i = 0; i < ITER; ++i) {
        if (verbose > 0)
        cout << "Iteration " << i << " ";
        if (TEST_ITER == 1) 
        cout << "Iteration " << i << endl;
        vector<pair<float,float>> means;
        float silh = Kmeans(N, CAP, IDX, means, MAX);
        if (silh > max_silh) {
            max_silh = silh;
            for (unsigned j = 0; j < flops.size(); ++j)
                sol[j] = flops[j]->match_idx[IDX];
            _means.resize(means.size());
            for (unsigned j = 0; j < means.size(); ++j)
                _means[j] = means[j];
        }
    }
    
    for (unsigned i = 0; i < flops.size(); ++i) 
        flops[i]->match_idx[IDX] = sol[i];

    // print clustering solution
    if (TEST_LAYOUT == 1) {
        ofstream outFile;
        outFile.open("cluster.sol");
        for (unsigned i = 0; i < _means.size(); ++i) {
            outFile << "TRAY " << i << " " << _means[i].first << " " << _means[i].second << endl;
        }
        for (unsigned i = 0; i < flops.size(); ++i) {
            flop * f = flops[i];
            outFile << "FLOP " << f->name << " " << flops[i]->match_idx[IDX].first << endl;
        }
        outFile.close();
    }

    if (CAP < 10) 
        cout << "Best SILH  (" << CAP  << ") is " << max_silh << endl;
    else 
        cout << "Best SILH (" << CAP  << ") is " << max_silh << endl;

}

float clustering::Kmeans (unsigned N, unsigned CAP, unsigned IDX, vector<pair<float, float>>& means, unsigned MAX) {
	vector<vector<flop*>> clusters;

    // Kmeans++ to generate initial points
    means.clear();
    // initialize distances to 1
    for (unsigned i = 0; i < flops.size(); ++i) 
        flops[i]->dists[IDX] = 1;    
    // random indexes
    vector<unsigned> rand_idx;
    for (unsigned i = 0; i < flops.size(); ++i) 
        rand_idx.push_back(i);
    // iteratively select initial points
    for (unsigned i = 0; i < N; ++i) {
        float sum_dist = 0;
        for (unsigned i = 0; i < flops.size(); ++i) 
            sum_dist += flops[i]->dists[IDX];
        float rand_num = myrandom(int(sum_dist*100))/100.0;
        float sum = 0;
        random_shuffle(rand_idx.begin(), rand_idx.end(), myrandom);
        for (unsigned j = 0; j < rand_idx.size(); ++j) {
            flop * f = flops[rand_idx[j]];
                sum += f->dists[IDX];
            if (sum > rand_num) {
                means.push_back(make_pair(f->x, f->y));
                if (i == 0)
                    for (unsigned k = 0; k < flops.size(); ++k)
                        flops[k]->dists[IDX] = 0;
                for (unsigned k = 0; k < flops.size(); ++k) {
                    if (TEST_LAYOUT == 1) 
                        //flops[k]->dists[IDX] += calcDist(make_pair(f->x, f->y), flops[k]);
                        flops[k]->dists[IDX] += calcDistAR(make_pair(f->x, f->y), flops[k], CAP);
                    else 
                        flops[k]->dists[IDX] += calcDistAR(make_pair(f->x, f->y), flops[k], CAP);
                }
                break;
            }
        }
    }

    // initialize matching indexes for flops
    //for (unsigned i = 0; i < flops.size(); ++i) {
    //    flop * f = flops[i];
    //    f->match_idx[IDX] = make_pair(-1, -1);
    //}

    //bool stop = false;
    //// Kmeans optimization
    //unsigned iter = 1;
    //while (!stop) {

    //    if (TEST_LAYOUT == 1 || verbose > 1)
    //    cout << "ITERATION " << iter << endl;

    //    // report initial means
    //    if (TEST_LAYOUT == 1 || verbose > 1) {
    //    cout << "INIT Tray locations " << endl;
    //    for (unsigned i = 0; i < N; ++i)
    //        cout << means[i].first << " " << means[i].second << endl;
    //    cout << endl;
    //    }

    //    if (verbose > 1) 
    //    cout << "match .." << endl;
    //    // flop to slot matching based on min-cost flow
    //    if (iter == 1) 
    //        minCostFlow(means, CAP, IDX, 200);
    //    else if (iter == 2) 
    //        minCostFlow(means, CAP, IDX, 100);
    //    else if (iter > 2) 
    //        minCostFlow(means, CAP, IDX, 50);

    //    // collect results
    //    clusters.clear();
    //    clusters.resize(N);
    //    for (unsigned i = 0; i < flops.size(); ++i) {
    //        flop * f = flops[i];
    //        clusters[f->match_idx[IDX].first].push_back(f);
    //    }

    //    // always use mode 0, mode 1 is just for comparison
    //    unsigned update_mode = 0;

    //    if (verbose > 1) 
    //    cout << "move .." << endl;
    //    float delta = 0;
    //    if (update_mode == 0) {
    //        // LP-based tray movement
    //        delta = LPMove(means, CAP, IDX);
    //    } else if (update_mode == 1) {
    //        // use weighted center
    //        for (unsigned i = 0; i < N; ++i) {
    //            float sum_x = 0, sum_y = 0;
    //            for (unsigned j = 0; j < clusters[i].size(); ++j) {
    //                sum_x += clusters[i][j]->x;
    //                sum_y += clusters[i][j]->y;
    //            }
    //            float pre_x = means[i].first;
    //            float pre_y = means[i].second;
    //            means[i] = make_pair(sum_x/clusters[i].size(), sum_y/clusters[i].size());
    //            delta += abs(pre_x - means[i].first) + abs(pre_y - means[i].second);
    //        }
    //    }

    //    // report clustering solution
    //    if (TEST_LAYOUT == 1 || verbose > 1) {
    //    for (unsigned i = 0; i < N; ++i) {
    //        cout << "Cluster " << i << endl;
    //        for (unsigned j = 0; j < clusters[i].size(); ++j) {
    //            cout << clusters[i][j]->x << " " << clusters[i][j]->y << endl;
    //        }
    //        cout << endl;
    //    }
    //    }

    //    // report final means
    //    if (TEST_LAYOUT == 1 || verbose > 1) {
    //    cout << "FINAL Tray locations " << endl;
    //    for (unsigned i = 0; i < N; ++i) {
    //        cout << means[i].first << " " << means[i].second << endl;
    //    }
    //    cout << endl;
    //    }

    //    if (TEST_LAYOUT == 1 || TEST_ITER == 1) {
    //    float silh = calcSilh(means, CAP, IDX);
    //    //cout << "delta dist = " << delta << "um" << endl;
    //    cout << "SILH is " << silh << endl;
    //    }

    //    if (iter > MAX || delta < 0.5)
    //        stop = true;

    //    ++iter;
    //}

    float silh = calcSilh(means, CAP, IDX);
    if (verbose > 0)
    cout << "SILH (" << CAP  << ") is " << silh << endl;

    return silh;
}

float clustering::calcSilh(const vector<pair<float, float>>& means, unsigned CAP, unsigned IDX) {
    float sum_silh = 0;
    for (unsigned i = 0; i < flops.size(); ++i) {
        flop * f = flops[i];
        float in_d = 0, out_d = INT_MAX;
        for (unsigned j = 0; j < means.size(); ++j) {
            float _x = means[j].first;
            float _y = means[j].second;
            if (f->match_idx[IDX].first == j) {
                // within the cluster
                unsigned k = f->match_idx[IDX].second;
                in_d = calcDist(make_pair(_x+shifts[CAP][k].first, _y+shifts[CAP][k].second), f);
            } else {
                // outside of the cluster
                for (unsigned k = 0; k < CAP; ++k) {
                    float d = calcDist(make_pair(_x+shifts[CAP][k].first, _y+shifts[CAP][k].second), f);
                    if (d < out_d)
                        out_d = d;
                }
            }
        }
        float temp = max(out_d, in_d);
        if (temp == 0) {
            if (out_d == 0) 
                f->silhs[IDX] = -1;
            if (in_d == 0) 
                f->silhs[IDX] =  1;
        } else  {
            f->silhs[IDX] = (out_d - in_d) / temp;
        }
        sum_silh += f->silhs[IDX]; 
    }
    return sum_silh / flops.size();
}

}
