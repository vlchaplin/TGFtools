/*
 *  utc2met.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 8/16/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "time2met.hh"

using namespace std;

int main (int argc, char * const argv[]) { 

	char ** argitr;
	char ** argend;
	char ** argPtr2;
	int argnum=1;
	double MET;
	string timesys;
	
	argitr = (char**)argv+1;
	argend = (char**)argv+argc;
	
	if ( getCmdOption(argitr,argend,"--help",0) ) printSingleTimeHelp();

	while ( argnum < argc ) {
		
		//if ( getCmdOption(argitr,argend,"-fmt",1) ) cout << "has format arg" << endl;
		
		//cout << argv[argnum] << endl;
		
		if ( getTimeArgMET( argitr, argitr+(argc-argnum), MET, timesys, NULL, 2 ) ) {
			//cout << setprecision(16) << *(argitr+1) << " ["<<timesys<<"] = " << MET << endl;
			cout << setprecision(16) << MET << endl;
		} else {
			//cout << "error parsing time" << endl;
		}
		
		//cout << *(argitr+1) << " ["<<timesys<<"] = " << MET << endl;
		argnum++;
		argitr++;
		//argitr = argPtr2;
	}


	
	
	return 0;

};