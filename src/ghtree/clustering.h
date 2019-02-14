#include <vector>
#include <cmath>

namespace CKMeans {
using namespace std;

class flop {
public:
    flop(string, float, float);
    // gate name
    string name;
    // location
    float x, y;
    unsigned x_idx, y_idx;
    vector<float> dists;
    unsigned idx;
    vector<flop*> nbrs;
    vector<pair<int, int>> match_idx;
    vector<float> silhs;
    float d_x, d_y;
	//float skew;
};


class clustering {
	vector<flop*> flops;
	map<unsigned, vector<pair<float, float>>> shifts;  // relative x- and y-shifts of slots w.r.t tray 
	map<unsigned, float> tray_ars;
	int verbose;
	int TEST_LAYOUT;
	int TEST_ITER;

public:
	float Kmeans(unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned);
	void iterKmeans(unsigned, unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned MAX = 15);
	float calcSilh(const vector<pair<float,float>>&, unsigned, unsigned);
	
	inline float calcDist (const pair<float, float>& loc, flop * f) {
    	return (fabs(loc.first - f->x) + fabs(loc.second - f->y));
	}

	inline float calcDistAR (const pair<float, float>& loc, flop * f, unsigned size) {
	    float dx = fabs(f->x - loc.first);
    	float dy = fabs(f->y - loc.second);
    	return (dy + dx / tray_ars[size]);
	}
};

}
