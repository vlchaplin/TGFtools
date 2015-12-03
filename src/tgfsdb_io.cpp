/*
 *  tgfsdb_io.cpp
 *  gbmtgf_rdx
 *
 *  Created by Vandiver L. Chapin on 3/14/11.
 *  Copyright 2011 The University of Alabama in Huntsville. All rights reserved.
 *
 */

#include "tgfsdb_io.h"

using namespace std;


const int TGFdb::tstartcol = 4;
const int TGFdb::tstopcol = 5;
const int TGFdb::XScol = 6;
const int TGFdb::XEcol = 7;
const int TGFdb::DXcol = 8;
const int TGFdb::tstartmask = 16;
const int TGFdb::tstopmask = 32;

static char * ascttype[] = {	
						(char *)"SRC",    (char *)"TIME",  (char *)"OFFSET",  (char *)"LON",      (char *)"LAT",      (char *)"RA",  (char *)"DEC", 
						(char *)"SC_AZ",  (char *)"SC_EL", (char *)"SC_DIST", (char *)"BEAM_OFF", (char *)"OPEN_ANG", (char *)"GRND_OFF",
						(char *)"GEO_AZ", (char *)"GEO_EL",(char *)"ALT" 
						};
						
static char * asctunit[] = {	
						(char *)"",       (char *)"UTC",   (char *)"s",       (char *)"deg",      (char *)"deg",      (char *)"deg", (char *)"deg", 
						(char *)"deg",    (char *)"deg",   (char *)"km",      (char *)"km",       (char *)"deg",      (char *)"km",      
						(char *)"deg",    (char *)"deg",   (char *)"km"  
						};
						
static char * asctform[] = {	
						(char *)"12A",    (char *)"20A",   (char *)"1D",      (char *)"1E",       (char *)"1E",       (char *)"1E",  (char *)"1E",  
						(char *)"1E",     (char *)"1E",    (char *)"1E",      (char *)"1E",       (char *)"1E",       (char *)"1E",      
						(char *)"1E",     (char *)"1E",    (char *)"1E"  
						};

int AddTGFStroke_fitsTable( string file, std::vector<TransfSrcCrd>& events ) 
{
	fitsfile * fptr;
	int status=0;
	if ( ffopen( &fptr, (char*)file.c_str(), READWRITE, &status ) != 0 ) {
		ffrprt( stdout, status);
		return status;
	}

	char * ttype[] = { (char *)"SRC", (char *)"TIME", (char *)"MET", (char *)"LON", (char *)"LAT", (char *)"RA",  (char *)"DEC", (char *)"SC_AZ", (char *)"SC_EL", (char *)"SC_DIST", (char *)"GEO_AZ", (char *)"GEO_EL", (char *)"ALT" };
	char * tunit[] = { (char *)"",    (char *)"UTC",  (char *)"s",   (char *)"deg", (char *)"deg", (char *)"deg", (char *)"deg", (char *)"deg",   (char *)"deg",   (char *)"km",      (char *)"deg",    (char *)"deg",   (char *)"km"};
	char * tform[] = { (char *)"12A", (char *)"20A",  (char *)"1D",  (char *)"1E",  (char *)"1E",  (char *)"1E",  (char *)"1E",  (char *)"1E",    (char *)"1E",    (char *)"1E",      (char *)"1E",     (char *)"1E",    (char *)"1E"};
	
	
	ffcrtb( fptr, BINARY_TBL, 0, 13, ttype, tform, tunit, (char*)"LIGHTNING", &status );
	
	if (status != 0) {
		ffrprt( stdout, status);
		ffclos(fptr, &status );
		return -1;
	} 
		
	long row=1;
	double stroke_met;
	double v, ra, dec, dist, az, el, alt, gaz, gel;
	
	std::vector<TransfSrcCrd>::iterator t = events.begin();
	
	ffuky( fptr, TSTRING, (char*)"TGFID", (char*)t->gbmTgfEntry.tgfid.c_str(), (char*)"Primary key to TGFdb", &status );
	ffuky( fptr, TDOUBLE, (char*)"TGFTIME", &t->gbmTgfEntry.met, (char*)"MET of TGF", &status );
	
	while (t != events.end() ) 
	{
	
		//strcpy( utc, t->strokeEntry.utc.c_str() );
		//strcpy( src, t->strokeEntry.locsrc.c_str() );
		
		char * utc = (char*)t->strokeEntry.utc.c_str();
		char * src = (char*)t->strokeEntry.locsrc.c_str();
		stroke_met = glast_utc_to_met( utc );
		stroke_met+= t->strokeEntry.fsec;
	
		ffpcl(fptr, TSTRING, 1, row, 1, 1, &src, &status);
		ffpcl(fptr, TSTRING, 2, row, 1, 1, &utc, &status);
		ffpcl(fptr, TDOUBLE, 3, row, 1, 1, &(stroke_met), &status);
		ffpcl(fptr, TDOUBLE, 4, row, 1, 1, &(t->strokeEntry.lon), &status);
		ffpcl(fptr, TDOUBLE, 5, row, 1, 1, &(t->strokeEntry.lat), &status);
	
		geom::cart2sphere( t->stroke_EciCoords, v, ra, dec, 1 );
		geom::cart2sphere( t->stroke_ScCoords, dist, az, el, 1 );
		geom::cart2sphere( t->geocen_ScCoords, alt, gaz, gel, 1 );

		ffpcl(fptr, TDOUBLE, 6, row, 1, 1, &ra, &status);
		ffpcl(fptr, TDOUBLE, 7, row, 1, 1, &dec, &status);
		ffpcl(fptr, TDOUBLE, 8, row, 1, 1, &az, &status);
		ffpcl(fptr, TDOUBLE, 9, row, 1, 1, &el, &status);
		ffpcl(fptr, TDOUBLE,10, row, 1, 1, &dist, &status);
		ffpcl(fptr, TDOUBLE,11, row, 1, 1, &gaz, &status);
		ffpcl(fptr, TDOUBLE,12, row, 1, 1, &gel, &status);
		ffpcl(fptr, TDOUBLE,13, row, 1, 1, &alt, &status);
		row++;
		t++;
	}
	
	if (status != 0) {
		ffrprt( stdout, status);
		ffclos( fptr, &status );
		return -1;
	} 
	
	ffpdat(fptr, &status);
	ffpcks(fptr, &status);
	
	ffclos(fptr, &status );
	
	
	cout << "Wrote LIGHTNING extension to "+file << endl << endl;
	
	return 0;
}

