/*
 *  make_geocenter_drms.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 7/19/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */


#include <cstdio>
#include <cstdlib>
#include <vector>
#include <iostream>
#include <iomanip>
 
#include "tgfsdb_io.h"
#include "gbm_geometry.hh"
#include "gbmrsp_interface.hh"

#include "TTE_IO.hh"
#include "GeoTransform.h"
#include "Gbm_PositionIO.h"

#ifndef TTEARCH
#define TTEARCH getenv("GBMDB")
#endif

#ifndef TGFDB 
#define TGFDB getenv("TGFDB")
#endif

#ifndef TTE_DEFAULT_PAD
#define TTE_DEFAULT_PAD 2.0
#endif

using namespace std;
using namespace geom;


int main (int argc, char * const argv[]) { 


	char ** argItr;
	string tgfSelector = "";
	
	argItr = getCmdOption( (char**)argv, (char**)argv+argc, "-w", 1 );
	
	if ( argItr != NULL ) {
		tgfSelector = *(argItr+1);
		cout << "Using selection criterion: " << tgfSelector << endl;
	}
	

	GbmDataDb datadb( string(TTEARCH) );
	TGFdb tgfdb( string(TGFDB) );
	GbmPosReader * posReader;
	geom::gquat qEv;
	geom::x3 xEv;
	
	datadb.printQry = 0;
	
	vector< TGFdb::result_set > tgfs;
	vector< TGFdb::result_set >::iterator tgfsItr;
	vector< GbmDataDb::result_set > posFiles;
	vector< GbmDataDb::result_set > tteFiles;
	vector< GbmDataDb::result_set >::iterator dItr;
	
	vector< TGFdb::result_set > tgfs_noTTE;

	TTEWriter ttewriter;
	TTEReader ttereader;
	TTEventTable<double,int,float> * gbm_tte_data=NULL;

	tgfdb.getEvents( tgfs, tgfSelector );
	
	tgfsItr = tgfs.begin();
	
	string file;
	
	string userInput;
	int method=0;
	string rootOutput;
	
	bool reduceTTE=0;
	double tteMargin;
	int haveData=0;
	int nohaveData=0;
	int numRspsMade=0;
	char detname[20];
	
	if ( tgfs.size() > 0 ) {
	
		cout << "Making geo-center DRMs for " << tgfs.size() << " event times" << endl;
	
		cout << endl << "Also make TTE files? [n]" << endl;

		getline( cin, userInput );
		if ( userInput.size() != 0 && (userInput == "y" || userInput == "Y") ) {
			reduceTTE=1;
			double tempmargin=-1.0;
			
			while ( tempmargin <= 0 ) {
			
				cout << "How much time around the TGF/trigger time, in seconds? [Default="<<setprecision(1) << TTE_DEFAULT_PAD<<"]"<<endl;
				getline( cin, userInput );
			
				if ( userInput.length() == 0 ) {
					tteMargin = TTE_DEFAULT_PAD;
					tempmargin=1.0;
				} else {
					tempmargin = atof( userInput.c_str() );
				
					if ( tempmargin <= 0 ) {
						cout << "Must be > 0" << endl;
						continue;
					} else {
						tteMargin = tempmargin;
						cout << "TGF Time +/- " << tteMargin << endl;
					}
				}
				
			}
		}
	
		while ( method == 0 ) {
			cout << endl;
			cout << "Where should the files be placed?" << endl;
			cout << "  1: in sub-directories 'bn' + TGF_ID" << endl;
			cout << "  2: all in one directory" << endl;
			cout << "  3: in the same directory as the TTE file" << endl;
			getline( cin, userInput );
			
			if ( userInput == "q" || userInput == "Q" ) return 0;
			
			if ( userInput != "1" && userInput != "2" && userInput != "3" ) continue;
			
			method = atoi( userInput.c_str() );
		 
		}
		
		string pwd = getenv((char*)"PWD");
		
		//if (pwd.size() > 0) pwd += path_sep();
		
		switch (method) {
			case 1: cout << "Enter the root directory ["+pwd+"]:" << endl;
					getline( cin, userInput );
					if ( userInput.size() == 0 ) rootOutput = pwd;
						else rootOutput = userInput;
		
					cout << endl << "Base directory = " << rootOutput << endl;
					break;
			case 2: cout << "Enter directory ["+pwd+"]:" << endl;
					getline( cin, userInput );
					if ( userInput.size() == 0 ) rootOutput = pwd;
						else rootOutput = userInput;
		
					cout << endl << "Base directory = " << rootOutput << endl;
					break;
			default: cout << "Use data directories" << endl;
		}
		
		cout << endl << "Continue? [y]" << endl;

		getline( cin, userInput );
		if ( userInput.size() != 0 && userInput != "y" && userInput != "Y" ) return 0;
	
	}
	
	
	
	while( tgfsItr != tgfs.end() ) {
	
		cout << endl << "-----------  TGF " << tgfsItr->tgfid << "  -----------" << endl;
	
		posFiles.clear();
		datadb.table = "pos";
		datadb.getCoveringData( tgfsItr->met, tgfsItr->met, posFiles, "", " order by type asc " );
		
		if ( posFiles.size() == 0) {
			cout << "No position file for TGF " << tgfsItr->tgfid << " at " << tgfsItr->utc << ", " << tgfsItr->met << endl;
			tgfsItr++;
			continue;
		}
		
		file = posFiles[0].path + path_sep() + posFiles[0].file;
		
		cout << "Found: "<< file << endl;
		
		
		posReader = NewPosReader::type(posFiles[0].type);
		
		if (posReader == NULL) {
			cout << "Unrecognized type: " << posFiles[0].type << endl;
			cout << "Type should be 'trigdat' or 'poshist'" << endl;
			tgfsItr++;
			continue;
		}
		
		posReader->setFile( file );
		
		if ( posReader->open() != 0 ) {
			cout << "Unable to open file" << endl;
			posReader->close();
			delete posReader;
			tgfsItr++;
			continue;
		}
		long row = posReader->findRow( tgfsItr->met );
		
		cout << "Row Num: " << row << endl;
		
		if ( row <= 0 ) {
			cout << "error finding row " << endl;
			posReader->close();
			delete posReader;
			tgfsItr++;
			continue;
		}
		
		tteFiles.clear();
			
		datadb.table = "tte";
		datadb.getCoveringData( tgfsItr->met, tgfsItr->met, tteFiles );
		dItr = tteFiles.begin();
		
		if ( dItr != tteFiles.end() ) {
			cout << "Found TTE data" << endl;
			haveData++; 
		} else  {
			cout << "Did not find TTE data (needed to get energy edges)" << endl;
			tgfs_noTTE.push_back( *tgfsItr );
			nohaveData++;
			posReader->close();
			delete posReader;
			tgfsItr++;
			continue;
		}
		
		double rowStart,rowStop;
		posReader->getScAttPos(&rowStart,&rowStop,&qEv,&xEv,row);
		
		double r, az, el;
		x3 geodir = xEv;
		geodir*=-1;
		qEv.rot( geodir );
		cart2sphere( geodir,r, az, el, 1 );
		vGeographic geo = gei2geo( rowStart, xEv );
		
		posReader->close();
		delete posReader;
		
		int status;
		int numDetsMade=0;
		while ( dItr != tteFiles.end() ) 
		{
			if (gbm_tte_data != NULL) {
				delete gbm_tte_data;
				gbm_tte_data=NULL;
			}
			
			cout << "--------"<<endl<<"Found: " << dItr->file << endl;
			
			string tteInFile = dItr->path + path_sep() + dItr->file;
			
			ttereader.setFile( tteInFile );
			gbm_tte_data = new TTEventTable<double,int,float>;
			
			string rspOutDir;
			switch (method) {
				case 1: rspOutDir = rootOutput + path_sep() + "bn"+tgfsItr->tgfid;
						break;
				case 2: rspOutDir = rootOutput;
						break;
				default:
						rspOutDir = dItr->path;
			}
			
			if (reduceTTE) {
				long start,end;
				status = ttereader.findRowRange( tgfsItr->met - tteMargin, tgfsItr->met + tteMargin, start, end );
				if (status==0) status = ttereader.ReadDataFile( gbm_tte_data, 0, start, end );
			} else {
				status = ttereader.ReadDataFile( gbm_tte_data, 0, 1, 2 );
			}	
			
			if (status != 0) {
				dItr++;
				continue;
			}
			
			EdgeSet<float> edges;
			gbm_tte_data->getEdgeSet(edges);
			gbm_tte_data->getMiscFields()->getDetName(detname);
			int det = gbmDetname2Num(detname);
			
			if ( ! FileExists( rspOutDir ) ) makeDir( rspOutDir );
			
			string obj = "tgf"+tgfsItr->tgfid;
			
			rspOutDir += path_sep();
			
			string nml = rspOutDir + TGFTLS_OUTFILE_CANON_NAME("nml", tgfsItr->tgfid, gbmDetShortname((char*)dItr->det.c_str()), "_nadir.nml" );
			string rsp = rspOutDir + TGFTLS_OUTFILE_CANON_NAME("rsp", tgfsItr->tgfid, gbmDetShortname((char*)dItr->det.c_str()), "_nadir.rsp" );
			
		
			WriteNMLFile( (char*)"MakeOneDbDRM", nml, rsp, obj , edges, det, az, el, tgfsItr->met, tgfsItr->met );
			if ( ! FileExists(nml) ) {
				cout << "Error making NML file:" << endl << nml << endl;
			} else {
				if (FileExists(rsp) ) remove( rsp.c_str() );
				string cmd = string(WHICH_ENV)+string(" GBMRSP_NML=")+nml+" gbmrsp.exe";
				system( cmd.c_str() );
				
				remove( nml.c_str() );
				numDetsMade++;
			}
			
			if ( reduceTTE ) {
				string tteFileName = rspOutDir + TTX_CANON_NAME(tgfsItr->tgfid, gbmDetShortname((char*)dItr->det.c_str()));
				ttewriter.setFile( tteFileName );
				ttewriter.WriteDataFile( *gbm_tte_data );
			}
			
			dItr++;
		}
		
		if (numDetsMade>0) numRspsMade++;
		
		tgfsItr++;
	}		
	cout << "---------------------------" << endl;
	cout << setw(10) << tgfs.size() << " events in the database table" << endl;
	cout << setw(10) << numRspsMade << " event drms made" << endl;
	cout << setw(10) << nohaveData << " had no TTE data" << endl;
	
	tgfsItr = tgfs_noTTE.begin();
	while ( tgfsItr != tgfs_noTTE.end() ) {
		cout << setw(10) << " " << tgfsItr->tgfid << endl;
		tgfsItr++;
	}
	
	return 0;
}