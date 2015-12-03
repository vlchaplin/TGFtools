/*
 *  algorithms_t.h
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/21/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include <vector>

#ifndef AGLORITHM_T_H
#define AGLORITHM_T_H


using namespace std;

template<class T>
long findNearestElement( vector<T>& points, T toValue) {
	
	long m=0;
	long n=points.size()-1;
	
	if ( toValue < points[m] ) return -1;
	if ( toValue > points[n] ) return -1;
	
	return nearestBinStep( points, toValue, m, n );

};

template<class T>
long nearestBinStep( vector<T>& points, T& toValue, long& lowBx, long& upBx ) {

	long dx = upBx - lowBx;
	
	if (dx == 0 || upBx == 0) return upBx;
	
	long mdx = ( lowBx+upBx ) / 2;

	T mtime = points[mdx];
	
	if (toValue < mtime) {
		//Branch to the lower half
		upBx = mdx;
		return nearestBinStep( points, toValue, lowBx, upBx );
	}
	else if (toValue > mtime) {
		lowBx = mdx+1;
		return nearestBinStep( points, toValue, lowBx, upBx );
	} else {
		
		//if ( (toValue - mtime) > (points[mdx+1] - toValue) ) return mdx+1;
	
		return mdx;
	}
	
};

#endif