int AddTGFStroke_fitsTable( string file, std::vector<AssociationDb::result_set>& events ) 
{
	fitsfile * fptr;
	int status=0;
	if ( ffopen( &fptr, (char*)file.c_str(), READWRITE, &status ) != 0 ) {
		ffrprt( stdout, status);
		return status;
	}

		
	ffcrtb( fptr, BINARY_TBL, 0, 16, ascttype, asctform, asctunit, ASSOC_FITS_TABLE, &status );
	
	if (status != 0) {
		ffrprt( stdout, status);
		ffclos(fptr, &status );
		return -1;
	} 
		
	long row=1;
	double stroke_met;
	double alt, gaz, gel;
	
	char keyBuff[12];
	
	std::vector<AssociationDb::result_set>::iterator t = events.begin();
	
	ffuky( fptr, TSTRING, (char*)"TGFID", (char*)t->tgf.tgfid.c_str(), (char*)"Primary key to TGFdb", &status );
	ffuky( fptr, TDOUBLE, (char*)"TGFTIME", &t->tgf.met, (char*)"MET of TGF", &status );

	while (t != events.end() ) 
	{
	
		//strcpy( utc, t->strokeEntry.utc.c_str() );
		//strcpy( src, t->strokeEntry.locsrc.c_str() );
		
		char * utc = (char*)t->stroke.utc.c_str();
		char * src = (char*)t->stroke.locsrc.c_str();
		stroke_met = glast_utc_to_met( utc );
		stroke_met+= t->stroke.fsec;
		
		stroke_met-=t->tgf.met;
	
		x3 nadirVector = t->xEv*(-1);
		t->qEv.rot( nadirVector );
		geom::cart2sphere( nadirVector, alt, gaz, gel, 1 );
		alt -= (EARTH_AVG_RAD/1000.0);
	
		ffpcl(fptr, TSTRING, 1, row, 1, 1, &src, &status);
		ffpcl(fptr, TSTRING, 2, row, 1, 1, &utc, &status);
		ffpcl(fptr, TDOUBLE, 3, row, 1, 1, &(stroke_met), &status);
		ffpcl(fptr, TDOUBLE, 4, row, 1, 1, &(t->stroke.lon), &status);
		ffpcl(fptr, TDOUBLE, 5, row, 1, 1, &(t->stroke.lat), &status);

		ffpcl(fptr, TDOUBLE, 6, row, 1, 1, &(t->ra), &status);
		ffpcl(fptr, TDOUBLE, 7, row, 1, 1, &(t->dec), &status);
		ffpcl(fptr, TDOUBLE, 8, row, 1, 1, &(t->az), &status);
		ffpcl(fptr, TDOUBLE, 9, row, 1, 1, &(t->el), &status);
		ffpcl(fptr, TDOUBLE,10, row, 1, 1, &(t->dxSrc), &status);
		ffpcl(fptr, TDOUBLE,11, row, 1, 1, &(t->dxBeam), &status);
		ffpcl(fptr, TDOUBLE,12, row, 1, 1, &(t->bmAng), &status);
		ffpcl(fptr, TDOUBLE,13, row, 1, 1, &(t->dxGnd), &status);
		
		ffpcl(fptr, TDOUBLE,14, row, 1, 1, &gaz, &status);
		ffpcl(fptr, TDOUBLE,15, row, 1, 1, &gel, &status);
		ffpcl(fptr, TDOUBLE,16, row, 1, 1, &alt, &status);
		
		sprintf(keyBuff, (char*)"OPENANG%ld", row);
		ffuky( fptr, TDOUBLE, keyBuff, &(t->bmAng), (char*)"[deg] Open angle from beam axis", &status );
		sprintf(keyBuff, (char*)"BEAMOFF%ld", row);
		ffuky( fptr, TDOUBLE, keyBuff, &(t->dxBeam), (char*)"[km] Radial offset from beam axis", &status );
		
		row++;
		t++;
	}
	
	if (status != 0) {
		ffrprt( stdout, status);
		ffpdat( fptr, &status);
		ffpcks( fptr, &status);
		ffclos( fptr, &status );
		return -1;
	} 
	
	ffpdat(fptr, &status);
	ffpcks(fptr, &status);
	
	ffclos(fptr, &status );
	
	cout << "Wrote " << ASSOC_FITS_TABLE << " extension to "+file << endl;
	
	if (ffopen( &fptr, (char*)file.c_str(), READWRITE, &status ) != 0 ) {
		ffrprt( stdout, status);
		return status;
	}
	t = events.begin();
	
	ffuky( fptr, TSTRING, (char*)"TGFID", (char*)t->tgf.tgfid.c_str(), (char*)"Primary key to TGFdb", &status );
	ffuky( fptr, TDOUBLE, (char*)"TGFTIME", &t->tgf.met, (char*)"MET of TGF", &status );
	row=1;
	while (t != events.end() ) 
	{
		sprintf(keyBuff, (char*)"OPENANG%ld", row);
		ffuky( fptr, TDOUBLE, keyBuff, &(t->bmAng), (char*)"[deg] Open angle from beam axis", &status );
		sprintf(keyBuff, (char*)"BEAMOFF%ld", row);
		ffuky( fptr, TDOUBLE, keyBuff, &(t->dxBeam), (char*)"[km] Radial offset from beam axis", &status );
		t++;
	}
	if (status != 0) {
		ffrprt( stdout, status);
		ffpcks( fptr, &status);
		ffclos( fptr, &status );
		return -1;
	} 
	ffpcks(fptr, &status);
	ffclos(fptr, &status );
	
	cout << "Update primary header" << endl;
	
	return 0;
}


