/*
 *  make_tgf_drms.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 5/18/11.
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

#ifndef TTE_DEFAULT_PAD
#define TTE_DEFAULT_PAD 10.0
#endif

const char progname[] = "make_match_data";

using namespace std;
using namespace geom;

void printUsage() {
	cout << endl;
	cout << "Usage: " << progname << " [--help] ";
	cout << endl;
	
	cout << "  Displays the current matches table and asks the whether to create DRMs and TTE data" << endl ;
	
//	printSingleTimeHelp();
//	cout << endl << "Additional arguments: " << endl;
	cout << endl << endl;

	return;
};

inline void PromptUserSelection( string& choice )
{
	cout << endl << ">>> Select results ('1,2,3,...' or '1,3-5,7,8,...' etc; 'all' for All; 'q' to Quit)"<< endl;
	getline( cin, choice );
};


int main (int argc, char * const argv[]) { 


	char** argitr = getCmdOption( (char**)argv, (char**)argv+argc, "--help", 0);
	if ( argitr != NULL ) {
		printUsage();
		return 0;
	}

	GbmDataDb data_db( string(TTEARCH) ) ;
	AssociationDb assoc_db;
	
	TTEReader ttereader;
	TTEWriter ttewriter;
	TTEventTable<double,int,float> * gbm_tte_data=NULL;
	TTEventTable<double,int,float>::fields_class * keywordFields;
	double tteMargin=TTE_DEFAULT_PAD;
	
	assoc_db.open_db( string(TGFDB) );

	std::vector< AssociationDb::result_set* > entries;
	std::vector< AssociationDb::result_set* >::iterator eItr, epItr;
	std::vector< GbmDataDb::result_set > data;
	std::vector< GbmDataDb::result_set >::iterator dItr;
	
	VectorSpan<size_t, size_t> userIndices;
	VectorSpan< AssociationDb::result_set*, vector< AssociationDb::result_set* >::iterator > userItems;

	char thdr[250];
	char rowbr[250];
	char line[250];
	char * cols[] = { (char*)"TGF id",
						(char*)"Stroke", (char*)"Str.Off[s]", (char*)"axis[km]", (char*)"ground[km]", (char*)"beam[deg]",
						(char*)"az[deg]", (char*)"el[deg]" };
						
	char fmtRow[] = "%20s, %20s.%06ld, %10.6f, %10.2f, %10.2f, %10.2f, %7.2f, %7.2f";
	sprintf( thdr, (char*)"%20s, %27s, %10s, %10s, %10s, %10s, %7s, %7s", 
					cols[0], cols[1], cols[2], cols[3], cols[4], cols[5], cols[6], cols[7] );

	int recnum;
	int linelength = strlen( thdr )+1;
	int q=0;
	for (q=0;q<linelength+6;q++) *(rowbr+q) = '-';
	*(rowbr+q) = '\0';

	
	string userinput;
	bool quit = 0;
	bool all;
	bool firstloop=1;

	std::vector <size_t> randomIndices;
	std::vector < std::vector< AssociationDb::result_set* >::iterator > userSubset;
	std::vector < std::vector< AssociationDb::result_set* >::iterator >::iterator handle1, handle2;
	std::vector <AssociationDb::result_set*> multiplesQueue;
	std::vector <AssociationDb::result_set*>::iterator multRef;
	
	std::vector< AssociationDb::result_set > temporaryMatchListForFITS;

	//multiplesQueue.reserve(10);
	
	bool rdxTTE=0;
	bool doDRMs=0;
	bool keepNML=0;
	bool tteok=0;
	bool ttedefPad=1;
	int multiple=0;
	int degenNum=0;
	int haveData,nohaveData;
	long firstRow, lastRow;
	int det=0;
	
	string tteInFile, tteOutFile;
	
	char tid[20];
	
	assoc_db.printQry = 0;
	assoc_db.getEntries( entries, "", " order by tmet, bmAng asc " );

	if ( entries.size() == 0 ) {
		cout << "Table appears to be empty" << endl;
		return 0;
	}
	
	while( ! quit ) {
	
		haveData=0;
		nohaveData=0;
	
		recnum=0;
		userIndices.clear();
		userItems.clear();
		randomIndices.clear();
		userSubset.clear();
		//multiplesQueue.clear();
		
		if (! firstloop) {
			PromptUserSelection( userinput );
			quit = ( string::npos != userinput.find_first_of( "qQ" ) );
			all = ( userinput == "all" );
		} else {
			all=1;
		}
		
		if ( all ) {
			userItems.Insert( entries.begin(), entries.end() );
			userIndices.Insert( 1, entries.size()+1 );
		} else if ( quit ) {
			continue;
		} else {
			parseRecordSelction( userinput, userIndices );
			if ( userIndices.nslices() == 0 ) {
				cout << "hmm......" << endl;
				continue;
			}
			//cout << "non-zero: nslices = " << userIndices.nslices() << endl;
			userItems.index( entries, userIndices, -1 );
		}
	
		userItems.Enumerate(userSubset);
		userIndices.Enumerate( randomIndices );
	
		handle1 = userSubset.begin();
		handle2 = handle1;
		
		if ( handle1 != userSubset.end() ) {
			cout << endl << setw(6) << "|" << thdr << endl;
			cout << rowbr << endl;
		} else {
			continue;
		}
		
		degenNum=0;
		
		eItr = *handle1;
		epItr = *handle2;
		
		while ( handle1 != userSubset.end() ) 
		{
			
			multiple = assoc_db.getStrokeCount((*eItr)->tgf.tgfid );
			
			if ( multiple > 1 ) {
			
				degenNum++;
			
				sprintf( tid, "(*) %s", (*eItr)->tgf.tgfid.c_str() );
				sprintf( line, fmtRow, tid,
						(*eItr)->stroke.utc.c_str(), (long)floor((*eItr)->stroke.fsec*1000000.0),
						(*eItr)->stroke.t_offset, (*eItr)->dxBeam, (*eItr)->dxGnd, (*eItr)->bmAng,
						(*eItr)->az, (*eItr)->el );
						
				
			} else {
				sprintf( line, fmtRow, (*eItr)->tgf.tgfid.c_str(),
							(*eItr)->stroke.utc.c_str(), (long)floor((*eItr)->stroke.fsec*1000000.0),
							(*eItr)->stroke.t_offset, (*eItr)->dxBeam, (*eItr)->dxGnd, (*eItr)->bmAng,
							(*eItr)->az, (*eItr)->el );
			}
			
			cout << setw(5) << randomIndices[recnum++] << "|"  << line << endl;
			
			handle2 = handle1;
			handle1++; 
			
			eItr = *handle1;
			epItr = eItr-1;
		
		}
		
		//multRef = multiplesQueue.begin();
		if (degenNum>=1 ) cout << "-----------------" << endl << "(*) TGFs with multiple matches (spatially sorted)" << endl << endl;
	
	
		if (firstloop) firstloop=0;
		
		if (userSubset.size() == 0) continue;
		
		cout << "Make DRMs? [n]" << endl;
		getline( cin, userinput );
		doDRMs = ( userinput == "y" );
		quit = ( userinput == "q" );
		
		if ( quit ) continue;
		
		if ( doDRMs ) {
			cout << "Keep .nml file? [n]" << endl;
			getline( cin, userinput );
			keepNML = ( userinput == "y" );
		}
		
		cout << "Extract TTE subsets? [n]" << endl;
		getline( cin, userinput );
		quit = ( userinput == "q" );
		rdxTTE = ( userinput == "y" );
		
		if ( quit ) continue;
		
		if ( rdxTTE ) {
			ttedefPad=1;
			cout << "How much time around the TGF/trigger time, in seconds? [Default="<<setprecision(1) << TTE_DEFAULT_PAD<<"]"<<endl;
			cout << "[Existing TTE subsets for this TGF are not overwritten if the default time is used]"<<endl;
			getline( cin, userinput );
			
			if ( userinput.length() == 0 ) {
				tteMargin = TTE_DEFAULT_PAD;
			} else {
				double tempmargin = atof( userinput.c_str() );
			
				if ( tempmargin <= 0 ) {
					cout << "Must be > 0" << endl;
					continue;
				} else {
					tteMargin = tempmargin;
					ttedefPad=0;
					cout << "TGF Time +/- " << tteMargin << endl;
				}
			}
		}
		
		if (! (rdxTTE || doDRMs)) continue;
		
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
			default: cout << "Use TTE directories" << endl;
		}
		
		cout << endl << "Continue? [y]" << endl;

		getline( cin, userInput );
		if ( userInput.size() != 0 && userInput != "y" && userInput != "Y" ) return 0;
	

	
		data_db.table = "tte";
		handle1 = userSubset.begin();
		string baseName, phaOutFile;
		while ( handle1 != userSubset.end() && (rdxTTE || doDRMs) ) 
		{
			
			eItr = *handle1;
			data.clear();
			
			
			data_db.getCoveringData( (*eItr)->tgf.met, (*eItr)->tgf.met, data );
			dItr = data.begin();
			
			if ( dItr != data.end() ) {
				cout << "Found TTE data" << endl;
				haveData++; 
			} else  {
				cout << "Did not find TTE data (needed to get energy edges)" << endl;
				nohaveData++;
			}
			int status;
			while ( dItr != data.end() ) 
			{
				if (gbm_tte_data != NULL) {
					delete gbm_tte_data;
					gbm_tte_data=NULL;
				}
				
				cout << "--------"<<endl<<"Found: " << dItr->file << endl;
				
				tteInFile = dItr->path + path_sep() + dItr->file;
				
				ttereader.setFile( tteInFile );
				gbm_tte_data = new TTEventTable<double,int,float>;
				
				string tteOutDir;
				switch (method) {
					case 1: tteOutDir = rootOutput + path_sep() + "bn"+(*eItr)->tgf.tgfid;
							break;
					case 2: tteOutDir = rootOutput;
							break;
					default:
							tteOutDir = dItr->path;
				}
				
				//tteOutFile = tteOutDir + path_sep() + "ttx_"+string("tgf"+(*eItr)->tgf.tgfid+"_")+gbmDetShortname((char*)dItr->det.c_str())+".fit";
				tteOutFile = tteOutDir + path_sep() + TTX_CANON_NAME( (*eItr)->tgf.tgfid, gbmDetShortname((char*)dItr->det.c_str()) );
				
				if ( rdxTTE && ttedefPad && FileExists( tteOutFile ) ) {
					cout << "File exists: " << tteOutFile << endl;
					tteok=0;
				} else {
					if ( ! FileExists( tteOutDir ) ) makeDir( tteOutDir );
					tteok=1;
				}
				
				if ( rdxTTE && tteok) {
					ttereader.open();
					ttereader.findRowRange( (*eItr)->tgf.met-tteMargin, (*eItr)->tgf.met+tteMargin, firstRow, lastRow, 1 );
					ttereader.close();
					status = ttereader.ReadDataFile( gbm_tte_data, 0, firstRow, lastRow );
					gbm_tte_data->sortEvents();
				} else {
					status = ttereader.ReadDataFile( gbm_tte_data, 0, 1, 2 );
				}
				
				if (status != 0) {
					dItr++;
					continue;
				}
				EdgeSet<float> edges;
				gbm_tte_data->getEdgeSet(edges);
				
				keywordFields = gbm_tte_data->getMiscFields();
				keywordFields->object = (*eItr)->tgf.tgfid;			
				keywordFields->getDetName(tid);
				det = gbmDetname2Num(tid);
				
				
				if ( rdxTTE && tteok ) {
			
					gbm_tte_data->setTzero( (*eItr)->tgf.met );
					
					remove( (char*)tteOutFile.c_str() );
					
					ttewriter.setFile( tteOutFile );
					ttewriter.WriteDataFile( *gbm_tte_data );
					
					temporaryMatchListForFITS.clear();
					assoc_db.getKnownMatches( temporaryMatchListForFITS, (*eItr)->tgf.tgfid );
					AddTGFStroke_fitsTable( tteOutFile, temporaryMatchListForFITS );
					
				}
				
				
				
				if ( doDRMs ) {
					string obj = "tgf"+(*eItr)->tgf.tgfid;
					baseName = "rsp_"+obj+"_"+gbmDetShortname((char*)dItr->det.c_str());
					string nml = tteOutDir + path_sep() + baseName + ".nml";
					string rsp = tteOutDir + path_sep() + baseName + ".rsp";
				
					WriteNMLFile( (char*)"MakeOneDbDRM", nml, rsp, obj , edges, det, (*eItr)->az, (*eItr)->el, (*eItr)->tgf.met, (*eItr)->tgf.met );
					if ( ! FileExists(nml) ) {
						cout << "Error making NML file:" << endl << nml << endl;
					} else {
						if (FileExists(rsp) ) remove( rsp.c_str() );
						string cmd = string(WHICH_ENV)+string(" GBMRSP_NML=")+nml+" gbmrsp.exe";
						system( cmd.c_str() );
						
						if (! keepNML) remove( nml.c_str() );
					}
				}
				
				dItr++;
			}
			
			handle1++;
		}
		
	
	}

	return 0;
}
