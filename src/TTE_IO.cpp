/*
 *  TTE_IO.cpp
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 4/7/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#ifndef TTEIODEFINED
#define TTEIODEFINED


#include "TTE_IO.hh"
//#define TTETEMPLATE_FIT "/gbm/Analysis/LightcurveSimulator/fits_templates/glg_tte_b1_bn090510016_v00.fit"


/***

	binSortedIntersection()
	
	Recursive binary search of the TTE event table.
	The sectioned data in each step is denoted by 
	rowMIN and rowMAX.  tstart and tstop is the interval
	defined in tteFindRowRange(), and are constant throughout
	the recursion.
	
		
	This algorithm becomes ineffecient as the number of rows in [tstart, tstop] increases.
	The standard binary search is O(log_2(N)). Adding the stepwise search adds an n1+n2,
	dependency, where n1 is the number of rows between tstart and the middle of this branch.
	n2 is the number of rows between the middle of this branch and tstop. These are encountered
	in the final branch, during a linear search where adjacent elements are examined 
	until the edge is reached.  The total number of steps taken in the linear part is n1 + n2.
	
	--- IMPACT of the linear edge search ---
	In general, the algorithm is O(log_2(N) + n1 + n2)
	
	Of course, n1 + n2 <= (N/2^k) so O <= O( log_2(N) + (N/2^k) ), where k is the total number
	of branches taken in the binary search to this point (aka recursion depth), and k=0,1,...log_2(N).
	The algorithm is as efficient as binary sort in the limit (N/2^k) << log_2(N).  When k=log_2(N),
	it's maximum value, the added term = 1.
	
	O( log_2(N) + (N/2^k) ) is the worst case, so let us consider it the efficiency bound.
	Then k = log_2( N/(n1+n2) ) which should be floor()'ed for non-whole numbers.
	
	For TTE data, N, n1 and n2 depend on the count rate and the width of [tstart,tstop],
	
		N = (FileStop - FileStart) * avg. rate
	
	and the size of the intersection "nn":
		
		nn = total counts in [tstart,tstop] 
		   = (tstop - tstart) * avg.rate
	
	Since in the worst case the size of the intersection nn = n1 + n2, the maximum recursion
	depth can be found:
	
		N / nn = (FileStop - FileStart) / (tstop - tstart) = DT / dt
		nn = n1 + n2
	
	=>	k = log_2( N/nn ) = log_2 ( DT / dt )

	Since it is O( log_2(N) + (N/2^k) ), the algorithm improves with larger k
	=>	dt should be small compared to DT.
		
	
	This means that for large dt's it may be more efficient to perform two seperate recursions: 
	once to find tstart, and again to find tstop.  It this case the comparison is whether
	
		log_2(N) < (N/2^k)  [ since O( 2*log_2(N) ) < O( log_2(N) + (N/2^k) ) ]
	
		If k is small the latter is probably more effecient.


***/

int TTEReader::binSortedIntersection(long& rowMIN, long& rowMAX, double& tstart, double& tstop ) {

	if ( rowMIN >= rowMAX || rowMIN < 1 ) return -1;
	
	long median = ( rowMIN+rowMAX ) / 2;
	double mtime;
	
	ffgcv( stat.fptr, TDOUBLE, 1, median, 1, 1, 0, &mtime, NULL, &(stat.status) );
	
	if (mtime > tstop) {
		//The middle time is past the entire [tstart, tstop] interval
		//Branch to the lower half
		rowMAX = median;
		return binSortedIntersection( rowMIN, rowMAX, tstart, tstop );
	}
	else if (mtime < tstart) {
		//The middle time is before the entire [tstart, tstop] interval
		//Branch to the upper half 
		rowMIN = median+1;
		return binSortedIntersection( rowMIN, rowMAX, tstart, tstop );
	} else {
		
		long bndryItr;
		
		//Step backward to find the first intersection
		bndryItr = median;
		while ( tstart < mtime && bndryItr > 0 ) {
			ffgcv( stat.fptr, TDOUBLE, 1, bndryItr--, 1, 1, 0, &mtime, NULL, &(stat.status) ); 
		}
		rowMIN = bndryItr+1;
		
		//Step forward to find the last intersection
		bndryItr = median;
		while ( tstop > mtime && bndryItr <= rowMAX ) {
			ffgcv( stat.fptr, TDOUBLE, 1, bndryItr++, 1, 1, 0, &mtime, NULL, &(stat.status) ); 
		}
		rowMAX = bndryItr-1;
		
		return 0;
	}
	
};


int TTEReader::findRowRange(  double tstart, double tstop, long& minr, long& maxr, int mode ) {
	
	if ( stat.fptr == NULL || stat.status != 0 ) return stat.status;
	
	if ( stat.move2hdu( (char*)"EVENTS" ) != 0 ) return stat.status;

	minr = 1;
	maxr = stat.n_rows;

	if (mode == 1 && (tstart - tstop) > 0.1) {
		
		double dummyTime = tstart+0.1;
		long dummyRow = maxr;
		
		this->binSortedIntersection( minr, dummyRow, tstart, dummyTime );
		
		dummyTime = tstop-0.1;
		dummyRow = minr;
		this->binSortedIntersection( dummyRow, maxr, dummyTime, tstop );
		
		return stat.status;
	}

	this->binSortedIntersection( minr, maxr, tstart, tstop );
	return stat.status;
};

#endif