/*
 *  integrate_tgf_times.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 6/6/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
 
#include "tgfsdb_io.h"
#include "gbm_geometry.hh"
#include "gbmrsp_interface.hh"

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

using namespace std;

int main (int argc, char * const argv[]) { 

	GbmDataDb data_db( string(TTEARCH) ) ;
	TGFdb tgf_db( string(TGFDB) ) ;
	AssociationDb assoc_db;
	
	TTEReader ttereader;
	PHAWriter phawriter;
	TTEventTable<double,int,float> * gbm_tte_data=NULL;
	TTEventTable<double,int,float>::fields_class * keywordFields;

	vector<TGFdb::result_set> tgfs;
	vector<TGFdb::result_set>::iterator tgfitr;
	vector< GbmDataDb::result_set > data;
	vector< GbmDataDb::result_set >::iterator dItr;
	
	char * cols[] = { (char*)"TGF id", (char*)"TSTART", (char*)"TSTOP", (char*)"Dt[ms]" };
	char fmtRow[] = "%15s, %15.6f : %15.6f, %6.3f";
	char thdr[200];
	char line[200];
	char rowb[210];
	
	string tgfTbl = tgf_db.table;
	string ascTbl = assoc_db.table;
	sprintf( line, "%s.tgfid = %s.tgfid AND (%s.tstart NOT NULL) AND (%s.tstop NOT NULL) ", 
		tgfTbl.c_str(), ascTbl.c_str(), tgfTbl.c_str(), tgfTbl.c_str() );
		
	tgf_db.table = "("+tgfTbl+","+ascTbl+")";
	tgf_db.printQry=1;
	tgf_db.getEvents( tgfs, string(line) );
	
	sprintf( thdr, "%15s, %16s : %-16s, %8s", cols[0], cols[1], cols[2], cols[3] );
	
	int linelength = strlen( thdr )+1;
	int q=0;
	for (q=0;q<linelength+6;q++) *(rowb+q) = '-';
	*(rowb+q) = '\0';
	
	
	tgfitr = tgfs.begin();
	
	cout << endl;
	cout << thdr << endl;
	cout << rowb << endl;
	
	while ( tgfitr != tgfs.end() ) {
		
		
		sprintf( line, fmtRow, tgfitr->tgfid.c_str(), tgfitr->tstart, tgfitr->tstop, 
								1000.0*(tgfitr->tstop - tgfitr->tstart) );
		
		cout << line << endl;
		
		tgfitr++;
	}
	

	string userInput;
	int method=0;
	string rootOutput;
	while ( method == 0 ) {
		cout << endl;
		cout << "Where should the files be placed?" << endl;
		cout << "  1: in sub-directories 'bn' + TGF_ID" << endl;
		cout << "  2: all in one directory" << endl;
		cout << "  3: in the same directory as the original TTE" << endl;
		getline( cin, userInput );
		
		if ( userInput == "q" || userInput == "Q" ) return 0;
			
		if ( userInput != "1" && userInput != "2" && userInput != "3" ) continue;
		
		method = atoi( userInput.c_str() );
	 
	}
	
	string pwd = getenv((char*)"PWD");
	
	//if (pwd.size() > 0) pwd += path_sep();
	
	switch (method) {
		case 1: cout << "Enter the root directory ["+pwd+"]:" << endl; break;
		case 2: cout << "Enter directory ["+pwd+"]:" << endl;
	}
	
	getline( cin, userInput );
	
	if ( userInput.size() == 0 ) rootOutput = pwd;
	else rootOutput = userInput;
	
	cout << endl << "Base directory = " << rootOutput << endl;



	cout << "Continue? [y]" << endl;

	getline( cin, userInput );
	if ( userInput.size() != 0 && userInput != "y" && userInput != "Y" ) return 0;



	int haveData=0;
	int nohaveData=0;
	long firstRow, lastRow;
	double readMargin = 2.0;
	string phaOutFile;
	data_db.table = "tte";
	
	assoc_db.set_db_ptr( tgf_db.get_db_ptr() );
	
	tgfitr = tgfs.begin();
	while ( tgfitr != tgfs.end() ) {
	
		data.clear();
		data_db.getCoveringData( tgfitr->tstart, tgfitr->tstart, data );
		dItr = data.begin();
		dItr = data.begin();
		if ( dItr != data.end() ) {
			cout << "Found TTE data" << endl;
			haveData++; 
		} else  {
			cout << "Did not find TTE data" << endl;
			nohaveData++;
		}
		int status;
		
		TTEventTable<double,int,float>::iterator countItr;
		SinglePHA<long, float, double>::fields_class * phaFields;
		
		phawriter.setSpecComposition(2);
		
		
		
		while ( dItr != data.end() ) 
		{
			gbm_tte_data = new TTEventTable<double,int,float>;
			EdgeSet<float> * edges;// = new EdgeSet<float>;
			SinglePHA<long, float, double> pha;
			SinglePHA<long, float, double>::size_type nchan, i;
			vector< AssociationDb::result_set > assocs;
			vector< AssociationDb::result_set >::iterator ascItr;
			
			cout << "--------"<<endl<<"Found: " << dItr->file << endl;
			ttereader.setFile( dItr->path + path_sep() + dItr->file );
			ttereader.open();
			ttereader.findRowRange( tgfitr->tstart-readMargin, tgfitr->tstop+readMargin, firstRow, lastRow, 1 );
			ttereader.close();
			status = ttereader.ReadDataFile( gbm_tte_data, 0, firstRow, lastRow );
			
			countItr = gbm_tte_data->begin();
			edges = gbm_tte_data->getEdgeSet();
			pha.IsDifferential(0);
			pha.SetExposure(tgfitr->tstop - tgfitr->tstart);
			
			pha.ShareBinning( edges );
			phaFields = pha.getMiscFields();
			
			phaFields->setDetName( dItr->det );
			phaFields->tmin = tgfitr->tstart;
			phaFields->tmax = tgfitr->tstop;
			phaFields->tzero = tgfitr->met;
			phaFields->object = "tgf"+tgfitr->tgfid ;
			phaFields->bakfile = "pha_zero.bak";
			
			while ( countItr != gbm_tte_data->end() ) {
				if ( countItr->time >= tgfitr->tstart && countItr->time <= tgfitr->tstop ) {
					pha.AddCount( countItr->chan );
				}
				countItr++;
			}
			
			pha.UsePoissonErrors(1);
		//	for (i=0; i < pha.nchannels(); i++) {
		//		pha[i].stat_err /= pha.GetExposure();
		//		pha[i].cval /= pha.GetExposure();
		//	}
		//	pha.IsDifferential(1);
			
			string baseName = "pha_"+string("tgf"+tgfitr->tgfid+"_")+gbmDetShortname((char*)dItr->det.c_str())+".pha";
			string dnam;
			phaFields->getDetName(dnam);
			
			string phaOutRoot;
			switch (method) {
				case 1: phaOutRoot = rootOutput + path_sep() + "bn"+tgfitr->tgfid;
						break;
				case 2: phaOutRoot = rootOutput;
						break;
				default:
						phaOutRoot = dItr->path;
			}
			
			if ( ! FileExists( phaOutRoot ) ) makeDir( phaOutRoot );
			
			phaOutFile = phaOutRoot + path_sep() + baseName;
			
			phawriter.setFile( phaOutFile );
			
			phawriter.WriteDataFile( pha );
			
			
			assoc_db.getKnownMatches( assocs, tgfitr->tgfid );
			if ( assocs.size() != 0 ) AddTGFStroke_fitsTable( phaOutFile, assocs );
			
			if ( ! FileExists( phaOutRoot + path_sep() + phaFields->bakfile ) ) {
				pha.nullify();
				phawriter.setFile( phaOutRoot + path_sep() + phaFields->bakfile );
				phawriter.WriteDataFile( pha );
			}
			delete gbm_tte_data;
			
			dItr++;
			cout << endl;
		}
		tgfitr++;
		//cout << "outer" << endl;
		//return 0;
	}

	
	//cout << "here" << endl;
	return 0;
};