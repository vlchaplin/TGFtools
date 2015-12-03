/*
 *  get_tgf_direction.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 4/12/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
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

#ifndef DEFAULT_GET_WINDOW_SECS
#define DEFAULT_GET_WINDOW_SECS 300
#endif

const char progname[] = "get_tgf_coords";

using namespace std;
using namespace geom;

void printUsage() {
	cout << endl;
	cout << "Usage: " << progname << " [-met -mjd -utc] time [-fmt utc_format] [-s window] [--help] lon lat";
	cout << endl;
	
	printSingleTimeHelp();
	cout << endl << "Additional arguments: " << endl;
	cout << "-s window   lookup strokes within time +/- window seconds. Default is " << DEFAULT_GET_WINDOW_SECS << " seconds." << endl;
	cout << "lon   longitude in degrees east" << endl;
	cout << "lat   latitude in degrees north" << endl;
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
	
	bool userLocation=0;
	double srcLat, srcLon;
	double met;
	string timesys;
	int xargnum=4;
	long int windowSeconds = DEFAULT_GET_WINDOW_SECS;
	char ** argitr=NULL;
	
	if ( ! getTimeArgMET( (char**)argv, (char**)argv+argc, met, timesys ) ) {
		cout << "Error parsing time argument" << endl;
		cout << met << endl;
		printSingleTimeHelp();
		cout << endl << endl;
		return 2;
	}
	
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-fmt", 1);
	if ( argitr != NULL ) xargnum+=2;
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-s", 1);
	if ( argitr != NULL ) {
		windowSeconds = atoi( *(argitr+1) );
		xargnum+=2;
	}

	
	if ( argc >= xargnum+1 ) {
		userLocation=1;
		srcLon = atof( argv[xargnum-1] );
		srcLat = atof( argv[xargnum] );
	} else {
		userLocation=0;
	}	
	
	string file;
	GbmDataDb datadb( string(TTEARCH) );
	StrokeDb strokedb;
	GbmPosReader * posReader;
	
	vector< GbmDataDb::result_set > matches;
	vector< StrokeDb::result_set > strokes;
	vector< StrokeDb::result_set >::iterator strItr;
	vector< vGeographic > ltngLocs;
	vector< vGeographic >::iterator ltngItr;
	
	if (! userLocation ) {
		char utc[30];
		glast_met_to_utc( met, utc );
		
		strokedb.open_db( string(TGFDB) );
		strokedb.getNearestEvents( strokes, string(utc), windowSeconds );
		strItr = strokes.begin();
		if ( strItr == strokes.end() ) {
			cout << "Unable to find a stroke in within a " << windowSeconds << "-sec window around time " << utc << endl; 
			return 0;
		}
		
		ltngLocs.resize( strokes.size() );
		ltngItr = ltngLocs.begin();
		//Copy the database location to a geodetic coordinate structure
		while ( strItr != strokes.end() ) {
			//ltngItr->h = 20.0*1000.0;
			ltngItr->r = EARTH_AVG_RAD+20.0*1000.0;
			ltngItr->lon = geom::deg2rad( strItr->lon );
			ltngItr->lat = geom::deg2rad( strItr->lat );
			strItr++;
			ltngItr++;
		}
	} else {
		//vGeodetic tgf;
		//tgf.h = 20.0*1000.0;
		vGeographic tgf;
		tgf.r = EARTH_AVG_RAD+20.0*1000.0;
		tgf.lon = geom::deg2rad( srcLon );
		tgf.lat = geom::deg2rad( srcLat );
		
		ltngLocs.push_back( tgf );
	}
	
	geom::gquat qEv;
	geom::x3 xEv;
	
	
	char * env = getenv("POS_TABLE");
	if ( env == NULL ) datadb.table = "pos"; else datadb.table = string(env);
	datadb.printQry = 0;
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
		
		if ( matches[0].type == "trigdat" ) xEv *= 1000.0;
		
		//cout << "  [ " << file << ", " << "Row# " << row  << ", " << setprecision(16) << rowStart << ", " << rowStop << " ]" << endl;
		
		char utc[30];
		char thdr[200];
		char line[200];
		double r, az, el, tr, taz, tel, tra, tdec;
		x3 geodir = xEv;
		vGeographic fermiGeoloc = gei2geo( rowStart, xEv );
		
		glast_met_to_utc( rowStart, utc );
		
		geodir*=-1;
		cart2sphere( geodir, tra, tdec, 1 );
		qEv.rot( geodir );
		cart2sphere( geodir,r, az, el, 1 );
		
		ltngItr = ltngLocs.begin();
		
		
		
		cout << endl;
		cout << "MET: " << setprecision(16) << rowStart << endl;
		cout << "UTC: " << utc << endl;
		
		
		char * cols[] = { (char*)"Lon,Lat", (char*)"Dist[km]",(char*)"Axial[km]", (char*)"az[deg]", (char*)"el[deg]", (char*)"RA[deg]", (char*)"Dec[deg]" };
		sprintf( thdr, "%13s| %9s, %9s, %7s, %7s, %7s, %7s", cols[0], cols[1], cols[2], cols[3], cols[4], cols[5], cols[6] );
		sprintf( line, "%6.1f,%+6.1f| %9.1f, %9.1f, %+7.1f, %+7.1f, %7.1f, %+7.1f", 
			rad2deg(fermiGeoloc.lon), rad2deg(fermiGeoloc.lat), (fermiGeoloc.r - EARTH_AVG_RAD)/1000.0, 0.0, az, el, tra, tdec );
		
		cout << endl << thdr << endl;
		int linelength = strlen( thdr );
		int q=0;
		for (q=0;q<linelength;q++) *(thdr+q) = '-';
		*(thdr+q) = '\0';
		
		cout << thdr << endl;
		
		cout << line << " (nadir)" << endl;
		
		double fermiPos = xEv.mag();
		while ( ltngItr != ltngLocs.end() ) {
		
			x3 tgfgei = geo2gei( rowStart, *ltngItr );
			x3 tgfdir = tgfgei - xEv;
			
			cart2sphere( tgfdir, tr, tra, tdec, 1 );
			
			
			double angle = acos( dot( tgfgei, xEv ) / ( tgfgei.mag()*fermiPos ) );
			double axDist = fermiPos*sin(angle);
			qEv.rot( tgfdir );
			cart2sphere( tgfdir, taz, tel, 1 );
			
			
			sprintf( line, "%6.1f,%+6.1f| %9.1f, %9.1f, %+7.1f, %+7.1f, %7.1f, %+7.1f", rad2deg(ltngItr->lon), rad2deg(ltngItr->lat), tr/1000.0, axDist/1000.0, taz, tel, tra, tdec );
			cout << line << endl;
			ltngItr++;
		}
		
		posReader->close();
		delete posReader;
	} else {
	
		cout << endl;
		cout << "Did not find any GBM position files with that time" << endl;
	}
	

	return 0;
}

