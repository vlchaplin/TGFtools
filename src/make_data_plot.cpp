/*
 *  make_data_plot.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 5/23/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */
 
/* 
#ifdef HAVE_DISLIN
namespace dis {
	extern "C" {
		#include "dislin.h"
	};
};
#endif
*/

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <iostream>

#include "VectorSpan.h"
#include "spoccExeUtilities.h"

#include "gbm_geometry.hh"
#include "gbmrsp_interface.hh"

#include "GeoTransform.h"
#include "tgfsdb_io.h"

#include "TTE_IO.hh"
#include "Gbm_PositionIO.h"

#include "disApp.h"

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
#define DEFAULT_GROUND_RADIUS_KM 500.0
#endif

using namespace std;


#include "dislin.h"
//using namespace dis;


const char progname[] = "make_data_plot";

const char multixVals[] = "1x|5x|10x|20x";

inline void histogramPlot( float * edges, float * yh, long nbins ) {

	long j=0;
	float hBuff[2];
	while ( j < nbins ) {
		hBuff[0] = yh[j];
		hBuff[1] = yh[j];
		curve( edges+j, hBuff, 2  );
		hBuff[0] = edges[j+1];
		hBuff[1] = edges[j+1];
		curve( hBuff, yh+j, 2  );
		j++;
	}
};


struct guiData {
	double tbase;
	float time0;
	float time1;
	long edge0;
	long edge1;
	long stride;
	int ndraws;
	int xwindow;
	
	float * histogram;
	float * xBins;
	float binSize;
	long nbins;
	
	float axY[2];
	float axX[2];
	
	char text0[25];
	char text1[25];
	char message[100];
	
	std::vector< StrokeDb::result_set > * strokes;
	std::vector< TGFdb::result_set > * tgfs;
	sqlite3 * tgfdbptr;
	
	guiData() {
		ndraws=0;
		stride=1;
		histogram=NULL;
		xBins = NULL;
		nbins = 0;
	}
};

struct guiWidgets {
	
	int base;
	int draw;
	int b0L;
	int b0R;
	int b1L;
	int b1R;
	int label;
	int label0;
	int label1;
	int eventLabel;
	int xxDrop;
	int saveTimes;
	int message;
	
	static void b_move(int bid) {
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
		
		if ( data->ndraws == 0 ) {
			data->time0 = data->xBins[data->edge0];
			data->time1 = data->xBins[data->edge1];
			wids->DrawPlot();
			return;
		};
		
		int idx = gwglis( wids->xxDrop );
		char * selection = itmstr( multixVals, idx );
		char * end = strstr( selection, "x" );
		*end = '\0';
		
		data->stride = atoi( selection );
		
		delete selection;
			
		if ( (bid == wids->b0L) && ( data->edge0 - data->stride < 0 ) ) return;
		if ( (bid == wids->b0R) && ( data->edge0 + data->stride >= data->edge1) ) return;
		if ( (bid == wids->b1L) && ( data->edge1 - data->stride <= data->edge0) ) return;
		if ( (bid == wids->b1R) && ( data->edge1 + data->stride > data->nbins ) ) return;
		
		if (bid == wids->b0L) data->edge0 -= data->stride;
		else if (bid == wids->b0R) data->edge0 += data->stride;
		else if (bid == wids->b1L) data->edge1 -= data->stride;
		else if (bid == wids->b1R) data->edge1 += data->stride;
		else if (bid != -1) return;

		if ( bid & ( wids->b0L | wids->b0R ) ) data->time0 = data->xBins[data->edge0];
		if ( bid & ( wids->b1L | wids->b1R ) ) data->time1 = data->xBins[data->edge1];
		
		wids->DrawPlot();
	};	
	
	static void b0_move_time(int bid) {
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
				
		cout << "right" << data->time1 << endl;
	};
	
	void UpdateLabel() {
	
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		
		sprintf( data->text0, "Tmin : %0.6f s", data->time0 );
		sprintf( data->text1, "Tmax : %0.6f s", data->time1 );
		
		swgtxt( this->label0, data->text0 );
		swgtxt( this->label1, data->text1 );
		
	};
	
