/*
 *  get_many_coords.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 4/18/11.
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
#include "TTE_IO.hh"
#include "VectorSpan.h"
#include "spoccExeUtilities.h"

#ifndef TTEARCH
#define TTEARCH getenv("GBMDB")
#endif

#ifndef TGFDB 
#define TGFDB getenv("TGFDB")
#endif

#ifndef DEFAULT_GET_WINDOW_SECS
#define DEFAULT_GET_WINDOW_SECS 300.0
#endif

#ifndef DEFAULT_GROUND_RADIUS_KM 
#define DEFAULT_GROUND_RADIUS_KM 800.0
#endif


#define NO_STROKE_FOUND 1
#define NO_POSDATA_FOUND 2
#define FITSERR_POSDATA 3


const char progname[] = "get_match_coords";

using namespace std;
using namespace geom;

void printUsage() {
	cout << endl;
	cout << "Usage: " << progname << " [-s window -r max_radius -n --help]";
	cout << endl;
	
//	printSingleTimeHelp();
//	cout << endl << "Additional arguments: " << endl;
	cout << "-s window       lookup strokes within time +/- window seconds. Default is " << DEFAULT_GET_WINDOW_SECS << " seconds." << endl;
	cout << "-r max_radius   lookup strokes within max_radius km of the Fermi nadir. Default is "<<DEFAULT_GROUND_RADIUS_KM<<" km." << endl;
	cout << "-n              also print the nadir coordinates" << endl;
	cout << endl << endl;

	return;
};

static string stdPrompt = "'all' for All; 'q' to Quit; '1,2,3,...' or '1-3,4,5,...' ";

inline void PromptUserSelection( string& choice, string& options=stdPrompt )
{
	cout << endl << ">>> Select results ('1,2,3,...' or '1,3-5,7,8,...' etc; 'all' for All; 'q' to Quit)"<< endl;
	getline( cin, choice );
};

int main (int argc, char * const argv[]) { 

	
	if (getCmdOption( (char**)argv, (char**)argv+argc, "--help", 0) !=NULL ) {
		printUsage();
		return 0;
	}
	
	double met;
	string timesys;
	double windowSeconds = DEFAULT_GET_WINDOW_SECS;
	double nadirRadius = DEFAULT_GROUND_RADIUS_KM;
	char ** argitr=NULL;
	
	int doNadir = 0;
	
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-s", 1);
	if ( argitr != NULL ) {
		windowSeconds = atof( *(argitr+1) );
	}
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-r", 1);
	if ( argitr != NULL ) {
		nadirRadius = atof( *(argitr+1) );
	}
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-n", 0);
	if ( argitr != NULL ) {
		doNadir = 1;
	}
	
	string file;
	TGFdb tgf_db( string(TGFDB) );
	StrokeDb stroke_db( string(TGFDB) );
	GbmDataDb data_db( string(TTEARCH) ) ;	
	
	AssociationDb assoc_db;
	
	assoc_db.open_db( string(TGFDB) );
	//assoc_db.set_db_ptr( tgf_db.get_db_ptr() );
	assoc_db.CreateTable();
	
	
	//Structures to hold database queries' results
	std::vector< TGFdb::result_set > gbmTgfs;
	std::vector< TGFdb::result_set >::iterator evItr;
	std::vector< StrokeDb::result_set > strokes;
	std::vector< StrokeDb::result_set >::iterator strItr;
	std::vector< StrokeDb::result_set > timeOnlyStrokes;
	std::vector< StrokeDb::result_set >::iterator tmostrItr;
	std::vector< GbmDataDb::result_set > data;
	std::vector< GbmDataDb::result_set >::iterator dItr;
	
	std::vector< TransfSrcCrd * > timeMatches;
	std::vector< TransfSrcCrd * > scEvCoords;
	
	data_db.printQry = 0;
	
	//Structures to hold trigdat/poshist data at event time
	//defined in gbm_geometry.hh
	geom::gquat qEv;
	geom::x3 xEv;
	double rowStart,rowStop;
	
	//GBM data handlers
	GbmPosReader * posReader=NULL;
	TTEReader ttereader;
	TTEWriter ttewriter;
//	TTEventTable<double,int,float> * gbm_tte_data = NULL;
//	TTEventTable<double,int,float>::fields_class * keywordFields;
	//int haveAssoc=0;
	//int haveData=0;
	int haveGbmPos=0;
	//int nohaveData=0;
	int nohaveGbmPos=0;
		
	tgf_db.getEvents( gbmTgfs, "" );
	evItr = gbmTgfs.begin();
	
	scEvCoords.reserve( 2*gbmTgfs.size() );
	while (evItr != gbmTgfs.end()) {
		
		
		TransfSrcCrd * newEntry = new TransfSrcCrd;;
		
		newEntry->flag = 0;
		newEntry->gbmTgfEntry = *evItr;
		
		data.clear();
		data_db.table = "pos";
		data_db.getCoveringData( evItr->met, evItr->met, data, "", " order by type asc " );
		dItr = data.begin();
		if ( dItr == data.end() ) {
			cout << "Did not find a GBM position file for time: "<< evItr->utc << ", tgfid = '" << evItr->tgfid << "'" << endl;
			nohaveGbmPos++;
			newEntry->flag = NO_POSDATA_FOUND;
			
			
			timeOnlyStrokes.clear();
			stroke_db.getNearestEvents( timeOnlyStrokes, evItr->utc, windowSeconds );
			
			strItr = timeOnlyStrokes.begin();
			
			while ( strItr != timeOnlyStrokes.end() ) {
				TransfSrcCrd * unmatched = new TransfSrcCrd;
				unmatched->gbmTgfEntry = *evItr;
				unmatched->strokeEntry = *strItr;
			
				timeMatches.push_back( unmatched );
				strItr++;
			}
			
		} else {
			haveGbmPos++;
			
			posReader = NewPosReader::type(dItr->type);
			if ( posReader == NULL ) {
				cout << "Unrecognized type: " << dItr->type << endl;
				cout << "Type should be 'trigdat' or 'poshist'" << endl;
				newEntry->flag = NO_POSDATA_FOUND;
				evItr++;
				continue;
			}
			
			posReader->setFile( dItr->path + path_sep() + dItr->file );
			if ( posReader->open() != 0 ) {
				cout << "unable to open file " << endl;
				evItr++;
				newEntry->flag = FITSERR_POSDATA;
				delete posReader;
				continue;
			}
			long row = posReader->findRow( evItr->met );
			if ( row <= 0 ) {
				cout << "error finding row " << endl;
				evItr++;
				newEntry->flag = FITSERR_POSDATA;
				posReader->close();
				delete posReader;
				continue;
			}
			
			posReader->getScAttPos(&rowStart,&rowStop,&qEv,&xEv,row);
		
			if (dItr->type == "trigdat" ) xEv *= 1000.0;
			
			newEntry->xEv = xEv;
			newEntry->qEv = qEv;
			newEntry->dataTime = rowStart;
			
			posReader->close();
			delete posReader;
			
			vGeographic fermiGeoloc = gei2geo( rowStart, xEv );
			
			strokes.clear();
			//stroke_db.getNearestEvents( strokes, evItr->utc, windowSeconds );
			stroke_db.getSpaceTimeEvents( strokes, evItr->met, 
				rad2deg(fermiGeoloc.lon), rad2deg(fermiGeoloc.lat), 
				windowSeconds,
				nadirRadius );
			
			strItr = strokes.begin();
			if ( strItr == strokes.end() ) {
				//cout << "Unable to find a stroke in within a " << windowSeconds << "-sec window around time " << evItr->utc << endl; 
				newEntry->flag = NO_STROKE_FOUND;
				if ( doNadir == 1 ) scEvCoords.push_back( newEntry );
				evItr++;
				continue;
			} else {
				scEvCoords.push_back( newEntry );
			}
			
			newEntry->flag = 0;
			newEntry->strokeEntry = *strItr;
			
			strItr++;
			//Copy the database location to a geodetic coordinate structure
			while ( strItr != strokes.end() ) {
				TransfSrcCrd * additionalEntry = new TransfSrcCrd;
				*additionalEntry = *newEntry;
				additionalEntry->strokeEntry = *strItr;
				scEvCoords.push_back( additionalEntry );
				strItr++;
			}
		}		
		evItr++;
	}
		
	char thdr[250];
	char rowbr[250];
	char line[250];
	double r, az, el, tr, taz, tel, tra, tdec;
	
	int recnum=1;
	
	TransfSrcCrd* mergedItr;
	std::vector< TransfSrcCrd* >::iterator mergedPtrItr;
	
	VectorSpan<size_t, size_t> userIndices;
	VectorSpan<TransfSrcCrd*, vector< TransfSrcCrd* >::iterator > userItems;
	vector< vector< TransfSrcCrd* >::iterator > enumeratedItems;
	vector< vector< TransfSrcCrd* >::iterator >::iterator enumHandle;
	
	char * cols[] = { (char*)"TGF id", (char*)"Loc.Src.", (char*)"(Lon, Lat)", 
						(char*)"Dist[km]", (char*)"Axial[km]", (char*)"SeaL.[km]",
						(char*)"az[deg]", (char*)"el[deg]", (char*)"RA[deg]", (char*)"Dec[deg]" };
						
	char fmtRow[] = "%15s, %10s, (%6.1f,%+6.1f), %9.1f, %9.1f, %9.1f, %+7.1f, %+7.1f, %7.1f, %+7.1f";
	sprintf( thdr, "%15s, %10s, %15s, %9s, %9s, %9s, %7s, %7s, %7s, %7s", 
					cols[0], cols[1], cols[2], cols[3], cols[4], cols[5], cols[6], cols[7], cols[8], cols[9] );

	//cout << endl << setw(6) << "|" << thdr << endl;
	int linelength = strlen( thdr )+1;
	int q=0;
	for (q=0;q<linelength;q++) *(rowbr+q) = '-';
	*(rowbr+q) = '\0';
	//cout << rowbr << endl;
	
	
	mergedPtrItr = timeMatches.begin();	
	
	if ( mergedPtrItr  != timeMatches.end() ) {
		cout << "Time-only matches" << endl;
		cout << setw(6) << " " << "|" ;
		cout << setw(15) << "TGF id";
		cout << setw(25) << "TGF Time";
		cout << setw(25) << "Stroke Time" << endl;
		cout << rowbr << endl;
	}
	q=1;
	string prev;
	while( mergedPtrItr != timeMatches.end() ) {
		
		if ( prev != (*mergedPtrItr)->gbmTgfEntry.tgfid ) cout << setw(6) << q++ << "|";
		else cout << setw(6) << "" << "|";

		cout << setw(15) << (*mergedPtrItr)->gbmTgfEntry.tgfid;
		cout << setw(25) << (*mergedPtrItr)->gbmTgfEntry.utc;
		cout << setw(25) << (*mergedPtrItr)->strokeEntry.utc << endl;
		
		prev = (*mergedPtrItr)->gbmTgfEntry.tgfid;
		mergedPtrItr++;
		
	}
	
	
	string userinput;
	bool quit = 0;
	bool all;
	bool save=0;
	bool firstloop=1;
	bool commit=1;
	int rc;
	int nAssocsOut;
	
	vector <size_t> randomIndices;
	
	while( ! quit ) {
	
		recnum=0;
		userIndices.clear();
		userItems.clear();
		
		randomIndices.clear();
		enumeratedItems.clear();
		
		
		if (! firstloop) {
			PromptUserSelection( userinput );
			quit = ( string::npos != userinput.find_first_of( "qQ" ) );
			all = ( userinput == "all" );
			save=0;
		} else {
			all=1;
		}
		
		if ( all ) {
			userItems.Insert( scEvCoords.begin(), scEvCoords.end() );
			userIndices.Insert( 1, scEvCoords.size()+1 );
			userItems.Enumerate(enumeratedItems);
		} else if ( quit ) {
			continue;
		}
		else {
		
			parseRecordSelction( userinput, userIndices );
			if ( userIndices.nslices() == 0 ) {
				cout << "hmm......" << endl;
				continue;
			}
			cout << "non-zero: nslices = " << userIndices.nslices() << endl;
			userItems.index( scEvCoords, userIndices, -1 );
			//cout << "index" << endl;
			userItems.Enumerate(enumeratedItems);
			//cout << "enum" << endl;
		}
		
		//mergedPtrItr = scEvCoords.begin();
		enumHandle = enumeratedItems.begin();
		//cout << "begun" << endl;
		userIndices.Enumerate( randomIndices );
		//cout << "listed" << endl;
		
		if ( enumHandle != enumeratedItems.end() ) {
			cout << endl << setw(6) << "|" << thdr << endl;
			cout << rowbr << endl;
		}
		
		while ( enumHandle != enumeratedItems.end() ) {
			
			mergedItr = **enumHandle;
			
			if ( ( doNadir == 1 ) && mergedItr->flag <= NO_STROKE_FOUND ) {
			
				vGeographic fermiGeoloc = gei2geo( mergedItr->dataTime, mergedItr->xEv );
				x3 geodir = mergedItr->xEv; geodir*= -1;
				cart2sphere( geodir, tra, tdec, 1 );
				mergedItr->qEv.rot( geodir );
				cart2sphere( geodir,r, az, el, 1 );

				sprintf( line, fmtRow, mergedItr->gbmTgfEntry.tgfid.c_str(), "n.", rad2deg(fermiGeoloc.lon), rad2deg(fermiGeoloc.lat),
					(fermiGeoloc.r - EARTH_AVG_RAD)/1000.0, 0.0, 0.0, az, el, tra, tdec );
				cout << setw(5) << "" << "|" << line << endl;
			}
			
			
			double fermiMag = mergedItr->xEv.mag();
			if ( mergedItr->flag == 0 ) {
			
				vGeodetic srcPos;
				srcPos.h = 20.0*1000.0;
				
				//vGeographic srcPos;
				//srcPos.r = EARTH_AVG_RAD+20.0*1000.0;
				srcPos.lon = geom::deg2rad( mergedItr->strokeEntry.lon );
				srcPos.lat = geom::deg2rad( mergedItr->strokeEntry.lat );
				
				x3 tgfgei = geo2gei( mergedItr->dataTime, srcPos );
				x3 tgfdir = tgfgei - mergedItr->xEv;
				
				cart2sphere( tgfdir, tr, tra, tdec, 1 );
				
				double angle = acos( dot( tgfgei, mergedItr->xEv ) / ( tgfgei.mag()*fermiMag ) );
				double axDist = fermiMag*sin(angle);
				mergedItr->qEv.rot( tgfdir );
				cart2sphere( tgfdir, taz, tel, 1 );
				
				sprintf( line, fmtRow, mergedItr->gbmTgfEntry.tgfid.c_str(), mergedItr->strokeEntry.locsrc.c_str(), mergedItr->strokeEntry.lon, mergedItr->strokeEntry.lat, 
							tr/1000.0, axDist/1000.0, mergedItr->strokeEntry.x_offset, taz, tel, tra, tdec );
				cout << setw(5) << randomIndices[recnum++] << "|" << line << endl;
				
			}
			enumHandle++;
		}
		
		if ( firstloop ) firstloop=0;
		else {
			cout << "Save associations (overwrites existing entry)? [n]" << endl;
			
			getline( cin, userinput );
		
			commit = ( userinput == "y" );
			
			if ( commit ) {
				nAssocsOut=0;
				enumHandle = enumeratedItems.begin();
				while ( enumHandle != enumeratedItems.end() ) {
					mergedItr = **enumHandle;
					
					if ( mergedItr->flag == 0 ) {
					
						rc = assoc_db.commitAssociation( mergedItr->gbmTgfEntry, mergedItr->strokeEntry, 
															mergedItr->xEv, mergedItr->qEv, mergedItr->dataTime );
					
						if ( rc == SQLITE_DONE ) {
							nAssocsOut++;
						}
					
					}
					
					
					enumHandle++;
				}
				
				cout << "**" << nAssocsOut << " written or updated**" << endl << endl;
				
			}
			
		}

	}
	
	
	return 0;
}
