/*
 *  get_fermi_nadir.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/29/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
 
#include "tgfsdb_io.h"
#include "gbm_geometry.hh"
#include "GeoTransform.h"

#include "Gbm_PositionIO.h"
#include "spoccExeUtilities.h"

#ifndef TTEARCH
#define TTEARCH getenv("GBMDB")
#endif

#ifndef TGFDB 
#define TGFDB getenv("TGFDB")
#endif

const char progname[] = "get_fermi_nadir";


using namespace std;
using namespace geom;

void printUsage() {
	cout << endl;
	cout << progname << endl;
	cout << "Usage: " << progname << " [-met -mjd -utc] time  [-fmt utc_format (only with -utc)]  [--help]";
	//	cout << "Lat is geographic latitude in degrees" << endl;
	printSingleTimeHelp();
	cout << endl << endl;

	return;
};



int main (int argc, char * const argv[]) { 

	if ( argc < 2 ) {
		printUsage();
		return 1;
	}
	
	if (getCmdOption( (char**)argv, (char**)argv+argc, "--help", 0) !=NULL ) {
		printUsage();
		return 0;
	}
	
	double met;
	string timesys;
	
	if ( ! getTimeArgMET( (char**)argv, (char**)argv+argc, met, timesys ) ) {
		cout << "Error parsing time argument" << endl;
		printSingleTimeHelp();
		cout << endl << endl;
		return 2;
	}
	
	string file;
	GbmDataDb datadb( string(TTEARCH) );
	GbmPosReader * posReader;
	geom::gquat qEv;
	geom::x3 xEv;
	
	
	datadb.table = "pos";
	datadb.printQry = 0;
	
	vector< GbmDataDb::result_set > matches;
	
	datadb.getCoveringData( met, met, matches, "", " order by type asc " );
	
	cout << "Input MET: " << setprecision(16) << met << endl;
	
	if ( matches.size() != 0) {
		
		file = matches[0].path + path_sep() + matches[0].file;
		
		cout << "Found: "<< file << endl;
		
		
		
		posReader = NewPosReader::type(matches[0].type);
		
		if (posReader == NULL) {
			cout << "Unrecognized type: " << matches[0].type << endl;
			cout << "Type should be 'trigdat' or 'poshist'" << endl;
			return 3;
		}
		
		posReader->setFile( file );
		
		if ( posReader->open() != 0 ) {
			cout << "Unable to open file" << endl;
			return 3;
		}
		long row = posReader->findRow( met );
		
		cout << "Row Num: " << row << endl;
		
		if ( row <= 0 ) {
			cout << "error finding row " << endl;
			posReader->close();
			delete posReader;
			return 4;
		}
		
		double rowStart,rowStop;
		posReader->getScAttPos(&rowStart,&rowStop,&qEv,&xEv,row);
		
		//cout << "  [ " << file << ", " << "Row# " << row  << ", " << setprecision(16) << rowStart << ", " << rowStop << " ]" << endl;
		
		
		double r, az, el;
		x3 geodir = xEv;
		geodir*=-1;
		qEv.rot( geodir );
		cart2sphere( geodir,r, az, el, 1 );
		
		vGeographic geo = gei2geo( met, xEv );
		char utc[30];
		glast_met_to_utc( rowStart, utc );
		cout << endl << "@MET = " << setprecision(16) << rowStart << ", " << utc << endl;
		cout << setw(14) << "Nadir[deg]" << setw(8) << "" << setw(14) << "Geocenter[deg]" << endl;
		cout << setw(5) << "Lon" << setw(4) << "" << setw(5) << "Lat" << setw(8) << "" << setw(5) << "ScZen" << setw(4) << "" << setw(5) << "ScAz" << endl;
		cout << setprecision(5) << rad2deg(geo.lon) << setw(4) << "" << setprecision(5) << rad2deg(geo.lat) << setw(8) << "";
		cout << setprecision(5) << 90.0 - el << setw(4) << "" << setprecision(5) << az << endl; 
		
		posReader->close();
		delete posReader;
	} else {
		cout << "Did not find any containing files" << endl;
	}
	

	return 0;
}