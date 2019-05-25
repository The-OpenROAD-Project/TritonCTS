#include <vector>
#include <cmath>
#include "tree.h"

namespace CKMeans {
using namespace std;

class flop {
public:
    // location
    float x, y;
    unsigned x_idx, y_idx;
    vector<float> dists;
    unsigned idx;
    vector<pair<int, int>> match_idx;
    vector<float> silhs;
	pin* p;
    flop(const float x, const float y, pin* p) : x(x), y(y), p(p) {};
};


class clustering {
	vector<flop*> flops;
	vector<vector<flop*>> clusters;
	
	//map<unsigned, vector<pair<float, float>>> shifts;  // relative x- and y-shifts of slots w.r.t tray 
	int verbose = 1;
	int TEST_LAYOUT = 1;
	int TEST_ITER = 1;
	std::string plotFile;
	
	//std::pair<float,float> branchPoint;
	//float minDist;
	//float maxDist;
	
	float segmentLength;
	std::pair<float, float> branchingPoint;
	
public:
	clustering(const vector<pin*>&, float, float);
	~clustering();
	float Kmeans(unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned);
	void iterKmeans(unsigned, unsigned, unsigned, unsigned, vector<pair<float, float>>&, unsigned MAX = 15);
	float calcSilh(const vector<pair<float,float>>&, unsigned, unsigned);
	void minCostFlow (const vector<pair<float, float>>&, unsigned, unsigned, float); 
	void setPlotFileName(const std::string fileName) { plotFile = fileName; }
	void getClusters(vector<vector<pin*>>&);
	void fixSegmentLengths(vector<pair<float, float>>&);
	void fixSegment(const pair<float, float>& fixedPoint, pair<float, float>& movablePoint, float targetDist);

	inline float calcDist (const pair<float, float>& loc, flop * f) const {
    	return (fabs(loc.first - f->x) + fabs(loc.second - f->y));
	}
	
	inline float calcDist (const pair<float, float>& loc1, pair<float, float>& loc2) const {
    	return (fabs(loc1.first - loc2.first) + fabs(loc1.second - loc2.second));
	}

	void plotClusters(const vector<vector<flop*>>&, const vector<pair<float, float>>&) const;
};

}