GbmDataDb::GbmDataDb() 
{
	printQry=0;
	db = NULL;
	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
}

GbmDataDb::GbmDataDb(string file) 
{ 
	printQry=0;
	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	open_db( file );
}

int GbmDataDb::open_db( string file ) 
{
	int status;
	status = sqlite3_open_v2( file.c_str() , &(this->db), this->sqliteflags, NULL );
	
	if (status != SQLITE_OK) {
	    cout << sqlite3_errmsg( this->db ) << ":" << endl;
	    cout << file.c_str() << endl;
		db = NULL;
		return status;
	} else 
		dbfile = file;


	cout << "Opened Data Db: "<<file << endl;
	
	return status;
}

void GbmDataDb::get_step_data( sqlite3_stmt *& ppStmt, result_set& data)
{	
	data.file = string((char*)sqlite3_column_text( ppStmt, 0) );
	data.path = string((char*)sqlite3_column_text( ppStmt, 1) );
	data.tmin = sqlite3_column_double( ppStmt, 4);
	data.tmax = sqlite3_column_double( ppStmt, 5);
	data.det = string((char*)sqlite3_column_text( ppStmt, 6) );
	data.type = string((char*)sqlite3_column_text( ppStmt, 7) );
	
}

int GbmDataDb::getCoveringData( double metstart, double metstop, vector<result_set>& output, string whr, string postOp ) 
{

	if (whr.length() > 0) whr = "AND ("+whr+")";

	string qry = "SELECT *, ( "+
					string("(?1 >= tmin AND ?2 <= tmax) OR (?1 < tmin AND ?2 > tmin) OR (?1 < tmax AND ?2 > tmax) ) AS ok ")+
					" FROM "+table+" WHERE ok "+whr+" "+postOp;
					
	if (this->printQry) cout << qry << endl << endl;				
	
	sqlite3_stmt * ppStmt;
	int rc;
	
	GbmDataDb::result_set temp;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	sqlite3_bind_double( ppStmt, 1, metstart );
	sqlite3_bind_double( ppStmt, 2, metstop );
	
	rc = sqlite3_step(ppStmt);
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		output.push_back( temp );
		
		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;
}



TGFdb::TGFdb() 
{
	db = NULL;
	char * env = getenv("TGFDB_TABLE");
	if ( env == NULL ) table = "tgflist"; else table = string(env);
	getLcParams = 0;
	printQry=0;
	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
}

TGFdb::TGFdb(string file) 
{ 
	char * env = getenv("TGFDB_TABLE");
	if ( env == NULL ) table = "tgflist"; else table = string(env);

	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	getLcParams = 0;
	printQry=0;
	open_db( file );
}

int TGFdb::open_db( string file ) 
{
	int status;
	status = sqlite3_open_v2( file.c_str() , &(this->db), this->sqliteflags, NULL );
	
	if (status != SQLITE_OK) {
	    cout << sqlite3_errmsg( this->db ) << ":" << endl;
	    cout << file.c_str() << endl;
		db = NULL;
		return status;
	} else 
		dbfile = file;

	sqlite3AddMyFunc_sepang( this->db );
	cout << "Opened TGF events db: "<<file << endl;
	cout << "Current table = '"<<table << "'"<< endl;
	return status;
}

