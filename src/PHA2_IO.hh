/*
 *  PHA2_IO.h
 *  LightcurveSimulator
 *
 *  Created by Vandiver L. Chapin on 5/18/10.
 *  Copyright 2010 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#ifndef PHA_2_IODECLARED
#define PHA_2_IODECLARED

#include "PHA_IO.hh"
#include "EdgeSet.hh"
#include "PHAStructures.hh"

extern "C" {
	#include "fitsio.h"
};

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

class PHA2Writer : public PHAWriter {

	private:
	
	template<typename C,typename E,typename T>
	int WriteSpectrumHDU(fitsfile** fptr, int * status, PHA2<C,E,T>& set, typename EdgeSet< E >::size_type& nEdges);
	template<typename Oclass>
	int WriteStandardKeys(fitsfile** fptr, int * status, Oclass *& fields);
	
	template<typename C,typename E,typename T>
	int formatSpecHDUCLAS(fitsfile** fptr, int * status, PHA2<C,E,T> * data);
	
	/*
	template<typename E>
	int WriteEboundsHDU(fitsfile** fptr, int * status, EdgeSet<E> *& edges);
	template<typename T,typename M,typename E>
	int WriteGTIHDU(fitsfile** fptr, int * status, TTEventTable<T,M,E>& set);
	*/
	public:
	PHA2Writer(){
		pha_col=1;
		err_col=2;
	};
	
	template<typename C,typename E,typename T>
	int WriteDataFile(PHA2<C,E,T>& set);

};


#include "PHA2_IO.cpp"

#endif