	void DrawPlot() {
		
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		
		data->ndraws++;
		
		metafl ( (char*)"const" );
		setpag ( (char*)"da4l" );
		
		//page(1000,1000);
		setxid( this->draw, "WIDGET" );
		
		disini();
		//axslen( 800,800);
		unit(0);
		complx();
		pagfll(255);
		
		setrgb(0,0,0);
		name("T - Tgf", "x");
		name("Counts", "y");
		
		labdig(6, "X");
		
		graf( data->axX[0], data->axX[1], data->axX[0], ( data->axX[1] - data->axX[0] )/ 4.0 ,
		      data->axY[0], data->axY[1], data->axY[0], ( data->axY[1] - data->axY[0] )/ 4.0 );
		
		polcrv( "STAIRS" );
		
		histogramPlot( data->xBins, data->histogram, data->nbins );
		
		
		setrgb(1,0,0);
		
		float lwb[] = { data->time0, data->time0 };
		float upb[] = { data->time1, data->time1 };
		
		curve( lwb, data->axY, 2 );
		curve( upb, data->axY, 2 );
		
		this->UpdateLabel();
		
	/*	
		strItr = nearbyStrokes.begin();
		float verticalPnts[] =  { ymin, ymax };
		
		size_t i=0;
		size_t ns=nearbyStrokes.size();
		while ( strItr != nearbyStrokes.end() ) {
			
			float strokePos[] = { strItr->t_offset, strItr->t_offset };
			
			strItr++;
			setrgb(  1.0 - ( float(i) / ns ) , 0.0,  ( float(i) / ns ) );
			curve( strokePos, verticalPnts, 2 );
			i++;
		}
		
	*/	

		endgrf();
		
		//std::cout << "loop is over" << endl;
		
		disfin();
		
		
	};
	
}; 