void TGFdb::get_step_data( sqlite3_stmt *& ppStmt, result_set& data)
{	
	data.tgfid = string((char*)sqlite3_column_text( ppStmt, 0) );
	data.utc = string((char*)sqlite3_column_text( ppStmt, 1) );
	data.fsec = sqlite3_column_double( ppStmt, 2);
	data.met = sqlite3_column_double( ppStmt, 3);
	
	data.colmask = 15; //Assume we have the first 4 columns (set bits 0-3).
	
	if ( sqlite3_column_type(ppStmt, TGFdb::tstartcol ) != SQLITE_NULL ) {
		data.tstart = sqlite3_column_double( ppStmt, TGFdb::tstartcol);
		data.colmask |= TGFdb::tstartmask;
	}
	if ( sqlite3_column_type(ppStmt, TGFdb::tstopcol ) != SQLITE_NULL ) {
		data.tstop  = sqlite3_column_double( ppStmt, TGFdb::tstopcol);
		data.colmask |= TGFdb::tstopmask;
	}
	
	if ( getLcParams && 
         ( 
		   sqlite3_column_type(ppStmt, TGFdb::XScol) != SQLITE_NULL &&
           sqlite3_column_type(ppStmt, TGFdb::XEcol) != SQLITE_NULL &&
	       sqlite3_column_type(ppStmt, TGFdb::DXcol) != SQLITE_NULL 
		 )
	   )
	{
		if ( data.lcview == NULL ) data.lcview = new histGenParams;
		data.lcview->xs = sqlite3_column_double( ppStmt, TGFdb::XScol);
		data.lcview->xe = sqlite3_column_double( ppStmt, TGFdb::XEcol);
		data.lcview->dx = sqlite3_column_double( ppStmt, TGFdb::DXcol);
	}
	//data.path = string((char*)sqlite3_column_text( ppStmt, 4) );
}

int TGFdb::getEvents(vector<result_set>& output, string whr)
{
	if (whr.length() > 0) whr = "WHERE "+whr;
	else whr = " ORDER BY met";

	sqlite3_stmt * ppStmt;
	string qry = "SELECT * FROM "+table+" "+whr;
	int rc;

	if ( printQry ) cout << qry << endl;

	TGFdb::result_set temp;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	rc = sqlite3_step(ppStmt);
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		output.push_back( temp );
		
		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;

}






StrokeDb::StrokeDb() 
{
	db = NULL;
	table = "lightning";
	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
}

StrokeDb::StrokeDb(string file) 
{ 
	table = "lightning";
	this->sqliteflags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
	open_db( file );
}

int StrokeDb::open_db( string file ) 
{
	int status;
	status = sqlite3_open_v2( file.c_str() , &(this->db), this->sqliteflags, NULL );
	
	if (status != SQLITE_OK) {
	    cout << sqlite3_errmsg( this->db ) << ":" << endl;
	    cout << file.c_str() << endl;
		db = NULL;
		return status;
	} else 
		dbfile = file;

	sqlite3AddMyFunc_sepang( this->db );

	cout << "Opened lightning events db: "<<file << endl;
	
	return status;
}

void StrokeDb::get_step_data( sqlite3_stmt *& ppStmt, result_set& data)
{	
	data.utc = string((char*)sqlite3_column_text( ppStmt, 0) );
	data.fsec = sqlite3_column_double( ppStmt, 1);
	data.tgfid = string((char*)sqlite3_column_text( ppStmt, 2) );
	data.lon = sqlite3_column_double( ppStmt, 3);
	data.lat = sqlite3_column_double( ppStmt, 4);
	data.locsrc = string((char*)sqlite3_column_text( ppStmt, 5) );
}

int StrokeDb::getEvents(vector<result_set>& output, string whr)
{
	if (whr.length() > 0) whr = "WHERE "+whr;

	sqlite3_stmt * ppStmt;
	string qry = "SELECT * FROM "+table+" "+whr;
	int rc;

	result_set temp;
	
	sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	rc = sqlite3_step(ppStmt);
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		output.push_back( temp );
		
		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;

}

int StrokeDb::getNearestEvents(vector<result_set>& output, string utctime, long int tdiff, string whr)
{
	if (whr.length() > 0) whr = " AND "+whr;
	
	char sdiff[10];
	sprintf(sdiff, (char*)"%ld", tdiff );

	sqlite3_stmt * ppStmt;
	string qry = "SELECT *, abs(strftime('%s',utc) - strftime('%s','"+utctime+"') ) as sdiff FROM "+table+" WHERE sdiff <= "+string(sdiff)+whr+" ORDER BY sdiff asc";
	
	//cout << qry << endl;
	int rc;

	result_set temp;
	
	rc =sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	if (rc != SQLITE_OK) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	rc = sqlite3_step(ppStmt);
	
	if (rc != SQLITE_ROW && rc !=SQLITE_DONE) {
		DB_EXPANDED_ERRMSG(this->db)
	}
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		output.push_back( temp );
		
		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;

}

