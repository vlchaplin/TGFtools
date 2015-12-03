/*
 *  set_tgf_bounds.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 6/3/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

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
#include <iomanip>

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

#ifndef DEFAULT_LC_PARAMS 
#define DEFAULT_LC_PARAMS { -0.016, 0.016, 0.0001 }
#endif

#define YAXIS_ENERGY_MIN_KEV 200.0
#define YAXIS_ENERGY_MAX_KEV 50000.0

#define MIN_PHA_CHAN 0
#define MAX_PHA_CHAN 127


using namespace std;


#include "dislin.h"
//using namespace dis;


const char progname[] = "make_data_plot";

const char multixVals[] = "1x|5x|20x|100x|1000x";

const float energy_range[] = { YAXIS_ENERGY_MIN_KEV, YAXIS_ENERGY_MAX_KEV };

struct graphics_opts {
	float rgb[3];
	char polcrv[15];
	
	int marker;
	int hsymbl;
	
	void scatter_defaults() {
		
		hsymbl = 15;
		marker = 2;
		
		rgb[0] = 0.0;
		rgb[1] = 0.0;
		rgb[2] = 0.0;
		
	};
	 
};
struct tte_set {

	float * times;
	float * energies;
	float tzero;
	std::string det;
	long nevents;
	
	graphics_opts opts;
	
	tte_set() {
		times=NULL;
		energies=NULL;
		tzero=0;
		opts.scatter_defaults();
	};
	~tte_set() {
		this->free();
	}
	
	void free() {
		if (times!=NULL) delete times;
		if (energies!=NULL) delete energies;
	}
	
};



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

inline void det_tte_plots(std::vector<tte_set*>& detectors_tte) {
	
	incmrk(-1);
	
	size_t d,nd;
	tte_set * detSet;
	nd = detectors_tte.size();
	for (d=0;d<nd;d++) {
		
		detSet = detectors_tte[d];
		
		hsymbl(detSet->opts.hsymbl);
		marker(detSet->opts.marker);
		
		setrgb( detSet->opts.rgb[0], 
				detSet->opts.rgb[1], 
				detSet->opts.rgb[2] );
		
		
		curve( detSet->times, detSet->energies, detSet->nevents );
		
	}
	
	
};


void update_table();
void adjust_apply( int bid );
void adjust_lightcurve_ev( int tbl, int row, int col );
void adjust_lightcurve_base(int bid);
void write_time_bounds(int bid);

float yrescale( float& original_y, float * o_yrange, float * ax_range ) 
{
	return 0.99*(ax_range[1] - ax_range[0]) * log10(original_y - o_yrange[0]) / log10(o_yrange[1] - o_yrange[0]) + ax_range[0] + 0.01*(ax_range[1] - ax_range[0]);
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
	float * scatterX;
	float * scatterY;
	
	std::vector<tte_set*> detectors_tte;
	
	float binSize;
	long nbins;
	int chRange[2];
	
	float axY[2];
	float axX[2];
	float dataWin[2];
	
	char text0[20];
	char text1[20];
	char message[105];
	
	std::vector< StrokeDb::result_set > * strokes;
	
	TGFdb::result_set currentTgf;
	
	std::vector< std::string > tgfIds;
	std::vector< GbmDataDb::result_set > activeFiles;
	
	int currentTgfItem;
	
	string tgfdbfile;
	
	string lastTIdir;
	
	guiData() {
		ndraws=0;
		stride=1;
		histogram=NULL;
		xBins = NULL;
		nbins = 0;
		
		double lcLu[] = DEFAULT_LC_PARAMS;
		dataWin[0] = lcLu[0];
		dataWin[1] = lcLu[1];
		binSize = lcLu[2];
		
		chRange[0] = MIN_PHA_CHAN;
		chRange[1] = MAX_PHA_CHAN;
	};
	
	~guiData() {
		
		//delete tgfdbptr;
		if ( histogram != NULL) delete [] histogram;
		if ( xBins != NULL) delete [] xBins;
		
	};
	
	inline int haveTGFLookup() {
		return this->currentTgf.colmask & ( TGFdb::tstartmask | TGFdb::tstopmask );
	};
	
	int queryTTEFiles(bool clear=1) {
		
		GbmDataDb data_db( string(TTEARCH) );
		
		if ( clear ) activeFiles.clear();
		
		data_db.table = "tte";
		int rc = data_db.getCoveringData( this->dataWin[0] + this->tbase,  this->dataWin[1] + this->tbase, activeFiles );
		
		if ( data_db.close_db() == SQLITE_BUSY ) cout << " Error closing TTE database " << endl;
		
		return rc;
	};
	
	void clearHistogram() {
		
		if ( this->histogram != NULL ) {
			delete this->histogram;
			this->histogram = NULL;
		}
		if ( this->xBins != NULL ) {
			delete this->xBins;
			this->xBins = NULL;
		}

		this->nbins = 0;
		
		size_t d;
		size_t ndets = this->detectors_tte.size();
		for(d=0;d<ndets;d++) {
			if ( detectors_tte[d] != NULL ) delete detectors_tte[d];
		}
		
		detectors_tte.clear();
		
	};
	
	void buildHistogram( ) {
	
		//disApp * theApp = disApp::Instance();
		//guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
		
		if ( this->activeFiles.size() == 0 ) return;
	
		TTEReader ttereader;
		TTEventTable<double,int,float> * gbm_tte_data = NULL;
		TTEventTable<double,int,float>::fields_class * keyFields;
		
		std::vector< GbmDataDb::result_set >::iterator dItr;

		dItr = this->activeFiles.begin();
		
		//if ( dItr == this->activeFiles.end() ) {
			//cout << "No TTE Available" << endl;
		//	return;
		//}
		
		this->clearHistogram();
		
		float ymin=99999999.9;
		float ymax=0;
		
		int status;
		long firstRow, lastRow;
		double tstart, tstop;
		tstart= this->dataWin[0] + this->tbase;
		tstop = this->dataWin[1] + this->tbase;
		while ( dItr != this->activeFiles.end() ) 
		{

			ttereader.setFile( dItr->path + path_sep() + dItr->file );
			ttereader.open();
			status = ttereader.findRowRange( tstart, tstop, firstRow, lastRow, 1 );
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
			
			if ( this->histogram == NULL )
				gbm_tte_data->buildHistogram( tstart, tstop, this->binSize, this->chRange[0], this->chRange[1], &(this->histogram), &(this->nbins), &(this->xBins), this->tbase );
			else 
				gbm_tte_data->addToHistogram( this->xBins, this->histogram, this->nbins, this->chRange[0], this->chRange[1], this->tbase );
			
			
			if ( 1 ) {
		
				long nEvents = gbm_tte_data->length();
				tte_set * newdet = new tte_set;
				newdet->times = new float[nEvents];
				newdet->energies = new float[nEvents];
				newdet->nevents=nEvents;
				newdet->det = dItr->det;
				newdet->tzero = this->tbase;
				
				gbm_tte_data->CopyEvents( newdet->times, newdet->energies, this->tbase );
				
				cout << "TTE " <<  dItr->det << " ["<<newdet->nevents<<"] " << endl;
				
				
				if ( newdet->det == "BGO_00" ) {
					newdet->opts.rgb[0] = 1.0;
					newdet->opts.rgb[1] = 0.0;
					newdet->opts.rgb[2] = 0.0;
				} else if ( newdet->det == "BGO_01" ) {
					newdet->opts.rgb[0] = 0.0;
					newdet->opts.rgb[1] = 0.0;
					newdet->opts.rgb[2] = 1.0;
				}
				
				
				detectors_tte.push_back(newdet);
				
			}
			
			std::cout << dItr->file << ", Nbins: " << this->nbins << endl;

			dItr++;
			delete gbm_tte_data;
			
			//dItr = data.end();
			
			//std::cout << "here" << endl;
					
		}
		
	//	stroke_db.getSpaceTimeEvents( nearbyStrokes, plotCenterMET, 
	//				0,0, 
	//				tteMargin,
	//				EARTH_AVG_RAD*2*geom::PI );
		
				
		for ( int j=0; j< nbins; j++ ) {
			if ( this->histogram[j] > ymax ) ymax = this->histogram[j];
			if ( this->histogram[j] < ymin ) ymin = this->histogram[j];
		}
		
		if ( ymin > 75 ) { ymin = ymin - exp( -float(ymax)/ymin )*( ymax - ymin ); } else ymin = 0;
		if ( ymax > 100 ) {
			ymax = ymax + exp( -(ymax-ymin)/(ymax+ymin) )*( ymax - ymin );
		} else {
			ymax += 5;
		}
		
		std::cout << "Done" << endl << endl;
		
	//	this->histogram = thist;
	//	wdata->xBins = edges;
	//	wdata->nbins = nbins;
	//	wdata->binSize = binSize;
	//	wdata->tbase = plotCenterMET;
		this->axX[0] = this->xBins[0];
		this->axX[1] = this->xBins[nbins];
		this->axY[0] = floor(ymin);
		this->axY[1] = ceil(ymax);
		this->edge0 = 1;
		this->edge1 = this->nbins-1;
		
		size_t ndets,d;
		long nEv,n;
		
		for (d=0;d<detectors_tte.size();d++) {
			cout << "Scaling " <<  detectors_tte[d]->det << endl;
			nEv = detectors_tte[d]->nevents;
			for( n=0; n<nEv; n++ ) {
				if ( (detectors_tte[d])->energies[n] <= energy_range[0] ) (detectors_tte[d])->energies[n] = energy_range[0] + 1;
				
				
			//	(detectors_tte[d])->energies[n] = 0.99*(this->axY[1] - this->axY[0]) * log10( (detectors_tte[d])->energies[n] - energy_range[0]) / log10(energy_range[1] - energy_range[0]) + this->axY[0];
				
				(detectors_tte[d])->energies[n] = yrescale( (detectors_tte[d])->energies[n], (float*)energy_range, this->axY );
				
			}
		}
		

	};
	
	
	
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
	int eventSelect;
	int timeSelect;
	int xxDrop;
	int saveTimes;
	int message;
	
	int histTableAdj;
	
	guiWidgets() {
		histTableAdj = -1;
	}
	
	static void select_tgf(int sbuttn) {
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
		
		char tempMsg[120];
		int sel, stat;
		
		TGFdb tgfdb(data->tgfdbfile);
		
		std::vector< TGFdb::result_set > container;
		
		std::string longlist;
		longlist = str_join( data->tgfIds, "|" );
		cout << "here" << __LINE__ << endl;
		
		sel = dwglis( "Get TTE for TGF:",  longlist.c_str(), data->currentTgfItem );
		
		stat = dwgerr();
	
		if ( stat != 0 ) return;
		cout << "here" << __LINE__ << endl;
		std::string tgfsel = data->tgfIds[sel-1];
		
		tgfdb.getLcParams = 1;
		tgfdb.getEvents( container , " tgfid = '"+tgfsel+"' " );
		
		if ( tgfdb.close_db() == SQLITE_BUSY ) cout << " Error closing database " << endl;
		
		if ( container.size() == 0 ) {
		
			sprintf( tempMsg, "Database error: TGF '%s' not found. Perhaps another process is changing the database.", tgfsel.c_str() );
		
			wids->UpdateMessageBar( tempMsg );
		
			//std::cout << "hmm... tgfid = '"+tgfsel+"' doesn't return any records " << endl;
			
			
			return ;
		}
		
		
		double tempTime = data->tbase;
		data->tbase = container[0].met;
		
		std::vector<string> tempFiles;
	//	for ( size_t i=0; i < data->activeFiles.size(); i++ ) tempFiles[i] = data->activeFiles[i];
		
		data->queryTTEFiles(1);
		
		std::cout << data->activeFiles.size() << " TTE files " << endl;
		
		if ( data->activeFiles.size() == 0 ) {
		
		//	for ( size_t i=0; i < tempFiles.size(); i++ ) data->activeFiles[i] = tempFiles[i];
		
			sprintf( tempMsg, "No TTE for TGF '%s', (%s).", container[0].tgfid.c_str(), container[0].utc.c_str()  );
			wids->UpdateMessageBar( tempMsg );
		
			data->tbase = tempTime;
			
			if ( container[0].lcview != NULL ) delete container[0].lcview;
			
			return;
		}
		else if ( container[0].lcview != NULL ) {
			
			data->dataWin[0] = container[0].lcview->xs;
			data->dataWin[1] = container[0].lcview->xe;
			data->binSize = container[0].lcview->dx;
			
			delete container[0].lcview;
		}
		else if ( container[0].colmask & ( TGFdb::tstartmask | TGFdb::tstopmask ) ) {
		
			double integration = container[0].tstop - container[0].tstart;
		
			data->dataWin[0] = ( container[0].tstart - container[0].met) - 5*integration;
			data->dataWin[1] = ( container[0].tstop - container[0].met ) + 5*integration;
		} 
		else {
			
			double lcLu[] = DEFAULT_LC_PARAMS;
			
			data->dataWin[0] = lcLu[0];
			data->dataWin[1] = lcLu[1];
			data->binSize = lcLu[2];
		}
	
		sprintf( tempMsg, "Loaded TTE for TGF '%s'.   '%s'  + %0.6f s ", 
			container[0].tgfid.c_str(), 
			container[0].utc.c_str(),
			container[0].fsec  );
		cout << "here" << __LINE__ << endl;
		wids->UpdateMessageBar( tempMsg );
		
		data->currentTgf = container[0];
		data->currentTgfItem = sel;
		
		data->buildHistogram();
		update_table();
		
		b_move(-1);
		
		
	};
	
	static void b_move(int bid) {
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
		
		if ( data->nbins == 0 ) return;
		
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
		
	void UpdateLabel() {
	
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		
		sprintf( data->text0, "Tmin : %0.6f s", data->time0 );
		sprintf( data->text1, "Tmax : %0.6f s", data->time1 );
		
		swgtxt( this->label0, data->text0 );
		swgtxt( this->label1, data->text1 );
		
	};
	void UpdateMessageBar(char * msg) {
	
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		
		strcpy( data->message, msg);
		swgtxt( this->message, data->message);
	};
	
	//Unfortunately I can't figure out how to use dislin's backing 
	//store/PIXMAP routines so we have to redraw the entire scene every time.
	void DrawPlot() {
		
		disApp * theApp = disApp::Instance();
		guiData * data = (guiData *)theApp->getAppData();
		
		if ( data->nbins == 0 ) return;
		
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
		
		polcrv( (char*)"STAIRS" );
		
		histogramPlot( data->xBins, data->histogram, data->nbins );
		det_tte_plots( data->detectors_tte );
		
		incmrk(0);
		marker(0);
		
		float en511keV = 511.0;
		en511keV = yrescale( en511keV, (float*)energy_range, data->axY );
		float line511[] = { en511keV, en511keV };
		setrgb(0.75,0.75,0.75);
		curve( data->axX, line511, 2 );
		
		
		setrgb(1,0,0);
		
		float lwb[] = { data->time0, data->time0 };
		float upb[] = { data->time1, data->time1 };
		
		curve( lwb, data->axY, 2 );
		curve( upb, data->axY, 2 );
		
		this->UpdateLabel();
		
		
		if ( data->haveTGFLookup() ) {
		
			//cout << "have lookup" << endl;
			dot();
			setrgb(0.75,0,0);
		
			float lwInt[] = { data->currentTgf.tstart - data->tbase, data->currentTgf.tstart - data->tbase};
			float upInt[] = { data->currentTgf.tstop - data->tbase, data->currentTgf.tstop - data->tbase};	
			
			curve( lwInt, data->axY, 2 );
			curve( upInt, data->axY, 2 );
			
		} 
		
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

void update_table() {
	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
	
	if ( wids->histTableAdj == -1 ) return;
	
	int nBefore = floor( data->dataWin[0] / data->binSize );
	int nAfter = floor( data->dataWin[1] / data->binSize );
	swgtbf( wids->histTableAdj, data->binSize, 6, 1, 1, (char*)"VALUE" );
	swgtbi( wids->histTableAdj, nBefore, 2, 1, (char*)"VALUE" );
	swgtbi( wids->histTableAdj, nAfter , 3, 1, (char*)"VALUE" );
	swgtbf( wids->histTableAdj, data->binSize*nBefore, 6 , 4, 1, (char*)"VALUE" );
	swgtbf( wids->histTableAdj, data->binSize*nAfter, 6 , 5, 1, (char*)"VALUE" );
	
}

void adjust_apply( int bid )
{
	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
	
	if ( wids->histTableAdj == -1 ) return;

	data->binSize = gwgtbf( wids->histTableAdj, 1,1 );
	data->dataWin[0] = data->binSize*gwgtbf( wids->histTableAdj, 2,1 );
	data->dataWin[1] = data->binSize*gwgtbf( wids->histTableAdj, 3,1 );
	
	data->chRange[0] = gwgtbf( wids->histTableAdj, 6, 1 );
	data->chRange[1] = gwgtbf( wids->histTableAdj, 7, 1 );

	data->buildHistogram();
		
	guiWidgets::b_move(-1);
};

void adjust_lightcurve_ev( int tbl, int row, int col ) {
	
	if ( col != 1 ) return;
	
	float bSz = gwgtbf( tbl, 1,1 );

	if ( row == 1 ) {
	
		if ( bSz <= 1e-07 ) return;
	
		float tmin = gwgtbf( tbl, 4, 1 );
		float tmax = gwgtbf( tbl, 5, 1 );
		
		int nB = floor( tmin / bSz );
		int nA = floor( tmax / bSz );
	
		swgtbi( tbl, nB, 2, 1, (char*)"VALUE" );
		swgtbi( tbl, nA, 3, 1, (char*)"VALUE" );
		swgtbf( tbl, nB*bSz, 6, 4, 1, (char*)"VALUE" );
		swgtbf( tbl, nA*bSz, 6, 5, 1, (char*)"VALUE" );
	}
	else if ( row == 2 ) {
		swgtbf( tbl, bSz*gwgtbi( tbl, row, col ), 6, 4, 1, (char*)"VALUE" );
	}
	else if ( row == 3 ) {
		swgtbf( tbl, bSz*gwgtbi( tbl, row, col ), 6, 5, 1, (char*)"VALUE" );
	}
	else if ( row == 6 || row == 7 ) {
		int chval = gwgtbf( tbl, row, 1 );
		if ( chval < MIN_PHA_CHAN ) { 
			chval = 0; 
			swgtbi( tbl, chval, row, 1, (char*)"VALUE" );
		} else if ( chval > MAX_PHA_CHAN ) { 
			chval = 0; 
			swgtbi( tbl, chval, row, 1, (char*)"VALUE" );
		}
	}

}

void adjust_lightcurve_base(int bid) {

	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();

	//float values[] = { data->dataWin[0], data->dataWin[1], data->binSize
	
	if ( wids->histTableAdj != -1 ) {
		//swgfoc( wids->histTableAdj );
		return;
	}
	
	swgwth( -20 );
	swgclr( 1.0, 1.0, 1.0, "BACK");
	int diabase = wgini( "VERT" );

	//char * hdr[] = { "Bin size[s]", "Bins before", "Bins after" };

	int nBefore = floor( data->dataWin[0] / data->binSize );
	int nAfter = floor( data->dataWin[1] / data->binSize );
	
	
	float hdrWdth = 15;
	int hdrClr = intrgb( 0.0,0.5,0.5);

	swghlp( (char*)"Input histogram parameters. Trange = Binsize x (N.Before , N.After) " ); 

	swgopt( (char*)"ROWS", (char*)"HEADER" );
	swgpop( (char*)"NOQUIT" );
	swgray( &hdrWdth, 1, (char*)"TABLE" );
	int tbl = wgtbl( diabase, 7, 1 );
	swgtbi( tbl, hdrClr, -1, 1, (char*)"FORE" );
	//swgtbi( tbl, hdrClr, 2, 1, "FORE" );
	//swgtbi( tbl, hdrClr, 3, 1, "FORE" );
	swgtbs( tbl, (char*)"Bin size[s]", 1, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"Bins before", 2, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"Bins after", 3, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"T-Min [s] = ", 4, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"T-Max [s] = ", 5, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"Min Chan. = ", 6, 0, (char*)"VALUE" );
	swgtbs( tbl, (char*)"Max Chan. = ", 7, 0, (char*)"VALUE" );
	
	swgtbs( tbl, (char*)"ON", -1, 1, (char*)"EDIT" );
	swgtbs( tbl, (char*)"FLOAT", 1, 1, (char*)"VERIFY" );
	swgtbs( tbl, (char*)"INTEGER", 2, 1, (char*)"VERIFY" );
	swgtbs( tbl, (char*)"INTEGER", 3, 1, (char*)"VERIFY" );
	swgtbi( tbl, intrgb( 0.6, 0.6, 0.6), 4, 1, (char*)"FORE" );
	swgtbi( tbl, intrgb( 0.6, 0.6, 0.6), 5, 1, (char*)"FORE" );
	swgtbs( tbl, (char*)"OFF", 4, 1, (char*)"EDIT" );
	swgtbs( tbl, (char*)"OFF", 5, 1, (char*)"EDIT" );
	swgtbs( tbl, (char*)"INTEGER", 6, 1, (char*)"VERIFY" );
	swgtbs( tbl, (char*)"INTEGER", 7, 1, (char*)"VERIFY" );
	
	swgtbf( tbl, data->binSize, 6, 1, 1, (char*)"VALUE" );
	swgtbi( tbl, nBefore, 2, 1, (char*)"VALUE" );
	swgtbi( tbl, nAfter , 3, 1, (char*)"VALUE" );
	swgtbf( tbl, data->binSize*nBefore, 6 , 4, 1, (char*)"VALUE" );
	swgtbf( tbl, data->binSize*nAfter, 6 , 5, 1, (char*)"VALUE" );
	swgtbi( tbl, data->chRange[0], 6, 1, (char*)"VALUE" );
	swgtbi( tbl, data->chRange[1], 7, 1, (char*)"VALUE" );
	
	int applyB = wgpbut( diabase, (char*)"Apply" );
	//swgtbl( tbl, &data->binSize, 1, 6, 2, "COLUMN" );

	swgcbk( applyB, adjust_apply );
	swgcb2( tbl, adjust_lightcurve_ev );

	wids->histTableAdj = tbl;

	wgfin();
	
	wids->histTableAdj = -1;
	
}


void write_time_bounds(int bid) {

	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();
	
	
	//std::cout << "here at save start" << endl;

	if ( data->nbins == 0 ) {
		std::cout << "No TGF" << endl;
		return;
	}
	sqlite3_stmt * ppStmt;
	//sqlite3 * ptr = data->tgfdbptr->get_db_ptr();
	
	TGFdb tgfdb(data->tgfdbfile);
	
	char lcparm_assign[40];
	char tempMsg[100];
	char tempId[25];
	sprintf( lcparm_assign, "lcXs=%0.6f, lcXe=%0.6f, lcDx=%0.6f", data->dataWin[0], data->dataWin[1], data->binSize );
	
	string qry = " UPDATE or REPLACE "+tgfdb.table+" SET tstart=?, tstop=?, "+string(lcparm_assign)+" WHERE tgfid = '"+data->currentTgf.tgfid+"' ";
	
	//std::cout << qry << endl;
	
	
	sqlite3_prepare_v2( tgfdb.get_db_ptr(), qry.c_str(), qry.size()+1, &ppStmt, NULL );
	sqlite3_bind_double(ppStmt, 1, data->tbase + data->time0 );
	sqlite3_bind_double(ppStmt, 2, data->tbase + data->time1 );
//	sqlite3_bind_double(ppStmt, 3, (double) data->dataWin[0] );
//	sqlite3_bind_double(ppStmt, 4, (double) data->dataWin[1] );
//	sqlite3_bind_double(ppStmt, 5, (double) data->binSize );
	int rc = sqlite3_step(ppStmt);
	sqlite3_finalize( ppStmt );
	
	if ( tgfdb.close_db() == SQLITE_BUSY ) cout << " Error closing database " << endl;
	
	if ( rc == SQLITE_DONE ) {
	
		strcpy( tempId, data->currentTgf.tgfid.c_str() );
	
		sprintf( tempMsg, "Saved integration time for TGF '%s':   %f   to   %f  ,  dt = %0.3f ms", 
								tempId, 
								data->tbase + data->time0, data->tbase + data->time1,
								1000.0*(data->time1 - data->time0) );
		wids->UpdateMessageBar( tempMsg );
	}
	
	std::cout << rc << endl;

};

void write_ti_file(int bid) {

	disApp * theApp = disApp::Instance();
	guiData * data = (guiData *)theApp->getAppData();
	guiWidgets * wids = (guiWidgets *)theApp->getAppWidgets();

	if ( data->nbins == 0 ) {
		std::cout << "No TGF" << endl;
		return;
	}
	
	std::ofstream output;
	char tiPath[200];
	strcpy( tiPath, data->lastTIdir.c_str() );
	
	chdir( tiPath );
	
	char * pathSelection = dwgfil( "Save TI files in directory:", tiPath, "*" );
	
	if ( dwgerr() != 0 ) {
		free( pathSelection );
		return;
	}
	
	char * base = get_basename( pathSelection, tiPath );
		
	chdir( tiPath );

	data->lastTIdir= string( tiPath );
	std::vector< GbmDataDb::result_set >::iterator dItr;

	dItr = data->activeFiles.begin();

	string outfile;

	while ( dItr != data->activeFiles.end() ) {
		
		outfile = string(tiPath) + path_sep() + TGFTLS_OUTFILE_CANON_NAME( string("ttx"), data->currentTgf.tgfid, gbmDetShortname((char*)dItr->det.c_str()), string(".ti") );
		
		cout << "Writing lookup:" << outfile << endl;
		
		output.open( (char*)outfile.c_str() );
		output << std::setw(12) << data->nbins << endl;
		output << fixed;
		for (int k=0; k < data->nbins; k++)
			output << std::setw(16) << std::setprecision(6) << data->xBins[k] << endl;
		
		output.close();
		dItr++;
	}

	cout << "Done" << endl << endl;

	free( pathSelection );

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
		
	
	guiWidgets * wids = new guiWidgets;
	guiData * wdata = new guiData;
	TGFdb tgfdb( string(TGFDB) );
	
	//std::vector< std::string > * tgfIds = new std::vector< std::string >;
	
	GenericSQL sql;
	//sql.set_db_ptr( tgfdb.get_db_ptr() );
	//sql.set_table ( tgfdb.table );
	//sql.get_column( "tgfid", wdata->tgfIds , " order by met asc" );
	
	
	AssociationDb assc_db;
	assc_db.set_db_ptr( tgfdb.get_db_ptr() );
	
	sql.set_db_ptr( assc_db.get_db_ptr() );
	sql.set_table ( assc_db.table );
	sql.get_column( "tgfid", wdata->tgfIds , " order by tmet asc" );
	
	if ( tgfdb.close_db() == SQLITE_BUSY ) cout << " Error closing database " << endl;
	
	if ( wdata->tgfIds.size() == 0 ) {
		cout << tgfdb.table << " in " << TGFDB << " is empty" << endl;
		cout << "No TGFs to make selections for" << endl;
		return 0;
	}
	
	//string longlist = str_join( wdata->tgfIds , "|" );

	
	disApp * theApp = disApp::Instance();
	
	theApp->setAppData( (void*) wdata );
	theApp->setAppWidgets( (void*) wids );
	
	wdata->tgfdbfile = string(TGFDB);
	wdata->currentTgfItem = 1;
	
	swgwth( -45 );
	swgdrw( 0.706 );
	swgmrg( 12, "TOP");
	wids->base = wgini( "VERT" );
	int dwbase = wgbas( wids->base, "VERT" );
	swgclr( 1.0, 1.0, 1.0, "BACK");
	wids->draw = wgdraw( dwbase );

	swgjus("CENTER", "LABEL");
	
	swgwth( 20 );
	
	int bbRow11 = wgbas( wids->base, "HORI" );
	wids->label0 = wglab( bbRow11, " - " );
	wids->label1 = wglab( bbRow11, " - " );
	
	swgwth( 17 );
	wids->eventSelect = wgpbut( bbRow11, " Select a TGF... ");
	wids->timeSelect = wgpbut( bbRow11, " Plot time... ");
	int adjust = wgpbut( bbRow11, " Adjust curve ");
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
	
	int writeTI = wgpbut( bbRow21, "Write TI files..." );
	
	swgwth( 100 );
	sprintf( wdata->message, "TGF db: %s", TGFDB );
	int bbRow31 = wgbas( wids->base, "HORI" );
	wids->message = wglab( bbRow31, wdata->message );
	
	swgcbk( wids->eventSelect, guiWidgets::select_tgf );
	
	swgcbk( wids->b0L, guiWidgets::b_move );
	swgcbk( wids->b0R, guiWidgets::b_move );
	swgcbk( wids->b1L, guiWidgets::b_move );
	swgcbk( wids->b1R, guiWidgets::b_move );
	
	swgcbk( wids->saveTimes, write_time_bounds );
	swgcbk( writeTI, write_ti_file );
	swgcbk( adjust, adjust_lightcurve_base );
	
	//guiWidgets::b_move( -1 );


	wgfin();
	
	return 0;
}