void write_time_bounds(int bid) {

	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
	
	std::cout << "here" << endl;

	if ( data->tgfs->size() == 0 ) {
		std::cout << "No TGF" << endl;
		return;
	}
	sqlite3_stmt * ppStmt;
	
	string qry = " UPDATE or REPLACE tgflist SET tstart=?, tstop=? WHERE tgfid = '"+data->tgfs->begin()->tgfid+"' ";
	
	std::cout << qry << endl;
	
	sqlite3_prepare_v2( data->tgfdbptr, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	sqlite3_bind_double(ppStmt, 1, data->tbase + data->time0 );
	sqlite3_bind_double(ppStmt, 2, data->tbase + data->time1 );
	int rc = sqlite3_step(ppStmt);
	sqlite3_finalize( ppStmt );
	
	if ( rc == SQLITE_DONE ) {
		sprintf( data->message, "Saved integration time for TGF '%s':   %f   to   %f  ,  dt = %0.3f ms", 
								data->tgfs->begin()->tgfid.c_str(), 
								data->tbase + data->time0, data->tbase + data->time1,
								1000.0*(data->time1 - data->time0) );
		swgtxt( wids->message, data->message);
	}
	
	std::cout << rc << endl;

};


void printUsage() {
	cout << endl;
	cout << "Usage: " << progname << " [-tgf tgfid] or [-met -mjd -utc time] [-w -wms window] [-dx -dxms binsize]  ";
	cout << endl;
	
	cout << "  Plot TTE lightcurve with the given parameters " << endl ;
	cout << "  The time must be specified either by a time argument or by " << endl ;
	cout << "  specifying a TGF ID string in -tgf " << endl ;
	cout << "  If -tgf is used, T0 is the corresponding tgf time." << endl;
	printSingleTimeHelp();
	
	cout << "Histogram parameters:" << endl;
	cout << " -w     Set the time range in seconds around the input time(T0)." << endl;
	cout << " -wms   Same as above but in milliseconds" << endl;
	cout << " -dx    Set the histogram binsisze in seconds" << endl;
	cout << " -dxms       ''   ''   ''    ''    in milliseconds" << endl;


	cout << endl << endl;

	return;
};


int main (int argc, char * const argv[]) {   

	char ** argitr=NULL;
	
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "--help", 0);
	if ( argitr != NULL ) {
		printUsage();
		return 0;
	}

	TGFdb tgf_db( string(TGFDB) );
	std::vector<TGFdb::result_set> * qry_result = new std::vector<TGFdb::result_set>;

	double plotCenterMET;
	double tteMargin=0.016;
	float binSize=0.0001;
	long nbins, firstRow, lastRow;
	float * thist=NULL;
	float * edges=NULL;
	int status;
	string timesys;
	std::string tgfid;
	
	
	
	
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-dx", 1);
	if ( argitr != NULL ) {
		binSize=atof( *(argitr+1) );
	}
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-dxms", 1);
	if ( argitr != NULL ) {
		binSize=atof( *(argitr+1) ) / 1000.0;
	}
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-w", 1);
	if ( argitr != NULL ) {
		tteMargin=atof( *(argitr+1) ) ;
	}
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-wms", 1);
	if ( argitr != NULL ) {
		tteMargin=atof( *(argitr+1) ) / 1000.0 ;
	}
	
	if ( tteMargin*2.0 < binSize ) {
		
		cout << "Step size is larger than the window:  Integrating just the window" << endl;
		binSize = 2.0*tteMargin;
		
	}
	
	
	argitr = getCmdOption( (char**)argv, (char**)argv+argc, "-tgf", 1);
	if ( argitr != NULL ) {
		
		tgfid = *(argitr+1) ;
		
		tgf_db.getEvents( *qry_result, " tgfid = '"+tgfid+"';" );
		
		if ( qry_result->size() == 0 ) {
			cout << " No TGFs matching \" tgfid = '"+tgfid+"' \"" << endl;
			return 0;
		}
		
		plotCenterMET = (*qry_result)[0].met;
		
	} else if (! getTimeArgMET( (char**)argv, (char**)argv+argc, plotCenterMET, timesys ) ) {
		
		cout << "Error parsing time argument" << endl;
		
		return 2;
	} else {
	
		char where[60];
		
		sprintf( where, " abs(met - %f) < %f ", plotCenterMET, tteMargin );
	
		tgf_db.getEvents( *qry_result, string(where) );
		
		
	}
	
	
	GbmDataDb data_db( string(TTEARCH) ) ;
	StrokeDb stroke_db( string(TGFDB) );
	
	TTEReader ttereader;
	TTEventTable<double,int,float> * gbm_tte_data = NULL;
	TTEventTable<double,int,float>::fields_class * keyFields;
	
	std::vector< GbmDataDb::result_set > data;
	std::vector< GbmDataDb::result_set >::iterator dItr;
	std::vector< StrokeDb::result_set > nearbyStrokes;
	std::vector< StrokeDb::result_set >::iterator strItr;

	
	data_db.table = "tte";
	data_db.getCoveringData( plotCenterMET-tteMargin, plotCenterMET+tteMargin, data );
	
	dItr = data.begin();
	
	if ( dItr == data.end() ) {
		cout << "No TTE Found" << endl;
		return 0;
	}
	
	float ymin=99999999.9;
	float ymax=0;
	
	double tstart, tstop;
	tstart=plotCenterMET-tteMargin;
	tstop =plotCenterMET+tteMargin;
	while ( dItr != data.end() ) 
	{

		ttereader.setFile( dItr->path + path_sep() + dItr->file );
		ttereader.open();
		status = ttereader.findRowRange( plotCenterMET-tteMargin, plotCenterMET+tteMargin, firstRow, lastRow, 1 );
		ttereader.close();
		
		if ( status != 0 ) {
			dItr++;
			continue;
		}
		
		
		gbm_tte_data = new TTEventTable<double,int,float>;
		status = ttereader.ReadDataFile( gbm_tte_data, 0, firstRow, lastRow );
		if ( status != 0 ) {
			dItr++;
			delete gbm_tte_data;
			continue;
		}
		gbm_tte_data->sortEvents();
		keyFields = gbm_tte_data->getMiscFields();
		
		if ( thist == NULL )
			gbm_tte_data->buildHistogram( tstart, tstop, binSize, 0, 128, &thist, &nbins, &edges, plotCenterMET );
		else 
			gbm_tte_data->addToHistogram( edges, thist, nbins, 0, 128, plotCenterMET );
		
		std::cout << dItr->file << ", Nbins: " << nbins << endl;

		dItr++;
		delete gbm_tte_data;
		
		//dItr = data.end();
		
		//std::cout << "here" << endl;
				
	}
	
	stroke_db.getSpaceTimeEvents( nearbyStrokes, plotCenterMET, 
				0,0, 
				tteMargin,
				EARTH_AVG_RAD*2*geom::PI );
	
			
	for ( int j=0; j<nbins; j++ ) {
		if ( thist[j] > ymax ) ymax = thist[j];
		if ( thist[j] < ymin ) ymin = thist[j];
	}
	
	if ( ymin > 75 ) { ymin = ymin - exp( -float(ymax)/ymin )*( ymax - ymin ); } else ymin = 0;
	if ( ymax > 100 ) {
		ymax = ymax + exp( -(ymax-ymin)/(ymax+ymin) )*( ymax - ymin );
	} else {
		ymax += 5;
	}
	
	disApp * theApp = disApp::Instance();
	
	guiWidgets * wids = new guiWidgets;
	guiData * wdata = new guiData;
	
	theApp->setAppData( (void*) wdata );
	theApp->setAppWidgets( (void*) wids );
	
	wdata->tgfdbptr = tgf_db.get_db_ptr();
	wdata->tgfs = qry_result;
	wdata->histogram = thist;
	wdata->xBins = edges;
	wdata->nbins = nbins;
	wdata->binSize = binSize;
	wdata->tbase = plotCenterMET;
	wdata->axX[0] = edges[0];
	wdata->axX[1] = edges[nbins];
	wdata->axY[0] = floor(ymin);
	wdata->axY[1] = ceil(ymax);
	wdata->edge0 = 1;
	wdata->edge1 = nbins-1;
	
	//swgsiz( 900, 900 );
	swgwth( -50 );
	swgdrw( 0.706 );
	swgmrg( 12, "TOP");
	wids->base = wgini( "VERT" );
	int dwbase = wgbas( wids->base, "VERT" );
	swgclr( 1.0, 1.0, 1.0, "BACK");
	wids->draw = wgdraw( dwbase );
		
	swgwth( 25 );
	swgjus("CENTER", "LABEL");
	
	char tgfname[25];
	strcpy( tgfname, qry_result->begin()->tgfid.c_str() );
	

	
	int bbRow11 = wgbas( wids->base, "HORI" );
	wids->label0 = wglab( bbRow11, "Press a button" );
	wids->label1 = wglab( bbRow11, "Press a button" );
	wids->eventLabel = wglab( bbRow11, tgfname );
	swgwth( 4 );
	int bbRow21 = wgbas( wids->base, "HORI" );
	wids->b0L = wgpbut( bbRow21, "<<" );
	wids->b0R = wgpbut(bbRow21, ">>");
	swgwth( 15 );
	wids->xxDrop = wgdlis( bbRow21, multixVals, 1 );
	swgwth( 4 );
	wids->b1L = wgpbut( bbRow21, "<<" );
	wids->b1R = wgpbut(bbRow21, ">>");
	swgwth( 20 );
	wids->saveTimes = wgpbut( bbRow21, "Save selection..." );
	
	swgwth( 100 );
	sprintf( wdata->message, "TGF db: %s", TGFDB );
	int bbRow31 = wgbas( wids->base, "HORI" );
	wids->message = wglab( bbRow31, wdata->message );
	
	swgcbk( wids->b0L, guiWidgets::b_move );
	swgcbk( wids->b0R, guiWidgets::b_move );
	swgcbk( wids->b1L, guiWidgets::b_move );
	swgcbk( wids->b1R, guiWidgets::b_move );
	
	swgcbk( wids->saveTimes, write_time_bounds );
	
	//guiWidgets::b_move( -1 );
	
	wgfin();
	
	return 0;
}