int StrokeDb::getSpaceTimeEvents(vector<result_set>& output, double evMET, double refLon, double refLat, 
		double tdiff, double arcSep, string whr) 
{
	if (whr.length() > 0) whr = " AND "+whr;
	
	char sdiff[50];
	
	string evUtc = glast_met_to_utc( evMET );
	double evFsec = evMET - floor(evMET);

	sprintf(sdiff, (char*)"(sdiff <= %f) AND (xpdist <= %f)", tdiff, arcSep );

	sqlite3_stmt * ppStmt;
	string qry = "SELECT *, abs(strftime('%s',utc) - strftime('%s','"+evUtc+"') + (fsec - ?) ) as sdiff, sepang(lon,lat,?,?,1)*? as xpdist FROM "
				+table+" WHERE "+string(sdiff)+whr+" ORDER BY sdiff, xpdist asc";
	//cout << qry << endl;
	int rc;

	result_set temp;
	
	rc = sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	if (rc != SQLITE_OK) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	sqlite3_bind_double(ppStmt, 1, evFsec );
	sqlite3_bind_double(ppStmt, 2, refLon );
	sqlite3_bind_double(ppStmt, 3, refLat );
	sqlite3_bind_double(ppStmt, 4, EARTH_AVG_RAD/1000.0 );
	
	rc = sqlite3_step(ppStmt);
	
	if (rc != SQLITE_ROW && rc !=SQLITE_DONE) {
		DB_EXPANDED_ERRMSG(this->db)
	}
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		temp.t_offset = sqlite3_column_double( ppStmt, 6);
		temp.x_offset = sqlite3_column_double( ppStmt, 7);
		//temp.x_offset = deg2rad( temp.x_offset );
		output.push_back( temp );

		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;
}

int StrokeDb::getSpaceTimeEvents(vector<result_set>& output, string utctime, double refLon, double refLat, 
		long int tdiff, double arcSep, string whr) 
{
	if (whr.length() > 0) whr = " AND "+whr;
	
	char sdiff[50];
	//AND xpdist <= %f
	sprintf(sdiff, (char*)"(sdiff <= %ld) AND (xpdist <= %f)", tdiff, arcSep );

	sqlite3_stmt * ppStmt;
	string qry = "SELECT *, abs(strftime('%s',utc) - strftime('%s','"+utctime+"') ) as sdiff, sepang(lon,lat,?,?,1)*? as xpdist FROM "
				+table+" WHERE "+string(sdiff)+whr+" ORDER BY sdiff, xpdist asc";
	//cout << qry << endl;
	int rc;

	result_set temp;
	
	rc = sqlite3_prepare_v2( db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	if (rc != SQLITE_OK) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	sqlite3_bind_double(ppStmt, 1, refLon );
	sqlite3_bind_double(ppStmt, 2, refLat );
	sqlite3_bind_double(ppStmt, 3, EARTH_AVG_RAD/1000.0 );
	
	rc = sqlite3_step(ppStmt);
	
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		//temp.t_offset = (double)sqlite3_column_int( ppStmt, 6);
		temp.x_offset = sqlite3_column_double( ppStmt, 7);
		//temp.x_offset = deg2rad( temp.x_offset );
		output.push_back( temp );

		//sqlite3_reset( ppStmt );
		rc = sqlite3_step(ppStmt);
		
		//cout << temp.file << endl;
		//cout << rc << endl;
	}
	
	
	sqlite3_finalize( ppStmt );
	
	return rc;
}


int AssociationDb::open_db( string file ) 
{
	int status;
	status = sqlite3_open_v2( file.c_str() , &(this->db), this->sqliteflags, NULL );
	
	if (status != SQLITE_OK) {
	    DB_EXPANDED_ERRMSG(this->db)
		db = NULL;
		return status;
	}
	sqlite3AddMyFunc_sepang( this->db );
	return status;
}

int AssociationDb::CreateTable() {
	
	if ( this->table.length() == 0 ) {
		cerr << "Error: AssociationDb table name must be non-null string" << endl;
		return -1;
	}
	
	stringstream qry;
	qry << "CREATE TABLE IF NOT EXISTS '" << table << "' ("
		<<"tgfid char(15), " << endl
		<<"tmet FLOAT(16), " << endl
		<<"sutc DATETIME, " << endl
		<<"sfsec FLOAT(6)," << endl
		<<"tdiff FLOAT(6)," << endl
		<<"lon FLOAT(3,3)," << endl
		<<"lat FLOAT(3,3)," << endl
		<<"az FLOAT(3,3), " << endl
		<<"el FLOAT(3,3), " << endl
		<<"RA FLOAT(3,3), " << endl
		<<"Dec FLOAT(3,3),"   << endl
		<<"bmAng FLOAT(3,3),"<< endl
		<<"dxSrc FLOAT(4),"<< endl
		<<"dxBeam FLOAT(4),"<< endl
		<<"dxSurf FLOAT(4),"<< endl
		<<"pos_x FLOAT(10,1),"<< endl
		<<"pos_y FLOAT(10,1),"<< endl
		<<"pos_z FLOAT(10,1),"<< endl
		<<"qs FLOAT,"<< endl
		<<"qx FLOAT,"<< endl
		<<"qy FLOAT,"<< endl
		<<"qz FLOAT,"<< endl
		<<"source char(10),"<< endl
		<<"UNIQUE (tgfid, sutc, sfsec) );";

	sqlite3_stmt * ppStmt;
	int rc;

	rc = sqlite3_prepare_v2( this->db, qry.str().c_str(), qry.str().size()+1, &ppStmt, NULL );
	
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	rc = sqlite3_step(ppStmt);
	if ( rc != SQLITE_DONE ) {
		DB_EXPANDED_ERRMSG(this->db)
		 sqlite3_finalize( ppStmt );
		 return rc;
	}
	
	sqlite3_finalize( ppStmt );
	
	qry.str("");
	string units_table = table+"_units";
	stringstream units;
	qry << "CREATE TABLE IF NOT EXISTS '" << units_table << "' ("
		<<"tgfid char(12) DEFAULT 'x', " << endl
		<<"tmet char(12) DEFAULT 'MET', " << endl
		<<"sutc char(12) DEFAULT 'UTC', " << endl
		<<"sfsec char(12) DEFAULT 'seconds'," << endl
		<<"tdiff char(12) DEFAULT 'seconds'," << endl
		<<"lon char(12) DEFAULT 'deg'," << endl
		<<"lat char(12) DEFAULT 'deg'," << endl
		<<"az char(12) DEFAULT 'deg', " << endl
		<<"el char(12) DEFAULT 'deg', " << endl
		<<"RA char(12) DEFAULT 'deg', " << endl
		<<"Dec char(12) DEFAULT 'deg',"   << endl
		<<"bmAng char(12) DEFAULT 'deg',"<< endl
		<<"dxSrc char(12) DEFAULT 'km',"<< endl
		<<"dxBeam char(12) DEFAULT 'km',"<< endl
		<<"dxSurf char(12) DEFAULT 'km',"<< endl
		<<"pos_x char(12) DEFAULT 'km',"<< endl
		<<"pos_y char(12) DEFAULT 'km',"<< endl
		<<"pos_z char(12) DEFAULT 'km',"<< endl
		<<"qs char(12) DEFAULT '',"<< endl
		<<"qx char(12) DEFAULT '',"<< endl
		<<"qy char(12) DEFAULT '',"<< endl
		<<"qz char(12) DEFAULT '',"<< endl
		<<"source char(12) DEFAULT '', "<< endl
		<<" UNIQUE (tgfid) );";
	
	rc = sqlite3_prepare_v2( this->db, qry.str().c_str(), qry.str().size()+1, &ppStmt, NULL );
	
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)		
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	rc = sqlite3_step(ppStmt);
	if ( rc != SQLITE_DONE ) {
		DB_EXPANDED_ERRMSG(this->db)
		 sqlite3_finalize( ppStmt );
		 return rc;
	} else {
		sqlite3_finalize( ppStmt );
		qry.str("");
		
		qry << "INSERT OR IGNORE INTO '" << units_table << "' DEFAULT VALUES ";
		sqlite3_prepare_v2( this->db, qry.str().c_str(), qry.str().size()+1, &ppStmt, NULL );
		sqlite3_step(ppStmt);
		sqlite3_finalize( ppStmt );
	}
	
	return rc;
}

AssociationDb::result_set& AssociationDb::data2record ( result_set& data, TGFdb::result_set& tgf, StrokeDb::result_set& stroke, geom::x3& xEv, geom::gquat& qEv, double& posTime  )
{
	
	double tr,tra,tdec,taz,tel;
	
	//Set-up the coordinate structure for the stroke Geo-location, for the source at 20km.
	vGeographic srcPos;
	srcPos.r = EARTH_AVG_RAD+20.0*1000.0;
	srcPos.lon = geom::deg2rad( stroke.lon );
	srcPos.lat = geom::deg2rad( stroke.lat );
	/*
	vGeodetic srcPos;
	srcPos.h = 20.0*1000.0;
	srcPos.lon = geom::deg2rad( stroke.lon );
	srcPos.lat = geom::deg2rad( stroke.lat );
	*/
	//Convert the stroke Geo-location to GEI (a.k.a. ECI) coordinates.
	//If the TGF is vertically beamed, this vector is the same as the beam axis.
	x3 tgfgei = geo2gei( posTime, srcPos );
	
	//Calculate the straight line between the spacecraft and the stroke (vector superposition).
	//This gives a vector pointing from fermi to the stroke position, in the ECI frame.
	x3 tgfdir = tgfgei - xEv;
	//Convert to RA,Dec.  This gives the RA, Dec, and distance to the TGF from the spacecraft.
	cart2sphere( tgfdir, tr, tra, tdec, 1 );

	//Calculate the beam offset assuming a beam axis which is radially outwards from the (spherical) Earth.
	double eci_cosangle = dot( tgfgei, xEv ) / ( tgfgei.mag()*xEv.mag() );
	double eci_angle = acos( eci_cosangle );
	
	//Calculate the distance from the beam axis in three ways.
	double axDist = xEv.mag()*sqrt( 1.0 -  eci_cosangle*eci_cosangle );  // = |x|sin(theta)
	double groundDist = eci_angle*EARTH_AVG_RAD/1000.0;
	double beamAngle = asin( axDist / tgfdir.mag() );
	
	//Now apply the quaternion to get the source in spacecraft coordinates.
	qEv.rot( tgfdir );
	cart2sphere( tgfdir, taz, tel, 1 );

	double smet = glast_utc_to_met( (char*)stroke.utc.c_str() );
	smet += stroke.fsec;
	
	stroke.t_offset = smet - tgf.met;
	
	//result_set entry = {tgf, stroke, qEv, xEv/1000.0, posTime, tra, tdec, taz, tel, rad2deg(beamAngle), tr/1000.0, axDist/1000.0, groundDist };
	data.tgf = tgf;
	data.stroke = stroke;
	data.xEv = xEv / 1000.0;
	data.qEv = qEv;
	data.postime = posTime;
	data.ra = tra;
	data.dec = tdec;
	data.az = taz;
	data.el = tel;
	data.bmAng = rad2deg(beamAngle);
	data.dxSrc = tr/1000.0;
	data.dxBeam = axDist/1000.0;
	data.dxGnd = groundDist;
	
	return data;
}	

void AssociationDb::get_step_data( sqlite3_stmt *& ppStmt, result_set& data )
{
	int c=0;
	
	data.tgf.tgfid = string( (char*)sqlite3_column_text( ppStmt, c++) );
	data.tgf.met = sqlite3_column_double( ppStmt, c++ );
	
	data.tgf.utc = glast_met_to_utc( data.tgf.met );
	data.tgf.fsec = data.tgf.met - floor( data.tgf.met );
	
	data.stroke.utc = string( (char*)sqlite3_column_text( ppStmt, c++) );
	data.stroke.fsec = sqlite3_column_double( ppStmt, c++ );
	data.stroke.t_offset = sqlite3_column_double( ppStmt, c++ );
	data.stroke.lon = sqlite3_column_double( ppStmt, c++ );
	data.stroke.lat = sqlite3_column_double( ppStmt, c++ );
	
	data.az = sqlite3_column_double( ppStmt, c++ );
	data.el = sqlite3_column_double( ppStmt, c++ );
	data.ra = sqlite3_column_double( ppStmt, c++ );
	data.dec = sqlite3_column_double( ppStmt, c++ );
	data.bmAng = sqlite3_column_double( ppStmt, c++ );
	data.dxSrc = sqlite3_column_double( ppStmt, c++ );
	data.dxBeam = sqlite3_column_double( ppStmt, c++ );
	data.dxGnd = sqlite3_column_double( ppStmt, c++ );
	
	data.xEv.x0 = sqlite3_column_double( ppStmt, c++ );
	data.xEv.x1 = sqlite3_column_double( ppStmt, c++ );
	data.xEv.x2 = sqlite3_column_double( ppStmt, c++ );
	
	data.qEv.qs = sqlite3_column_double( ppStmt, c++ );
	data.qEv.qx = sqlite3_column_double( ppStmt, c++ );
	data.qEv.qy = sqlite3_column_double( ppStmt, c++ );
	data.qEv.qz = sqlite3_column_double( ppStmt, c++ );
	
	data.stroke.locsrc = string( (char*)sqlite3_column_text( ppStmt, c++) );
}

int AssociationDb::getEntries(vector<result_set>& entries, string whr, string groupby) {
	
	if ( this->table.length() == 0 ) {
		cerr << "Error: AssociationDb table name must be non-null string" << endl;
		return -1;
	}
	
	sqlite3_stmt * ppStmt;
	int rc;
	
	if (whr.length() > 0) whr = " WHERE "+whr;
	string qry = "select * from "+table+" "+whr+" "+groupby;

	rc = sqlite3_prepare_v2( this->db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		return rc;
	}
	
	result_set temp;
	
	rc = sqlite3_step(ppStmt);
	while (rc == SQLITE_ROW) {
		this->get_step_data( ppStmt, temp );
		entries.push_back( temp );
		rc = sqlite3_step(ppStmt);
	}
	
	sqlite3_finalize( ppStmt );
	
	return rc;
}

int AssociationDb::getEntries(vector<result_set*>& entries, string whr, string groupby) {
	
	if ( this->table.length() == 0 ) {
		cerr << "Error: AssociationDb table name must be non-null string" << endl;
		return -1;
	}
	
	sqlite3_stmt * ppStmt;
	int rc;
	
	if (whr.length() > 0) whr = " WHERE "+whr;
	string qry = "select * from "+table+" "+whr+" "+groupby;

	if ( this->printQry ) {
		cout << qry << endl;
	}

	rc = sqlite3_prepare_v2( this->db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		return rc;
	}
	
	rc = sqlite3_step(ppStmt);
	while (rc == SQLITE_ROW) {
		result_set * temp = new result_set;
		this->get_step_data( ppStmt, *temp );
		entries.push_back( temp );
		rc = sqlite3_step(ppStmt);
	}
	
	sqlite3_finalize( ppStmt );
	
	return rc;
}

int AssociationDb::getKnownMatches(vector<result_set>& entries, string tgfid) {
	
	return this->getEntries( entries, " tgfid = '"+tgfid+"' " );
	
}

int AssociationDb::getStrokeCount(string& tgfid) {
	
	if ( this->table.length() == 0 ) {
		cerr << "Error: AssociationDb table name must be non-null string" << endl;
		return -1;
	}
	
	sqlite3_stmt * ppStmt;
	int rc;
	int count;

	string qry = "select count(*) from "+table+" where tgfid='"+tgfid+"' ";

	if ( this->printQry ) {
		cout << qry << endl;
	}

	rc = sqlite3_prepare_v2( this->db, qry.c_str(), qry.size()+1, &ppStmt, NULL );
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		return -1;
	}
	
	rc = sqlite3_step(ppStmt);
	if (rc == SQLITE_ROW) {
		count = sqlite3_column_int( ppStmt, 0 );
	} else if (rc == SQLITE_DONE) {
		count = 0;
	} else {
		count = -1;
	} 
	
	sqlite3_finalize( ppStmt );
	
	return count;
}

int AssociationDb::commitAssociation( TGFdb::result_set& tgf, StrokeDb::result_set& stroke, 
	geom::x3 xEv, geom::gquat& qEv, double& posTime ) 
{
/*	
	double tr,tra,tdec,taz,tel;
	
	//Set-up the coordinate structure for the stroke Geo-location, for the source at 20km.
	vGeographic srcPos;
	srcPos.r = EARTH_AVG_RAD+20.0*1000.0;
	srcPos.lon = geom::deg2rad( stroke.lon );
	srcPos.lat = geom::deg2rad( stroke.lat );
	
	//Convert the stroke Geo-location to GEI (a.k.a. ECI) coordinates.
	//If the TGF is vertically beamed, this vector is the same as the beam axis.
	x3 tgfgei = geo2gei( posTime, srcPos );
	
	//Calculate the straight line between the spacecraft and the stroke (vector superposition).
	//This gives a vector pointing from fermi to the stroke position, in the ECI frame.
	x3 tgfdir = tgfgei - xEv;
	//Convert to RA,Dec.  This gives the RA, Dec, and distance to the TGF from the spacecraft.
	cart2sphere( tgfdir, tr, tra, tdec, 1 );

	//Calculate the beam offset assuming a beam axis which is radially outwards from the (spherical) Earth.
	double eci_cosangle = dot( tgfgei, xEv ) / ( tgfgei.mag()*xEv.mag() );
	double eci_angle = acos( eci_cosangle );
	
	//Calculate the distance from the beam axis in three ways.
	double axDist = xEv.mag()*sqrt( 1.0 -  eci_cosangle*eci_cosangle );
	double groundDist = eci_angle*EARTH_AVG_RAD/1000.0;
	double beamAngle = asin( axDist / tgfdir.mag() );
	
	//Now apply the quaternion to get the source in spacecraft coordinates.
	qEv.rot( tgfdir );
	cart2sphere( tgfdir, taz, tel, 1 );

	double smet = glast_utc_to_met( (char*)stroke.utc.c_str() );
	smet += stroke.fsec;
	
	stroke.t_offset = smet - tgf.met;
	
	result_set entry = {tgf, stroke, qEv, xEv/1000.0, posTime, tra, tdec, taz, tel, rad2deg(beamAngle), tr/1000.0, axDist/1000.0, groundDist };
*/

	result_set entry;
	this->data2record( entry, tgf, stroke, xEv, qEv, posTime );
	
	return this->makeEntry( entry, "OR REPLACE" );
}

int AssociationDb::makeEntry( result_set& data, string onconflict )
{
	
	string assignment = "INSERT "+onconflict+" INTO "+table+
		" VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)";
		
	sqlite3_stmt * ppStmt;
	
	int rc = sqlite3_prepare_v2( this->db, assignment.c_str(), assignment.size()+1, &ppStmt, NULL );
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	
	rc = data.bind( ppStmt );
	if ( rc != SQLITE_OK ) {
		DB_EXPANDED_ERRMSG(this->db)
		sqlite3_finalize( ppStmt );
		return rc;
	}
	rc = sqlite3_step( ppStmt );
	if ( rc != SQLITE_DONE ) {
		sqlite3_finalize( ppStmt );
		
		if ( rc == SQLITE_CONSTRAINT ) {
			
			rc = this->makeEntry( data, "OR REPLACE" );
			
			if ( rc != SQLITE_DONE ) {
				DB_EXPANDED_ERRMSG(this->db)
			}
			
		} else {
			DB_EXPANDED_ERRMSG(this->db)
		}
	} else {
		sqlite3_finalize( ppStmt );
	}
	
	return rc;
}


